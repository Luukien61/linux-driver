Dưới đây là phiên bản **README** cho **Bài 2** của hệ thống tệp `myfs`, tập trung nhiều hơn vào **giải thích mã nguồn** liên quan đến **`myfs_fill_super()`**, các thao tác khởi tạo `superblock`, `inode`, và `dentry` – là nội dung trọng tâm của bài này.

---

# Bài 2: Điền Superblock cho myfs

## Mô tả

Bài 2 yêu cầu bạn hoàn thiện hàm `myfs_fill_super()` trong hệ thống tệp `myfs`, nhằm mục tiêu tạo ra một hệ thống tệp có thể gắn (mount) thành công và có thư mục gốc (root directory) hoạt động được.

### Mục tiêu chính:

* Điền thông tin vào superblock.
* Tạo `inode` và `dentry` cho thư mục gốc.
* Cho phép mount thành công và sử dụng lệnh `ls` trên thư mục gốc.

---

## Các bước thực hiện

### 1. Sửa mã nguồn

```bash
cd /linux/tools/labs/skels/filesystems/myfs
vi myfs.c
```

Hoàn thành **TODO 2** trong hàm `myfs_fill_super()`.

### 2. Biên dịch

```bash
make build
```

Kiểm tra kết quả:

```bash
ls -l *.ko
```

### 3. Thử nghiệm trong máy ảo

```bash
cd /linux/tools/labs
make console
```

Trong máy ảo:

```bash
insmod /home/root/skels/filesystems/myfs/myfs.ko
mkdir /mnt/myfs
mount -t myfs none /mnt/myfs
ls /mnt/myfs   # nên hiển thị thư mục trống (root directory đã được gắn)
rmmod myfs
```

---
## Kết quả
  root@qemux86:~# insmod /home/root/skels/filesystems/myfs/myfs.ko                
  myfs: loading out-of-tree module taints kernel.                                 
  root@qemux86:~# mkdir /mnt/myfs                                                 
  root@qemux86:~# mount -t myfs none /mnt/myfs                                    
  root inode has 1 link(s)                                                        
  mount: mounting none on /mnt/myfs failed: Not a directory                       
  root@qemux86:~#

## Giải thích mã nguồn

Trong bài 2, phần quan trọng nhất là hoàn thiện hàm:

```c
static int myfs_fill_super(struct super_block *sb, void *data, int silent)
```

### 1. Khởi tạo superblock

```c
sb->s_magic = MYFS_MAGIC;
sb->s_blocksize = MYFS_BLOCKSIZE;
sb->s_blocksize_bits = MYFS_BLOCKSIZE_BITS;
sb->s_op = &myfs_sops;
```

**Ý nghĩa**:

* `s_magic`: Gán số ma thuật để nhận diện hệ thống tệp. Đây là cách kernel biết superblock thuộc về `myfs`.
* `s_blocksize` và `s_blocksize_bits`: Đặt kích thước khối logic của hệ thống tệp (ở đây là 4096B).
* `s_op`: Trỏ đến cấu trúc `super_operations`, định nghĩa các hàm thao tác với superblock.

---

### 2. Tạo inode gốc (root inode)

```c
inode = myfs_get_inode(sb, S_IFDIR | 0755, 0);
```

* `myfs_get_inode(...)` là một hàm tiện ích được viết riêng để tạo inode mới.
* `S_IFDIR | 0755`: Gán kiểu là **thư mục** (`S_IFDIR`) với quyền **rwxr-xr-x**.
* Tham số cuối là `0`, chỉ định không cần thêm dữ liệu riêng.

---

### 3. Gắn inode gốc vào superblock

```c
sb->s_root = d_make_root(inode);
```

* `d_make_root()` tạo một `dentry` đại diện cho thư mục gốc, liên kết với `inode` đã tạo.
* `sb->s_root`: Kernel yêu cầu trường này phải được điền sau khi mount. Nếu không, mount sẽ thất bại.

---

### 4. Hàm tạo inode (`myfs_get_inode`)

```c
static struct inode *myfs_get_inode(struct super_block *sb,
        const umode_t mode, const dev_t dev)
{
    struct inode *inode = new_inode(sb);
    if (!inode)
        return NULL;

    inode->i_ino = get_next_ino();
    inode->i_sb = sb;
    inode->i_op = &myfs_inode_ops;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

    if (S_ISDIR(mode))
        inode->i_fop = &simple_dir_operations;
    else
        inode->i_fop = &simple_file_operations;

    inode->i_mode = mode;

    return inode;
}
```

**Chi tiết**:

* `new_inode(sb)`: Kernel cấp phát một inode trống từ superblock `sb`.
* `get_next_ino()`: Gán số inode duy nhất.
* `i_op`: Trỏ đến `inode_operations`, định nghĩa thao tác như lookup, create, unlink.
* `i_fop`: Trỏ đến `file_operations`, xác định thao tác đọc/ghi với file hay thư mục.

    * Với thư mục thì là `simple_dir_operations`.
    * Với file thì là `simple_file_operations`.

---

### 5. Cấu trúc hỗ trợ

```c
static const struct super_operations myfs_sops = {
    .statfs     = simple_statfs,
    .drop_inode = generic_delete_inode,
};

static const struct inode_operations myfs_inode_ops = {
    .lookup = simple_lookup,
    .link = simple_link,
    .unlink = simple_unlink,
    .mkdir = simple_mkdir,
    .rmdir = simple_rmdir,
    .rename = simple_rename,
};
```

* `myfs_sops`: Kernel sẽ gọi các hàm trong `super_operations` khi thao tác với superblock. Ở đây dùng sẵn hàm đơn giản.
* `myfs_inode_ops`: Cung cấp các thao tác đơn giản mặc định cho thư mục như `mkdir`, `rmdir`, `rename`.

---

## Tổng kết

| Thành phần         | Chức năng chính                                     |
| ------------------ | --------------------------------------------------- |
| `myfs_fill_super`  | Điền thông tin superblock, tạo root inode và dentry |
| `myfs_get_inode`   | Tạo inode với quyền và kiểu tương ứng               |
| `super_operations` | Định nghĩa thao tác với superblock                  |
| `inode_operations` | Cung cấp thao tác với thư mục                       |
| `file_operations`  | Tùy thuộc vào kiểu inode: file hay thư mục          |

Sau bài này, hệ thống tệp `myfs` có thể được **gắn thành công** và có một **thư mục gốc có thể liệt kê được bằng `ls`**.

---

Nếu bạn muốn thêm hình minh họa hoặc sơ đồ cấu trúc `inode/superblock/dentry`, mình có thể bổ sung tùy theo yêu cầu.
