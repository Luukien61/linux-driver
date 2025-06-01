/*
 * SO2 Lab - Filesystem drivers
 * Exercise #1 (no-dev filesystem)
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

MODULE_DESCRIPTION("Simple no-dev filesystem");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define MYFS_BLOCKSIZE		4096
#define MYFS_BLOCKSIZE_BITS	12
#define MYFS_MAGIC		0xbeefcafe
#define LOG_LEVEL		KERN_ALERT

/* declarations of functions that are part of operation structures */
static int myfs_mknod(struct inode *dir,
		struct dentry *dentry, umode_t mode, dev_t dev);
static int myfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl);
static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

/* TODO 2: define super_operations structure */
static const struct super_operations myfs_super_ops = {
	.drop_inode = generic_drop_inode,
	.statfs = simple_statfs,
};

/* TODO 5: Fill dir inode operations structure. */
static const struct inode_operations myfs_dir_inode_operations = {
	.create = myfs_create,           // Tạo tệp thông thường
	.lookup = simple_lookup,         // Tìm kiếm mục trong thư mục
	.link = simple_link,             // Tạo liên kết cứng
	.unlink = simple_unlink,         // Xóa tệp
	.mkdir = myfs_mkdir,             // Tạo thư mục
	.rmdir = simple_rmdir,           // Xóa thư mục
	.mknod = myfs_mknod,             // Tạo node (tệp, thư mục, hoặc thiết bị)
	.rename = simple_rename,         // Đổi tên
};

static const struct file_operations myfs_file_operations = {
	/* TODO 6: Fill file operations structure. */
	/* Minimum operations for regular files */
};

static const struct inode_operations myfs_file_inode_operations = {
	/* TODO 6: Fill file inode operations structure. */
	/* Minimum operations for regular files */
};

static const struct address_space_operations myfs_aops = {
	/* TODO 6: Fill address space operations structure. */
};

struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir,
		int mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	/* TODO 3: fill inode structure
	 *     - mode
	 *     - uid
	 *     - gid
	 *     - atime,ctime,mtime
	 *     - ino
	 */
	inode_init_owner(inode, dir, mode); /* Khởi tạo uid, gid, mode */
	inode->i_atime = inode->i_ctime = inode->i_mtime = current_time(inode); // Thời gian
	inode->i_ino = get_next_ino(); // Số inode

	/* TODO 5: Init i_ino using get_next_ino */

	/* TODO 6: Initialize address space operations. */

	if (S_ISDIR(mode)) {
		/* TODO 3: set inode operations for dir inodes. */
		inode->i_op = &myfs_dir_inode_operations; // Thao tác inode thư mục
		inode->i_fop = &simple_dir_operations; // Thao tác tệp thư mục
		/* TODO 3: directory inodes start off with i_nlink == 2 (for "." entry).
		 * Directory link count should be incremented (use inc_nlink).
		 */
		set_nlink(inode, 2); // i_nlink = 2 cho . và ..
	} else if (S_ISREG(mode)) {
		/* TODO 6: Set file inode and file operations for regular files
		 * (use the S_ISREG macro).
		 */
		inode->i_op = &myfs_file_inode_operations; // Thao tác inode cho tệp
		inode->i_fop = &myfs_file_operations; // Thao tác tệp cho tệp
		set_nlink(inode, 1); // i_nlink = 1 cho tệp thông thường
	}

	return inode;
}

/* TODO 5: Implement myfs_mknod, myfs_create, myfs_mkdir. */
static int myfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode *inode = myfs_get_inode(dir->i_sb, dir, mode);
	int error = -ENOMEM;

	if (!inode)
		goto out;

	d_instantiate(dentry, inode);
	dget(dentry); /* Tăng tham chiếu cho dentry */
	inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
	if (S_ISDIR(mode))
		inc_nlink(dir); /* Tăng số liên kết của thư mục cha nếu tạo thư mục */

	return 0;

out:
	return error;
}

static int myfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	return myfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int ret;

	ret = myfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (ret)
		return ret;

	inc_nlink(dir); /* Tăng số liên kết của thư mục cha */

	return 0;
}

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct dentry *root_dentry;

	/* TODO 2: fill super_block
	 *   - blocksize, blocksize_bits
	 *   - magic
	 *   - super operations
	 *   - maxbytes
	 */
	sb->s_blocksize = MYFS_BLOCKSIZE;
	sb->s_blocksize_bits = MYFS_BLOCKSIZE_BITS;
	sb->s_magic = MYFS_MAGIC;
	sb->s_op = &myfs_super_ops;
	sb->s_maxbytes = MAX_LFS_FILESIZE;

	/* mode = directory & access rights (755) */
	root_inode = myfs_get_inode(sb, NULL,
			S_IFDIR | S_IRWXU | S_IRGRP |
			S_IXGRP | S_IROTH | S_IXOTH);

	printk(LOG_LEVEL "root inode has %d link(s)\n", root_inode->i_nlink);

	if (!root_inode)
		return -ENOMEM;

	root_dentry = d_make_root(root_inode);
	if (!root_dentry)
		goto out_no_root;
	sb->s_root = root_dentry;

	return 0;

out_no_root:
	iput(root_inode);
	return -ENOMEM;
}

static struct dentry *myfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	/* TODO 1: call superblock mount function */
	return mount_nodev(fs_type, flags, data, myfs_fill_super);
}

/* TODO 1: define file_system_type structure */
static struct file_system_type myfs_fs_type = {
	.owner = THIS_MODULE,
	.name = "myfs",
	.mount = myfs_mount,
	.kill_sb = kill_litter_super,
};

static int __init myfs_init(void)
{
	int err = 0; /* Sửa lỗi: khởi tạo err */

	/* TODO 1: register */
	err = register_filesystem(&myfs_fs_type);
	if (err) {
		printk(LOG_LEVEL "register_filesystem failed\n");
		return err;
	}

	return 0;
}

static void __exit myfs_exit(void)
{
	/* TODO 1: unregister */
	unregister_filesystem(&myfs_fs_type);
}

module_init(myfs_init);
module_exit(myfs_exit);