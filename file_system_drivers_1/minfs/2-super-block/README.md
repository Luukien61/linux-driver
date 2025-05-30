# minfs TODO 2: Completing minfs Superblock

## 📋 Mục tiêu
Hoàn thành việc đọc và khởi tạo superblock cho minfs filesystem để có thể mount được filesystem.

## 🎯 Yêu cầu
- Đọc superblock từ disk (block đầu tiên - index 0)
- Validate magic number để đảm bảo đúng filesystem type
- Khởi tạo VFS superblock structure
- Tạo root inode sử dụng `myfs_get_inode` (temporary solution)
- Filesystem có thể mount được nhưng chưa thể thực hiện các thao tác khác (đây là điều bình thường)

## 🔧 Build và Setup

### Build kernel module:
```bash
# Trong thư mục chứa source code
make build

# Kiểm tra file .ko đã được tạo
ls -la /home/root/skels/filesystems/minfs/kernel/minfs.ko
```

### Tạo filesystem trên device:
```bash
# Sử dụng mkfs.minfs để format device
/home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc

# mkfs.minfs sẽ:
# - Tạo superblock ở block 0
# - Tạo inode table ở block 1  
# - Khởi tạo root directory
# - Set up filesystem structures
```

## 💻 Implementation Chi Tiết

### 1. Forward Declaration
```c
/* Forward declaration for myfs_get_inode */
static struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode);
```
**Giải thích:**
- Cần declare function trước khi sử dụng trong C
- `myfs_get_inode` được định nghĩa ở cuối file nhưng sử dụng trong `minfs_fill_super`
- `static` có nghĩa function chỉ visible trong file này

### 2. Đọc Superblock từ Disk
```c
/* TODO 2: Read block with superblock. It's the first block on
 * the device, i.e. the block with the index 0. This is the index
 * to be passed to sb_bread().
 */
bh = sb_bread(s, 0);
if (!bh) {
    printk(LOG_LEVEL "unable to read superblock\n");
    goto out_bad_sb;
}
```
**Giải thích từng dòng:**
- `sb_bread(s, 0)`: Đọc block 0 từ device được mount
    - `s`: pointer đến VFS superblock structure
    - `0`: block number (superblock luôn ở block đầu tiên)
    - Trả về `buffer_head` pointer hoặc NULL nếu lỗi
- `if (!bh)`: Kiểm tra lỗi đọc
- `printk()`: In thông báo lỗi vào kernel log (dmesg)
- `goto out_bad_sb`: Jump đến error handling code để cleanup

### 3. Parse và Validate Superblock
```c
/* TODO 2: interpret read data as minfs_super_block */
ms = (struct minfs_super_block *)bh->b_data;

/* TODO 2: check magic number with value defined in minfs.h */
if (ms->magic != MINFS_MAGIC) {
    printk(LOG_LEVEL "wrong magic number\n");
    goto out_bad_magic;
}
```
**Giải thích chi tiết:**
- `bh->b_data`: Raw data của block đã đọc (void pointer)
- `(struct minfs_super_block *)`: Cast sang struct để access các fields
- `ms->magic`: Access magic number field từ superblock
- `MINFS_MAGIC`: Constant được định nghĩa trong `minfs.h` (ví dụ: 0x4d494e46)
- Magic number validation đảm bảo device thực sự chứa minfs filesystem

### 4. Khởi tạo VFS Superblock
```c
/* TODO 2: fill super_block with magic_number, super_operations */
s->s_magic = MINFS_MAGIC;
s->s_op = &minfs_ops;
```
**Giải thích:**
- `s->s_magic`: Set magic number cho VFS superblock
- `s->s_op`: Gán pointer đến operations structure
- `minfs_ops`: Đã được định nghĩa trước đó, chứa các function pointers:
  ```c
  static const struct super_operations minfs_ops = {
      .statfs     = simple_statfs,
      .put_super  = minfs_put_super,
      // TODO 3 sẽ thêm alloc_inode và destroy_inode
  };
  ```

