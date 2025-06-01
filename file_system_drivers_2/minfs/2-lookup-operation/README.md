
# minfs - Hệ thống Tệp Đơn giản Không Thiết bị

## Tổng quan
`minfs` là một hệ thống tệp đơn giản không sử dụng thiết bị (no-device filesystem), 
được triển khai dưới dạng module kernel Linux nhằm mục đích học tập trong bài tập SO2 Lab. 
Hệ thống tệp này không lưu trữ dữ liệu vĩnh viễn trên thiết bị mà hỗ trợ các thao tác cơ bản như tạo tệp, tạo thư mục, 
liệt kê nội dung thư mục, và quản lý inode trong bộ nhớ.

Bài tập này tập trung vào việc triển khai thao tác `lookup` 
để hỗ trợ liệt kê nội dung thư mục (`ls -l`) thông qua hàm `minfs_lookup` và hàm phụ trợ `minfs_find_entry`.

## Giải thích Mã nguồn

### Các Thành phần Chính
Module kernel `minfs` triển khai một hệ thống tệp tối thiểu với các thành phần sau:

1. **Loại Hệ thống Tệp (`myfs_fs_type`)**:
    - Được định nghĩa trong cấu trúc `struct file_system_type`, chỉ định tên hệ thống tệp ("minfs"), hàm gắn kết (`myfs_mount`), và hàm dọn dẹp superblock (`kill_litter_super`).
    - Được đăng ký trong quá trình khởi tạo module (`myfs_init`) và hủy đăng ký khi thoát module (`myfs_exit`).

2. **Thao tác Superblock (`myfs_super_ops`)**:
    - Quản lý các thao tác cấp superblock như xóa inode (`generic_drop_inode`) và thống kê hệ thống tệp (`simple_statfs`).
    - Thiết lập các thuộc tính hệ thống tệp như kích thước khối (4096 byte), số ma thuật (`0xbeefcafe`), và kích thước tệp tối đa (`MAX_LFS_FILESIZE`).

3. **Tạo Inode (`myfs_get_inode`)**:
    - Tạo một inode mới cho tệp hoặc thư mục.
    - Khởi tạo các thuộc tính inode như chế độ (mode), UID, GID, thời gian (atime, ctime, mtime), và số inode (`i_ino`) bằng `get_next_ino`.
    - Gán các thao tác phù hợp dựa trên loại inode:
        - Thư mục: Sử dụng `myfs_dir_inode_operations` và `simple_dir_operations`.
        - Tệp thông thường: Sử dụng `myfs_file_inode_operations` và `myfs_file_operations`.
    - Gán thao tác ánh xạ bộ nhớ (`myfs_aops`) cho tất cả các inode.

4. **Thao tác Inode Thư mục (`myfs_dir_inode_operations`)**:
    - Xử lý các thao tác liên quan đến thư mục, bao gồm:
        - Tạo tệp (`myfs_create`).
        - Tìm kiếm mục (`minfs_lookup`).
        - Tạo thư mục (`myfs_mkdir`).
        - Xóa thư mục (`simple_rmdir`).
        - Tạo liên kết cứng (`simple_link`).
        - Xóa tệp (`simple_unlink`).
        - Đổi tên tệp/thư mục (`simple_rename`).
        - Tạo node đặc biệt (`myfs_mknod`).

5. **Thao tác Tệp (`myfs_file_operations`)**:
    - Quản lý các thao tác trên tệp thông thường, bao gồm:
        - Mở tệp (`simple_open`).
        - Đọc tệp (`generic_file_read_iter`).
        - Ghi tệp (`generic_file_write_iter`).
        - Di chuyển con trỏ tệp (`generic_file_llseek`).

6. **Thao tác Inode Tệp (`myfs_file_inode_operations`)**:
    - Quản lý thuộc tính inode của tệp thông thường, bao gồm:
        - Thiết lập thuộc tính như quyền hoặc kích thước (`simple_setattr`).
        - Lấy thuộc tính (`simple_getattr`).

7. **Thao tác Ánh xạ Bộ nhớ (`myfs_aops`)**:
    - Xử lý ánh xạ bộ nhớ cho tệp, bao gồm:
        - Đọc một trang (`simple_readpage`).
        - Bắt đầu ghi (`simple_write_begin`).
        - Kết thúc ghi (`simple_write_end`).

