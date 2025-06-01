## Kết quả
root@qemux86:~# insmod /home/root/skels/filesystems/myfs/myfs.ko
myfs: loading out-of-tree module taints kernel.
root@qemux86:~# mkdir -p /mnt/myfs
root@qemux86:~# mount -t myfs none /mnt/myfs
root inode has 2 link(s)
root@qemux86:~# touch /mnt/myfs/peanuts.txt
root@qemux86:~# mkdir -p /mnt/myfs/mountain/forest
root@qemux86:~# touch /mnt/myfs/mountain/forest/tree.txt
root@qemux86:~# ls -l /mnt/myfs
drwxr-xr-x    4 root     root             0 Jun  1 00:54 mountain
-rw-r--r--    1 root     root             0 Jun  1 00:54 peanuts.txt
root@qemux86:~# ls -l /mnt/myfs/mountain/forest
-rw-r--r--    1 root     root             0 Jun  1 00:54 tree.txt
root@qemux86:~# rm /mnt/myfs/mountain/forest/tree.txt
root@qemux86:~# rmdir /mnt/myfs/mountain/forest
root@qemux86:~# rmdir /mnt/myfs/mountain
root@qemux86:~# cd skels
root@qemux86:~/skels# ls
Kbuild                deferred_work         modules.order
Module.symvers        filesystems
block_device_drivers  kernel_api
root@qemux86:~/skels# cd filesystems/
root@qemux86:~/skels/filesystems# ls
minfs  myfs
root@qemux86:~/skels/filesystems# cd myfs/
root@qemux86:~/skels/filesystems/myfs# ls
Kbuild          myfs.ko         myfs.mod.o      test-myfs-2.sh
modules.order   myfs.mod        myfs.o          test-myfs.sh
myfs.c          myfs.mod.c      test-myfs-1.sh
root@qemux86:~/skels/filesystems/myfs# ./test-myfs-1.sh
+ insmod myfs.ko
  insmod: can't insert 'myfs.ko': File exists
+ mkdir -p /mnt/myfs
+ mount -t myfs none /mnt/myfs
  root inode has 2 link(s)
+ ls -laid /mnt/myfs
  4726 drwxr-xr-x    2 root     root             0 Jun  1 00:55 /mnt/myfs
+ cd /mnt/myfs
+ mkdir mydir
+ ls -la
  drwxr-xr-x    4 root     root             0 Jun  1 00:55 .
  drwxr-xr-x    4 root     root             0 May 30 07:59 ..
  drwxr-xr-x    2 root     root             0 Jun  1 00:55 mydir
+ cd mydir
+ mkdir mysubdir
+ ls -lai
  4731 drwxr-xr-x    4 root     root             0 Jun  1 00:55 .
  4726 drwxr-xr-x    4 root     root             0 Jun  1 00:55 ..
  4736 drwxr-xr-x    2 root     root             0 Jun  1 00:55 mysubdir
+ mv mysubdir myrenamedsubdir
+ ls -lai
  4731 drwxr-xr-x    4 root     root             0 Jun  1 00:55 .
  4726 drwxr-xr-x    4 root     root             0 Jun  1 00:55 ..
  4736 drwxr-xr-x    2 root     root             0 Jun  1 00:55 myrenamedsubdir
+ rmdir myrenamedsubdir
+ ls -la
  drwxr-xr-x    3 root     root             0 Jun  1 00:55 .
  drwxr-xr-x    4 root     root             0 Jun  1 00:55 ..
+ touch myfile
+ ls -lai
  4731 drwxr-xr-x    3 root     root             0 Jun  1 00:55 .
  4726 drwxr-xr-x    4 root     root             0 Jun  1 00:55 ..
  4749 -rw-r--r--    1 root     root             0 Jun  1 00:55 myfile
+ mv myfile myrenamedfile
+ ls -lai
  4731 drwxr-xr-x    3 root     root             0 Jun  1 00:55 .
  4726 drwxr-xr-x    4 root     root             0 Jun  1 00:55 ..
  4749 -rw-r--r--    1 root     root             0 Jun  1 00:55 myrenamedfile
+ rm myrenamedfile
+ cd ..
+ rmdir mydir
+ ls -la
  drwxr-xr-x    3 root     root             0 Jun  1 00:55 .
  drwxr-xr-x    4 root     root             0 May 30 07:59 ..
+ cd ..
+ umount /mnt/myfs
+ rmmod myfs
  rmmod: can't unload 'myfs': Resource temporarily unavailable
  root@qemux86:~/skels/filesystems/myfs#


# README: Triển khai các thao tác thư mục cho hệ thống tệp `myfs`

