
# README: Bài tập MinFS Filesystem #2 - Triển khai chức năng tạo tệp

Tệp README này cung cấp hướng dẫn để triển khai hệ thống tệp `minfs`,
tập trung vào chức năng tạo tệp trong bài tập mô-đun kernel Linux. Mục tiêu là hoàn thành các nhiệm vụ `TODO 7`,
bao gồm việc triển khai các hàm `minfs_new_inode` và `minfs_add_link` để hỗ trợ việc tạo tệp trong hệ thống tệp `minfs`. 
Các hàm này rất quan trọng để tạo và liên kết các inode mới trong hệ thống tệp.

## Mục tiêu

Hệ thống tệp `minfs` là một hệ thống tệp đơn giản trong bộ nhớ, được thiết kế cho mục đích học tập.
Bài tập này tập trung vào triển khai thao tác `create`, cho phép người dùng tạo tệp mới trong một thư mục. 
Thao tác `create` được thực hiện thông qua hàm `minfs_create`, dựa trên hai hàm hỗ trợ: `minfs_new_inode` 
(tạo và khởi tạo một inode mới) và `minfs_add_link` (thêm một mục nhập thư mục cho inode mới).

Các nhiệm vụ `TODO 7` hướng dẫn bạn qua các bước để triển khai các hàm này,
cũng như thiết lập các thao tác inode và không gian địa chỉ cần thiết cho các tệp thông thường.

## Yêu cầu tiên quyết

- Hiểu biết cơ bản về lập trình kernel Linux, đặc biệt là hệ thống tệp.
- Quen thuộc với các cấu trúc inode và superblock.
- Kiến thức về các thao tác bitwise để xử lý bitmap.
- Môi trường Linux với các công cụ phát triển mô-đun kernel (ví dụ: `gcc`, `make`).
- Tệp tiêu đề `minfs.h`, định nghĩa các hằng số như `MINFS_NUM_INODES`, `MINFS_NUM_ENTRIES`, `MINFS_NAME_LEN`, 
- và `MINFS_FIRST_DATA_BLOCK`.

## Các bước triển khai

Các nhiệm vụ `TODO 7` nằm trong tệp `minfs.c` và bao gồm các bước sau:

### 1. Thiết lập hàm `create` trong `minfs_dir_inode_operations`
Gán hàm `minfs_create` vào trường `.create` của cấu trúc `minfs_dir_inode_operations` 
để kích hoạt chức năng tạo tệp trong các thư mục.

**Mã nguồn** (trong `minfs_dir_inode_operations`):
```c
static const struct inode_operations minfs_dir_inode_operations = {
    .lookup = minfs_lookup,
    .create = minfs_create, /* Thêm hàm create */
};
```

### 2. Thiết lập thao tác không gian địa chỉ trong `minfs_iget`
Đối với tất cả các inode, gán cấu trúc `minfs_aops` vào `inode->i_mapping->a_ops` 
để xử lý các thao tác đọc/ghi trên không gian địa chỉ của inode.

**Mã nguồn** (trong `minfs_iget`):
```c
inode->i_mapping->a_ops = &minfs_aops;
```

### 3. Thiết lập thao tác inode và tệp cho tệp thông thường trong `minfs_iget`
Đối với các inode là tệp thông thường (kiểm tra bằng `S_ISREG`), 
thiết lập các thao tác inode và tệp bằng cách sử dụng `minfs_file_inode_operations` và `minfs_file_operations`.

**Mã nguồn** (trong `minfs_iget`):
```c
if (S_ISREG(inode->i_mode)) {
    inode->i_op = &minfs_file_inode_operations;
    inode->i_fop = &minfs_file_operations;
}
```

### 4. Triển khai hàm `minfs_new_inode`
Hàm `minfs_new_inode` tạo và khởi tạo một inode mới. Nó tìm inode trống đầu tiên trong bitmap `imap`, 
đánh dấu nó là đã sử dụng, và khởi tạo các trường cơ bản của inode.