8. **Thao tác Tạo Node**:
    - `myfs_mknod`: Tạo một inode mới và liên kết với một `dentry`, hỗ trợ tệp, thư mục, hoặc node đặc biệt.
    - `myfs_create`: Tạo một tệp thông thường bằng cách gọi `myfs_mknod` với cờ `S_IFREG`.
    - `myfs_mkdir`: Tạo một thư mục bằng cách gọi `myfs_mknod` với cờ `S_IFDIR` và quản lý số liên kết (link count).

9. **Thao tác Tìm kiếm (`minfs_lookup` và `minfs_find_entry`)**:
    - `minfs_lookup`: Thay thế hàm `simple_lookup` trong `myfs_dir_inode_operations` để hỗ trợ tìm kiếm mục trong thư mục.
    - `minfs_find_entry`: Duyệt qua các mục trong khối dữ liệu của thư mục để tìm một mục có tên khớp với tên được cung cấp.

### Triển khai Thao tác Tìm kiếm (TODO 6)
Để liệt kê nội dung thư mục chính xác, cần triển khai thao tác `lookup` thông qua hàm `minfs_lookup`, sử dụng hàm phụ trợ `minfs_find_entry`. Dưới đây là chi tiết triển khai:

- **Hàm `minfs_find_entry`**:
    - **Mục đích**: Tìm một mục (dentry) trong thư mục dựa trên tên được cung cấp.
    - **Cách hoạt động**:
        - Lấy inode của thư mục cha từ `dentry->d_parent->d_inode`.
        - Lấy chỉ số khối dữ liệu từ cấu trúc `struct minfs_inode_info` của thư mục.
        - Đọc khối dữ liệu bằng `sb_bread` để truy cập nội dung khối (`bh->b_data`).
        - Duyệt qua mảng các mục `struct minfs_dir_entry` (tối đa `MINFS_NUM_ENTRIES` mục) trong khối dữ liệu.
        - So sánh tên của mỗi mục với tên được cung cấp bằng `strcmp`. Bỏ qua các mục có `ino` bằng 0 (các slot trống).
        - Nếu tìm thấy mục khớp, lưu vào biến `final_de`. Nếu không tìm thấy, `final_de` giữ giá trị `NULL`.
    - **Lưu ý**: Sử dụng phép toán con trỏ để truy cập các mục `struct minfs_dir_entry` trong `bh->b_data`.

- **Hàm `minfs_lookup`**:
    - **Mục đích**: Xử lý thao tác tìm kiếm trong thư mục, gọi `minfs_find_entry` để tìm mục và trả về `dentry` tương ứng.
    - **Cách hoạt động**:
        - Thay thế lời gọi `simple_lookup` trong `myfs_dir_inode_operations` bằng `minfs_lookup`.
        - Gọi `minfs_find_entry` để tìm mục trong thư mục.
        - Nếu tìm thấy, liên kết `dentry` với inode tương ứng bằng `d_add`. Nếu không, trả về `dentry` âm (negative dentry).

### Mã nguồn Triển khai Thao tác Tìm kiếm
Dưới đây là đoạn mã ví dụ cho `minfs_lookup` và `minfs_find_entry`:

```c
#include <linux/buffer_head.h>
#include <linux/string.h>

/* Định nghĩa cấu trúc minfs_dir_entry và minfs_inode_info */
#define MINFS_NUM_ENTRIES 32  // Giả định số mục tối đa trong một khối

struct minfs_dir_entry {
    __u32 ino;           // Số inode
    char name[256];      // Tên mục (giả định độ dài tối đa)
};

struct minfs_inode_info {
    __u32 data_block;    // Chỉ số khối dữ liệu
    struct inode vfs_inode;
};

/* Hàm minfs_find_entry */
static struct minfs_dir_entry *minfs_find_entry(struct dentry *dentry, struct buffer_head **bh)
{
    struct inode *dir = dentry->d_parent->d_inode;
    struct minfs_inode_info *minfs_ii = container_of(dir, struct minfs_inode_info, vfs_inode);
    const char *name = dentry->d_name.name;
    struct minfs_dir_entry *final_de = NULL;
    int i;

    *bh = sb_bread(dir->i_sb, minfs_ii->data_block);
    if (!*bh)
        return NULL;

    struct minfs_dir_entry *de = (struct minfs_dir_entry *)(*bh)->b_data;

    for (i = 0; i < MINFS_NUM_ENTRIES; i++) {
        if (de[i].ino != 0 && strcmp(name, de[i].name) == 0) {
            final_de = &de[i];
            break;
        }
    }

    return final_de;
}

/* Hàm minfs_lookup */
static struct dentry *minfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    struct buffer_head *bh;
    struct minfs_dir_entry *de = minfs_find_entry(dentry, &bh);
    struct inode *inode = NULL;

    if (de) {
        inode = myfs_get_inode(dir->i_sb, dir, 0); // Lấy inode từ ino trong de
        if (inode)
            inode->i_ino = de->ino;
    }

    brelse(bh);
    d_add(dentry, inode);
    return NULL;
}
```

