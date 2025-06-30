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
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mailbox Driver");
MODULE_DESCRIPTION("A dual-ended mailbox character device driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mailbox"
#define CLASS_NAME "mailbox_class"
#define MAILBOX_SIZE 1024
#define NUM_DEVICES 2

// Device structure
struct mailbox_dev {
    struct cdev cdev; //Character device structure.
    struct device *device; // Device structure for /sys/class and /dev entries.
    int minor; // Minor number to identify the device (0 or 1).
    bool is_open; // Flag to track if the device is already opened.
    struct mutex open_mutex; // Mutex to synchronize access to the is_open flag.
};

// Shared mailbox structure
struct mailbox_data {
    struct kfifo fifo; // Kernel FIFO buffer for storing data.
    struct mutex fifo_mutex; // Mutex to protect access to the FIFO.
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue; // Wait queues for blocking reads and writes when the FIFO is empty or full.
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

// This function is invoked when a user-space process calls open() on a device file open("/dev/mailbox0", O_RDWR).
static int mailbox_open(struct inode *inode, struct file *file)
{
    struct mailbox_dev *dev;
    int minor = iminor(inode);
    /*
    Extracts the minor number from the inode using the iminor macro.
    The minor number identifies which device is being opened (0 for mailbox0, 1 for mailbox1).
    */

    printk(KERN_INFO "Mailbox: Attempting to open device %d\n", minor);

    if (minor >= NUM_DEVICES) {
        return -ENODEV;
    }

    dev = &mailbox_devices[minor];

    /*
    mutex_lock_interruptible locks the mutex but allows the operation to be interrupted by signals (if the user presses Ctrl+C).
    If the mutex is already locked, the calling process waits until it’s available.
    If interrupted by a signal, it returns -ERESTARTSYS, indicating that the system call should be restarted or an error returned to user-space.
    */
    if (mutex_lock_interruptible(&dev->open_mutex)) {
        return -ERESTARTSYS;
    }

    if (dev->is_open) {
        mutex_unlock(&dev->open_mutex);
        printk(KERN_WARNING "Mailbox: Device %d already open\n", minor);
        return -EBUSY;
    }

    dev->is_open = true;
    file->private_data = dev;
    mutex_unlock(&dev->open_mutex);

    printk(KERN_INFO "Mailbox: Device %d opened successfully\n", minor);
    return 0;
}

static int mailbox_release(struct inode *inode, struct file *file)
{
    struct mailbox_dev *dev = file->private_data;

    mutex_lock(&dev->open_mutex);
    dev->is_open = false;
    mutex_unlock(&dev->open_mutex);

    printk(KERN_INFO "Mailbox: Device %d closed\n", dev->minor);
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
            /*
            wait_event_interruptible puts the process to sleep until the condition is true or a signal interrupts it.
            */
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
        /*
        Wakes up all processes waiting on shared_mailbox->write_queue.
        These are processes that called mailbox_write but were put to sleep because the FIFO lacked enough space (kfifo_avail(&shared_mailbox->fifo) < len).
        Without these calls, blocked processes would never wake up, causing deadlocks or requiring inefficient polling.
        */
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
    // Uses kfifo_avail to check if the FIFO has enough free space for len bytes.
    while (kfifo_avail(&shared_mailbox->fifo) < len) {

        /*
        This adds the process to shared_mailbox->write_queue and puts it to sleep until the FIFO has at least len bytes of free space.
        */
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
    /*
    After mailbox_write adds data to the FIFO, wake_up_interruptible(&shared_mailbox->read_queue)
    signals all processes in the read_queue to wake up. These processes then recheck the condition
    (!kfifo_is_empty) and proceed to read the newly available data if the condition is true.
    */
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
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DEVICE_NAME); // allow kernel automatcally allocate the device major number instead of pre-defined.
    /*
    0: The starting minor number for the range
    NUM_DEVICES: The number of minor numbers to allocate
    The kernel finds an unused major number and assigns it to the device.
    The allocated major and minor numbers are stored in dev_num. The major number can later be extracted using MAJOR(dev_num)
    */

    if (ret < 0) {
        printk(KERN_ALERT "Mailbox: Failed to allocate major number\n");
        goto cleanup_fifo;
    }
    major_number = MAJOR(dev_num);

    // Tạo device class
    /*
    Creates a device class named CLASS_NAME (defined as "mailbox_class") under /sys/class/.
    This class is a kernel abstraction that groups related devices and provides a way to manage their properties and device nodes.
    */
    mailbox_class = class_create(CLASS_NAME);
    if (IS_ERR(mailbox_class)) {
        ret = PTR_ERR(mailbox_class);
        goto cleanup_chrdev;
    }

    // Khởi tạo các device
    for (i = 0; i < NUM_DEVICES; i++) {
        mailbox_devices[i].minor = i;
        mailbox_devices[i].is_open = false;
        mutex_init(&mailbox_devices[i].open_mutex);

        // Khởi tạo cdev
        // Initializes the character device structure (cdev) for the i-th device, associating it with the file operations (fops) defined earlier.
        cdev_init(&mailbox_devices[i].cdev, &fops);
        mailbox_devices[i].cdev.owner = THIS_MODULE;

        ret = cdev_add(&mailbox_devices[i].cdev, MKDEV(major_number, i), 1);
        /*
        Registers the character device with the kernel,
        associating it with a specific device number (major number from major_number, minor number i) and a count of 1 (one device per minor number).
        The 1 indicates that this cdev corresponds to exactly one device number
        */
        if (ret) {
            printk(KERN_ALERT "Mailbox: Failed to add cdev %d\n", i);
            goto cleanup_devices;
        }

        /*
        This creates the device files in /dev that user-space applications can open, read from, or write to.
        It also creates sysfs entries for device management (e.g., /sys/class/mailbox_class/mailbox0).
        mailbox_class: The device class created earlier, associating the device with /sys/class/mailbox_class.
        NULL: The parent device (none in this case, as these are virtual character devices).
        MKDEV(major_number, i): The device number (major number and minor number i).
        NULL: No additional driver data (can be used to pass custom data to the device).
        "mailbox%d", i: The format string for the device name, creating /dev/mailbox0 and /dev/mailbox1.
        with this macro, there is no need to call mknod in userspace.
        */
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

