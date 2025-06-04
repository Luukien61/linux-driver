/*
 * Kernel API lab
 * 
 * mem.c - Memory allocation in Linux
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ctype.h>

MODULE_DESCRIPTION("Print memory");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

static char *mem; // 1 byte

static int mem_init(void)
{
	size_t i;

	mem = kmalloc(4096 * sizeof(*mem), GFP_KERNEL); // trả về con trỏ trỏ tới khối bộ nhớ đã được cấp phát
	if (mem == NULL)
		goto err_mem;
	strncpy(mem, "kien", sizeof("kien"));

	pr_info("chars: ");
	for (i = 0; mem[i] != '\0'; i++) {
		if (isalpha(mem[i]))
			printk("%c ", mem[i]);
	}
	pr_info("\n");

	return 0;

err_mem:
	return -1;
}

static void mem_exit(void)
{
	kfree(mem);
}

module_init(mem_init);
module_exit(mem_exit);
