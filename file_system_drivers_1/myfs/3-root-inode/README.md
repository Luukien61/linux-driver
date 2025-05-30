# chạy trong qemu:
    insmod /home/root/skels/filesystems/myfs/myfs.ko
# Kết quả:
    root@qemux86:~# insmod /home/root/skels/filesystems/myfs/myfs.ko                
    myfs: loading out-of-tree module taints kernel.                                 
    root@qemux86:~# mkdir -p /mnt/myfs                                              
    root@qemux86:~# mount -t myfs none /mnt/myfs                                    
    root inode has 2 link(s)                                                        
    root@qemux86:~# cat /proc/mounts                                                
    //10.0.2.1/rootfs / cifs rw,relatime,vers=1.0,cache=strict,username=dummy,uid=00
    proc /proc proc rw,relatime 0 0                                                 
    sysfs /sys sysfs rw,relatime 0 0                                                
    debugfs /sys/kernel/debug debugfs rw,relatime 0 0                               
    configfs /sys/kernel/config configfs rw,relatime 0 0                            
    devtmpfs /dev devtmpfs rw,relatime,size=247876k,nr_inodes=61969,mode=755 0 0    
    tmpfs /run tmpfs rw,nosuid,nodev,mode=755 0 0                                   
    tmpfs /var/volatile tmpfs rw,relatime 0 0                                       
    //10.0.2.1/skels /home/root/skels cifs rw,relatime,vers=3.1.1,cache=strict,user0
    devpts /dev/pts devpts rw,relatime,gid=5,mode=620,ptmxmode=000 0 0              
    none /mnt/myfs myfs rw,relatime 0 0                                             
    root@qemux86:~# random: crng init done
    
    root@qemux86:~# ls -di /mnt/myfs                                                
    4710 /mnt/myfs                                                               
    root@qemux86:~# 

### **README - Cách làm Bài 3 MyFS **

### Mô tả
Bài 3 yêu cầu hoàn thiện hàm `myfs_get_inode` trong tệp `/linux/tools/labs/skels/filesystems/myfs/myfs.c` 
để khởi tạo **root inode** (inode của thư mục gốc `/`) cho hệ thống tệp `myfs`. 
Root inode được tạo khi hệ thống tệp được gắn (mount) thông qua `myfs_fill_super`. Bài này thuộc **TODO 3**, tập trung vào:

- Khởi tạo các trường inode: `mode`, `uid`, `gid`, `atime`, `ctime`, `mtime`, `ino`.
- Xử lý inode loại thư mục (directory) bằng cách gán thao tác inode (`i_op`), thao tác tệp (`i_fop`), và đặt số liên kết (`i_nlink = 2`).

Mã nguồn xây dựng trên bài 1 (đăng ký hệ thống tệp) và bài 2 (điền superblock), cho phép mount hệ thống tệp thành công.

### Bước 1: Lưu mã nguồn
1. Mở tệp `myfs.c`:
   ```bash
   vi /linux/tools/labs/skels/filesystems/myfs/myfs.c
   ```
   
### Bước 2: Biên dịch module
1. Xóa tệp biên dịch cũ:
   ```bash
   cd /linux/tools/labs/skels
   make clean
   ```
2. Biên dịch:
   ```bash
   make build
   ```
3. Kiểm tra tệp `myfs.ko`:
   ```bash
   ls -l /linux/tools/labs/skels/filesystems/myfs/*.ko
   ```

### Bước 3: Kiểm tra
1. Chạy VM:
   ```bash
   cd /linux/tools/labs
   make console
   ```
