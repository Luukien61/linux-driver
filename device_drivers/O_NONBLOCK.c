
/*
 * Character device drivers lab
 * Updated for non-blocking reads, no exclusive access, and initial data_size=0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL KERN_INFO

#define MY_MAJOR 42
#define MY_MINOR 0
#define NUM_MINORS 1
#define MODULE_NAME "so2_cdev"
#define IOCTL_MESSAGE "Hello ioctl\n"

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

struct so2_device_data {
    struct cdev cdev;
    char buffer[BUFSIZ];
    size_t data_size;         // Track valid data in buffer
    wait_queue_head_t queue;  // Wait queue for reads and MY_IOCTL_DOWN/UP
    int is_blocked;           // Flag for MY_IOCTL_DOWN/UP
};

struct so2_device_data devs[NUM_MINORS];


static int so2_cdev_open(struct inode *inode, struct file *file)
{
    struct so2_device_data *data;

    pr_info("%s: Device opened\n", MODULE_NAME);

    data = container_of(inode->i_cdev, struct so2_device_data, cdev);
    file->private_data = data;

    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(10 * HZ);

    return 0;
}

static int so2_cdev_release(struct inode *inode, struct file *file)
{
    pr_info("%s: Device closed\n", MODULE_NAME);
    return 0;
}

static ssize_t so2_cdev_read(struct file *file, char __user *user_buffer,
                             size_t size, loff_t *offset)
{
    struct so2_device_data *data = file->private_data;
    size_t to_read;

    // Wait for data if blocking mode, or return -EAGAIN if non-blocking
    if (data->data_size == 0) {
        if (file->f_flags & O_NONBLOCK) {
            pr_info("%s: No data available, returning -EAGAIN\n", MODULE_NAME);
            return -EAGAIN;
        }
        wait_event_interruptible(data->queue, data->data_size > 0);
    }

    to_read = min(size, data->data_size);
    if (copy_to_user(user_buffer, data->buffer, to_read)) {
        pr_info("%s: copy_to_user failed\n", MODULE_NAME);
        return -EFAULT;
    }

    pr_info("%s: Read %zu bytes\n", MODULE_NAME, to_read);
    return to_read;
}

static ssize_t so2_cdev_write(struct file *file, const char __user *user_buffer,
                              size_t size, loff_t *offset)
{
    struct so2_device_data *data = file->private_data;

    size = min(size, (size_t)BUFSIZ);
    if (copy_from_user(data->buffer, user_buffer, size)) {
        pr_info("%s: copy_from_user failed\n", MODULE_NAME);
        return -EFAULT;
    }

    data->data_size = size;
    wake_up_interruptible(&data->queue); // Wake up blocked readers
    pr_info("%s: Wrote %zu bytes\n", MODULE_NAME, size);
    return size;
}

static long so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct so2_device_data *data = file->private_data;
    int ret = 0;
    char *user_buffer;
    char temp_buffer[64];

    switch (cmd) {
    case MY_IOCTL_PRINT:
        pr_info("%s: %s", MODULE_NAME, IOCTL_MESSAGE);
        break;
    case MY_IOCTL_CLEAR_BUFFER:
        memset(data->buffer, 0, BUFSIZ);
        data->data_size = 0;
        wake_up_interruptible(&data->queue);
        pr_info("%s: Buffer cleared\n", MODULE_NAME);
        break;
    case MY_IOCTL_SET_BUFFER:
        user_buffer = (char *)arg;
        if (copy_from_user(data->buffer, user_buffer, BUFSIZ)) {
            pr_info("%s: MY_IOCTL_SET_BUFFER copy_from_user failed\n", MODULE_NAME);
            return -EFAULT;
        }
        data->data_size = strnlen(data->buffer, BUFSIZ);
        strncpy(temp_buffer, data->buffer, 63);
        temp_buffer[63] = '\0';
        pr_info("%s: Buffer set via ioctl to: %s%s\n", MODULE_NAME, temp_buffer,
                data->data_size > 63 ? "..." : "");
        wake_up_interruptible(&data->queue);
        break;
    case MY_IOCTL_GET_BUFFER:
        user_buffer = (char *)arg;
        if (copy_to_user(user_buffer, data->buffer, data->data_size)) {
            pr_info("%s: MY_IOCTL_GET_BUFFER copy_to_user failed\n", MODULE_NAME);
            return -EFAULT;
        }
        strncpy(temp_buffer, data->buffer, 63);
        temp_buffer[63] = '\0';
        pr_info("%s: Buffer read via ioctl: %s%s\n", MODULE_NAME, temp_buffer,
                data->data_size > 63 ? "..." : "");
        break;
    case MY_IOCTL_DOWN:
        pr_info("%s: Process %d added to queue\n", MODULE_NAME, current->pid);
        data->is_blocked = 1;
        wait_event_interruptible(data->queue, !data->is_blocked);
        pr_info("%s: Process %d woken up\n", MODULE_NAME, current->pid);
        break;
    case MY_IOCTL_UP:
        pr_info("%s: Waking up all processes in queue\n", MODULE_NAME);
        data->is_blocked = 0;
        wake_up_interruptible(&data->queue);
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}

static const struct file_operations so2_fops = {
    .owner = THIS_MODULE,
    .open = so2_cdev_open,
    .release = so2_cdev_release,
    .read = so2_cdev_read,
    .write = so2_cdev_write,
    .unlocked_ioctl = so2_cdev_ioctl,
};

static int so2_cdev_init(void)
{
    int err;
    int i;

    err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS, MODULE_NAME);
    if (err) {
        pr_info("%s: register_chrdev_region failed\n", MODULE_NAME);
        return err;
    }
    pr_info("%s: Registered character device with major %d\n", MODULE_NAME, MY_MAJOR);

    for (i = 0; i < NUM_MINORS; i++) {
        init_waitqueue_head(&devs[i].queue);
        devs[i].is_blocked = 0;
        devs[i].data_size = 0; // Initialize with no data
        cdev_init(&devs[i].cdev, &so2_fops);
        devs[i].cdev.owner = THIS_MODULE;
        err = cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, MY_MINOR + i), 1);
        if (err) {
            pr_info("%s: cdev_add failed for minor %d\n", MODULE_NAME, i);
            goto cleanup;
        }
    }

    return 0;

cleanup:
    for (i = 0; i < NUM_MINORS; i++) {
        cdev_del(&devs[i].cdev);
    }
    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
    return err;
}

static void so2_cdev_exit(void)
{
    int i;

    for (i = 0; i < NUM_MINORS; i++) {
        cdev_del(&devs[i].cdev);
    }

    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
    pr_info("%s: Unregistered character device\n", MODULE_NAME);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
