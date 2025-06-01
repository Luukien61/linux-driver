
# MinFS - README cho TODO 5 (Iterate Operation)

## Tổng quan
**TODO 5** thuộc bài lab **File System Drivers (Part 2)**,
tập trung vào triển khai thao tác lặp (iterate) cho hệ thống tệp MinFS. 
Mục tiêu là hoàn thiện hàm `minfs_readdir` để liệt kê các mục trong thư mục gốc, 
hỗ trợ lệnh `ls -a /mnt/minfs` hiển thị các mục như `.` và `..`.

## Môi trường
- **Hệ điều hành**: Linux (QEMU, `qemux86`).
- **Thiết bị**: `/dev/vdc` (khối giả lập).
- **Công cụ**: `mkfs.minfs`, `insmod`, `mount`, `ls`, `umount`, `rmmod`.
- **Mã nguồn**: Mô-đun `minfs.ko` (`minfs.c`, `minfs.h`).

## Mục tiêu
- **Chức năng**: Triển khai `minfs_readdir` trong `minfs_dir_operations` 
  để đọc các mục `minfs_dir_entry` từ khối dữ liệu của inode thư mục.
- **Kết quả**: Lệnh `ls -a /mnt/minfs` hiển thị các mục `.` và `..` (được tạo bởi `mkfs.minfs`).

## Thay đổi mã nguồn
### 1. Cập nhật `minfs_iget`
- **Vị trí**: Hàm `minfs_iget` trong `minfs.c`.
- **Thay đổi**: Gán các thao tác thư mục cho inode thư mục.
```c
if (S_ISDIR(inode->i_mode)) {
    inode->i_op = &minfs_dir_inode_operations;
    inode->i_fop = &minfs_dir_operations;
    inc_nlink(inode); /* i_nlink = 2 cho thư mục */
}
```
- **Giải thích**:
    - `minfs_dir_inode_operations`: Chứa `minfs_lookup` (sẽ triển khai sau).
    - `minfs_dir_operations`: Chứa `minfs_readdir` để liệt kê mục.
    - `inc_nlink`: Đặt số liên kết là 2 cho thư mục (do có `.` và `..`).

### 2. Triển khai `minfs_readdir`
- **Vị trí**: Hàm `minfs_readdir` trong `minfs.c`.
- **Mã nguồn**:
```c
static int minfs_readdir(struct file *filp, struct dir_context *ctx)
{
    struct buffer_head *bh;
    struct minfs_dir_entry *de;
    struct minfs_inode_info *mii;
    struct inode *inode;
    struct super_block *sb;
    int over;
    int err = 0;

    /* Lấy inode của thư mục và minfs_inode_info */
    inode = filp->f_path.dentry->d_inode;
    mii = container_of(inode, struct minfs_inode_info, vfs_inode);

    /* Lấy siêu khối */
    sb = inode->i_sb;

    /* Đọc khối dữ liệu của thư mục */
    bh = sb_bread(sb, mii->data_block);
    if (!bh) {
        printk(LOG_LEVEL "không thể đọc khối dữ liệu thư mục\n");
        return -EIO;
    }

    /* Duyệt các mục thư mục */
    for (; ctx->pos < MINFS_NUM_ENTRIES; ctx->pos++) {
        de = (struct minfs_dir_entry *)bh->b_data + ctx->pos;
        if (de->ino == 0)
            continue;

        over = dir_emit(ctx, de->name, MINFS_NAME_LEN, de->ino, DT_UNKNOWN);
        if (!over) {
            printk(KERN_DEBUG "Đọc %s từ thư mục %s, ctx->pos: %lld\n",
                   de->name, filp->f_path.dentry->d_name.name, ctx->pos);
            ctx->pos++;
            break;
        }
    }

done:
    brelse(bh);
out_bad_sb:
    return err;
}
```
- **Giải thích**:
    - **Lấy inode**: `filp->f_path.dentry->d_inode` lấy inode của thư mục.
    - **Lấy `minfs_inode_info`**: Sử dụng `container_of` để truy cập `data_block`.
    - **Đọc khối dữ liệu**: `sb_bread` đọc khối dữ liệu chứa mảng `minfs_dir_entry`.
    - **Duyệt mục**: Lặp qua `MINFS_NUM_ENTRIES`, bỏ qua mục trống (`de->ino == 0`).
    - **Gửi dữ liệu**: `dir_emit` gửi tên (`de->name`), số inode (`de->ino`), và loại (`DT_UNKNOWN`) tới người dùng.
    - **Quản lý vị trí**: Tăng `ctx->pos` để theo dõi mục đã đọc.
    - **Giải phóng**: `brelse` giải phóng `buffer_head`.

## Kiểm tra
### Bước thực hiện
1. **Biên dịch và sao chép**:
   ```bash
   make build
   make copy
   ```
2. **Khởi động QEMU**:
   ```bash
   make console
   ```
3. **Chạy kiểm tra**:
   ```bash
   insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko
   /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc
   mkdir -p /mnt/minfs
   mount -t minfs /dev/vdc /mnt/minfs
   ls -a /mnt/minfs  # Hiển thị . ..
   umount /mnt/minfs
   rmmod minfs
   ```
4. **Chạy script**:
   ```bash
   /home/root/skels/filesystems/minfs/user/test-minfs-0.sh
   /home/root/skels/filesystems/minfs/user/test-minfs-1.sh
   ```

### Kết quả mong đợi
- **Lệnh `ls -a /mnt/minfs`**:
  ```
  .  ..
  ```
- **Script kiểm tra**: Chạy không báo lỗi.
- **Log kernel**: `dmesg` có thể hiển thị debug từ `printk` trong `minfs_readdir`.

## Lưu ý
- **Vấn đề `make copy`**: Nếu gặp lỗi do URL Yocto không hợp lệ, 
- xây dựng `core-image-minimal` cục bộ bằng BitBake và thêm `kmod` vào `IMAGE_INSTALL`.
- **Gỡ lỗi**:
    - Nếu `ls -a` không hiển thị `.` và `..`, kiểm tra `dmesg | tail -20`.
    - Dùng `hexdump -C -s 2048 /dev/vdc` để kiểm tra khối dữ liệu của inode gốc.
- **Tiếp theo**: Triển khai **TODO 6** (tra cứu - `minfs_lookup`) để hỗ trợ `ls -l`.

## Kết luận
**TODO 5** đã hoàn thành thao tác lặp, cho phép MinFS liệt kê nội dung thư mục gốc. 
Hàm `minfs_readdir` đọc các mục `minfs_dir_entry` và gửi chúng qua `dir_emit`, hỗ trợ lệnh `ls -a`.
Đây là nền tảng cho các thao tác tiếp theo như tra cứu và tạo tệp.


---
