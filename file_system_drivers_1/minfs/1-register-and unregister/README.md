## Sửa code:
     vi /linux/tools/labs/skels/filesystems/minfs/kernel/minfs.c
## Chạy trong qemu
    1. /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc
    2. insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko

## Kết quả:
    root@qemux86:~/skels/filesystems/minfs/kernel# insmod minfs.ko
    minfs: loading out-of-tree module taints kernel.
    root@qemux86:~/skels/filesystems/minfs/kernel# cat /proc/filesystems
    nodev   sysfs
    nodev   tmpfs
    nodev   bdev
    nodev   proc
    nodev   devtmpfs
    nodev   binfmt_misc
    nodev   configfs
    nodev   debugfs
    nodev   tracefs
    nodev   sockfs
    nodev   pipefs
    nodev   ramfs
    nodev   devpts
    ext3
    ext2
    ext4
    nodev   cifs
    nodev   smb3
    minfs           ```Mong đợi thấy```

## Thử mount: (chưa mount được vì chưa là TODO 2)
    root@qemux86:~/skels/filesystems/minfs/kernel# mkdir -p /mnt/minfs
    root@qemux86:~/skels/filesystems/minfs/kernel# mount -t minfs /dev/vdc /mnt/minf
    s
    mount: mounting /dev/vdc on /mnt/minfs failed: Not a directory
    root@qemux86:~/skels/filesystems/minfs/kernel# 

### **README cho Bài 1 - MinFS**

# Bài 1 - MinFS: Đăng ký hệ thống tệp

## Mô tả
Bài 1 yêu cầu đăng ký hệ thống tệp `minfs` (dựa trên đĩa) trong kernel Linux, 
sao cho nó xuất hiện trong `/proc/filesystems`.
Cần sửa các hàm `minfs_fs_type`, `minfs_mount`, và `minfs_lookup` trong tệp `/linux/tools/labs/skels/filesystems/minfs/kernel/minfs.c`
(TODO 1). Sử dụng đĩa ảo `disk1.img` (100MB), định dạng bằng `mkfs.minfs`. 
Mount sẽ thất bại do chưa triển khai `minfs_fill_super` (TODO 2).

## Yêu cầu
- Đĩa ảo: `disk1.img` (100MB), ánh xạ vào `/dev/vdc`.
- Môi trường: QEMU với Yocto rootfs.
- Công cụ: `mkfs.minfs`, `minfs.ko`.
- Kết quả mong đợi:
    - `minfs` xuất hiện trong `/proc/filesystems`.
    - Định dạng `/dev/vdc` thành công.
    - Mount thất bại (bình thường ở TODO 1).

## Các bước thực hiện

### Bước 1: Kiểm tra đĩa ảo
1. Xác nhận `disk1.img`:
   ```bash
   ls -lh /linux/tools/labs/disk1.img
   ```
    - Kích thước: ~100MB (104857600 bytes).
    - Nếu không tồn tại hoặc sai kích thước:
      ```bash
      rm /linux/tools/labs/disk1.img
      dd if=/dev/zero of=/linux/tools/labs/disk1.img bs=1M count=100
      ```

2. Kiểm tra `qemu/Makefile`:
   ```bash
   nano /linux/tools/labs/qemu/Makefile
   ```
    - Tìm biến `QEMU_OPTS`, đảm bảo có:
      ```makefile
      -drive file=disk1.img,if=virtio,format=raw
      ```
    - Lưu ý: `disk1.img` ánh xạ vào `/dev/vdc` (thay vì `/dev/vdd`) do cấu hình QEMU.

### Bước 2: Sửa mã nguồn minfs.c
1. Mở tệp:
   ```bash
   vi /linux/tools/labs/skels/filesystems/minfs/kernel/minfs.c
   ```