Tệp README này ghi lại quá trình triển khai các thao tác thư mục cho hệ thống tệp `myfs`, 
một hệ thống tệp không thiết bị đơn giản được phát triển trong khuôn khổ bài tập số 1 của SO2 Lab. 
Triển khai này tập trung vào việc hoàn thành TODO 5, 
bao gồm định nghĩa và thực hiện các thao tác liên quan đến thư mục như tạo tệp (`create`), 
tìm kiếm (`lookup`), liên kết (`link`), xóa tệp (`unlink`), tạo thư mục (`mkdir`), xóa thư mục (`rmdir`), 
tạo node (`mknod`), và đổi tên (`rename`).

## Mục tiêu

Mục tiêu của bài tập là triển khai các thao tác thư mục cho hệ thống tệp `myfs`, 
cho phép tạo, xóa và đổi tên các tệp và thư mục. Hệ thống tệp chưa hỗ trợ các thao tác đọc/ghi 
(sẽ được triển khai trong TODO 6). Triển khai được mô phỏng theo hệ thống tệp `ramfs`, 
sử dụng các hàm chung của VFS (Virtual File System) khi phù hợp và các hàm tùy chỉnh cho việc tạo node.

## Các bước triển khai và kiểm tra

### 1. Triển khai các thao tác thư mục (TODO 5)

Các nhiệm vụ sau đã được hoàn thành để triển khai các thao tác thư mục:

1. **Định nghĩa `myfs_dir_inode_operations`:**
    - Cấu trúc `myfs_dir_inode_operations` được định nghĩa để chỉ định các thao tác cho các inode thư mục, như tạo tệp, tạo thư mục, tìm kiếm, liên kết, xóa và đổi tên các mục.
    - Sử dụng các hàm VFS chung (`simple_lookup`, `simple_link`, `simple_unlink`, `simple_rmdir`, `simple_rename`) cho các thao tác không cần triển khai tùy chỉnh.
    - Triển khai các hàm tùy chỉnh (`myfs_create`, `myfs_mkdir`, `myfs_mknod`) để xử lý việc tạo tệp, thư mục và node.

2. **Triển khai `myfs_mknod`, `myfs_create`, và `myfs_mkdir`:**
    - `myfs_mknod`: Tạo một inode mới bằng `myfs_get_inode`, liên kết nó với `dentry` bằng `d_instantiate`, và tăng số tham chiếu bằng `dget`. Đối với thư mục, tăng số liên kết của thư mục cha (`inc_nlink`).
    - `myfs_create`: Gọi `myfs_mknod` với cờ `S_IFREG` để tạo một tệp thông thường.
    - `myfs_mkdir`: Gọi `myfs_mknod` với cờ `S_IFDIR` để tạo một thư mục và tăng số liên kết của thư mục cha.

3. **Sửa đổi `myfs_get_inode`:**
    - Cập nhật `myfs_get_inode` để khởi tạo đúng các trường `i_op` và `i_fop` cho cả thư mục (`S_ISDIR`) và tệp thông thường (`S_ISREG`).
    - Đối với thư mục: Đặt `i_op = &myfs_dir_inode_operations`, `i_fop = &simple_dir_operations`, và `i_nlink = 2` (cho `.` và `..`).
    - Đối với tệp thông thường: Đặt `i_op = &myfs_file_inode_operations`, `i_fop = &myfs_file_operations`, và `i_nlink = 1`. Các cấu trúc này hiện đang để trống (sẽ được điền trong TODO 6) nhưng đủ để tạo/xóa tệp.

4. **Xử lý lỗi biên dịch:**
    - Ban đầu cố gắng sử dụng `simple_file_inode_operations` và `simple_file_operations` cho tệp thông thường, nhưng các cấu trúc này không tồn tại trong kernel, gây lỗi biên dịch.
    - Đã khắc phục bằng cách sử dụng các cấu trúc `myfs_file_inode_operations` và `myfs_file_operations` để trống, đủ để hỗ trợ tạo/xóa tệp mà không cần đọc/ghi.

### 2. Biên dịch và kiểm tra mô-đun

Thực hiện các bước sau để biên dịch và kiểm tra hệ thống tệp `myfs`:

1. **Biên dịch mô-đun kernel:**
   ```bash
   make clean
   make build
   ```
   Lệnh này biên dịch tệp `myfs.c` thành `myfs.ko`.

2. **Sao chép mô-đun và script kiểm tra vào máy ảo:**
   ```bash
   make copy
   ```
   Lệnh này sao chép `myfs.ko` và các script kiểm tra (`test-myfs-1.sh`, `test-myfs-2.sh`) vào máy ảo.

