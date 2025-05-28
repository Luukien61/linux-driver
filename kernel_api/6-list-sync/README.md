#### **TODO 1: Sử dụng spinlock hoặc read-write lock để đồng bộ hóa**

**Thêm spinlock**:
1. **Khai báo spinlock**:
    - Thêm biến spinlock toàn cục:
      ```c
      static DEFINE_SPINLOCK(list_lock);
      ```
    - `DEFINE_SPINLOCK` khởi tạo spinlock tĩnh, tương đương với `spinlock_t list_lock; spin_lock_init(&list_lock);`.

2. **Bảo vệ danh sách trong các hàm**:
    - Sử dụng `spin_lock` và `spin_unlock` để khóa và mở khóa.


- **Hàm `task_info_add_to_list`**:
    - **Giải thích**:
        - **Đọc**: `task_info_find_pid` duyệt danh sách, cần khóa đọc.
        - Nếu tìm thấy `ti`, cập nhật `timestamp` và `count` (ghi vào `task_info`, không phải danh sách), nên có thể mở khóa ngay sau khi tìm.
        - **Ghi**: `list_add` thay đổi danh sách, cần khóa ghi.
        - **Lý do có comment `return`**: Comment được thêm để nhắc rằng nếu `ti` đã tồn tại, hàm thoát sớm sau khi cập nhật, tránh cấp phát và thêm mới.

- **Hàm `task_info_print_list`**:
    - **Giải thích**: Đây là truy cập đọc (duyệt danh sách), cần khóa để đảm bảo danh sách không bị thay đổi trong khi duyệt.


**Lưu ý**: Vì `task_info_find_pid` được gọi trong `task_info_add_to_list` (đã khóa), không cần khóa riêng trong `task_info_find_pid`.

- Hàm `list_sync_exit` thực hiện:
    - Lấy phần tử cuối cùng trong danh sách (vì dùng head.prev).
    - Thiết lập lại count = 10 để nó không đáp ứng điều kiện "hết hạn" .
---

#### **TODO 2: Xuất kernel symbol**
**Mục tiêu**: Xuất các hàm `task_info_add_for_current`, `task_info_print_list`, và `task_info_remove_expired` để các module khác có thể sử dụng.

**Cách thực hiện**:
- Sử dụng macro `EXPORT_SYMBOL_GPL` để xuất các hàm.
- Các hàm được xuất cho các module khác sử dụng, nhưng module đó phải có license GPL (ví dụ: MODULE_LICENSE("GPL")).

---
### **Biên dịch, nạp, và gỡ module**

#### **1. Biên dịch module**
- **Chạy lệnh**:
  ```bash
  make
  ```
    - Tạo file `list-sync.ko`.
    - Nếu có lỗi, kiểm tra kernel headers:
      ```bash
      ls /lib/modules/$(uname -r)/build
      ```

#### **2. Sao chép module (nếu cần)**
- Sao chép `list-sync.ko`:
  ```bash
  sudo cp list-sync.ko /lib/modules/$(uname -r)/kernel/drivers/misc/
  sudo depmod
  ```



### **Giải thích thông điệp kernel**
- **Khi nạp**:
    - `taints kernel`: Module ngoài kernel, bình thường.
    - `after first add`: In bốn `task_info` với `pid` và `timestamp`.
- **Khi gỡ**:
    - `after removing expired`: Chỉ phần tử cuối còn lại, vì `count = 10`.

---

### Tìm kiếm hàm sau khi export

```shell
grep task_info_remove_expired /proc/kallsyms                                     
 ```