#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched/task.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include "../include/deferred.h"

#define MY_MAJOR		42
#define MY_MINOR		0
#define MODULE_NAME		"deferred"

#define TIMER_TYPE_NONE	-1
#define TIMER_TYPE_SET	0
#define TIMER_TYPE_ALLOC	1
#define TIMER_TYPE_MON	2

MODULE_DESCRIPTION("Deferred work character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct mon_proc {
        struct task_struct *task;
        struct list_head list;
};

static struct my_device_data {
        struct cdev cdev;
        /* TODO 1: add timer */
        struct timer_list timer;
        /* TODO 2: add flag */
        int timer_type;
        /* TODO 3: add work (skip for now) */
        /* TODO 4: add list for monitored processes (skip for now) */
        /* TODO 4: add spinlock to protect list (skip for now) */
} dev;

static void alloc_io(void)
{
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(5 * HZ);
        pr_info("Yawn! I've been sleeping for 5 seconds.\n");
}

static struct mon_proc *get_proc(pid_t pid)
{
        struct task_struct *task;
        struct mon_proc *p;

        rcu_read_lock();
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
        rcu_read_unlock();
        if (!task)
                return ERR_PTR(-ESRCH);

        p = kmalloc(sizeof(*p), GFP_ATOMIC);
        if (!p)
                return ERR_PTR(-ENOMEM);

        get_task_struct(task);
        p->task = task;

        return p;
}

/* TODO 3: define work handler (skip for now) */

#define ALLOC_IO_DIRECT
/* TODO 3: undef ALLOC_IO_DIRECT (skip for now) */

static void timer_handler(struct timer_list *tl)
{
        /* TODO 1: implement timer handler */
        struct my_device_data *my_data = container_of(tl, struct my_device_data, timer);
        /* TODO 2: check flags: TIMER_TYPE_SET or TIMER_TYPE_ALLOC */
        if (!my_data) {
                pr_err("Error: my_data is NULL in timer_handler\n");
                return;
        }
        if (my_data->timer_type == TIMER_TYPE_SET) {
                pr_info("Timer fired: PID=%d, Process=%s\n", current->pid, current->comm);
        }
        /* TODO 4: iterate the list and check the proccess state (skip for now) */
        else if (my_data->timer_type == TIMER_TYPE_ALLOC) {
                /* TODO 3: schedule work (skip for now) */
                /* Trong Bài 4, gọi alloc_io() trực tiếp để gây lỗi blocking */
                alloc_io();
        }
}

static int deferred_open(struct inode *inode, struct file *file)
{
        struct my_device_data *my_data =
                container_of(inode->i_cdev, struct my_device_data, cdev);
        file->private_data = my_data;
        pr_info("[deferred_open] Device opened\n");
        return 0;
}

static int deferred_release(struct inode *inode, struct file *file)
{
        pr_info("[deferred_release] Device released\n");
        return 0;
}

static long deferred_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        struct my_device_data *my_data = (struct my_device_data*) file->private_data;

        pr_info("[deferred_ioctl] Command: %s\n", ioctl_command_to_string(cmd));

        switch (cmd) {
        case MY_IOCTL_TIMER_SET:
                /* TODO 2: set flag */
                my_data->timer_type = TIMER_TYPE_SET;
                /* TODO 1: schedule timer */
                mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(arg * 1000));
                break;
        case MY_IOCTL_TIMER_CANCEL:
                /* TODO 1: cancel timer */
                del_timer_sync(&my_data->timer);
                break;
        case MY_IOCTL_TIMER_ALLOC:
                /* TODO 2: set flag and schedule timer */
                my_data->timer_type = TIMER_TYPE_ALLOC;
                mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(arg * 1000));
                break;
        case MY_IOCTL_TIMER_MON:
                /* TODO 4: use get_proc() and add task to list (skip for now) */
                break;
        default:
                return -ENOTTY;
        }
        return 0;
}

struct file_operations my_fops = {
        .owner = THIS_MODULE,
        .open = deferred_open,
        .release = deferred_release,
        .unlocked_ioctl = deferred_ioctl,
};

static int deferred_init(void)
{
        int err;

        pr_info("[deferred_init] Init module\n");
        err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME);
        if (err) {
                pr_info("[deferred_init] register_chrdev_region: %d\n", err);
                return err;
        }

        /* TODO 2: Initialize flag. */
        dev.timer_type = TIMER_TYPE_NONE;
        /* TODO 3: Initialize work. (skip for now) */

        /* TODO 4: Initialize lock and list. (skip for now) */

        cdev_init(&dev.cdev, &my_fops);
        cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);

        /* TODO 1: Initialize timer. */
        timer_setup(&dev.timer, timer_handler, 0);

        return 0;
}

static void deferred_exit(void)
{
        struct mon_proc *p, *n;

        pr_info("[deferred_exit] Exit module\n" );

        cdev_del(&dev.cdev);
        unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);

        /* TODO 1: Cleanup: make sure the timer is not running after exiting. */
        del_timer_sync(&dev.timer);
        /* TODO 3: Cleanup: make sure the work handler is not scheduled. (skip for now) */

        /* TODO 4: Cleanup the monitored process list (skip for now) */
}

module_init(deferred_init);
module_exit(deferred_exit);