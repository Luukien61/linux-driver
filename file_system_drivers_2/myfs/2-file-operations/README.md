# Kết quả
    root@qemux86:~# insmod /home/root/skels/filesystems/myfs/myfs.ko
    myfs: loading out-of-tree module taints kernel.
    root@qemux86:~# mkdir -p /mnt/myfs
    root@qemux86:~# mount -t myfs none /mnt/myfs
    root inode has 2 link(s)
    root@qemux86:~# echo "chocolate" > /mnt/myfs/peanuts.txt
    root@qemux86:~# cat /mnt/myfs/peanuts.txt
    chocolate
    root@qemux86:~# touch /mnt/myfs/test.txt
    root@qemux86:~# echo "hello world" > /mnt/myfs/test.txt
    root@qemux86:~# cat /mnt/myfs/test.txt
    hello world
    root@qemux86:~# ls -l /mnt/myfs
    -rw-r--r--    1 root     root            10 Jun  1 01:16 peanuts.txt
    -rw-r--r--    1 root     root            12 Jun  1 01:17 test.txt


# README: Triển khai hệ thống tệp `myfs` với các thao tác thư mục và tệp

Tệp README này ghi lại quá trình triển khai hệ thống tệp `myfs`,
một hệ thống tệp không thiết bị đơn giản được phát triển trong khuôn khổ bài tập số 1 của SO2 Lab. Triển khai bao gồm hai phần chính:
- **TODO 5**: Các thao tác thư mục (tạo, xóa, đổi tên tệp và thư mục).
- **TODO 6**: Các thao tác tệp (đọc, ghi, ánh xạ bộ nhớ).

Hệ thống tệp được mô phỏng theo `ramfs`, sử dụng các hàm chung của VFS (Virtual File System) và các hàm tùy chỉnh khi cần thiết.

## Mục tiêu

Mục tiêu của bài tập là xây dựng hệ thống tệp `myfs` hỗ trợ:
- Tạo, xóa, và đổi tên tệp/thư mục (TODO 5).
- Đọc, ghi, và thay đổi nội dung tệp (TODO 6), cho phép các lệnh như `echo "chocolate" > /mnt/myfs/peanuts.txt` và `cat /mnt/myfs/peanuts.txt`.

Triển khai sử dụng các hàm VFS chung và tham khảo hệ thống tệp `ramfs` để đảm bảo tính đơn giản và đúng yêu cầu.

## Các bước triển khai và kiểm tra

### 1. Triển khai các thao tác thư mục (TODO 5)

Các nhiệm vụ sau đã được hoàn thành để hỗ trợ thao tác thư mục:

1. **Định nghĩa `myfs_dir_inode_operations`:**
    - Cấu trúc này chỉ định các thao tác cho inode thư mục, bao gồm tạo tệp (`create`), thư mục (`mkdir`), tìm kiếm (`lookup`), liên kết (`link`), xóa (`unlink`, `rmdir`), đổi tên (`rename`), và tạo node (`mknod`).
    - Sử dụng các hàm VFS chung: `simple_lookup`, `simple_link`, `simple_unlink`, `simple_rmdir`, `simple_rename`.
    - Triển khai các hàm tùy chỉnh: `myfs_create`, `myfs_mkdir`, `myfs_mknod`.

2. **Triển khai `myfs_mknod`, `myfs_create`, `myfs_mkdir`:**
    - `myfs_mknod`: Tạo inode mới, liên kết với `dentry`, cập nhật thời gian, và tăng số liên kết thư mục cha nếu cần.
    - `myfs_create`: Gọi `myfs_mknod` với cờ `S_IFREG` để tạo tệp thông thường.
    - `myfs_mkdir`: Gọi `myfs_mknod` với cờ `S_IFDIR` để tạo thư mục.

3. **Sửa đổi `myfs_get_inode`:**
    - Khởi tạo inode với `mode`, `uid`, `gid`, thời gian, và số inode.
    - Đối với thư mục (`S_ISDIR`): Gán `i_op = &myfs_dir_inode_operations`, `i_fop = &simple_dir_operations`, `i_nlink = 2`.
    - Đối với tệp (`S_ISREG`): Gán `i_op = &myfs_file_inode_operations`, `i_fop = &myfs_file_operations`, `i_nlink = 1`.

4. **Khắc phục lỗi biên dịch:**
    - Ban đầu sử dụng `simple_file_inode_operations` và `simple_file_operations` cho tệp, nhưng các cấu trúc này không tồn tại trong kernel.
    - Sử dụng các cấu trúc `myfs_file_inode_operations` và `myfs_file_operations` để trống, đủ để hỗ trợ tạo/xóa tệp.

### 2. Triển khai các thao tác tệp (TODO 6)

Các nhiệm vụ sau đã được hoàn thành để hỗ trợ đọc/ghi tệp:

