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
#include <linux/list.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "mailbox"
#define CLASS_NAME "mailbox_class"
#define MAX_MESSAGE_SIZE 1024
#define MAX_TAG_SIZE 32
#define NUM_DEVICES 2
#define DEFAULT_TAG "public"

// IOCTL commands
#define MAILBOX_IOC_MAGIC 'm'
#define MAILBOX_IOC_SET_TAG _IOW(MAILBOX_IOC_MAGIC, 1, char*)
#define MAILBOX_IOC_GET_TAG _IOR(MAILBOX_IOC_MAGIC, 2, char*)
#define MAILBOX_IOC_CLEAR_TAG _IO(MAILBOX_IOC_MAGIC, 3)

// Message structure với tag
struct tagged_message {
    char tag[MAX_TAG_SIZE];
    size_t data_len;
    char data[MAX_MESSAGE_SIZE];
    struct list_head list;
};

// Device structure
struct mailbox_dev {
    struct cdev cdev;
    struct device *device;
    int minor;
    bool is_open;
    struct mutex open_mutex;
    char current_tag[MAX_TAG_SIZE];  // Tag hiện tại của device
    bool has_tag;                    // Có tag được set hay không
};

// Shared mailbox structure
struct mailbox_data {
    struct list_head message_list;   // Danh sách các message
    struct mutex list_mutex;         // Bảo vệ danh sách
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
    int message_count;
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
static long mailbox_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .open = mailbox_open,
    .release = mailbox_release,
    .read = mailbox_read,
    .write = mailbox_write,
    .unlocked_ioctl = mailbox_ioctl,
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

    // Lock để kiểm tra và set trạng thái open
    if (mutex_lock_interruptible(&dev->open_mutex)) {
        return -ERESTARTSYS;
    }

    if (dev->is_open) {
        mutex_unlock(&dev->open_mutex);
        printk(KERN_WARNING "Mailbox: Device %d already open\n", minor);
        return -EBUSY;
    }

    dev->is_open = true;
    dev->has_tag = true; // Default to using public tag
    strncpy(dev->current_tag, DEFAULT_TAG, MAX_TAG_SIZE - 1);
    dev->current_tag[MAX_TAG_SIZE - 1] = '\0';
    file->private_data = dev;
    mutex_unlock(&dev->open_mutex);

    printk(KERN_INFO "Mailbox: Device %d opened successfully with default tag '%s'\n", minor, dev->current_tag);
    return 0;
}

static int mailbox_release(struct inode *inode, struct file *file)
{
    struct mailbox_dev *dev = file->private_data;

    mutex_lock(&dev->open_mutex);
    dev->is_open = false;
    dev->has_tag = false;
    memset(dev->current_tag, 0, MAX_TAG_SIZE);
    mutex_unlock(&dev->open_mutex);

    printk(KERN_INFO "Mailbox: Device %d closed\n", dev->minor);
    return 0;
}

// Tìm message với tag phù hợp (phải được gọi với mutex đã lock)
static struct tagged_message *find_message_by_tag(const char *tag)
{
    struct tagged_message *msg;

    list_for_each_entry(msg, &shared_mailbox->message_list, list) {
        if (strcmp(msg->tag, tag) == 0) {
            return msg;
        }
    }
    return NULL;
}