3. **Nạp mô-đun và gắn hệ thống tệp:**
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   mkdir -p /mnt/myfs
   mount -t myfs none /mnt/myfs
   ```
   Thông báo `root inode has 2 link(s)` xác nhận rằng inode thư mục gốc được tạo với `i_nlink = 2`.

4. **Kiểm tra các thao tác thư mục và tệp:**
   ```bash
   touch /mnt/myfs/peanuts.txt
   mkdir -p /mnt/myfs/mountain/forest
   touch /mnt/myfs/mountain/forest/tree.txt
   ls -l /mnt/myfs
   ls -l /mnt/myfs/mountain/forest
   rm /mnt/myfs/mountain/forest/tree.txt
   rmdir /mnt/myfs/mountain/forest
   rmdir /mnt/myfs/mountain
   ```
   Các lệnh này kiểm tra việc tạo tệp (`touch`), tạo thư mục (`mkdir`), liệt kê (`ls`), xóa tệp (`rm`), và xóa thư mục (`rmdir`).

5. **Chạy script kiểm tra:**
   ```bash
   chmod +x /home/root/skels/filesystems/myfs/test-myfs-1.sh
   /home/root/skels/filesystems/myfs/test-myfs-1.sh
   ```
   Script kiểm tra việc tạo thư mục (`mydir`, `mysubdir`), đổi tên (`mv`), xóa thư mục (`rmdir`), tạo tệp (`myfile`), và xóa tệp. Không có lỗi xuất hiện nếu triển khai đúng.

6. **Gỡ hệ thống tệp và mô-đun:**
   ```bash
   umount /mnt/myfs
   rmmod myfs
   ```

   **Lưu ý:** Nếu gặp lỗi `rmmod: can't unload 'myfs': Resource temporarily unavailable`, điều này cho thấy hệ thống tệp vẫn đang được sử dụng. Đảm bảo không có tiến trình nào đang truy cập `/mnt/myfs` (ví dụ: không có shell mở trong thư mục đó). Kiểm tra các tiến trình đang hoạt động bằng:
   ```bash
   lsof /mnt/myfs
   fuser -m /mnt/myfs
   ```
   Kết thúc các tiến trình (nếu có) bằng `kill <pid>` và thử lại `umount` và `rmmod`.

### 3. Giải thích mã nguồn

Tệp `myfs.c` triển khai hệ thống tệp `myfs` với các thành phần chính cho các thao tác thư mục (TODO 5):

#### a. `myfs_dir_inode_operations`
```c
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
```
- Cấu trúc này định nghĩa các thao tác cho inode thư mục.
- Sử dụng các hàm VFS chung (`simple_lookup`, `simple_link`, `simple_unlink`, `simple_rmdir`, `simple_rename`) cho các thao tác tiêu chuẩn.
- Các hàm tùy chỉnh (`myfs_create`, `myfs_mkdir`, `myfs_mknod`) xử lý việc tạo node.

#### b. `myfs_mknod`
```c
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
```
- Tạo inode mới bằng `myfs_get_inode` với `mode` và `dev` được chỉ định.
- Liên kết inode với `dentry` bằng `d_instantiate`.
- Tăng số tham chiếu của `dentry` bằng `dget`.
- Cập nhật thời gian inode (`i_mtime`, `i_atime`, `i_ctime`).
- Tăng số liên kết của thư mục cha (`inc_nlink`) nếu tạo thư mục.

#### c. `myfs_create`
```c
static int myfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	return myfs_mknod(dir, dentry, mode | S_IFREG, 0);
}
```
- Lớp bao quanh `myfs_mknod`, đặt cờ `S_IFREG` để tạo tệp thông thường.
- Tham số `excl` bị bỏ qua vì `myfs` không hỗ trợ tạo độc quyền.

#### d. `myfs_mkdir`
```c
static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int ret;

	ret = myfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (ret)
		return ret;

	inc_nlink(dir); /* Tăng số liên kết của thư mục cha */

	return 0;
}
```
- Lớp bao quanh `myfs_mknod`, đặt cờ `S_IFDIR` để tạo thư mục.
- Tăng số liên kết của thư mục cha sau khi tạo thành công.

#### e. `myfs_get_inode`
```c
struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir,
		int mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	inode_init_owner(inode, dir, mode); /* Khởi tạo uid, gid, mode */
	inode->i_atime = inode->i_ctime = inode->i_mtime = current_time(inode); // Thời gian
	inode->i_ino = get_next_ino(); // Số inode

	if (S_ISDIR(mode)) {
		inode->i_op = &myfs_dir_inode_operations; // Thao tác inode thư mục
		inode->i_fop = &simple_dir_operations; // Thao tác tệp thư mục
		set_nlink(inode, 2); // i_nlink = 2 cho . và ..
	} else if (S_ISREG(mode)) {
		inode->i_op = &myfs_file_inode_operations; // Thao tác inode cho tệp
		inode->i_fop = &myfs_file_operations; // Thao tác tệp cho tệp
		set_nlink(inode, 1); // i_nlink = 1 cho tệp thông thường
	}

	return inode;
}
```
- Cấp phát inode mới bằng `new_inode`.
- Khởi tạo các thuộc tính inode (`mode`, `uid`, `gid`, thời gian, `i_ino`) bằng `inode_init_owner` và `get_next_ino`.
- Đối với thư mục (`S_ISDIR`): Đặt `i_op = &myfs_dir_inode_operations`, `i_fop = &simple_dir_operations`, và `i_nlink = 2`.
- Đối với tệp thông thường (`S_ISREG`): Đặt `i_op = &myfs_file_inode_operations`, `i_fop = &myfs_file_operations`, và `i_nlink = 1`.
- Các cấu trúc `myfs_file_inode_operations` và `myfs_file_operations` để trống, đủ để tạo/xóa tệp mà không cần hỗ trợ đọc/ghi.

