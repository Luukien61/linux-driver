/*
 * PSO - Memory Mapping Lab
 *
 * Exercise #1: memory mapping using kmalloc'd kernel areas
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <linux/sched/mm.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/highmem.h>
#include <linux/rmap.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "../test/mmap-test.h"

MODULE_DESCRIPTION("simple mmap driver");
MODULE_AUTHOR("PSO");
MODULE_LICENSE("Dual BSD/GPL");

#define MY_MAJOR	42
/* how many pages do we actually kmalloc */
#define NPAGES		16
#define PROC_ENTRY_NAME "my-proc-entry" /* Synchronized with mmap-test.h */

/* character device basic structure */
static struct cdev mmap_cdev;

/* pointer to kmalloc'd area */
static void *kmalloc_ptr;

/* pointer to the kmalloc'd area, rounded up to a page boundary */
static char *kmalloc_area;

/* Function declarations */
static int my_open(struct inode *inode, struct file *filp);
static int my_release(struct inode *inode, struct file *filp);
static int my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static int my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset);
static int my_mmap(struct file *filp, struct vm_area_struct *vma);
static int my_seq_show(struct seq_file *seq, void *v);
static int my_seq_open(struct inode *inode, struct file *file);

/* File operations structure */
static const struct file_operations mmap_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .mmap = my_mmap,
    .read = my_read,
    .write = my_write
};

/* Function definitions */
static int my_open(struct inode *inode, struct file *filp)
{
    int i;

    /* Initialize each page with test values */
    for (i = 0; i < NPAGES; i++) {
        char *page = kmalloc_area + i * PAGE_SIZE;
        page[0] = 0xaa;
        page[1] = 0xbb;
        page[2] = 0xcc;
        page[3] = 0xdd;
    }

    return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static int my_read(struct file *file, char __user *user_buffer,
        size_t size, loff_t *offset)
{
    /* Reset offset to 0 to ensure reading from start */
    *offset = 0;

    /* Check size doesn't exceed our mapped area size */
    if (*offset + size > NPAGES * PAGE_SIZE)
        size = NPAGES * PAGE_SIZE - *offset;

    if (size <= 0)
        return 0;

    /* Ensure offset is valid */
    if (*offset < 0)
        return -EINVAL;

    /* Copy from mapped area to user buffer */
    if (copy_to_user(user_buffer, kmalloc_area + *offset, size)) {
        pr_err("copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += size;
    return size;
}

static int my_write(struct file *file, const char __user *user_buffer,
        size_t size, loff_t *offset)
{
    /* Check size doesn't exceed our mapped area size */
    if (*offset + size > NPAGES * PAGE_SIZE)
        size = NPAGES * PAGE_SIZE - *offset;

    if (size <= 0)
        return 0;

    /* Ensure offset is valid */
    if (*offset < 0)
        return -EINVAL;

    /* Copy from user buffer to mapped area */
    if (copy_from_user(kmalloc_area + *offset, user_buffer, size)) {
        pr_err("copy_from_user failed\n");
        return -EFAULT;
    }

    *offset += size;
    return size;
}

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret;
    long length = vma->vm_end - vma->vm_start;
    unsigned long pfn;

    /* Do not map more than we can */
    if (length > NPAGES * PAGE_SIZE)
        return -EIO;

    /* Map the whole physically contiguous area in one piece */
    pfn = virt_to_phys(kmalloc_area) >> PAGE_SHIFT;
    ret = remap_pfn_range(vma, vma->vm_start, pfn, length, vma->vm_page_prot);
    if (ret < 0) {
        pr_err("remap_pfn_range failed: %d\n");
        return ret;
    }

    return 0;
}

static int my_seq_show(struct seq_file *seq, void *v)
{
    struct mm_struct *mm;
    struct vm_area_struct *vma_iterator;
    unsigned long total = 0;

    /* Get current process' mm_struct */
    mm = get_task_mm(current);
    if (!mm)
        return -ENOMEM;

    /* Iterate through all memory mappings */
    down_read(&mm->mmap_lock); /* Updated for kernel >= 5.8 */
    for (vma_iterator = mm->mmap; vma_iterator; vma_iterator = vma_iterator->vm_next)
        total++;

    /* Release mm_struct */
    up_read(&mm->mmap_lock); /* Updated for kernel >= 5.8 */
    mmput(mm);

    /* Write only the number to match mmap-test.c parsing */
    seq_printf(seq, "%lu\n", total);
    return 0;
}

static int my_seq_open(struct inode *inode, struct file *file)
{
    /* Register the display function */
    return single_open(file, my_seq_show, NULL);
}

static const struct proc_ops my_proc_ops = {
    .proc_open    = my_seq_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init my_init(void)
{
    int ret = 0;
    int i;
    struct proc_dir_entry *proc_entry;

    /* Create a new entry in procfs */
    proc_entry = proc_create(PROC_ENTRY_NAME, 0444, NULL, &my_proc_ops);
    if (!proc_entry) {
        pr_err("could not create proc entry\n");
        return -ENOMEM;
    }

    ret = register_chrdev_region(MKDEV(MY_MAJOR, 0), 1, "mymap");
    if (ret < 0) {
        pr_err("could not register region\n");
        goto out_no_chrdev;
    }

    /* Allocate NPAGES+2 pages using kmalloc */
    kmalloc_ptr = kmalloc((NPAGES + 2) * PAGE_SIZE, GFP_KERNEL);
    if (!kmalloc_ptr) {
        pr_err("kmalloc failed\n");
        ret = -ENOMEM;
        goto out_unreg;
    }

    /* Round kmalloc_ptr to nearest page start address */
    kmalloc_area = (char *)PAGE_ALIGN((unsigned long)kmalloc_ptr);

    /* Mark pages as reserved */
    for (i = 0; i < NPAGES; i++)
        SetPageReserved(virt_to_page(kmalloc_area + i * PAGE_SIZE));

    /* Initialize each page with test values */
    for (i = 0; i < NPAGES; i++) {
        char *page = kmalloc_area + i * PAGE_SIZE;
        page[0] = 0xaa;
        page[1] = 0xbb;
        page[2] = 0xcc;
        page[3] = 0xdd;
    }

    /* Init device */
    cdev_init(&mmap_cdev, &mmap_fops);
    ret = cdev_add(&mmap_cdev, MKDEV(MY_MAJOR, 0), 1);
    if (ret < 0) {
        pr_err("could not add device\n");
        goto out_kfree;
    }

    return 0;

out_kfree:
    for (i = 0; i < NPAGES; i++)
        ClearPageReserved(virt_to_page(kmalloc_area + i * PAGE_SIZE));
    kfree(kmalloc_ptr);
out_unreg:
    unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1);
out_no_chrdev:
    remove_proc_entry(PROC_ENTRY_NAME, NULL);
out:
    return ret;
}

static void __exit my_exit(void)
{
    int i;

    cdev_del(&mmap_cdev);

    /* Clear reservation on pages and free mem */
    for (i = 0; i < NPAGES; i++)
        ClearPageReserved(virt_to_page(kmalloc_area + i * PAGE_SIZE));
    kfree(kmalloc_ptr);

    unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1);
    /* Remove proc entry */
    remove_proc_entry(PROC_ENTRY_NAME, NULL);
}

module_init(my_init);
module_exit(my_exit);