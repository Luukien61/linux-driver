
### **README - Tổng kết Lab MyFS **


## Tổng kết Lab MyFS

### Tổng quan
Lab **MyFS** thuộc bài tập số 1 của **SO2 Lab - Filesystem drivers**, triển khai hệ thống tệp không dùng thiết bị (no-dev filesystem) tên `myfs`. Mục tiêu là tạo một hệ thống tệp đơn giản, đăng ký với kernel Linux, và hỗ trợ mount cơ bản. Lab bao gồm 4 bài chính:

- **Bài 1**: Đăng ký hệ thống tệp, xuất hiện trong `/proc/filesystems`.
- **Bài 2**: Điền superblock để chuẩn bị mount.
- **Bài 3**: Khởi tạo root inode cho thư mục gốc.
- **Bài 4**: Kiểm tra mount, unmount, và quan sát đặc điểm hệ thống tệp.

Mã nguồn chính nằm trong `/linux/tools/labs/skels/filesystems/myfs/myfs.c`. Lab tập trung vào các TODO 1, 2, 3, với bài 4 kiểm tra kết quả. Các TODO 5 và 6 (tạo tệp/thư mục, thao tác tệp) thuộc phần mở rộng, chưa thực hiện trong phạm vi này.

### Cách làm

#### Bài 1: Đăng ký hệ thống tệp (TODO 1)
**Mục tiêu**: Định nghĩa `struct file_system_type`, triển khai `myfs_mount`, `myfs_init`, `myfs_exit` để đăng ký/hủy đăng ký `myfs`.

**Bước thực hiện**:
1. Mở `myfs.c`:
   ```bash
   vi /linux/tools/labs/skels/filesystems/myfs/myfs.c
   ```
2. Cập nhật mã với `struct file_system_type`, `myfs_mount`, `myfs_init`, `myfs_exit`.
3. Biên dịch:
   ```bash
   make build
   ```
4. Kiểm tra:
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   cat /proc/filesystems
   ```
    - Tìm: `myfs`.
   ```bash
   mount -t myfs none /mnt/myfs  # Thất bại (chưa có superblock)
   rmmod myfs
   ```

#### Bài 2: Điền superblock (TODO 2)
**Mục tiêu**: Triển khai `myfs_fill_super` và định nghĩa `super_operations` để cấu hình superblock.

**Bước thực hiện**:
1. Cập nhật `myfs.c` với:
    - `struct super_operations myfs_super_ops`.
    - Điền `sb->s_blocksize`, `s_blocksize_bits`, `s_magic`, `s_op`, `s_maxbytes` trong `myfs_fill_super`.
2. Biên dịch và kiểm tra như bài 1.
3. Kết quả: Mount vẫn thất bại do chưa có root inode.

#### Bài 3: Khởi tạo root inode (TODO 3)
**Mục tiêu**: Hoàn thiện `myfs_get_inode` để khởi tạo root inode (thư mục gốc `/`).

**Bước thực hiện**:
1. Cập nhật `myfs_get_inode` trong `myfs.c`:
    - Dùng `inode_init_owner` cho `i_uid`, `i_gid`, `i_mode`.
    - Gán `i_atime`, `i_ctime`, `i_mtime` bằng `current_time`.
    - Gán `i_ino` bằng `get_next_ino`.
    - Với thư mục: Gán `i_op`, `i_fop`, đặt `i_nlink = 2`.
2. Biên dịch:
   ```bash
   make clean
   make build
   ```
3. Kiểm tra:
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   mkdir -p /mnt/myfs
   mount -t myfs none /mnt/myfs
   cat /proc/mounts
   dmesg | tail  # Tìm: "root inode has 2 link(s)"
   umount /mnt/myfs
   rmmod myfs
   ```

#### Bài 4: Kiểm tra mount và unmount
**Mục tiêu**: Kiểm tra mount, unmount, số inode, thống kê, và thử tạo tệp.

**Bước thực hiện**:
1. Biên dịch và sao chép:
   ```bash
   chmod +x /linux/tools/labs/skels/filesystems/myfs/test-myfs.sh
   cd /linux/tools/labs
   make copy
   make console
   ```
