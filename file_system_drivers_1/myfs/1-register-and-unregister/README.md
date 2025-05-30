## Dẫn sửa code
    cd /linux/tools/labs/skels/filesystems/myfs
    vi myfs.c

- **Mô tả**: Giới thiệu mục tiêu bài 1 và các nhiệm vụ chính (TODO 1).
- **Các bước thực hiện**: Liệt kê các bước lưu mã, biên dịch, và kiểm tra.
- **Giải thích mã nguồn**: Chi tiết các phần mã trong `myfs.c`, 
- tập trung vào `struct file_system_type`, `myfs_mount`, `myfs_init`, `myfs_exit`, và các hằng số.


## Cách làm

### Mô tả
Bài 1 yêu cầu triển khai hệ thống tệp `myfs` (không dùng thiết bị) để đăng ký với kernel Linux, xuất hiện trong `/proc/filesystems`. Mã nguồn được viết trong tệp `/linux/tools/labs/skels/filesystems/myfs/myfs.c`, hoàn thành **TODO 1**:

- Định nghĩa và điền cấu trúc `struct file_system_type`.
- Triển khai hàm `myfs_mount`.
- Hoàn thiện `myfs_init` và `myfs_exit` để đăng ký/hủy đăng ký.

Dưới đây là các bước thực hiện và giải thích mã nguồn.

### Bước 1: Lưu mã nguồn
1. Mở tệp `myfs.c`:
   ```bash
   vi /linux/tools/labs/skels/filesystems/myfs/myfs.c
   ```
   
### Bước 2: Biên dịch module
1. Biên dịch:
   ```bash
   make build 
   ```
2. Kiểm tra tệp `myfs.ko`:
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
3. Kiểm tra `/proc/filesystems`:
   ```bash
   cat /proc/filesystems
   ```
    - Tìm dòng chứa `myfs`.
4. Thử gắn (sẽ thất bại):
   ```bash
   mkdir -p /mnt/myfs
   mount -t myfs none /mnt/myfs
   ```
5. Gỡ module:
   ```bash
   rmmod myfs
   ```

### Giải thích mã nguồn
Mã nguồn trong `myfs.c` hoàn thành **TODO 1**, tập trung vào việc đăng ký hệ thống tệp. Dưới đây là các phần chính và giải thích:

#### 1. Định nghĩa `struct file_system_type`
```c
static struct file_system_type myfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = kill_litter_super,
};
```
**Giải thích**:
- `.owner = THIS_MODULE`: Liên kết hệ thống tệp với module hiện tại, đảm bảo kernel không gỡ module khi hệ thống tệp đang được sử dụng.
- `.name = "myfs"`: Tên hệ thống tệp, hiển thị trong `/proc/filesystems` và dùng khi gắn (`mount -t myfs`).
- `.mount = myfs_mount`: Hàm được gọi khi gắn hệ thống tệp, sẽ triển khai ở dưới.
- `.kill_sb = kill_litter_super`: Hàm kernel có sẵn để dọn dẹp superblock khi hệ thống tệp được tháo (unmount). Phù hợp cho hệ thống tệp không dùng thiết bị.

#### 2. Triển khai `myfs_mount`
```c
static struct dentry *myfs_mount(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, myfs_fill_super);
}
```
**Giải thích**:
- Hàm này được gọi khi người dùng chạy lệnh `mount -t myfs`.
- `mount_nodev`: Hàm kernel dùng cho hệ thống tệp không cần thiết bị (như `myfs`), thay vì `mount_bdev` (dùng cho thiết bị block).
- **Tham số**:
    - `fs_type`: Con trỏ đến `myfs_fs_type`.
    - `flags`: Cờ mount (ví dụ: read-only).
    - `data`: Dữ liệu tùy chọn từ người dùng.
    - `myfs_fill_super`: Hàm để điền superblock (chưa triển khai ở bài 1, nên mount sẽ thất bại).
- Trả về `struct dentry *` (con trỏ đến thư mục gốc của hệ thống tệp), nhưng ở bài 1, `myfs_fill_super` chưa hoàn thiện nên trả về lỗi.

#### 3. Triển khai `myfs_init` và `myfs_exit`
```c
static int __init myfs_init(void)
{
    int err = 0; /* Sửa lỗi: khởi tạo err */
    err = register_filesystem(&myfs_fs_type);
    if (err) {
        printk(LOG_LEVEL "register_filesystem failed\n");
        return err;
    }
    return 0;
}

static void __exit myfs_exit(void)
{
    unregister_filesystem(&myfs_fs_type);
}
```
**Giải thích**:
- **`myfs_init`**:
    - Được gọi khi module được nạp (`insmod`).
    - Khởi tạo `err = 0` để tránh cảnh báo `-Wuninitialized` từ trình biên dịch.
    - `register_filesystem(&myfs_fs_type)`: Đăng ký `myfs` với kernel, làm nó xuất hiện trong `/proc/filesystems`.
    - Nếu đăng ký thất bại, in thông báo lỗi và trả về mã lỗi.
    - Trả về 0 nếu thành công.
- **`myfs_exit`**:
    - Được gọi khi module được gỡ (`rmmod`).
    - `unregister_filesystem(&myfs_fs_type)`: Hủy đăng ký `myfs`, xóa nó khỏi `/proc/filesystems`.

#### 4. Các phần khác trong mã
**Hằng số**:
```c
#define MYFS_BLOCKSIZE      4096
#define MYFS_BLOCKSIZE_BITS 12
#define MYFS_MAGIC          0xbeefcafe
#define LOG_LEVEL           KERN_ALERT
```
- `MYFS_BLOCKSIZE`: Kích thước khối (4KB), sẽ dùng trong superblock (bài 2).
- `MYFS_BLOCKSIZE_BITS`: Logarit cơ số 2 của blocksize (`2^12 = 4096`).
- `MYFS_MAGIC`: Số ma thuật để nhận diện hệ thống tệp, dùng trong superblock.
- `LOG_LEVEL`: Mức log cho `printk`, xuất thông báo ở mức cảnh báo.

**Các hàm chưa triển khai**:
- `myfs_fill_super`, `myfs_get_inode`, v.v. được để trống hoặc chỉ có khung (skeleton) vì thuộc các bài sau (2, 3).
- Ở bài 1, mount thất bại do `myfs_fill_super` chưa hoàn thiện.