1. **Định nghĩa `myfs_file_inode_operations`:**
    - Gán `.setattr = simple_setattr` và `.getattr = simple_getattr` để xử lý thuộc tính tệp (kích thước, quyền).

2. **Định nghĩa `myfs_file_operations`:**
    - Gán các hàm VFS chung:
        - `.read_iter = generic_file_read_iter`: Đọc dữ liệu.
        - `.write_iter = generic_file_write_iter`: Ghi dữ liệu.
        - `.mmap = generic_file_mmap`: Ánh xạ bộ nhớ.
        - `.fsync = noop_fsync`: Đồng bộ hóa (không làm gì).
        - `.splice_read = generic_file_splice_read`: Đọc qua `splice`.
        - `.splice_write = iter_file_splice_write`: Ghi qua `splice`.
        - `.llseek = generic_file_llseek`: Di chuyển con trỏ tệp.

3. **Định nghĩa `myfs_aops`:**
    - Gán các hàm VFS chung:
        - `.readpage = simple_readpage`: Đọc trang dữ liệu.
        - `.write_begin = simple_write_begin`: Chuẩn bị ghi.
        - `.write_end = simple_write_end`: Hoàn tất ghi.

4. **Sửa đổi `myfs_get_inode`:**
    - Thêm `inode->i_mapping->a_ops = &myfs_aops` để khởi tạo thao tác không gian địa chỉ.
    - Đảm bảo `i_op` và `i_fop` được gán đúng cho tệp thông thường.

### 3. Biên dịch và kiểm tra

Thực hiện các bước sau để biên dịch và kiểm tra `myfs`:

1. **Biên dịch mô-đun:**
   ```bash
   cd /linux/tools/labs/skels/filesystems/myfs
   make clean
   make build
   ```

2. **Sao chép mô-đun và script:**
   ```bash
   make copy
   ```

3. **Nạp mô-đun và gắn hệ thống tệp:**
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   mkdir -p /mnt/myfs
   mount -t myfs none /mnt/myfs
   ```

4. **Kiểm tra thao tác thư mục (TODO 5):**
   ```bash
   touch /mnt/myfs/peanuts.txt
   mkdir -p /mnt/myfs/mountain/forest
   touch /mnt/myfs/mountain/forest/tree.txt
   ls -l /mnt/myfs
   rm /mnt/myfs/mountain/forest/tree.txt
   rmdir /mnt/myfs/mountain/forest
   rmdir /mnt/myfs/mountain
   ```

5. **Kiểm tra thao tác tệp (TODO 6):**
   ```bash
   echo "chocolate" > /mnt/myfs/peanuts.txt
   cat /mnt/myfs/peanuts.txt
   echo "hello world" > /mnt/myfs/test.txt
   cat /mnt/myfs/test.txt
   ls -l /mnt/myfs
   ```

6. **Chạy script kiểm tra:**
    - Script cho TODO 5:
      ```bash
      chmod +x /home/root/skels/filesystems/myfs/test-myfs-1.sh
      /home/root/skels/filesystems/myfs/test-myfs-1.sh
      ```
    - Script cho TODO 6:
      ```bash
      chmod +x /home/root/skels/filesystems/myfs/test-myfs-2.sh
      /home/root/skels/filesystems/myfs/test-myfs-2.sh
      ```

7. **Gỡ hệ thống tệp và mô-đun:**
   ```bash
   umount /mnt/myfs
   rmmod myfs
   ```

   **Lưu ý:** Nếu gặp lỗi `rmmod: can't unload 'myfs': Resource temporarily unavailable`, kiểm tra và kết thúc các tiến trình sử dụng `/mnt/myfs`:
   ```bash
   lsof /mnt/myfs
   fuser -m /mnt/myfs
   kill <pid>
   ```

## Giải thích mã nguồn

Tệp `myfs.c` triển khai hệ thống tệp `myfs` với các thành phần chính:

### a. `myfs_dir_inode_operations` (TODO 5)
```c
static const struct inode_operations myfs_dir_inode_operations = {
	.create = myfs_create,           // Tạo tệp thông thường
	.lookup = simple_lookup,         // Tìm kiếm mục
	.link = simple_link,             // Tạo liên kết cứng
	.unlink = simple_unlink,         // Xóa tệp
	.mkdir = myfs_mkdir,             // Tạo thư mục
	.rmdir = simple_rmdir,           // Xóa thư mục
	.mknod = myfs_mknod,             // Tạo node
	.rename = simple_rename,         // Đổi tên
};
```
- Định nghĩa các thao tác cho inode thư mục.
- Sử dụng hàm VFS chung cho các thao tác tiêu chuẩn, hàm tùy chỉnh cho tạo node.

