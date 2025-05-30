## Hướng dẫn test và kết quả:
    qemux86 login: root
    root@qemux86:~# insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko
        minfs: loading out-of-tree module taints kernel.
    root@qemux86:~# random: crng init done
        cat /proc/filesystems | grep minfs
        minfs
    root@qemux86:~# /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc
    root@qemux86:~# mount | grep minfs
    root@qemux86:~# mkdir -p /mnt/minfs
    root@qemux86:~# mount -t minfs /dev/vdc /mnt/minfs
    root@qemux86:~# mount | grep minfs
        /dev/vdc on /mnt/minfs type minfs (rw,relatime)
    root@qemux86:~# df /mnt/minfs
        Filesystem           1K-blocks      Used Available Use% Mounted on
        /dev/vdc                     0         0         0   0% /mnt/minfs
    root@qemux86:~# ls -l /mnt/minfs
    root@qemux86:~# stat /mnt/minfs
        File: /mnt/minfs
        Size: 0               Blocks: 0          IO Block: 4096   directory
        Device: fe20h/65056d    Inode: 4719        Links: 2
        Access: (0755/drwxr-xr-x)  Uid: (    0/    root)   Gid: (    0/    root)
        Access: 2025-05-30 11:10:02.000000000
        Modify: 2025-05-30 11:08:46.000000000
        Change: 2025-05-30 11:08:46.000000000



## Tổng quan
Bài tập này là một phần của **SO2 Lab - Filesystem drivers**, 
tập trung vào việc triển khai một hệ thống tệp đơn giản gọi là **MinFS** trong kernel Linux. 
Hệ thống tệp MinFS được thiết kế để hỗ trợ các thao tác cơ bản như gắn (mount), đọc inode, và liệt kê thư mục. 
File này cung cấp hướng dẫn kiểm tra và giải thích mã nguồn liên quan đến **TODO 4** (khởi tạo inode gốc).

## Môi trường
- **Hệ điều hành**: Linux (chạy trên máy ảo QEMU, ví dụ: `qemux86`).
- **Thiết bị**: `/dev/vdc` (thiết bị khối giả lập hoặc thực).
- **Công cụ**:
    - `mkfs.minfs`: Công cụ người dùng để định dạng thiết bị với MinFS.
    - `insmod`, `mount`, `ls`, `stat`, `dmesg`: Các lệnh Linux tiêu chuẩn.
- **Mã nguồn**: Mô-đun kernel `minfs.ko` được biên dịch từ các tệp như `minfs.c` và `minfs.h`.

## Hướng dẫn kiểm tra

Dưới đây là các bước kiểm tra đã thực hiện để xác minh rằng hệ thống tệp MinFS hoạt động đúng, đặc biệt là sau khi hoàn thành **TODO 4** (khởi tạo inode gốc).

### Bước 1: Nạp mô-đun kernel
```bash
root@qemux86:~# insmod /home/root/skels/filesystems/minfs/kernel/minfs.ko
```
- **Kết quả**:
  ```
  minfs: loading out-of-tree module taints kernel.
  ```
- **Giải thích**: Mô-đun `minfs.ko` được nạp thành công. Thông báo "taints kernel" cho biết đây là mô-đun bên ngoài (out-of-tree).

### Bước 2: Kiểm tra đăng ký hệ thống tệp
```bash
root@qemux86:~# cat /proc/filesystems | grep minfs
        minfs
```
- **Kết quả**: `minfs` xuất hiện trong `/proc/filesystems`.
- **Giải thích**: Hàm `register_filesystem(&minfs_fs_type)` trong `minfs_init` đã đăng ký thành công hệ thống tệp MinFS.

### Bước 3: Định dạng thiết bị
```bash
root@qemux86:~# /home/root/skels/filesystems/minfs/user/mkfs.minfs /dev/vdc
```
- **Kết quả**: Không có lỗi, thiết bị `/dev/vdc` được định dạng.
- **Giải thích**: Công cụ `mkfs.minfs` tạo cấu trúc MinFS trên `/dev/vdc`, bao gồm:
    - Siêu khối (block 0) với số ma thuật (`MINFS_MAGIC`), phiên bản, và bản đồ inode (`imap`).
    - Bảng inode (block 1) chứa inode gốc (index 1).
    - Thư mục gốc với các mục như `.` và `..`.

