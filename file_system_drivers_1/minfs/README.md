
# Hệ thống tệp MinFS - Tổng kết bài lab minfs1

## Tổng quan
Bài lab **MinFS** là một phần của **SO2 Lab - Filesystem drivers**, tập trung vào việc triển khai một hệ thống tệp đơn giản trong kernel Linux.
Mục tiêu là xây dựng một hệ thống tệp MinFS với các tính năng cơ bản: đăng ký, gắn (mount), đọc inode, liệt kê thư mục, và hỗ trợ tạo tệp. 
Bài lab gồm 5 bài tương ứng với 5 TODO, mỗi bài giải quyết một phần của hệ thống tệp.

## Môi trường
- **Hệ điều hành**: Linux (máy ảo QEMU, `qemux86`).
- **Thiết bị**: `/dev/vdc` (thiết bị khối giả lập).
- **Công cụ**: `mkfs.minfs` (định dạng MinFS), `insmod`, `mount`, `ls`, `stat`, `umount`, `rmmod`, `dmesg`.
- **Mã nguồn**: Mô-đun kernel `minfs.ko` (`minfs.c`, `minfs.h`) và công cụ người dùng `mkfs.minfs`.

## Nội dung các bài (TODO)

### Bài 1: Định nghĩa cấu trúc và hàm cơ bản (TODO 1)
- **Mục tiêu**: Thiết lập các cấu trúc dữ liệu và hàm khởi tạo cho MinFS.
- **Công việc**:
    - Định nghĩa `minfs_super_block`, `minfs_inode`, `minfs_inode_info`, `minfs_dir_entry` trong `minfs.h`.
    - Triển khai `minfs_init` và `minfs_exit` để đăng ký/gỡ hệ thống tệp.
- **Kết quả**: MinFS được đăng ký trong `/proc/filesystems` (`cat /proc/filesystems | grep minfs`).

### Bài 2: Đọc siêu khối và gắn hệ thống tệp (TODO 2)
- **Mục tiêu**: Đọc siêu khối từ đĩa và khởi tạo siêu khối VFS.
- **Công việc**:
    - Triển khai `minfs_fill_super` để đọc siêu khối (block 0), kiểm tra số ma thuật (`MINFS_MAGIC`),
    - và điền `super_block` (sử dụng `myfs_get_inode`).
    - Thiết lập `s_magic`, `s_op`, và `sbi->version`, `sbi->imap`.
- **Kết quả**: Hệ thống tệp được gắn thành công (`mount -t minfs /dev/vdc /mnt/minfs`).

### Bài 3: Quản lý inode (TODO 3)
- **Mục tiêu**: Triển khai cấp phát và giải phóng inode.
- **Công việc**:
    - Thêm `minfs_alloc_inode` để cấp phát `minfs_inode_info` (chứa inode VFS và `data_block`).
    - Thêm `minfs_destroy_inode` để giải phóng bộ nhớ.
- **Kết quả**: Chuẩn bị cho việc đọc inode từ đĩa, nhưng chưa kiểm tra trực tiếp.

### Bài 4: Khởi tạo inode gốc (TODO 4)
- **Mục tiêu**: Đọc inode gốc từ đĩa và điền vào inode VFS.
- **Công việc**:
    - Cập nhật `minfs_ops` với `alloc_inode` và `destroy_inode`.
    - Triển khai `minfs_iget` để đọc inode từ block 1, điền các trường (`i_mode`, `i_uid`, `i_gid`, `i_size`, thời gian), 
    - và thiết lập thao tác thư mục (`simple_dir_*`).
    - Thay `myfs_get_inode` bằng `minfs_iget` trong `minfs_fill_super`.
- **Kết quả**: Inode gốc được khởi tạo đúng (`stat /mnt/minfs` hiển thị `Links: 2`, quyền `0755`, `Uid: 0`, `Gid: 0`).

### Bài 5: Liệt kê nội dung thư mục (TODO 5)
- **Mục tiêu**: Hỗ trợ liệt kê nội dung thư mục gốc.
- **Công việc**:
    - Cập nhật `minfs_iget` để sử dụng `minfs_dir_inode_operations` và `minfs_dir_operations` cho inode thư mục.
    - Triển khai `minfs_readdir` để đọc các mục `minfs_dir_entry` từ khối dữ liệu của inode.
- **Kết quả**: Lệnh `ls -a /mnt/minfs` hiển thị `.` và `..`, mount/unmount thành công (`umount /mnt/minfs`, `rmmod minfs`).

## Kết quả kiểm tra
- **Thực hiện**:
  ```bash
  insmod minfs.ko
  mkfs.minfs /dev/vdc
  mkdir -p /mnt/minfs
  mount -t minfs /dev/vdc /mnt/minfs
  ls -a /mnt/minfs  # Hiển thị . ..
  stat /mnt/minfs   # Hiển thị thông tin inode gốc
  umount /mnt/minfs
  rmmod minfs
  ```
- **Kết quả**:
    - Hệ thống tệp được đăng ký, định dạng, gắn, và tháo thành công.
    - Thư mục gốc hiển thị đúng các mục `.` và `..`.
    - Inode gốc có `Links: 2`, quyền `0755`, và thông tin người dùng (`Uid: 0`, `Gid: 0`).

## Vấn đề được giải quyết
Bài lab giải quyết các vấn đề cốt lõi của việc triển khai một hệ thống tệp trong kernel Linux:
- **Đăng ký hệ thống tệp**: Sử dụng `register_filesystem` để tích hợp MinFS vào kernel.
- **Quản lý siêu khối**: Đọc và xác minh siêu khối từ đĩa.
- **Quản lý inode**: Cấp phát, đọc, và khởi tạo inode từ đĩa sang VFS.
- **Hỗ trợ thư mục**: Liệt kê nội dung thư mục gốc, chuẩn bị cho việc tra cứu và tạo tệp.
- **Tương tác với đĩa**: Sử dụng `sb_bread` để đọc siêu khối, inode, và dữ liệu thư mục.

## Lưu ý
- **TODO 7 (tùy chọn)**: Hỗ trợ tạo tệp mới (`minfs_create`, `minfs_new_inode`, `minfs_add_link`) để mở rộng chức năng.
- **Kiểm tra tự động**: Sử dụng `test-minfs.sh` (cần sửa lỗi `make copy` nếu URL tải hình ảnh Yocto không hợp lệ).
- **Gỡ lỗi**: Sử dụng `dmesg` và `hexdump` để kiểm tra lỗi khi mount hoặc liệt kê nội dung.

## Kết luận
Bài lab MinFS cung cấp cái nhìn thực tế về cách triển khai một hệ thống tệp trong kernel Linux, từ cấu trúc dữ liệu,
quản lý siêu khối và inode, đến hỗ trợ thao tác thư mục cơ bản. Các bước từ TODO 1 đến TODO 5 đã được hoàn thành,
cho phép gắn, liệt kê, và tháo hệ thống tệp MinFS thành công.