static ssize_t mailbox_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    struct mailbox_dev *dev = file->private_data;
    struct tagged_message *msg;
    ssize_t bytes_to_copy;
    int ret;

    if (len == 0) {
        return 0;
    }

    // Use default tag if no tag is explicitly set
    if (!dev->has_tag) {
        strncpy(dev->current_tag, DEFAULT_TAG, MAX_TAG_SIZE - 1);
        dev->current_tag[MAX_TAG_SIZE - 1] = '\0';
        dev->has_tag = true;
    }

    // Chờ cho đến khi có message với tag phù hợp
    while (true) {
        if (mutex_lock_interruptible(&shared_mailbox->list_mutex)) {
            return -ERESTARTSYS;
        }

        msg = find_message_by_tag(dev->current_tag);
        if (msg) {
            // Tìm thấy message, remove khỏi list
            list_del(&msg->list);
            shared_mailbox->message_count--;
            mutex_unlock(&shared_mailbox->list_mutex);
            break;
        }

        mutex_unlock(&shared_mailbox->list_mutex);

        // Không tìm thấy message phù hợp
        if (file->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        ret = wait_event_interruptible(shared_mailbox->read_queue, ({
            int found = 0;
            if (!mutex_lock_interruptible(&shared_mailbox->list_mutex)) {
                found = (find_message_by_tag(dev->current_tag) != NULL);
                mutex_unlock(&shared_mailbox->list_mutex);
            }
            found;
        }));

        if (ret) {
            return ret; // Signal interrupt
        }
    }

    // Copy data từ message
    bytes_to_copy = min(len, msg->data_len);
    if (copy_to_user(buffer, msg->data, bytes_to_copy)) {
        kfree(msg);
        return -EFAULT;
    }

    printk(KERN_INFO "Mailbox: Read %zd bytes with tag '%s'\n", bytes_to_copy, msg->tag);

    // Wake up writer
    wake_up_interruptible(&shared_mailbox->write_queue);

    kfree(msg);
    return bytes_to_copy;
}

static ssize_t mailbox_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)
{
    struct mailbox_dev *dev = file->private_data;
    struct tagged_message *msg;
    int ret;

    if (len == 0) {
        return 0;
    }

    if (len > MAX_MESSAGE_SIZE) {
        return -EINVAL;
    }

    // Use default tag if no tag is explicitly set
    if (!dev->has_tag) {
        strncpy(dev->current_tag, DEFAULT_TAG, MAX_TAG_SIZE - 1);
        dev->current_tag[MAX_TAG_SIZE - 1] = '\0';
        dev->has_tag = true;
    }

    // Tạo message mới
    msg = kmalloc(sizeof(struct tagged_message), GFP_KERNEL);
    if (!msg) {
        return -ENOMEM;
    }

    // Copy tag và data
    strncpy(msg->tag, dev->current_tag, MAX_TAG_SIZE - 1);
    msg->tag[MAX_TAG_SIZE - 1] = '\0';
    msg->data_len = len;

    if (copy_from_user(msg->data, buffer, len)) {
        kfree(msg);
        return -EFAULT;
    }

    // Thêm message vào list
    if (mutex_lock_interruptible(&shared_mailbox->list_mutex)) {
        kfree(msg);
        return -ERESTARTSYS;
    }

    list_add_tail(&msg->list, &shared_mailbox->message_list);
    shared_mailbox->message_count++;

    mutex_unlock(&shared_mailbox->list_mutex);

    // Wake up reader
    wake_up_interruptible(&shared_mailbox->read_queue);

    printk(KERN_INFO "Mailbox: Written %zu bytes with tag '%s'\n", len, msg->tag);
    return len;
}

static long mailbox_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct mailbox_dev *dev = file->private_data;
    char tag_buffer[MAX_TAG_SIZE];
    int ret = 0;

    switch (cmd) {
    case MAILBOX_IOC_SET_TAG:
        if (copy_from_user(tag_buffer, (char __user *)arg, MAX_TAG_SIZE)) {
            return -EFAULT;
        }
        tag_buffer[MAX_TAG_SIZE - 1] = '\0'; // Đảm bảo null-terminated

        // Kiểm tra tag không rỗng
        if (strlen(tag_buffer) == 0) {
            return -EINVAL;
        }

        mutex_lock(&dev->open_mutex);
        strncpy(dev->current_tag, tag_buffer, MAX_TAG_SIZE - 1);
        dev->current_tag[MAX_TAG_SIZE - 1] = '\0';
        dev->has_tag = true;
        mutex_unlock(&dev->open_mutex);

        printk(KERN_INFO "Mailbox: Set tag '%s' for device %d\n", dev->current_tag, dev->minor);
        break;

    case MAILBOX_IOC_GET_TAG:
        mutex_lock(&dev->open_mutex);
        if (!dev->has_tag) {
            strncpy(dev->current_tag, DEFAULT_TAG, MAX_TAG_SIZE - 1);
            dev->current_tag[MAX_TAG_SIZE - 1] = '\0';
        }
        if (copy_to_user((char __user *)arg, dev->current_tag, strlen(dev->current_tag) + 1)) {
            mutex_unlock(&dev->open_mutex);
            return -EFAULT;
        }
        mutex_unlock(&dev->open_mutex);
        break;

    case MAILBOX_IOC_CLEAR_TAG:
        mutex_lock(&dev->open_mutex);
        dev->has_tag = true; // Revert to default tag
        strncpy(dev->current_tag, DEFAULT_TAG, MAX_TAG_SIZE - 1);
        dev->current_tag[MAX_TAG_SIZE - 1] = '\0';
        mutex_unlock(&dev->open_mutex);
        printk(KERN_INFO "Mailbox: Reset tag to default '%s' for device %d\n", dev->current_tag, dev->minor);
        break;

    default:
        ret = -ENOTTY;
        break;
    }

    return ret;
}