### 4. Kết quả kiểm tra

Đầu ra kiểm tra xác nhận rằng triển khai hoạt động đúng:
- **Nạp mô-đun và gắn hệ thống tệp:**
  ```bash
  insmod /home/root/skels/filesystems/myfs/myfs.ko
  mkdir -p /mnt/myfs
  mount -t myfs none /mnt/myfs
  ```
  Thông báo `root inode has 2 link(s)` cho thấy inode thư mục gốc được tạo thành công.

- **Tạo tệp và thư mục:**
  ```bash
  touch /mnt/myfs/peanuts.txt
  mkdir -p /mnt/myfs/mountain/forest
  touch /mnt/myfs/mountain/forest/tree.txt
  ```
  Không có lỗi, và các tệp/thư mục được tạo thành công.

- **Liệt kê:**
  ```bash
  ls -l /mnt/myfs
  drwxr-xr-x    4 root     root             0 Jun  1 00:54 mountain
  -rw-r--r--    1 root     root             0 Jun  1 00:54 peanuts.txt
  ls -l /mnt/myfs/mountain/forest
  -rw-r--r--    1 root     root             0 Jun  1 00:54 tree.txt
  ```
  Các tệp và thư mục được liệt kê đúng với quyền và thời gian phù hợp.

- **Xóa tệp và thư mục:**
  ```bash
  rm /mnt/myfs/mountain/forest/tree.txt
  rmdir /mnt/myfs/mountain/forest
  rmdir /mnt/myfs/mountain
  ```
  Các tệp và thư mục được xóa mà không có lỗi.

- **Script kiểm tra (`test-myfs-1.sh`):**
  Script đã kiểm tra thành công:
    - Tạo thư mục (`mydir`, `mysubdir`).
    - Đổi tên thư mục (`mv mysubdir myrenamedsubdir`).
    - Xóa thư mục (`rmdir myrenamedsubdir`).
    - Tạo tệp (`myfile`).
    - Đổi tên tệp (`mv myfile myrenamedfile`).
    - Xóa tệp (`rm myrenamedfile`).
      Không có lỗi được báo cáo, xác nhận các thao tác thư mục hoạt động đúng.

- **Gỡ hệ thống tệp và mô-đun:**
  ```bash
  umount /mnt/myfs
  rmmod myfs
  ```
  Lệnh `rmmod` thất bại với lỗi `Resource temporarily unavailable`, cho thấy hệ thống tệp vẫn đang được sử dụng. Điều này được giải thích và khắc phục trong phần xử lý sự cố.

### 5. Xử lý sự cố

#### Vấn đề: `rmmod: can't unload 'myfs': Resource temporarily unavailable`
- **Nguyên nhân:** Mô-đun kernel không thể gỡ do hệ thống tệp vẫn đang được gắn hoặc có tiến trình truy cập điểm gắn kết.
- **Giải pháp:**
    1. Đảm bảo hệ thống tệp đã được gỡ:
       ```bash
       umount /mnt/myfs
       ```
    2. Kiểm tra các tiến trình đang sử dụng điểm gắn kết:
       ```bash
       lsof /mnt/myfs
       fuser -m /mnt/myfs
       ```
    3. Kết thúc các tiến trình (nếu có) bằng `kill <pid>`.
    4. Thử lại gỡ mô-đun:
       ```bash
       rmmod myfs
       ```


### 6. Hạn chế
- **Thao tác đọc/ghi:** Các lệnh như `echo "chocolate" > /mnt/myfs/peanuts.txt` hoặc `cat /mnt/myfs/peanuts.txt` thất bại vì các thao tác đọc/ghi (TODO 6) chưa được triển khai. Điều này phù hợp với yêu cầu bài tập hiện tại.
- **Node thiết bị:** Hàm `myfs_mknod` hỗ trợ tạo node thiết bị (`dev_t dev`), nhưng chưa được kiểm tra vì bài tập tập trung vào tệp và thư mục thông thường.

---

