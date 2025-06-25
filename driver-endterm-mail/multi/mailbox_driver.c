#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kfifo.h>

#define DEVICE_NAME "mailbox"
#define CLASS_NAME "mailbox_class"
#define MAILBOX_SIZE 1024
#define NUM_DEVICES 2

// Device structure
struct mailbox_dev {
    struct cdev cdev;
    struct device *device;
    int minor;
    int open_count;
    struct mutex open_mutex;
};

// Shared mailbox structure
struct mailbox_data {
    struct kfifo fifo;
    struct mutex fifo_mutex;
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
};

static int major_number;
static struct class *mailbox_class = NULL;
static struct mailbox_dev mailbox_devices[NUM_DEVICES];
static struct mailbox_data *shared_mailbox;

// Function prototypes
static int mailbox_open(struct inode *inode, struct file *file);
static int mailbox_release(struct inode *inode, struct file *file);
static ssize_t mailbox_read(struct file *file, char __user *buffer, size_t len, loff_t *offset);
static ssize_t mailbox_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset);

static struct file_operations fops = {
    .open = mailbox_open,
    .release = mailbox_release,
    .read = mailbox_read,
    .write = mailbox_write,
    .owner = THIS_MODULE,
};

static int mailbox_open(struct inode *inode, struct file *file)
{
    struct mailbox_dev *dev;
    int minor = iminor(inode);

    printk(KERN_INFO "Mailbox: Attempting to open device %d\n", minor);

    if (minor >= NUM_DEVICES) {
        return -ENODEV;
    }

    dev = &mailbox_devices[minor];

    // Lock device để tăng open count
    if (mutex_lock_interruptible(&dev->open_mutex)) {
        return -ERESTARTSYS;
    }

    dev->open_count++;
    file->private_data = dev;

    mutex_unlock(&dev->open_mutex);

    printk(KERN_INFO "Mailbox: Device %d opened successfully (count: %d)\n", minor, dev->open_count);
    return 0;
}

static int mailbox_release(struct inode *inode, struct file *file)
{
    struct mailbox_dev *dev = file->private_data;

    mutex_lock(&dev->open_mutex);
    dev->open_count--;

    mutex_unlock(&dev->open_mutex);

    printk(KERN_INFO "Mailbox: Device %d closed (remaining count: %d)\n", dev->minor, dev->open_count);
    return 0;
}

static ssize_t mailbox_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    int ret;
    unsigned int copied;
    size_t total_copied = 0;
    size_t remaining = len;

    if (len == 0) {
        return 0;
    }

    // Đọc cho đến khi có đủ len bytes hoặc bị interrupt
    while (total_copied < len) {
        // Chờ cho đến khi có dữ liệu trong FIFO
        while (kfifo_is_empty(&shared_mailbox->fifo)) {
            if (file->f_flags & O_NONBLOCK) {
                // Nếu đã đọc được một phần, trả về số bytes đã đọc
                if (total_copied > 0) {
                    return total_copied;
                }
                return -EAGAIN;
            }

            ret = wait_event_interruptible(shared_mailbox->read_queue,
                                         !kfifo_is_empty(&shared_mailbox->fifo));
            if (ret) {
                // Nếu đã đọc được một phần, trả về số bytes đã đọc
                if (total_copied > 0) {
                    return total_copied;
                }
                return ret; // Signal interrupt
            }
        }

        // Lock FIFO để đọc dữ liệu
        if (mutex_lock_interruptible(&shared_mailbox->fifo_mutex)) {
            // Nếu đã đọc được một phần, trả về số bytes đã đọc
            if (total_copied > 0) {
                return total_copied;
            }
            return -ERESTARTSYS;
        }

        // Tính số bytes còn lại cần đọc
        remaining = len - total_copied;

        // Đọc dữ liệu từ FIFO (tối đa remaining bytes)
        ret = kfifo_to_user(&shared_mailbox->fifo, buffer + total_copied, remaining, &copied);

        mutex_unlock(&shared_mailbox->fifo_mutex);

        if (ret) {
            // Nếu đã đọc được một phần, trả về số bytes đã đọc
            if (total_copied > 0) {
                return total_copied;
            }
            return ret;
        }

        total_copied += copied;

        // Wake up writer nếu có space
        wake_up_interruptible(&shared_mailbox->write_queue);

        printk(KERN_INFO "Mailbox: Read %u bytes (total so far: %zu/%zu)\n",
               copied, total_copied, len);
    }

    printk(KERN_INFO "Mailbox: Completed reading %zu bytes\n", total_copied);
    return total_copied;
}