### Giải thích Mã nguồn Tìm kiếm
- **Cấu trúc `minfs_dir_entry`**:
    - Chứa số inode (`ino`) và tên mục (`name`).
    - Giả định độ dài tên tối đa là 256 ký tự.
- **Cấu trúc `minfs_inode_info`**:
    - Lưu trữ chỉ số khối dữ liệu (`data_block`) của thư mục.
- **Hàm `minfs_find_entry`**:
    - Đọc khối dữ liệu của thư mục bằng `sb_bread`.
    - Duyệt qua các mục trong `bh->b_data`, kiểm tra tên bằng `strcmp`.
    - Trả về con trỏ đến `minfs_dir_entry` nếu tìm thấy, hoặc `NULL` nếu không.
- **Hàm `minfs_lookup`**:
    - Gọi `minfs_find_entry` để tìm mục.
    - Nếu tìm thấy, tạo inode mới bằng `myfs_get_inode` và gán số inode từ `de->ino`.
    - Gắn inode vào `dentry` bằng `d_add`.
    - Giải phóng bộ đệm (`brelse`) và trả về `NULL` (theo chuẩn kernel).

### Cập nhật `myfs_dir_inode_operations`
Để sử dụng `minfs_lookup`, cần cập nhật `myfs_dir_inode_operations`:

```c
static const struct inode_operations myfs_dir_inode_operations = {
    .create = myfs_create,
    .lookup = minfs_lookup,  // Thay simple_lookup bằng minfs_lookup
    .link = simple_link,
    .unlink = simple_unlink,
    .mkdir = myfs_mkdir,
    .rmdir = simple_rmdir,
    .mknod = myfs_mknod,
    .rename = simple_rename,
};
```

## Yêu cầu
Để biên dịch và kiểm tra module kernel `minfs`, bạn cần:
- Môi trường Linux (ví dụ: máy ảo chạy Linux với quyền root).
- Bộ công cụ phát triển kernel (bao gồm `gcc`, `make`, và `linux-headers`).
- Một thiết bị khối (ví dụ: `/dev/vdb` hoặc `/dev/vdc`) để kiểm tra, mặc dù `minfs` không yêu cầu lưu trữ vĩnh viễn.
- Công cụ `mkfs.minfs` để khởi tạo cấu trúc hệ thống tệp.

## Biên dịch
1. **Chuyển đến thư mục module kernel**:
   ```bash
   cd /path/to/skels/filesystems/minfs/kernel
   ```
2. **Biên dịch module**:
   Đảm bảo có file `Makefile` được cấu hình đúng. Ví dụ `Makefile`:
   ```makefile
   obj-m += minfs.o
   all:
       make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
   clean:
       make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
   ```
   Chạy lệnh:
   ```bash
   make
   ```
   Kết quả sẽ tạo file `minfs.ko` trong thư mục hiện tại.

## Các Bước Kiểm tra
Thực hiện các bước sau để kiểm tra hệ thống tệp `minfs`:

1. **Cài đặt module kernel**:
   ```bash
   insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko
   ```
   Kiểm tra module đã được nạp:
   ```bash
   lsmod | grep minfs
   ```
   Kết quả mong đợi:
   ```
   minfs 16384 0 - Live 0xe0831000 (O)
   ```