### b. `myfs_file_inode_operations` (TODO 6)
```c
static const struct inode_operations myfs_file_inode_operations = {
	.setattr = simple_setattr,  // Thiết lập thuộc tính tệp
	.getattr = simple_getattr,  // Lấy thuộc tính tệp
};
```
- Xử lý thuộc tính tệp (kích thước, quyền) bằng các hàm VFS chung.

### c. `myfs_file_operations` (TODO 6)
```c
static const struct file_operations myfs_file_operations = {
	.read_iter  = generic_file_read_iter,    // Đọc dữ liệu tệp
	.write_iter = generic_file_write_iter,   // Ghi dữ liệu tệp
	.mmap       = generic_file_mmap,         // Ánh xạ bộ nhớ
	.fsync      = noop_fsync,                // Đồng bộ hóa
	.splice_read  = generic_file_splice_read, // Đọc qua splice
	.splice_write = iter_file_splice_write,   // Ghi qua splice
	.llseek     = generic_file_llseek,       // Di chuyển con trỏ tệp
};
```
- Hỗ trợ đọc, ghi, ánh xạ bộ nhớ, và các thao tác khác bằng hàm VFS chung.

### d. `myfs_aops` (TODO 6)
```c
static const struct address_space_operations myfs_aops = {
	.readpage    = simple_readpage,    // Đọc trang dữ liệu
	.write_begin = simple_write_begin, // Bắt đầu ghi
	.write_end   = simple_write_end,   // Kết thúc ghi
};
```
- Quản lý đọc/ghi trang bộ nhớ bằng hàm VFS chung.

### e. `myfs_get_inode`
```c
struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir,
		int mode)
{
	...
	inode->i_mapping->a_ops = &myfs_aops; // Thao tác không gian địa chỉ
	if (S_ISDIR(mode)) {
		inode->i_op = &myfs_dir_inode_operations; // Thao tác inode thư mục
		inode->i_fop = &simple_dir_operations; // Thao tác tệp thư mục
		set_nlink(inode, 2); // i_nlink = 2 cho . và ..
	} else if (S_ISREG(mode)) {
		inode->i_op = &myfs_file_inode_operations; // Thao tác inode cho tệp
		inode->i_fop = &myfs_file_operations; // Thao tác tệp cho tệp
		set_nlink(inode, 1); // i_nlink = 1 cho tệp thông thường
	}
	...
}
```
- Khởi tạo inode với các thao tác phù hợp cho thư mục và tệp.

### f. `myfs_mknod`, `myfs_create`, `myfs_mkdir`
- `myfs_mknod`: Tạo inode, liên kết với `dentry`, và cập nhật thuộc tính.
- `myfs_create`: Tạo tệp thông thường.
- `myfs_mkdir`: Tạo thư mục và tăng số liên kết thư mục cha.

## Kết quả kiểm tra

Kết quả kiểm tra xác nhận triển khai hoạt động đúng:

- **Nạp mô-đun và gắn hệ thống tệp:**
  ```bash
  insmod /home/root/skels/filesystems/myfs/myfs.ko
  mkdir -p /mnt/myfs
  mount -t myfs none /mnt/myfs
  ```
  Thông báo `root inode has 2 link(s)` xác nhận inode gốc được tạo đúng.

- **Thao tác đọc/ghi:**
  ```bash
  echo "chocolate" > /mnt/myfs/peanuts.txt
  cat /mnt/myfs/peanuts.txt
  echo "hello world" > /mnt/myfs/test.txt
  cat /mnt/myfs/test.txt
  ```
  Kết quả hiển thị đúng nội dung (`chocolate`, `hello world`).

- **Liệt kê tệp:**
  ```bash
  ls -l /mnt/myfs
  -rw-r--r--    1 root     root            10 Jun  1 01:16 peanuts.txt
  -rw-r--r--    1 root     root            12 Jun  1 01:17 test.txt
  ```

- **Script `test-myfs-2.sh`:**
  Script kiểm tra:
    - Tạo tệp (`myfile`).
    - Đổi tên (`mv myfile myrenamedfile`).
    - Tạo liên kết cứng (`ln myrenamedfile mylink`).
    - Ghi (`echo message > myrenamedfile`) và đọc (`cat myrenamedfile`).
    - Xóa liên kết (`rm mylink`) và tệp (`rm myrenamedfile`).
      Tất cả các thao tác đều thành công, nhưng script thất bại khi gỡ mô-đun do lỗi `rmmod`.

- **Vấn đề với `test-myfs-2.sh`:**
    - Lỗi `insmod: can't insert 'myfs.ko': No such file or directory` do script tìm `myfs.ko` trong thư mục hiện tại thay vì `/home/root/skels/filesystems/myfs/`.
    - Lỗi `rmmod: can't unload 'myfs': Resource temporarily unavailable` do hệ thống tệp hoặc tiến trình vẫn đang sử dụng mô-đun.