**Mô tả các bước**:
- Sử dụng `find_first_zero_bit` để tìm chỉ số inode trống đầu tiên trong `sbi->imap`.
- Nếu không còn inode trống (`idx >= MINFS_NUM_INODES`), trả về lỗi `-ENOSPC`.
- Đánh dấu inode là đã sử dụng bằng `set_bit`.
- Đánh dấu buffer superblock (`sbi->sbh`) là bẩn (dirty) bằng `mark_buffer_dirty`.
- Tạo inode mới bằng `new_inode`.
- Khởi tạo các trường cơ bản của inode: `i_ino`, `i_atime`, `i_mtime`, `i_ctime`, 
- và gọi `inode_init_owner` với `i_mode = 0` (sẽ được thiết lập sau trong `minfs_create`).
- Chèn inode vào bảng băm bằng `insert_inode_hash` và đánh dấu inode là bẩn.

**Mã nguồn**:
```c
static struct inode *minfs_new_inode(struct inode *dir)
{
    struct super_block *sb = dir->i_sb;
    struct minfs_sb_info *sbi = sb->s_fs_info;
    struct inode *inode;
    int idx;

    /* Tìm inode trống đầu tiên */
    idx = find_first_zero_bit(&sbi->imap, MINFS_NUM_INODES);
    if (idx >= MINFS_NUM_INODES) {
        return ERR_PTR(-ENOSPC);
    }

    /* Đánh dấu inode là đã sử dụng và đánh dấu superblock bẩn */
    set_bit(idx, &sbi->imap);
    mark_buffer_dirty(sbi->sbh);

    /* Tạo inode mới */
    inode = new_inode(sb);
    if (!inode) {
        clear_bit(idx, &sbi->imap); /* Hoàn tác bitmap nếu lỗi */
        return ERR_PTR(-ENOMEM);
    }

    /* Khởi tạo các trường cơ bản của inode */
    inode->i_ino = idx;
    inode_init_owner(inode, dir, 0); /* mode sẽ được set trong minfs_create */
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_size = 0;

    /* Chèn inode vào bảng băm */
    insert_inode_hash(inode);
    mark_inode_dirty(inode);

    return inode;
}
```

### 5. Triển khai hàm `minfs_add_link`
Hàm `minfs_add_link` thêm một mục nhập thư mục (`struct minfs_dir_entry`) vào khối dữ liệu của thư mục cha (`dentry->d_parent->d_inode`). Nó tìm vị trí trống đầu tiên trong khối dữ liệu và ghi thông tin mục nhập.

**Mô tả các bước**:
- Lấy inode của thư mục cha (`dir`) từ `dentry->d_parent->d_inode`.
- Lấy cấu trúc `minfs_inode_info` từ inode thư mục cha bằng `container_of`.
- Lấy superblock (`sb`) từ inode.
- Đọc khối dữ liệu của thư mục cha bằng `sb_bread` sử dụng `mii->data_block`.
- Duyệt qua các mục nhập trong khối dữ liệu (tối đa `MINFS_NUM_ENTRIES`) để tìm mục trống (`de->ino == 0`).
- Nếu không tìm thấy vị trí trống, trả về lỗi `-ENOSPC`.
- Khi tìm thấy vị trí trống, điền thông tin vào mục nhập:
    - Gán `inode->i_ino` vào `de->ino`.
    - Sao chép tên tệp từ `dentry->d_name.name` vào `de->name`, đảm bảo chuỗi kết thúc bằng null.
- Đánh dấu buffer là bẩn bằng `mark_buffer_dirty`.
- Giải phóng buffer bằng `brelse`.

**Mã nguồn**:
```c
static int minfs_add_link(struct dentry *dentry, struct inode *inode)
{
    struct buffer_head *bh;
    struct inode *dir;
    struct super_block *sb;
    struct minfs_inode_info *mii;
    struct minfs_dir_entry *de;
    int i;
    int err = 0;

    /* Lấy inode thư mục cha, minfs_inode_info và superblock */
    dir = dentry->d_parent->d_inode;
    mii = container_of(dir, struct minfs_inode_info, vfs_inode);
    sb = dir->i_sb;

    /* Đọc khối dữ liệu của thư mục */
    bh = sb_bread(sb, mii->data_block);
    if (!bh) {
        return -EIO;
    }

    /* Tìm vị trí trống đầu tiên */
    de = (struct minfs_dir_entry *)bh->b_data;
    for (i = 0; i < MINFS_NUM_ENTRIES; i++, de++) {
        if (de->ino == 0) {
            /* Tìm thấy vị trí trống */
            goto found_slot;
        }
    }
    /* Không tìm thấy vị trí trống */
    err = -ENOSPC;
    goto out;

found_slot:
    /* Ghi thông tin mục nhập */
    de->ino = inode->i_ino;
    strncpy(de->name, dentry->d_name.name, MINFS_NAME_LEN);
    de->name[MINFS_NAME_LEN - 1] = '\0'; /* Đảm bảo kết thúc chuỗi */

    /* Đánh dấu buffer là bẩn */
    mark_buffer_dirty(bh);

out:
    brelse(bh);
    return err;
}
```

