
# Bài 3 - MinFS: Creating and destroying minfs inodes

## 📋 Mô tả
Bài 3 yêu cầu triển khai hai hàm `minfs_alloc_inode` và `minfs_destroy_inode`
trong file `/linux/tools/labs/skels/filesystems/minfs/kernel/minfs.c`
để quản lý việc cấp phát và hủy inode trong hệ thống tệp `minfs`. 
Đây là một phần của bài tập Linux Kernel Labs,
nhằm chuẩn bị cho việc đăng ký các hàm này vào `super_operations` trong bài tập sau (**TODO 4**). 

## 🎯 Yêu cầu
- **minfs_alloc_inode**:
    - Cấp phát bộ nhớ cho cấu trúc `struct minfs_inode_info` bằng `kzalloc()`.
    - Khởi tạo inode VFS (trường `vfs_inode`) bằng `inode_init_once()`.
    - Trả về con trỏ `struct inode` (tức là `mii->vfs_inode`).
- **minfs_destroy_inode**:
    - Thêm khai báo `struct minfs_inode_info *mii` để sửa lỗi biên dịch `mii undeclared`.
    - Lấy `struct minfs_inode_info` từ `struct inode` bằng macro `container_of`.
    - Giải phóng bộ nhớ bằng `kfree()`.
- **Kết quả mong đợi**:
    - Code biên dịch không lỗi, tạo được file `minfs.ko`.
    - Không cần chạy hoặc kiểm tra mount, vì hai hàm này chưa được gọi trong **TODO 3**. Chúng sẽ được sử dụng trong **TODO 4**.

## 🔧 Các bước triển khai

### Bước 1: Sửa mã nguồn trong `minfs.c`
1. **Mở file**:
   ```bash
   vi /linux/tools/labs/skels/filesystems/minfs/kernel/minfs.c
   ```

2. **Sửa hàm `minfs_alloc_inode`**:
    - **Mã gốc** (chứa **TODO 3**):
      ```c
      static struct inode *minfs_alloc_inode(struct super_block *s)
      {
          struct minfs_inode_info *mii;
          /* TODO 3: Allocate minfs_inode_info. */
          /* TODO 3: init VFS inode in minfs_inode_info */
          return &mii->vfs_inode;
      }
      ```
    - **Mã đã sửa**:
      ```c
      static struct inode *minfs_alloc_inode(struct super_block *s)
      {
          struct minfs_inode_info *mii;
 
          /* Cấp phát bộ nhớ cho minfs_inode_info */
          mii = kzalloc(sizeof(struct minfs_inode_info), GFP_KERNEL);
          if (!mii) {
              printk(LOG_LEVEL "Không thể cấp phát bộ nhớ cho minfs_inode_info\n");
              return NULL;
          }
 
          /* Khởi tạo inode VFS trong minfs_inode_info */
          inode_init_once(&mii->vfs_inode);
 
          return &mii->vfs_inode;
      }
      ```
    - **Giải thích**:
        - **`struct minfs_inode_info *mii;`**: Khai báo con trỏ tới cấu trúc `minfs_inode_info`, chứa trường `data_block` và `vfs_inode`.
        - **`kzalloc(sizeof(struct minfs_inode_info), GFP_KERNEL)`**:
            - Cấp phát bộ nhớ cho `minfs_inode_info` và khởi tạo tất cả byte về 0.
            - `GFP_KERNEL` cho phép kernel ngủ nếu cần thêm bộ nhớ.
            - Kiểm tra lỗi: Nếu `kzalloc` trả về `NULL`, in thông báo lỗi và trả về `NULL`.
        - **`inode_init_once(&mii->vfs_inode)`**:
            - Khởi tạo inode VFS trong trường `vfs_inode` để sẵn sàng cho VFS (thiết lập trạng thái, khóa, v.v.).
        - **`return &mii->vfs_inode`**:
            - Trả về con trỏ tới `vfs_inode`, vì VFS chỉ làm việc với `struct inode`.

3. **Sửa hàm `minfs_destroy_inode`**:
    - **Mã gốc** (gây lỗi `mii undeclared`):
      ```c
      static void minfs_destroy_inode(struct inode *inode)
      {
          /* TODO 3: free minfs_inode_info */
          mii = container_of(inode, struct minfs_inode_info, vfs_inode);
          kfree(mii);
      }
      ```
    - **Mã đã sửa**:
      ```c
      static void minfs_destroy_inode(struct inode *inode)
      {
          struct minfs_inode_info *mii;
 
          /* Lấy minfs_inode_info từ inode bằng container_of */
          mii = container_of(inode, struct minfs_inode_info, vfs_inode);
 
          /* Giải phóng bộ nhớ minfs_inode_info */
          kfree(mii);
      }
      ```
    - **Giải thích**:
        - **`struct minfs_inode_info *mii;`**: Khai báo biến `mii` là con trỏ tới `struct minfs_inode_info`. Dòng này bị thiếu trong mã gốc, gây lỗi `mii undeclared`.
        - **`container_of(inode, struct minfs_inode_info, vfs_inode)`**:
            - Macro `container_of` tính toán địa chỉ của `minfs_inode_info` từ con trỏ `inode` (trỏ tới `vfs_inode`).
            - Điều này cho phép lấy cấu trúc cha (`minfs_inode_info`) từ trường con (`vfs_inode`).
        - **`kfree(mii)`**:
            - Giải phóng bộ nhớ của `minfs_inode_info` đã cấp phát bởi `kzalloc` trong `minfs_alloc_inode`.

