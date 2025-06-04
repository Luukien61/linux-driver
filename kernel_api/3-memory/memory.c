/*
 * SO2 lab3 - task 3
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 1: allocated and initialize a task_info struct */
	ti = kmalloc(sizeof(struct task_info), GFP_KERNEL);
	if (!ti)
		return NULL;

	ti->pid = pid;
	ti->timestamp = jiffies; // số s từ khi hệ thống khởi động

	return ti;
}

static int memory_init(void)
{
	struct task_struct *next_task_ptr, *next_next_task_ptr;

	/* TODO 2: call task_info_alloc for current pid */
	ti1 = task_info_alloc(current->pid); // Đây là một macro trong kernel Linux, trả về con trỏ tới cấu trúc `task_struct` của tiến trình hiện tại (tiến trình đang thực thi module này).
	if (!ti1) {
		printk(KERN_ERR "Failed to allocate ti1\n");
		return -ENOMEM;
	}

	/* TODO 2: call task_info_alloc for parent PID */
	ti2 = task_info_alloc(current->parent->pid); // when imsmod or run in a terminal(bash), the shell process will be the parent
	if (!ti2) {
		printk(KERN_ERR "Failed to allocate ti2\n");
		kfree(ti1);
		return -ENOMEM;
	}

	/* TODO 2: call task_info alloc for next process PID */
	next_task_ptr = next_task(current);
	ti3 = task_info_alloc(next_task_ptr->pid); // pid 0 for idle process
	if (!ti3) {
		printk(KERN_ERR "Failed to allocate ti3\n");
		kfree(ti1);
		kfree(ti2);
		return -ENOMEM;
	}

	/* TODO 2: call task_info_alloc for next process of the next process */
	next_next_task_ptr = next_task(next_task_ptr);
	ti4 = task_info_alloc(next_next_task_ptr->pid); // pid is 1 , init task, it's understandable because the task list is Traversal (vòng). so when insmmod may be the last task of the list, the next one is idle and the next one is init
	if (!ti4) {
		printk(KERN_ERR "Failed to allocate ti4\n");
		kfree(ti1);
		kfree(ti2);
		kfree(ti3);
		return -ENOMEM;
	}

	printk(KERN_INFO "Memory module loaded successfully\n");
	return 0;
}

static void memory_exit(void)
{
	/* TODO 3: print ti* field values */
	if (ti1)
		printk(KERN_INFO "ti1: pid=%d, timestamp=%lu\n", ti1->pid, ti1->timestamp);
	if (ti2)
		printk(KERN_INFO "ti2: pid=%d, timestamp=%lu\n", ti2->pid, ti2->timestamp);
	if (ti3)
		printk(KERN_INFO "ti3: pid=%d, timestamp=%lu\n", ti3->pid, ti3->timestamp);
	if (ti4)
		printk(KERN_INFO "ti4: pid=%d, timestamp=%lu\n", ti4->pid, ti4->timestamp);

	/* TODO 4: free ti* structures */
	kfree(ti1);
	kfree(ti2);
	kfree(ti3);
	kfree(ti4);

	printk(KERN_INFO "Memory module unloaded\n");
}

module_init(memory_init);
module_exit(memory_exit);