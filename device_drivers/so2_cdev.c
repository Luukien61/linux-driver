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
/* struct lưu trữ data của device, đọc thông qua cat (read)
struct cdev là một cấu trúc được định nghĩa trong kernel Linux để quản lý một thiết bị ký tự trong kernel.
Nó cung cấp một giao diện để liên kết thiết bị ký tự với các hàm xử lý (file operations)
struct cdev được sử dụng để:
Đăng ký thiết bị ký tự với kernel.
Liên kết thiết bị với các hàm như open, read, write, ioctl, v.v., thông qua cấu trúc struct file_operations.
*/

struct so2_device_data devs[NUM_MINORS];

static int so2_cdev_open(struct inode *inode, struct file *file)
/*
Hàm open được gọi khi một tiến trình người dùng mở device, cat /dev/so2_cdev (read)
*/
{
    struct so2_device_data *data;
    /* TODO 2: print message when the device file is open. */
    pr_info("%s: Device opened\n", MODULE_NAME);

    /* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
    data = container_of(inode->i_cdev, struct so2_device_data, cdev);
    /*Macro container_of được sử dụng để lấy con trỏ đến cấu trúc cha (struct so2_device_data) từ con trỏ đến thành viên cdev.*/
    file->private_data = data;
    //struct file, được kernel cung cấp để lưu trữ thông tin riêng của thiết bị.

    // TODO 3: Check if device is already opened
    if (atomic_cmpxchg(&data->access, 0, 1)) {
        pr_info("%s: Device already in use\n", MODULE_NAME);
        return -EBUSY;
    }

    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(3 * HZ); //Yêu cầu kernel tạm dừng tiến trình hiện tại trong 3s,  HZ là hằng số kernel, biểu thị số jiffies trong một giây

    return 0;
}

static int so2_cdev_release(struct inode *inode, struct file *file)
// Hàm so2_cdev_release là hàm xử lý cho thao tác đóng thiết bị, hoàn thành lệnh cat
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
    struct so2_device_data *data = (struct so2_device_data *)file->private_data;
    size_t to_read;

    /* Kiểm tra chế độ không chặn (non-blocking) */
    if (file->f_flags & O_NONBLOCK) {
        /* Nếu không có dữ liệu sẵn sàng, trả về -EWOULDBLOCK */
        if (data->is_blocked) {
            pr_info("%s: No data available, returning -EWOULDBLOCK\n", MODULE_NAME);
            return -EWOULDBLOCK;
        }
    } else {
        /* Chế độ chặn (blocking): chờ dữ liệu sẵn sàng
        Hàm macro wait_event_interruptible là một cơ chế trong kernel Linux dùng để đưa tiến trình hiện tại
        vào trạng thái chờ (sleep) cho đến khi một điều kiện cụ thể được thỏa mãn*/
        wait_event_interruptible(data->queue, !data->is_blocked);
    }

    /* Tính số byte có thể đọc */
    to_read = min(size, (size_t)(BUFSIZ - *offset));
    if (to_read <= 0) {
        return 0;
    }

    /* Sao chép dữ liệu từ data->buffer sang user_buffer */
    if (copy_to_user(user_buffer, data->buffer + *offset, to_read)) {
    /*copy_to_user là hàm an toàn để chuyển dữ liệu từ không gian kernel sang không gian người dùng
    Nếu copy_to_user thất bại (trả về số byte không sao chép được)*/
        pr_info("%s: copy_to_user failed\n", MODULE_NAME);
        return -EFAULT;
    }

    /* Cập nhật offset và trả về số byte đã đọc */
    *offset += to_read;
    return to_read;
}

static ssize_t so2_cdev_write(struct file *file,
                             const char __user *user_buffer,
                             size_t size, loff_t *offset)
// được gọi khi hệ thống gọi write(), user space gọi echo > /dev/...
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
    data->is_blocked = 0; // báo hiệu rằng data đã sẵn sàng
    return size;
}

static long so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
// được gọi khi một tiến trình người dùng thực hiện hệ thống gọi ioctl() trên device node
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
    /*
    int register_chrdev_region(dev_t first, unsigned int count, const char *name);
    Hàm MKDEV là một macro trong kernel Linux, dùng để tạo một giá trị dev_t (kiểu dữ liệu biểu diễn số hiệu thiết bị) từ hai tham số:
    MY_MAJOR: Số chính (major number) xác định loại thiết bị hoặc driver.
    MY_MINOR: Số phụ (minor number) xác định thiết bị cụ thể trong cùng một driver.
    Kết quả của MKDEV(MY_MAJOR, MY_MINOR) là số hiệu thiết bị bắt đầu (first device number) cho phạm vi bạn muốn đăng ký.
    */
    if (err!=0) {
        pr_info("%s: register_chrdev_region failed\n", MODULE_NAME);
        return err;
    }
        pr_info("%s: Registered character device with major %d\n", MODULE_NAME, MY_MAJOR);

    for (i = 0; i < NUM_MINORS; i++) {

        init_waitqueue_head(&devs[i].queue);
        devs[i].is_blocked = 1;
        // TODO 4: Initialize buffer
        //strncpy(devs[i].buffer, MESSAGE, BUFSIZ);

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