2. **Kiểm tra hệ thống tệp đã đăng ký**:
   ```bash
   cat /proc/filesystems | grep minfs
   ```
   Kết quả mong đợi:
   ```
           minfs
   ```

3. **Định dạng thiết bị**:
   Sử dụng công cụ `mkfs.minfs` để khởi tạo thiết bị (ví dụ: `/dev/vdb`):
   ```bash
   /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdb
   ```
   Đảm bảo `/dev/vdb` là thiết bị hợp lệ (kiểm tra bằng `lsblk`).

4. **Tạo điểm gắn kết**:
   ```bash
   mkdir -p /mnt/minfs
   ```

5. **Gắn kết hệ thống tệp**:
   ```bash
   mount -t minfs /dev/vdb /mnt/minfs
   ```
   Kiểm tra gắn kết:
   ```bash
   mount | grep minfs
   ```
   Kết quả mong đợi:
   ```
   /dev/vdb on /mnt/minfs type minfs (rw,relatime)
   ```

6. **Kiểm tra liệt kê nội dung thư mục**:
   ```bash
   ls -l /mnt/minfs
   ```
   Nếu thư mục rỗng (chưa có tệp/thư mục con), kết quả sẽ không hiển thị gì, điều này là bình thường.

7. **Chạy các script kiểm tra**:
   Sử dụng các script kiểm tra cung cấp sẵn:
   ```bash
   ./test-minfs-0.sh
   ./test-minfs-1.sh
   ```
   Nếu triển khai đúng, các script này sẽ chạy mà không báo lỗi.

8. **Thử tạo tệp (lưu ý)**:
   Chạy lệnh:
   ```bash
   touch /mnt/minfs/peanuts.txt
   ```
   Lệnh này có thể báo lỗi `Permission denied` nếu thao tác tạo tệp (`myfs_create`) chưa được triển khai đầy đủ. Điều này sẽ được giải quyết trong bài tập tiếp theo.

9. **Tháo gắn kết hệ thống tệp**:
   ```bash
   umount /mnt/minfs
   ```

10. **Gỡ module kernel**:
    ```bash
    rmmod minfs
    ```
    Kiểm tra module đã được gỡ:
    ```bash
    lsmod | grep minfs
    ```
    Kết quả mong đợi: Không có đầu ra, cho biết module đã được gỡ.

## Xử lý Lỗi
- **Lỗi `Permission denied` khi chạy `touch`**:
    - Kiểm tra quyền của thư mục gốc trong `myfs_fill_super`. Thử thay đổi thành `S_IRWXU | S_IRWXG | S_IRWXO` (777) để kiểm tra:
      ```c
      root_inode = myfs_get_inode(sb, NULL, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
      ```
    - Kiểm tra log kernel:
      ```bash
      dmesg | tail
      ```
    - Đảm bảo `myfs_create` và `myfs_mknod` được triển khai đúng.

- **Không có đầu ra khi chạy `ls -l`**:
    - Nếu thư mục `/mnt/minfs` rỗng, `ls -l` không hiển thị gì là bình thường.
    - Đảm bảo `minfs_lookup` và `minfs_find_entry` được triển khai đúng để hỗ trợ liệt kê nội dung.

- **Lỗi cài đặt module**:
    - Kiểm tra lỗi biên dịch trong đầu ra của `make`.
    - Đảm bảo kernel headers khớp với phiên bản kernel đang chạy (`uname -r`).

- **Lỗi gắn kết**:
    - Kiểm tra thiết bị `/dev/vdb` có hợp lệ không (`lsblk`).
    - Đảm bảo `mkfs.minfs` chạy thành công.

## Hạn chế
- **Không lưu trữ vĩnh viễn**: `minfs` là hệ thống tệp không sử dụng thiết bị, 
- dữ liệu chỉ tồn tại trong bộ nhớ và mất khi tháo gắn kết.
- **Chức năng tối thiểu**: Chỉ hỗ trợ các thao tác cơ bản như tạo tệp/thư mục và liệt kê nội dung.
- Các tính năng như lưu trữ nội dung tệp cần triển khai thêm trong `myfs_aops` và `mkfs.minfs`.
- **Xử lý lỗi**: Triển khai hiện tại sử dụng xử lý lỗi đơn giản. Hệ thống tệp sản xuất sẽ cần kiểm tra mạnh mẽ hơn.