static ssize_t mailbox_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)
{
    int ret;
    unsigned int copied;

    if (len == 0) {
        return 0;
    }

    // Chờ cho đến khi có space trong FIFO
    while (kfifo_avail(&shared_mailbox->fifo) < len) {
        if (file->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        ret = wait_event_interruptible(shared_mailbox->write_queue,
                                     kfifo_avail(&shared_mailbox->fifo) >= len);
        if (ret) {
            return ret; // Signal interrupt
        }
    }

    // Lock FIFO để ghi dữ liệu
    if (mutex_lock_interruptible(&shared_mailbox->fifo_mutex)) {
        return -ERESTARTSYS;
    }

    // Ghi dữ liệu vào FIFO
    ret = kfifo_from_user(&shared_mailbox->fifo, buffer, len, &copied);

    mutex_unlock(&shared_mailbox->fifo_mutex);

    if (ret) {
        return ret;
    }

    // Wake up reader
    wake_up_interruptible(&shared_mailbox->read_queue);

    printk(KERN_INFO "Mailbox: Written %u bytes\n", copied);
    return copied;
}

static int __init mailbox_init(void)
{
    int ret, i;
    dev_t dev_num;

    printk(KERN_INFO "Mailbox: Initializing mailbox driver\n");

    // Cấp phát shared mailbox
    shared_mailbox = kmalloc(sizeof(struct mailbox_data), GFP_KERNEL);
    if (!shared_mailbox) {
        return -ENOMEM;
    }

    // Khởi tạo KFIFO
    ret = kfifo_alloc(&shared_mailbox->fifo, MAILBOX_SIZE, GFP_KERNEL);
    if (ret) {
        kfree(shared_mailbox);
        return ret;
    }

    // Khởi tạo mutex và wait queue
    mutex_init(&shared_mailbox->fifo_mutex);
    init_waitqueue_head(&shared_mailbox->read_queue);
    init_waitqueue_head(&shared_mailbox->write_queue);

    // Cấp phát major number
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Mailbox: Failed to allocate major number\n");
        goto cleanup_fifo;
    }
    major_number = MAJOR(dev_num);

    // Tạo device class
    mailbox_class = class_create( CLASS_NAME);
    if (IS_ERR(mailbox_class)) {
        ret = PTR_ERR(mailbox_class);
        goto cleanup_chrdev;
    }

    // Khởi tạo các device
    for (i = 0; i < NUM_DEVICES; i++) {
        mailbox_devices[i].minor = i;
        mailbox_devices[i].open_count = 0;
        mutex_init(&mailbox_devices[i].open_mutex);

        // Khởi tạo cdev
        cdev_init(&mailbox_devices[i].cdev, &fops);
        mailbox_devices[i].cdev.owner = THIS_MODULE;

        ret = cdev_add(&mailbox_devices[i].cdev, MKDEV(major_number, i), 1);
        if (ret) {
            printk(KERN_ALERT "Mailbox: Failed to add cdev %d\n", i);
            goto cleanup_devices;
        }

        // Tạo device file
        mailbox_devices[i].device = device_create(mailbox_class, NULL,
                                                MKDEV(major_number, i), NULL,
                                                "mailbox%d", i);
        if (IS_ERR(mailbox_devices[i].device)) {
            ret = PTR_ERR(mailbox_devices[i].device);
            cdev_del(&mailbox_devices[i].cdev);
            goto cleanup_devices;
        }
    }

    printk(KERN_INFO "Mailbox: Driver initialized with major number %d\n", major_number);
    return 0;

cleanup_devices:
    for (i--; i >= 0; i--) {
        device_destroy(mailbox_class, MKDEV(major_number, i));
        cdev_del(&mailbox_devices[i].cdev);
    }
    class_destroy(mailbox_class);

cleanup_chrdev:
    unregister_chrdev_region(MKDEV(major_number, 0), NUM_DEVICES);

cleanup_fifo:
    kfifo_free(&shared_mailbox->fifo);
    kfree(shared_mailbox);

    return ret;
}

static void __exit mailbox_exit(void)
{
    int i;

    printk(KERN_INFO "Mailbox: Cleaning up mailbox driver\n");

    // Cleanup devices
    for (i = 0; i < NUM_DEVICES; i++) {
        device_destroy(mailbox_class, MKDEV(major_number, i));
        cdev_del(&mailbox_devices[i].cdev);
    }

    class_destroy(mailbox_class);
    unregister_chrdev_region(MKDEV(major_number, 0), NUM_DEVICES);

    // Cleanup shared mailbox
    kfifo_free(&shared_mailbox->fifo);
    kfree(shared_mailbox);

    printk(KERN_INFO "Mailbox: Driver cleanup completed\n");
}

module_init(mailbox_init);
module_exit(mailbox_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mailbox Driver");
MODULE_DESCRIPTION("A dual-ended mailbox character device driver");
MODULE_VERSION("1.0");