static int __init mailbox_init(void)
{
    int ret, i;
    dev_t dev_num;

    printk(KERN_INFO "Mailbox: Initializing tagged mailbox driver\n");

    // Cấp phát shared mailbox
    shared_mailbox = kmalloc(sizeof(struct mailbox_data), GFP_KERNEL);
    if (!shared_mailbox) {
        return -ENOMEM;
    }

    // Khởi tạo list và mutex
    INIT_LIST_HEAD(&shared_mailbox->message_list);
    mutex_init(&shared_mailbox->list_mutex);
    init_waitqueue_head(&shared_mailbox->read_queue);
    init_waitqueue_head(&shared_mailbox->write_queue);
    shared_mailbox->message_count = 0;

    // Cấp phát major number
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Mailbox: Failed to allocate major number\n");
        goto cleanup_mailbox;
    }
    major_number = MAJOR(dev_num);

    // Tạo device class
    mailbox_class = class_create(CLASS_NAME);
    if (IS_ERR(mailbox_class)) {
        ret = PTR_ERR(mailbox_class);
        goto cleanup_chrdev;
    }

    // Khởi tạo các device
    for (i = 0; i < NUM_DEVICES; i++) {
        mailbox_devices[i].minor = i;
        mailbox_devices[i].is_open = false;
        mailbox_devices[i].has_tag = false;
        memset(mailbox_devices[i].current_tag, 0, MAX_TAG_SIZE);
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

    printk(KERN_INFO "Mailbox: Tagged driver initialized with major number %d\n", major_number);
    return 0;

cleanup_devices:
    for (i--; i >= 0; i--) {
        device_destroy(mailbox_class, MKDEV(major_number, i));
        cdev_del(&mailbox_devices[i].cdev);
    }
    class_destroy(mailbox_class);

cleanup_chrdev:
    unregister_chrdev_region(MKDEV(major_number, 0), NUM_DEVICES);

cleanup_mailbox:
    kfree(shared_mailbox);

    return ret;
}

static void __exit mailbox_exit(void)
{
    int i;
    struct tagged_message *msg, *tmp;

    printk(KERN_INFO "Mailbox: Cleaning up tagged mailbox driver\n");

    // Cleanup devices
    for (i = 0; i < NUM_DEVICES; i++) {
        device_destroy(mailbox_class, MKDEV(major_number, i));
        cdev_del(&mailbox_devices[i].cdev);
    }

    class_destroy(mailbox_class);
    unregister_chrdev_region(MKDEV(major_number, 0), NUM_DEVICES);

    // Cleanup remaining messages
    mutex_lock(&shared_mailbox->list_mutex);
    list_for_each_entry_safe(msg, tmp, &shared_mailbox->message_list, list) {
        list_del(&msg->list);
        kfree(msg);
    }
    mutex_unlock(&shared_mailbox->list_mutex);

    // Cleanup shared mailbox
    kfree(shared_mailbox);

    printk(KERN_INFO "Mailbox: Tagged driver cleanup completed\n");
}

module_init(mailbox_init);
module_exit(mailbox_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tagged Mailbox Driver");
MODULE_DESCRIPTION("A dual-ended mailbox character device driver with tag support");
MODULE_VERSION("2.0");