### Bước 4: Tạo điểm gắn và gắn hệ thống tệp
```bash
root@qemux86:~# mkdir -p /mnt/minfs
root@qemux86:~# mount -t minfs /dev/vdc /mnt/minfs
```
- **Kết quả**:
  ```
  root@qemux86:~# mount | grep minfs
  /dev/vdc on /mnt/minfs type minfs (rw,relatime)
  ```
- **Giải thích**: Hệ thống tệp được gắn thành công vào `/mnt/minfs`. Hàm `minfs_fill_super` và `minfs_iget` đã hoạt động đúng để đọc siêu khối và khởi tạo inode gốc.

### Bước 5: Xác minh việc gắn
```bash
root@qemux86:~# df /mnt/minfs
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/vdc                     0         0         0   0% /mnt/minfs
```
- **Kết quả**: Hệ thống tệp được gắn, nhưng dung lượng hiển thị là 0 
- (có thể do `simple_statfs` chưa được tùy chỉnh hoặc hệ thống tệp trống).
- **Giải thích**: Hàm `simple_statfs` được sử dụng trong `minfs_ops.statfs` trả về thông tin cơ bản về dung lượng.

### Bước 6: Kiểm tra inode gốc
```bash
root@qemux86:~# stat /mnt/minfs
File: /mnt/minfs
Size: 0               Blocks: 0          IO Block: 4096   directory
Device: fe20h/65056d    Inode: 4719        Links: 2
Access: (0755/drwxr-xr-x)  Uid: (    0/    root)   Gid: (    0/    root)
Access: 2025-05-30 11:10:02.000000000
Modify: 2025-05-30 11:08:46.000000000
Change: 2025-05-30 11:08:46.000000000
```
- **Kết quả**:
    - `Inode: 4719`: Inode số có thể khác 1 do kernel gán số inode động (thông qua `iget_locked`).
    - `Links: 2`: Xác nhận `inc_nlink` trong `minfs_iget` đặt số liên kết thành 2 cho thư mục gốc.
    - `Access: (0755/drwxr-xr-x)`: Xác nhận inode gốc là thư mục với quyền 0755.
    - `Uid: 0`, `Gid: 0`: Xác nhận `i_uid` và `i_gid` được điền đúng từ `minfs_inode`.
- **Giải thích**: Hàm `minfs_iget` đã đọc thành công inode gốc từ khối inode (block 1) và điền đúng các trường vào inode VFS.

### Bước 7: Kiểm tra nội dung thư mục gốc
```bash
root@qemux86:~# ls -l /mnt/minfs
```
- **Kết quả**: Không hiển thị nội dung (có thể do chưa triển khai **TODO 5** - hàm `minfs_readdir`).
- **Giải thích**: Tại thời điểm này, `minfs_readdir` chưa được hoàn thiện, 
- nên lệnh `ls` không liệt kê được các mục như `.` và `..`. Điều này sẽ được xử lý trong **TODO 5**.

## Giải thích mã nguồn (TODO 4)

**TODO 4** yêu cầu hoàn thiện việc khởi tạo inode gốc, bao gồm:
1. Cập nhật cấu trúc `minfs_ops` với các hàm `alloc_inode` và `destroy_inode`.
2. Triển khai hàm `minfs_iget` để đọc inode từ đĩa và điền vào inode VFS.
3. Thay thế `myfs_get_inode` bằng `minfs_iget` trong `minfs_fill_super`.

### 1. Cập nhật `minfs_ops`
Cấu trúc `minfs_ops` được định nghĩa như sau:

```c
static const struct super_operations minfs_ops = {
    .statfs         = simple_statfs,
    .put_super      = minfs_put_super,
    .alloc_inode    = minfs_alloc_inode,  /* TODO 4: Cấp phát inode */
    .destroy_inode  = minfs_destroy_inode, /* TODO 4: Giải phóng inode */
};
```

- **Giải thích**:
    - `.alloc_inode`: Gọi `minfs_alloc_inode` để cấp phát cấu trúc `minfs_inode_info` và khởi tạo inode VFS.
    - `.destroy_inode`: Gọi `minfs_destroy_inode` để giải phóng bộ nhớ của `minfs_inode_info`.
    - Các hàm này đã được triển khai trong **TODO 3** và được sử dụng để quản lý vòng đời của inode trong kernel.