2. Sửa các phần sau (TODO 1):

   #### a. `minfs_fs_type` (khoảng dòng 500-510)
    - **Mã gốc**:
      ```c
      static struct file_system_type minfs_fs_type = {
          ...
      };
      ```
    - **Sửa thành**:
      ```c
      static struct file_system_type minfs_fs_type = {
          .owner      = THIS_MODULE,
          .name       = "minfs",
          .mount      = minfs_mount,
          .kill_sb    = kill_block_super,
          .fs_flags   = FS_REQUIRES_DEV,
      };
      ```
    - **Giải thích**:
        - `.owner = THIS_MODULE`: Liên kết hệ thống tệp với module kernel, ngăn gỡ module khi đang sử dụng.
        - `.name = "minfs"`: Tên hệ thống tệp, xuất hiện trong `/proc/filesystems`.
        - `.mount = minfs_mount`: Hàm xử lý mount, gọi `mount_bdev` để gắn thiết bị block.
        - `.kill_sb = kill_block_super`: Hàm chuẩn để hủy superblock khi gỡ mount.
        - `.fs_flags = FS_REQUIRES_DEV`: Yêu cầu thiết bị block (như `/dev/vdc`).

   #### b. `minfs_mount` (khoảng dòng 485-490)
    - **Mã gốc**:
      ```c
      static struct dentry *minfs_mount(struct file_system_type *fs_type,
              int flags, const char *dev_name, void *data)
      {
          // TODO: Implement
      }
      ```
    - **Sửa thành**:
      ```c
      static struct dentry *minfs_mount(struct file_system_type *fs_type,
              int flags, const char *dev_name, void *data)
      {
          return mount_bdev(fs_type, flags, dev_name, data, minfs_fill_super);
      }
      ```
    - **Giải thích**:
        - `mount_bdev`: Hàm kernel chuẩn để mount hệ thống tệp dựa trên block.
        - Tham số:
            - `fs_type`: Con trỏ đến `minfs_fs_type`.
            - `flags`: Cờ mount (ví dụ: read-only).
            - `dev_name`: Tên thiết bị (ví dụ: `/dev/vdc`).
            - `data`: Thông số mount tùy chọn.
            - `minfs_fill_super`: Hàm khởi tạo superblock (sẽ làm ở TODO 2).
        - Trả về `struct dentry *` cho thư mục gốc (root) của hệ thống tệp.

   #### c. `minfs_lookup` (khoảng dòng 210-220)
    - **Mã gốc**:
      ```c
      static struct dentry *minfs_lookup(struct inode *dir,
              struct dentry *dentry, unsigned int flags)
      {
          /* TODO 6: Comment line. */
          return simple_lookup(dir, dentry, flags);
 
          struct super_block *sb = dir->i_sb;
          struct minfs_dir_entry *de;
          struct buffer_head *bh = NULL;
          struct inode *inode = NULL;
          ...
      }
      ```
    - **Sửa thành**:
      ```c
      static struct dentry *minfs_lookup(struct inode *dir,
              struct dentry *dentry, unsigned int flags)
      {
          struct super_block *sb = dir->i_sb;
          struct minfs_dir_entry *de;
          struct buffer_head *bh = NULL;
          struct inode *inode = NULL;
 
          /* TODO 6: Comment line. */
          return simple_lookup(dir, dentry, flags);
          ...
      }
      ```
    - **Giải thích**:
        - Di chuyển khai báo biến (`sb`, `de`, `bh`, `inode`) lên đầu hàm để tuân thủ chuẩn C90, tránh lỗi biên dịch `-Wdeclaration-after-statement`.
        - `simple_lookup`: Hàm tạm thời trả về kết quả tra cứu đơn giản (sẽ sửa ở TODO 6).
        - Phần mã còn lại (sau `return`) chưa dùng ở TODO 1, nhưng khai báo sớm để tránh lỗi.

3. Lưu và thoát (`:wq`).

### Bước 4: Biên dịch module
1. Xóa tệp cũ:
   ```bash
   cd /linux/tools/labs/skels
   make clean
   ```
2. Biên dịch:
   ```bash
   make build
   ```
3. Kiểm tra:
   ```bash
   ls -l /linux/tools/labs/skels/filesystems/minfs/kernel/*.ko
   ```
    - **Mong đợi**: Thấy `minfs.ko`.

### Bước 5: Sao chép và chạy VM
1. Cấp quyền cho `mkfs.minfs`:
   ```bash
   chmod +x /linux/tools/labs/skels/filesystems/minfs/user/mkfs.minfs
   ```
2. Sao chép:
   ```bash
   cd /linux/tools/labs
   make copy
   ```
    - Copy `minfs.ko` và `mkfs.minfs` vào `/home/root` trong VM.
3. Chạy VM:
   ```bash
   make console
   ```

### Bước 6: Kiểm tra trong QEMU
1. Kiểm tra đĩa:
   ```bash
   cat /proc/partitions
   ls /dev/vd*
   ```
    - **Kết quả**:
      ```
      major minor  #blocks  name
       254        0     102400 vda
       254       16     102400 vdb
       254       32     102400 vdc
      ```
    - Tìm `/dev/vdc` (~100MB, ánh xạ từ `disk1.img`).

2. Định dạng đĩa:
   ```bash
   /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc
   ```
    - **Kết quả**: Thành công, không báo lỗi.

3. Nạp module:
   ```bash
   insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko
   ```
    - **Kết quả**:
      ```
      minfs: loading out-of-tree module taints kernel.
      ```
        - Cảnh báo bình thường, module nạp thành công.