2. Nạp module:
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   ```
3. Gắn hệ thống tệp:
   ```bash
   mkdir -p /mnt/myfs
   mount -t myfs none /mnt/myfs
   ```
4. Kiểm tra mount:
   ```bash
   cat /proc/mounts
   ```
    - Tìm dòng: `none /mnt/myfs myfs rw 0 0`.
5. Kiểm tra số liên kết của root inode:
   ```bash
   dmesg | tail
   ```
    - Tìm dòng: `root inode has 2 link(s)`.
6. Kiểm tra inode:
   ```bash
   ls -di /mnt/myfs
   ```
    - Inode thường là 2.
7. Tháo và gỡ module:
   ```bash
   umount /mnt/myfs
   rmmod myfs
   ```

### Giải thích mã nguồn
Mã nguồn trong `myfs.c` hoàn thành **TODO 3**, tập trung vào hàm `myfs_get_inode`. Dưới đây là hàm và giải thích:

#### Hàm `myfs_get_inode`
```c
struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir,
                             int mode)
{
    struct inode *inode = new_inode(sb);

    if (!inode)
        return NULL;

    /* TODO 3: fill inode structure */
    inode_init_owner(inode, dir, mode); /* Khởi tạo uid, gid, mode */
    inode->i_atime = inode->i_ctime = inode->i_mtime = current_time(inode); /* Thời gian */
    inode->i_ino = get_next_ino(); /* Số inode */

    if (S_ISDIR(mode)) {
        /* TODO 3: set inode operations for dir inodes */
        inode->i_op = &simple_dir_inode_operations; /* Thao tác inode thư mục */
        inode->i_fop = &simple_dir_operations; /* Thao tác tệp thư mục */
        /* TODO 3: directory inodes start off with i_nlink == 2 */
        set_nlink(inode, 2); /* i_nlink = 2 cho . và .. */
    }

    return inode;
}
```

**Giải thích**:
1. **Khởi tạo inode**:
    - `struct inode *inode = new_inode(sb)`: Tạo inode mới từ superblock `sb`.
    - Kiểm tra `if (!inode)` để xử lý trường hợp không đủ bộ nhớ.

2. **Điền trường inode**:
    - `inode_init_owner(inode, dir, mode)`:
        - Khởi tạo `i_uid`, `i_gid`, `i_mode`.
        - `inode`: Inode cần khởi tạo.
        - `dir`: Thư mục cha (NULL cho root inode, nên `uid`/`gid` mặc định là 0 - root).
        - `mode`: Chế độ từ `myfs_fill_super` (`S_IFDIR | 0755` cho thư mục gốc).
    - `inode->i_atime = inode->i_ctime = inode->i_mtime = current_time(inode)`:
        - Gán thời gian truy cập, tạo, và sửa đổi bằng `current_time`, định dạng phù hợp với VFS.
    - `inode->i_ino = get_next_ino()`:
        - Gán số inode duy nhất bằng `get_next_ino`, thường bắt đầu từ 2.

3. **Xử lý inode thư mục**:
    - `if (S_ISDIR(mode))`: Kiểm tra inode là thư mục bằng macro `S_ISDIR`.
    - `inode->i_op = &simple_dir_inode_operations`:
        - Gán thao tác inode cho thư mục, dùng hàm kernel có sẵn để xử lý `lookup`, `mkdir`, v.v.
    - `inode->i_fop = &simple_dir_operations`:
        - Gán thao tác tệp cho thư mục, hỗ trợ lệnh như `ls`, `cd`.
    - `set_nlink(inode, 2)`:
        - Đặt `i_nlink = 2` cho thư mục gốc (liên kết đến `.` và `..`).

#### Các phần liên quan khác
- **Trong `myfs_fill_super`**:
    - Gọi `myfs_get_inode(sb, NULL, S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)` để tạo root inode với quyền 755.
    - In số liên kết: `printk(LOG_LEVEL "root inode has %d link(s)\n", root_inode->i_nlink)`.

- **Hằng số**:
  ```c
  #define MYFS_BLOCKSIZE      4096
  #define MYFS_BLOCKSIZE_BITS 12
  #define MYFS_MAGIC          0xbeefcafe
  #define LOG_LEVEL           KERN_ALERT
  ```
    - Được dùng trong `myfs_fill_super` (bài 2) để cấu hình superblock.

- **Các hàm chưa triển khai**:
    - `myfs_mknod`, `myfs_create`, `myfs_mkdir` (TODO 5) và thao tác tệp (TODO 6) thuộc các bài sau.


### **Bước tiếp theo**

- **Bài 4 (kiểm tra)**:
    - Chạy các kiểm tra:
      ```bash
      ls -di /mnt/myfs
      stat -f /mnt/myfs
      touch /mnt/myfs/a.txt  # Sẽ thất bại
      ./skels/filesystems/myfs/test-myfs.sh
      ```
    - Mã bài 3 đủ để vượt qua bài 4.

- **TODO 5 và 6**:
    - TODO 5: Triển khai `myfs_mknod`, `myfs_create`, `myfs_mkdir`.
    - TODO 6: Điền thao tác tệp (`myfs_file_operations`, `myfs_aops`).