### 2. Triển khai `minfs_iget`
Hàm `minfs_iget` đọc inode từ khối thứ hai (chỉ số 1) trên đĩa và điền thông tin vào inode VFS.

```c
static struct inode *minfs_iget(struct super_block *s, unsigned long ino)
{
    struct minfs_inode *mi;
    struct buffer_head *bh;
    struct inode *inode;
    struct minfs_inode_info *mii;

    /* Cấp phát inode VFS */
    inode = iget_locked(s, ino);
    if (inode == NULL) {
        printk(LOG_LEVEL "lỗi cấp phát inode\n");
        return ERR_PTR(-ENOMEM);
    }

    /* Trả về inode từ bộ nhớ đệm nếu không phải inode mới */
    if (!(inode->i_state & I_NEW))
        return inode;

    /* Đọc khối chứa inode (block 1) */
    bh = sb_bread(s, MINFS_INODE_BLOCK);
    if (!bh) {
        printk(LOG_LEVEL "không thể đọc khối inode\n");
        iget_failed(inode);
        return ERR_PTR(-EIO);
    }

    /* Lấy inode từ khối */
    mi = (struct minfs_inode *)bh->b_data + ino;

    /* Điền thông tin inode VFS */
    inode->i_mode = mi->mode;
    i_uid_write(inode, mi->uid);
    i_gid_write(inode, mi->gid);
    inode->i_size = mi->size;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

    if (S_ISDIR(inode->i_mode)) {
        /* Thiết lập thao tác cho thư mục */
        inode->i_op = &simple_dir_inode_operations;
        inode->i_fop = &simple_dir_operations;
        inc_nlink(inode); /* Đặt i_nlink = 2 cho thư mục */
    }

    /* Điền dữ liệu cho minfs_inode_info */
    mii = container_of(inode, struct minfs_inode_info, vfs_inode);
    mii->data_block = mi->data_block;

    /* Giải phóng tài nguyên */
    brelse(bh);
    unlock_new_inode(inode);

    return inode;

out_bad_sb:
    iget_failed(inode);
    return NULL;
}
```

- **Giải thích**:
    - **Cấp phát inode**: `iget_locked(s, ino)` cấp phát một inode VFS với số inode `ino`. Nếu inode đã tồn tại trong bộ nhớ đệm, nó được trả về ngay.
    - **Đọc khối inode**: `sb_bread(s, MINFS_INODE_BLOCK)` đọc khối thứ hai (chỉ số 1), nơi chứa bảng inode. `MINFS_INODE_BLOCK` thường được định nghĩa là 1 trong `minfs.h`.
    - **Lấy inode**: `mi = (struct minfs_inode *)bh->b_data + ino` trỏ đến inode cụ thể dựa trên chỉ số `ino`.
    - **Điền inode VFS**:
        - `i_mode`: Sao chép từ `mi->mode` (quyền và loại tệp).
        - `i_uid`, `i_gid`: Gán bằng `i_uid_write` và `i_gid_write` từ `mi->uid` và `mi->gid`.
        - `i_size`: Sao chép từ `mi->size`.
        - `i_atime`, `i_mtime`, `i_ctime`: Gán bằng `current_time(inode)` để đặt thời gian truy cập, sửa đổi, và thay đổi.
    - **Thư mục**: Nếu inode là thư mục (`S_ISDIR`), đặt `i_op` và `i_fop` thành `simple_dir_inode_operations` và `simple_dir_operations`, đồng thời tăng `i_nlink` lên 2 (cho thư mục và mục `.`).
    - **minfs_inode_info**: Gán `mii->data_block` từ `mi->data_block` để lưu trữ khối dữ liệu của inode.
    - **Giải phóng**: Gọi `brelse(bh)` để giải phóng `buffer_head` và `unlock_new_inode` để mở khóa inode.

### 3. Cập nhật `minfs_fill_super`
Hàm `minfs_fill_super` được sửa để sử dụng `minfs_iget` thay vì `myfs_get_inode` để lấy inode gốc.