### 6. Thiết lập hàm `write_inode` trong `minfs_ops`
Gán hàm `minfs_write_inode` vào trường `.write_inode` của cấu trúc `minfs_ops` để đảm bảo nội dung inode được ghi ra đĩa.

**Mã nguồn** (trong `minfs_ops`):
```c
static const struct super_operations minfs_ops = {
    .statfs = simple_statfs,
    .put_super = minfs_put_super,
    .alloc_inode = minfs_alloc_inode,
    .destroy_inode = minfs_destroy_inode,
    .write_inode = minfs_write_inode, /* Thêm hàm write_inode */
};
```

## Kiểm tra triển khai

Để kiểm tra chức năng tạo tệp, làm theo các bước sau:

1. **Biên dịch và cài đặt mô-đun**:
    - Biên dịch mô-đun kernel bằng `make`.
    - Cài đặt mô-đun bằng `insmod minfs.ko`.

2. **Gắn hệ thống tệp**:
    - Tạo một thiết bị khối giả lập (nếu cần) hoặc sử dụng thiết bị có sẵn.
    - Gắn hệ thống tệp `minfs` vào một thư mục, ví dụ:
      ```bash
      mount -t minfs /dev/loop0 /mnt/minfs
      ```

3. **Tạo tệp**:
    - Tạo một tệp trong thư mục gắn kết:
      ```bash
      touch /mnt/minfs/peanuts.txt
      ```
    - Kiểm tra xem tệp đã được tạo bằng lệnh:
      ```bash
      ls -l /mnt/minfs
      ```
    - Kết quả mong đợi:
      ```bash
      -rw-r--r-- 1 root root 0 May 31 18:32 peanuts.txt
      ```

4. **Chạy tập lệnh kiểm tra**:
    - Sử dụng tập lệnh `test-minfs-2.sh` để kiểm tra tự động:
      ```bash
      ./test-minfs-2.sh
      ```
    - Nếu triển khai đúng, tập lệnh sẽ không hiển thị thông báo lỗi. Nếu có lỗi (ví dụ: "NOT OK. File creation failed"), 
    - kiểm tra lại mã nguồn, đặc biệt là các phần `TODO 7`.

## Lưu ý

- **Vấn đề quyền truy cập**: Nếu gặp lỗi "Permission denied" khi chạy `test-minfs-2.sh`, 
- hãy kiểm tra quyền của thư mục gắn kết hoặc chạy tập lệnh với quyền `root` (`sudo`).
- **Triển khai chưa hoàn chỉnh**: Hệ thống tệp `minfs` hiện tại chưa hỗ trợ xóa tệp, tạo/xóa thư mục, đổi tên mục nhập, 
- hoặc sửa đổi nội dung tệp. Những chức năng này cần được thêm vào để hoàn thiện hệ thống tệp.
- **Gỡ lỗi**: Sử dụng `printk` với mức `KERN_DEBUG` để ghi log thông tin (ví dụ: số inode, tên tệp) 
- và kiểm tra trong `/var/log/kern.log` hoặc bằng `dmesg`.

## Ví dụ đầu ra khi kiểm tra

Dưới đây là ví dụ đầu ra khi kiểm tra thành công:

```bash
root@qemux86:~/skels/filesystems/minfs/user# touch /mnt/minfs/peanuts.txt
root@qemux86:~/skels/filesystems/minfs/user# ls -l /mnt/minfs
-rw-r--r-- 1 root root 0 May 31 18:32 peanuts.txt
root@qemux86:~/skels/filesystems/minfs/user# ./test-minfs-2.sh
[No error messages]
```

Nếu gặp lỗi, ví dụ:

```bash
touch: b.txt: Permission denied
NOT OK. File creation failed.
```