4. Kiểm tra `/proc/filesystems`:
   ```bash
   cat /proc/filesystems
   ```
    - **Kết quả**:
      ```
      ...
      minfs
      ```
        - Xác nhận `minfs` đăng ký thành công.

5. Thử mount:
   ```bash
   mkdir -p /mnt/minfs
   mount -t minfs /dev/vdc /mnt/minfs
   ```
    - **Kết quả**:
      ```
      mount: mounting /dev/vdc on /mnt/minfs failed: Not a directory
      ```
        - Lỗi chấp nhận được, do chưa triển khai `minfs_fill_super` (TODO 2).

6. Gỡ module:
   ```bash
   rmmod minfs
   ```

### Giải thích mã nguồn
- **Đăng ký hệ thống tệp**:
    - Hàm `minfs_init` (gọi `register_filesystem(&minfs_fs_type)`) đăng ký `minfs` với kernel, làm nó xuất hiện trong `/proc/filesystems`.
    - `minfs_fs_type` định nghĩa metadata của hệ thống tệp, liên kết với `minfs_mount` để xử lý mount.
- **Mount thiết bị**:
    - `minfs_mount` gọi `mount_bdev`, truyền `minfs_fill_super` để khởi tạo superblock. Ở TODO 1, `minfs_fill_super` chưa hoàn thiện, dẫn đến lỗi mount.
- **Xử lý lỗi biên dịch**:
    - Lỗi `too few arguments to function 'mount_bdev'`: Sửa bằng cách thêm tham số `data` vào `mount_bdev`.
    - Cảnh báo C90 trong `minfs_lookup`: Di chuyển khai báo biến lên đầu hàm để tuân thủ chuẩn.
- **Lỗi mount**:
    - Lỗi `Not a directory` bất thường, có thể do `minfs_fill_super` trả về `-ENOTDIR`. Sẽ sửa ở TODO 2.

### Kết quả mong đợi
- `minfs` trong `/proc/filesystems`: **Đạt**.
- Định dạng `/dev/vdc`: **Đạt**.
- Mount thất bại: **Đạt** (lỗi `Not a directory`).

### Lưu ý
- **Lỗi `lsblk: not found`**:
    - Dùng:
      ```bash
      cat /proc/partitions
      ls /dev/vd*
      ```
    - Tìm `/dev/vdc` (~100MB).
- **Lỗi mount `Not a directory`**:
    - Kiểm tra `/mnt/minfs`:
      ```bash
      ls -ld /mnt/minfs
      ```
    - Nếu sai:
      ```bash
      rm -rf /mnt/minfs
      mkdir -p /mnt/minfs
      ```
    - Lỗi chấp nhận được, sẽ sửa ở TODO 2.
- **Không thấy `/dev/vdc`**:
    - Xem:
      ```bash
      dmesg | grep virtio
      ```
    - Kiểm tra `qemu/Makefile`.
- **Lỗi biên dịch**:
    - Đảm bảo `minfs_mount` gọi `mount_bdev(fs_type, flags, dev_name, data, minfs_fill_super)`.
    - Xem `dmesg | tail` nếu module không nạp.

### Bước tiếp theo
- **TODO 2**: Triển khai `minfs_fill_super` để đọc superblock từ `/dev/vdc`.
- **TODO 3**: Sửa `minfs_alloc_inode`, `minfs_destroy_inode`.



---
### **Giải thích thêm về mã nguồn**
- **Tại sao cần `minfs_fs_type`?**
    - `struct file_system_type` là cấu trúc kernel dùng để định nghĩa hệ thống tệp. Nó cung cấp thông tin như tên (`minfs`), hàm mount, và cờ (`FS_REQUIRES_DEV` yêu cầu thiết bị block).
    - `register_filesystem(&minfs_fs_type)` trong `minfs_init` thêm `minfs` vào danh sách hệ thống tệp của kernel.
- **Vai trò của `minfs_mount`?**
    - Là cầu nối giữa lệnh `mount` (user-space) và kernel. Nó gọi `mount_bdev` để xử lý thiết bị block, truyền `minfs_fill_super` để khởi tạo superblock.
- **Lỗi `Not a directory`**:
    - Có thể do `minfs_fill_super` trả về `-ENOTDIR` thay vì `-EINVAL`. Ở TODO 2, khi đọc superblock và kiểm tra magic number, lỗi sẽ rõ ràng hơn (như `bad superblock`).
- **Sửa `minfs_lookup`**:
    - Chỉ cần di chuyển khai báo để tránh lỗi biên dịch. Chức năng thực tế của `minfs_lookup` sẽ được triển khai ở TODO 6.

---