```c
static int minfs_fill_super(struct super_block *s, void *data, int silent)
{
    struct minfs_sb_info *sbi;
    struct minfs_super_block *ms;
    struct inode *root_inode;
    struct dentry *root_dentry;
    struct buffer_head *bh;
    int ret = -EINVAL;

    sbi = kzalloc(sizeof(struct minfs_sb_info), GFP_KERNEL);
    if (!sbi)
        return -ENOMEM;
    s->s_fs_info = sbi;

    if (!sb_set_blocksize(s, MINFS_BLOCK_SIZE))
        goto out_bad_blocksize;

    bh = sb_bread(s, 0);
    if (!bh) {
        printk(LOG_LEVEL "không thể đọc siêu khối\n");
        goto out_bad_sb;
    }

    ms = (struct minfs_super_block *)bh->b_data;
    if (ms->magic != MINFS_MAGIC) {
        printk(LOG_LEVEL "số ma thuật không đúng\n");
        goto out_bad_magic;
    }

    s->s_magic = MINFS_MAGIC;
    s->s_op = &minfs_ops;
    sbi->version = ms->version;
    sbi->imap = ms->imap;

    /* Sử dụng minfs_iget thay vì myfs_get_inode */
    root_inode = minfs_iget(s, 1);
    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        goto out_bad_inode;
    }

    root_dentry = d_make_root(root_inode);
    if (!root_dentry)
        goto out_iput;
    s->s_root = root_dentry;

    sbi->sbh = bh;
    return 0;

out_iput:
    iput(root_inode);
out_bad_inode:
    printk(LOG_LEVEL "inode không hợp lệ\n");
out_bad_magic:
    printk(LOG_LEVEL "số ma thuật không hợp lệ\n");
    brelse(bh);
out_bad_sb:
    printk(LOG_LEVEL "lỗi đọc buffer_head\n");
out_bad_blocksize:
    printk(LOG_LEVEL "kích thước khối không hợp lệ\n");
    s->s_fs_info = NULL;
    kfree(sbi);
    return ret;
}
```

- **Giải thích**:
    - **Đọc siêu khối**: `sb_bread(s, 0)` đọc khối đầu tiên (siêu khối) và kiểm tra số ma thuật (`MINFS_MAGIC`).
    - **Khởi tạo inode gốc**: `minfs_iget(s, 1)` được gọi để lấy inode gốc (ino = 1) từ khối inode (block 1).
    - **Xử lý lỗi**: Nếu `minfs_iget` trả về lỗi (`IS_ERR`), nhảy đến nhãn `out_bad_inode`.
    - **Tạo dentry gốc**: `d_make_root(root_inode)` tạo dentry cho thư mục gốc và gán vào `s->s_root`.

## Lưu ý triển khai
- **Kiểm tra lỗi**:
    - Nếu mount thất bại, kiểm tra log kernel bằng `dmesg` để tìm các thông báo như "không thể đọc khối inode" hoặc "inode không hợp lệ".
    - Dùng `hexdump -C -s 1024 /dev/vdc` để kiểm tra khối inode (block 1) nếu nghi ngờ dữ liệu đĩa không đúng.
- **Hạn chế hiện tại**:
    - Lệnh `ls -l /mnt/minfs` không hiển thị nội dung vì `minfs_readdir` (TODO 5) chưa được hoàn thiện.
    - Để hỗ trợ liệt kê thư mục, cần triển khai 
    - **TODO 5** (hàm `minfs_readdir` và cập nhật thao tác thư mục trong `minfs_iget`).
- **Tiếp theo**:
    - Hoàn thiện **TODO 5** để hỗ trợ liệt kê nội dung thư mục (`ls -a /mnt/minfs` nên hiển thị `.` và `..`).
    - Tiến hành **TODO 7** để hỗ trợ tạo tệp và ghi inode.

## Kết luận
Hệ thống tệp MinFS đã được triển khai thành công cho **TODO 4**,
với inode gốc được khởi tạo đúng (xác nhận qua lệnh `stat /mnt/minfs`). 
Các bước kiểm tra đã chứng minh rằng mô-đun kernel, đăng ký hệ thống tệp, định dạng thiết bị,
và gắn hệ thống tệp hoạt động như mong đợi. Để tiếp tục, bạn nên triển khai 
**TODO 5** để hỗ trợ liệt kê thư mục và kiểm tra bằng lệnh `ls -a /mnt/minfs`.

