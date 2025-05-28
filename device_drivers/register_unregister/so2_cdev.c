/*
 * Character device drivers lab
 *
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/atomic.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_INFO

#define MY_MAJOR 42
#define MY_MINOR 0
#define NUM_MINORS 1
#define MODULE_NAME "so2_cdev"
#define MESSAGE "hello\n"
#define IOCTL_MESSAGE "Hello ioctl\n"

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

struct so2_device_data {
    struct cdev cdev;           // TODO 2: add cdev member
    char buffer[BUFSIZ];        // TODO 4: add buffer
    atomic_t access;            // TODO 3: add atomic_t access variable
    wait_queue_head_t queue;
    int is_blocked;
};

struct so2_device_data devs[NUM_MINORS];

static int so2_cdev_open(struct inode *inode, struct file *file)
{
    struct so2_device_data *data;
    /* TODO 2: print message when the device file is open. */
    pr_info("%s: Device opened\n", MODULE_NAME);

    /* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
    data = container_of(inode->i_cdev, struct so2_device_data, cdev);
    file->private_data = data;

    // TODO 3: Check if device is already opened
    if (atomic_cmpxchg(&data->access, 0, 1)) {
        pr_info("%s: Device already in use\n", MODULE_NAME);
        return -EBUSY;
    }

    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(10 * HZ);

    return 0;
}

static int so2_cdev_release(struct inode *inode, struct file *file)
{
    struct so2_device_data *data =
        (struct so2_device_data *) file->private_data;

    /* TODO 2: print message when the device file is closed. */
    pr_info("%s: Device closed\n", MODULE_NAME);

    // TODO 3: Reset access variable
    atomic_set(&data->access, 0);

    return 0;
}

static ssize_t so2_cdev_read(struct file *file,
                            char __user *user_buffer,
                            size_t size, loff_t *offset)
{
    struct so2_device_data *data =
        (struct so2_device_data *) file->private_data;
    size_t to_read;

	if (file->f_flags & O_NONBLOCK) {
            pr_info("%s: No data available, returning -EAGAIN\n", MODULE_NAME);
            return -EAGAIN;
        }


    /* TODO 4: Copy data->buffer to user_buffer, use copy_to_user */
    to_read = min(size, (size_t)(BUFSIZ - *offset));
    if (to_read <= 0)
        return 0;

    if (copy_to_user(user_buffer, data->buffer + *offset, to_read)) {
        pr_info("%s: copy_to_user failed\n", MODULE_NAME);
        return -EFAULT;
    }

    *offset += to_read;
    return to_read;
}

static ssize_t so2_cdev_write(struct file *file,
                             const char __user *user_buffer,
                             size_t size, loff_t *offset)
{
    struct so2_device_data *data =
        (struct so2_device_data *) file->private_data;

    /* TODO 5: copy user_buffer to data->buffer, use copy_from_user */
    size = min(size, (size_t)(BUFSIZ - *offset));
    if(size< 0){
        return 0;
    }
    if (copy_from_user(data->buffer + *offset, user_buffer, size)) {
        pr_info("%s: copy_from_user failed\n", MODULE_NAME);
        return -EFAULT;
    }

    *offset += size;
    return size;
}

static long so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct so2_device_data *data =
        (struct so2_device_data *) file->private_data;
    int ret = 0;
	char __user *user_buffer = NULL;

    /* TODO 6: if cmd = MY_IOCTL_PRINT, display IOCTL_MESSAGE */
    switch (cmd) {
    case MY_IOCTL_PRINT:
        pr_info("%s: %s", MODULE_NAME, IOCTL_MESSAGE);
        break;
    case MY_IOCTL_SET_BUFFER: {
        char userBuffer[BUFSIZ];
        if (copy_from_user(userBuffer, (void __user *)arg, sizeof(userBuffer))) {
            pr_info("%s: copy_from_user failed\n", MODULE_NAME);
            return -EFAULT;
        }
        strncpy(data->buffer, userBuffer, BUFSIZ);
        break;
    }

    case MY_IOCTL_GET_BUFFER: {
        if (copy_to_user((void __user *)arg, data->buffer, strlen(data->buffer)+1)) {
            pr_info("%s: copy_to_user failed\n", MODULE_NAME);
            return -EFAULT;
        }
        break;
    }
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
    .open = so2_cdev_open,      // TODO 2
    .release = so2_cdev_release, // TODO 2
    .read = so2_cdev_read,      // TODO 4
    .write = so2_cdev_write,    // TODO 5
    .unlocked_ioctl = so2_cdev_ioctl, // TODO 6
};

static int so2_cdev_init(void)
{
    int err;
    int i;

    // TODO 1: Register char device region
    err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS, MODULE_NAME);
    if (err!=0) {
        pr_info("%s: register_chrdev_region failed\n", MODULE_NAME);
        return err;
    }
        pr_info("%s: Registered character device with major %d\n", MODULE_NAME, MY_MAJOR);

    for (i = 0; i < NUM_MINORS; i++) {

        init_waitqueue_head(&devs[i].queue);
        devs[i].is_blocked = 0;
        // TODO 4: Initialize buffer
        strncpy(devs[i].buffer, MESSAGE, BUFSIZ);

        // TODO 3: Initialize access variable
        atomic_set(&devs[i].access, 0);

        // TODO 2: Initialize and add cdev
        cdev_init(&devs[i].cdev, &so2_fops);

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

    // TODO 2: Delete cdevs
    for (i = 0; i < NUM_MINORS; i++) {
        cdev_del(&devs[i].cdev);
    }

    // TODO 1: Unregister char device region
    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
    pr_info("%s: Unregistered character device\n", MODULE_NAME);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);