4. **Lưu và thoát**:
   ```bash
   :wq
   ```

### Bước 2: Biên dịch module
1. **Xóa tệp cũ**:
   ```bash
   cd /linux/tools/labs/skels
   make clean
   ```

2. **Biên dịch**:
   ```bash
   make -C /linux M=/linux/tools/labs/skels ARCH=x86 modules
   ```

3. **Kiểm tra file `minfs.ko`**:
   ```bash
   ls -l /linux/tools/labs/skels/filesystems/minfs/kernel/minfs.ko
   ```
    - **Mong đợi**: File `minfs.ko` được tạo, không có lỗi biên dịch.

### Bước 3: Không cần chạy hoặc kiểm tra thêm
- **Lưu ý quan trọng**:
- Ở **TODO 3**, bạn **không cần chạy QEMU, nạp module, hoặc kiểm tra mount**.
- Mục tiêu chỉ là đảm bảo code biên dịch thành công. Hai hàm `minfs_alloc_inode` và `minfs_destroy_inode` chưa được gọi,
- vì chúng sẽ được đăng ký vào `super_operations` trong **TODO 4**.

## 🏗️ Structures và Constants
#### `struct minfs_inode_info` (từ `minfs.h`):
```c
struct minfs_inode_info {
    __u16 data_block;        // Block chứa dữ liệu của inode
    struct inode vfs_inode;  // Inode VFS nhúng trong cấu trúc
};
```
- **Giải thích**:
    - `data_block`: Lưu số block chứa dữ liệu của file/thư mục.
    - `vfs_inode`: Inode VFS được nhúng để tích hợp với kernel.

#### Constants trong `minfs.h` (liên quan đến **TODO 3**):
```c
#define MINFS_BLOCK_SIZE    4096    // Kích thước block
#define MINFS_ROOT_INODE    0       // Số inode của thư mục gốc
```

## ✅ Kết quả mong đợi
- **Thành công**:
    - ✅ Lỗi biên dịch `mii undeclared` trong `minfs_destroy_inode` được khắc phục.
    - ✅ Module `minfs.ko` biên dịch thành công.
- **Không cần kiểm tra thêm**:
    - Không cần nạp module hoặc chạy mount, vì các hàm chưa được gọi trong **TODO 3**.
    - Các lệnh như `insmod`, `mount`, `ls`, hoặc `touch` sẽ được kiểm tra trong các bài tập sau (ví dụ: **TODO 4**).

## 🧪 Kiểm tra biên dịch
1. **Kiểm tra log biên dịch** (nếu có lỗi):
   ```bash
   make build

2. **Kiểm tra file đầu ra**:
   ```bash
   ls -l /linux/tools/labs/skels/filesystems/minfs/kernel/minfs.ko
   ```
    - **Kết quả**: File `minfs.ko` tồn tại.

## 📁 Files Modified
- **minfs.c**:
    - Sửa hàm `minfs_alloc_inode` để cấp phát và khởi tạo inode.
    - Sửa hàm `minfs_destroy_inode` để thêm khai báo `mii` và giải phóng bộ nhớ.

- **Build artifacts**:
    - `minfs.ko`: Kernel module file.
    - `Module.symvers`: Symbol version info.
    - `modules.order`: Module dependency order.

## 🚀 Bước tiếp theo
- **TODO 4**:
    - Triển khai `minfs_iget` để đọc inode từ disk (block 1).
    - Đăng ký `minfs_alloc_inode` và `minfs_destroy_inode` vào `super_operations`:
      ```c
      static const struct super_operations minfs_ops = {
          .statfs       = simple_statfs,
          .put_super    = minfs_put_super,
          .alloc_inode  = minfs_alloc_inode,
          .destroy_inode = minfs_destroy_inode,
          /* TODO 7: .write_inode = minfs_write_inode */
      };
      ```
- Sau **TODO 4**, các hàm này sẽ được gọi khi VFS cần cấp phát hoặc hủy inode.

## 📝 Notes Quan Trọng
- **Mục tiêu duy nhất**: Đảm bảo hai hàm `minfs_alloc_inode` và `minfs_destroy_inode` biên dịch không lỗi.
- Không cần chạy hoặc kiểm tra chức năng ở giai đoạn này.

- **Chưa gọi hàm**: Các hàm này chưa được sử dụng trong **TODO 3**, chỉ được chuẩn bị cho **TODO 4**.