### 5. Lưu Filesystem-specific Information
```c
/* TODO 2: Fill sbi with rest of information from disk superblock
 * (i.e. version).
 */
sbi->version = ms->version;
sbi->imap = ms->imap;
```
**Giải thích:**
- `sbi`: `minfs_sb_info` structure - chứa thông tin riêng của minfs
- `ms->version`: Version của filesystem từ disk
- `ms->imap`: Inode bitmap từ disk - theo dõi inode nào đã được sử dụng
- Thông tin này không có trong VFS superblock nên phải lưu riêng

### 6. Tạo Root Inode
```c
/* TODO 2: use myfs_get_inode instead of minfs_iget */
root_inode = myfs_get_inode(s, NULL, S_IFDIR | 0755);
if (!root_inode)
    goto out_bad_inode;

root_dentry = d_make_root(root_inode);
if (!root_dentry)
    goto out_iput;
s->s_root = root_dentry;
```
**Giải thích từng bước:**
- `myfs_get_inode(s, NULL, S_IFDIR | 0755)`:
    - `s`: superblock
    - `NULL`: parent inode (root không có parent)
    - `S_IFDIR | 0755`: file type (directory) + permissions (rwxr-xr-x)
- `d_make_root(root_inode)`: Tạo root dentry từ inode
- `s->s_root = root_dentry`: Set root dentry cho filesystem

### 7. myfs_get_inode Implementation (Temporary)
```c
static struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode)
{
    struct inode *inode = new_inode(sb);
    
    if (!inode)
        return NULL;

    inode->i_ino = get_next_ino();
    inode_init_owner(inode, dir, mode);
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

    switch (mode & S_IFMT) {
    case S_IFDIR:
        inode->i_op = &simple_dir_inode_operations;
        inode->i_fop = &simple_dir_operations;
        inc_nlink(inode);
        break;
    case S_IFREG:
        inode->i_op = &simple_file_inode_operations;
        inode->i_fop = &simple_file_operations;
        break;
    }

    return inode;
}
```
**Giải thích từng phần:**
- `new_inode(sb)`: Allocate VFS inode structure
- `get_next_ino()`: Tạo unique inode number (temporary)
- `inode_init_owner()`: Set owner, group, permissions
- `current_time()`: Set timestamps
- `mode & S_IFMT`: Extract file type từ mode
- `simple_dir_*`: Use kernel's simple filesystem operations (temporary)
- `inc_nlink()`: Tăng link count cho directory (directories start with 2 links)

## 🏗️ Structures và Constants

### minfs_super_block (On-disk format)
```c
struct minfs_super_block {
    __u32 magic;           // 0x4d494e46 - "MINF" 
    __u8 version;          // Filesystem version
    unsigned long imap;    // Inode bitmap
    // Được đọc từ block 0 của device
};
```

### minfs_sb_info (In-memory format)
```c
struct minfs_sb_info {
    __u8 version;               // Copy từ disk
    unsigned long imap;         // Copy từ disk
    struct buffer_head *sbh;    // Reference đến superblock buffer
    // Được lưu trong s->s_fs_info
};
```

### Constants trong minfs.h
```c
#define MINFS_MAGIC     0x4d494e46  // Magic number
#define MINFS_BLOCK_SIZE    4096    // Block size
#define MINFS_ROOT_INODE    0       // Root inode number
```

## 🧪 Testing Chi Tiết

### Step 1: Load Module
```bash
# Load kernel module
root@qemux86:~# insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko

# Kiểm tra module đã load
root@qemux86:~# lsmod | grep minfs
minfs                  16384  0

# Kiểm tra kernel messages
root@qemux86:~# dmesg | tail -5
[ 1234.567890] minfs: loading out-of-tree module taints kernel.
```

### Step 2: Check Filesystem Registration
```bash
# Kiểm tra filesystem đã được register
root@qemux86:~# cat /proc/filesystems | grep minfs
        minfs

# minfs xuất hiện trong danh sách -> registration thành công
```