2. Kiểm tra:
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   mkdir -p /mnt/myfs
   mount -t myfs none /mnt/myfs
   cat /proc/mounts  # Tìm: "none /mnt/myfs myfs rw,relatime 0 0"
   ls -di /mnt/myfs  # Inode ~4714
   stat -f /mnt/myfs  # Block size: 4096, khối/inode = 0
   ls -la /mnt/myfs  # Chỉ có . và ..
   touch /mnt/myfs/a.txt  # Lỗi: "Permission denied"
   umount /mnt/myfs
   rmmod myfs
   ```
3. (Tùy chọn) Chạy script:
   ```bash
   /home/root/skels/filesystems/myfs/test-myfs.sh
   ```

### Giải thích mã nguồn
Mã trong `myfs.c` (phiên bản bài 3) hoàn thành TODO 1, 2, 3:

1. **Đăng ký hệ thống tệp (TODO 1)**:
    - `struct file_system_type myfs_fs_type`: Định nghĩa `myfs` với `name`, `mount`, `kill_sb`.
    - `myfs_mount`: Gọi `mount_nodev` để gắn hệ thống tệp không thiết bị.
    - `myfs_init`/`myfs_exit`: Đăng ký/hủy đăng ký với `register_filesystem`/`unregister_filesystem`.

2. **Superblock (TODO 2)**:
    - `myfs_fill_super`: Điền `s_blocksize` (4096), `s_magic` (0xbeefcafe), `s_op`, `s_maxbytes`.
    - `myfs_super_ops`: Dùng `simple_statfs`, `generic_drop_inode`.

3. **Root inode (TODO 3)**:
    - `myfs_get_inode`:
        - Khởi tạo `i_uid`, `i_gid`, `i_mode` bằng `inode_init_owner`.
        - Gán thời gian (`current_time`) và số inode (`get_next_ino`).
        - Với thư mục: Gán `simple_dir_inode_operations`, `simple_dir_operations`, `i_nlink = 2`.

4. **Chưa triển khai**:
    - `myfs_mknod`, `myfs_create`, `myfs_mkdir` (TODO 5).
    - Thao tác tệp (`myfs_file_operations`, `myfs_aops`, TODO 6).
    - Do đó, tạo tệp (`touch`) thất bại.

### Kết quả đạt được
Dựa trên kiểm tra bài 4:
- **Mount/unmount**: Thành công (`/proc/mounts` hiển thị `myfs`).
- **Số inode**: ~4714 (do `get_next_ino` cấp số tăng dần, phụ thuộc trạng thái kernel).
- **Số liên kết**: Root inode có `i_nlink = 2` (xác nhận qua `dmesg`).
- **Thống kê**: Block size 4096, khối/inode = 0 (do `simple_statfs`).
- **Nội dung**: Chỉ có `.` và `..` (quyền 755).
- **Tạo tệp**: Thất bại (`Permission denied`), đúng vì chưa có `myfs_create`.

### Lưu ý
  ```
- **Script `test-myfs.sh`**: Cần quyền thực thi (`chmod +x`).
- **TODO còn lại**: TODO 5 (tạo tệp/thư mục) và 6 (thao tác tệp) chưa làm.
   ```
### Kết luận
Lab MyFS đã hoàn thành các bài 1-4, đạt được hệ thống tệp cơ bản với khả năng đăng ký, cấu hình superblock, 
khởi tạo root inode, và mount thành công. Các bước kiểm tra bài 4 xác nhận hệ thống tệp hoạt động đúng, dù chưa hỗ trợ tạo tệp/thư mục.
Tiếp theo, có thể triển khai TODO 5 và 6 để mở rộng chức năng.

- **TODO 5 và 6**:
    - **TODO 5**: Triển khai `myfs_mknod`, `myfs_create`, `myfs_mkdir` để hỗ trợ tạo tệp/thư mục.
    - **TODO 6**: Điền `myfs_file_operations`, `myfs_aops` cho thao tác tệp.