### Step 3: Create Filesystem
```bash
# Format device với minfs
root@qemux86:~# /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc

# mkfs.minfs sẽ:
# 1. Write superblock to block 0
# 2. Initialize inode table at block 1  
# 3. Create root directory structure
```

### Step 4: Mount Filesystem
```bash
# Tạo mount point
root@qemux86:~# mkdir -p /mnt/minfs

# Mount filesystem
root@qemux86:~# mount -t minfs /dev/vdc /mnt/minfs

# Không có error message = mount thành công!
```

### Step 5: Verify Mount
```bash
# Kiểm tra mount
root@qemux86:~# mount | grep minfs
/dev/vdc on /mnt/minfs type minfs (rw,relatime)

# Kiểm tra disk usage
root@qemux86:~# df /mnt/minfs
Filesystem     1K-blocks  Used Available Use% Mounted on
/dev/vdc            1024     0      1024   0% /mnt/minfs
```

## ✅ Kết Quả Mong Đợi

### Thành công:
- ✅ **Mount successful**: Không có error message
- ✅ **Filesystem registered**: Xuất hiện trong `/proc/filesystems`
- ✅ **Superblock validation**: Magic number được kiểm tra đúng
- ✅ **Root inode created**: Filesystem có root directory

### Sẽ lỗi (đây là bình thường):
- ❌ **`ls /mnt/minfs`**: Lỗi vì `minfs_readdir` chưa implement (TODO 5)
- ❌ **`touch /mnt/minfs/file`**: Lỗi vì `minfs_create` chưa implement (TODO 7)
- ❌ **`mkdir /mnt/minfs/dir`**: Lỗi vì directory operations chưa hoàn chỉnh

### Test commands sẽ lỗi:
```bash
# Sẽ báo lỗi hoặc không hiển thị gì
root@qemux86:~# ls /mnt/minfs
ls: /mnt/minfs: Function not implemented

# Sẽ báo lỗi
root@qemux86:~# touch /mnt/minfs/testfile  
touch: /mnt/minfs/testfile: Function not implemented
```

## 🔧 Debugging Tips

### Kiểm tra kernel logs:
```bash
# Xem kernel messages
dmesg | grep minfs

# Xem live kernel messages  
dmesg -w
```

### Common errors và solutions:
1. **"wrong magic number"**: Device chưa được format với mkfs.minfs
2. **"unable to read superblock"**: Device không tồn tại hoặc permission issue
3. **"bad inode"**: myfs_get_inode function có lỗi

## 📁 Files Modified

### minfs.c changes:
1. **Added forward declaration** cho `myfs_get_inode`
2. **Completed TODO 2** trong `minfs_fill_super()`:
    - Đọc superblock từ disk
    - Validate magic number
    - Initialize VFS superblock
    - Copy filesystem-specific info
3. **Added myfs_get_inode()** function (temporary)

### Build artifacts:
- `minfs.ko`: Kernel module file
- `Module.symvers`: Symbol version info
- `modules.order`: Module dependency order

## 🚀 Next Steps

Sau khi hoàn thành TODO 2, các TODO tiếp theo sẽ làm:

- **TODO 3**: Implement `alloc_inode` và `destroy_inode` trong `minfs_ops`
- **TODO 4**: Hoàn thiện `minfs_iget` để đọc inode từ disk thay vì tạo in-memory
- **TODO 5**: Implement `minfs_readdir` để `ls` command hoạt động
- **TODO 6**: Implement `minfs_lookup` để tìm kiếm files
- **TODO 7**: Implement file creation (`minfs_create`) để `touch` command hoạt động

## 📝 Notes Quan Trọng

- **`myfs_get_inode` chỉ là temporary**: Tạo in-memory inode, không đọc từ disk
- **Simple operations được sử dụng**: `simple_dir_operations`, `simple_file_operations`
- **Error handling đầy đủ**: Mọi lỗi đều được handle với goto labels
- **Buffer management**: `bh` được release trong error paths
- **Superblock buffer được lưu**: `sbi->sbh` để sử dụng sau này