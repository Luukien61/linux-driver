### **TODO 1: Cấp phát và khởi tạo cấu trúc `task_info`**
**Mục tiêu**: Cấp phát bộ nhớ động cho một cấu trúc `task_info` và khởi tạo các trường của nó:
- Trường `pid`: Gán giá trị PID được truyền vào.
- Trường `timestamp`: Gán giá trị của biến `jiffies` (số tick kể từ khi hệ thống khởi động).

**Giải thích**:
- Hàm `task_info_alloc(int pid)` thực hiện nhiệm vụ này.
- **Cấp phát bộ nhớ**:
    - Sử dụng `kmalloc` để cấp phát bộ nhớ cho một cấu trúc `task_info` với kích thước `sizeof(struct task_info)`.
    - Cờ `GFP_KERNEL` cho biết việc cấp phát diễn ra trong không gian kernel và có thể tạm dừng (sleep) nếu cần.
    - Nếu cấp phát thất bại, hàm trả về `NULL` để báo lỗi.
- **Khởi tạo**:
    - Gán trường `pid` bằng giá trị PID được truyền vào.
    - Gán trường `timestamp` bằng giá trị của `jiffies`, một biến kernel lưu số lần tick của bộ đếm thời gian, biểu thị thời gian tương đối kể từ khi hệ thống khởi động.
- **Mục đích**: Hàm này tạo ra một cách tái sử dụng để cấp phát và khởi tạo các cấu trúc `task_info` cho bất kỳ PID nào, đảm bảo việc cấp phát bộ nhớ và khởi tạo được thực hiện nhất quán.

**Mã**:
```c
ti = kmalloc(sizeof(struct task_info), GFP_KERNEL);
if (!ti)
    return NULL;
ti->pid = pid;
ti->timestamp = jiffies;
return ti;
```

---

### **TODO 2: Cấp phát cấu trúc `task_info` cho các tiến trình cụ thể**
- **Chức năng**: Hàm `memory_init` được đăng ký làm hàm khởi tạo của module thông qua `module_init(memory_init)`. Nó chạy khi module được nạp (ví dụ, bằng lệnh `insmod`).
- **Nhiệm vụ**:
    - Cấp phát bốn cấu trúc `task_info` (`ti1`, `ti2`, `ti3`, `ti4`) cho bốn tiến trình khác nhau.
    - Lưu PID và timestamp (`jiffies`) của các tiến trình này.
    - Xử lý lỗi nếu việc cấp phát bộ nhớ thất bại.
    - In thông báo khi module được nạp thành công.
- **Đầu ra**:
    - Trả về `0` nếu khởi tạo thành công.
    - Trả về `-ENOMEM` (mã lỗi "hết bộ nhớ") nếu cấp phát thất bại.

---

- **1. Khai báo biến**
```c
struct task_struct *next_task_ptr, *next_next_task_ptr;
```
- **Giải thích**:
    - `task_struct` là một cấu trúc trong kernel Linux, đại diện cho một tiến trình. Nó chứa thông tin như PID, trạng thái tiến trình, tiến trình cha, v.v.
    - `next_task_ptr` và `next_next_task_ptr` là hai con trỏ kiểu `task_struct`, được dùng để lưu trữ thông tin về tiến trình tiếp theo và tiến trình tiếp theo của tiến trình tiếp theo trong danh sách tiến trình.
    - Các con trỏ này sẽ được gán giá trị khi gọi hàm `next_task`.

---

**2. Cấp phát `ti1` cho tiến trình hiện tại**
```c
ti1 = task_info_alloc(current->pid);
if (!ti1) {
    printk(KERN_ERR "Failed to allocate ti1\n");
    return -ENOMEM;
}
```
- **Giải thích**:
    - **`current`**: Đây là một macro trong kernel Linux, trả về con trỏ tới cấu trúc `task_struct` của tiến trình hiện tại (tiến trình đang thực thi module này).
    - **`current->pid`**: Lấy PID (mã định danh tiến trình) của tiến trình hiện tại. current: Là con trỏ tới cấu trúc task_struct của tiến trình đã gọi insmod (bash/shell)
    - **`task_info_alloc(current->pid)`**: Gọi hàm `task_info_alloc` (được định nghĩa trước đó) để:
        - Cấp phát bộ nhớ cho một cấu trúc `task_info` bằng `kmalloc`.
        - Khởi tạo `pid` bằng `current->pid` và `timestamp` bằng `jiffies` (số tick kể từ khi hệ thống khởi động).
        - Trả về con trỏ tới cấu trúc `task_info` hoặc `NULL` nếu cấp phát thất bại.
    - **Kiểm tra lỗi**:
        - Nếu `ti1` là `NULL` (cấp phát thất bại), in thông báo lỗi bằng `printk` với mức độ `KERN_ERR` (lỗi nghiêm trọng).
        - Trả về `-ENOMEM`, một mã lỗi kernel chuẩn, biểu thị "hết bộ nhớ" (Error: No Memory).
    - **Mục đích**: Lưu thông tin PID và timestamp của tiến trình hiện tại vào cấu trúc `ti1`.

---

 **3. Cấp phát `ti2` cho tiến trình cha**
```c
ti2 = task_info_alloc(current->parent->pid);
if (!ti2) {
    printk(KERN_ERR "Failed to allocate ti2\n");
    kfree(ti1);
    return -ENOMEM;
}
```
- **Giải thích**:
    - **`current->parent`**: Truy cập con trỏ tới cấu trúc `task_struct` của tiến trình cha của tiến trình hiện tại.
    - **`current->parent->pid`**: Lấy PID của tiến trình cha.
    - **`task_info_alloc(current->parent->pid)`**: Gọi hàm `task_info_alloc` để cấp phát và khởi tạo cấu trúc `task_info` cho PID của tiến trình cha.
    - **Kiểm tra lỗi**:
        - Nếu `ti2` là `NULL` (cấp phát thất bại), in thông báo lỗi.
        - Giải phóng bộ nhớ của `ti1` (đã cấp phát trước đó) bằng `kfree(ti1)` để tránh rò rỉ bộ nhớ.
        - Trả về `-ENOMEM`.
    - **Mục đích**: Lưu thông tin PID và timestamp của tiến trình cha vào cấu trúc `ti2`.

---

 **4. Cấp phát `ti3` cho tiến trình tiếp theo**
```c
next_task_ptr = next_task(current);
ti3 = task_info_alloc(next_task_ptr->pid);
if (!ti3) {
    printk(KERN_ERR "Failed to allocate ti3\n");
    kfree(ti1);
    kfree(ti2);
    return -ENOMEM;
}
```
- **Giải thích**:
    - **`next_task(current)`**: Hàm `next_task` nhận con trỏ `task_struct` của tiến trình hiện tại (`current`) và trả về con trỏ tới `task_struct` của tiến trình tiếp theo trong danh sách tiến trình của kernel.
        - Kernel Linux duy trì một danh sách liên kết  vòng đôi `task_list` của tất cả các tiến trình. `next_task` giúp duyệt qua danh sách này.
    - **`next_task_ptr`**: Lưu con trỏ tới tiến trình tiếp theo.
    - **`next_task_ptr->pid`**: Lấy PID của tiến trình tiếp theo.
    - **`task_info_alloc(next_task_ptr->pid)`**: Cấp phát và khởi tạo cấu trúc `task_info` cho PID này.
    - **Kiểm tra lỗi**:
        - Nếu `ti3` là `NULL`, in thông báo lỗi.
        - Giải phóng `ti1` và `ti2` để tránh rò rỉ bộ nhớ.
        - Trả về `-ENOMEM`.
    - **Mục đích**: Lưu thông tin PID và timestamp của tiến trình tiếp theo vào cấu trúc `ti3`.

---

**5. Cấp phát `ti4` cho tiến trình tiếp theo của tiến trình tiếp theo**
```c
next_next_task_ptr = next_task(next_task_ptr);
ti4 = task_info_alloc(next_next_task_ptr->pid);
if (!ti4) {
    printk(KERN_ERR "Failed to allocate ti4\n");
    kfree(ti1);
    kfree(ti2);
    kfree(ti3);
    return -ENOMEM;
}
```
- **Giải thích**:
    - **`next_task(next_task_ptr)`**: Gọi `next_task` trên `next_task_ptr` để lấy con trỏ tới tiến trình tiếp theo của tiến trình tiếp theo (so với tiến trình hiện tại).
    - **`next_next_task_ptr`**: Lưu con trỏ tới tiến trình này.
    - **`next_next_task_ptr->pid`**: Lấy PID của tiến trình này.
    - **`task_info_alloc(next_next_task_ptr->pid)`**: Cấp phát và khởi tạo cấu trúc `task_info` cho PID này.
    - **Kiểm tra lỗi**:
        - Nếu `ti4` là `NULL`, in thông báo lỗi.
        - Giải phóng `ti1`, `ti2`, và `ti3` để tránh rò rỉ bộ nhớ.
        - Trả về `-ENOMEM`.
    - **Mục đích**: Lưu thông tin PID và timestamp của tiến trình tiếp theo của tiến trình tiếp theo vào cấu trúc `ti4`.

---

**6. Thông báo thành công**
```c
printk(KERN_INFO "Memory module loaded successfully\n");
return 0;
```
- **Giải thích**:
    - Nếu tất cả các bước cấp phát thành công (không trả về `-ENOMEM`), in thông báo với mức độ `KERN_INFO` để xác nhận module đã được nạp thành công.
    - Trả về `0` để báo hiệu kernel rằng quá trình khởi tạo hoàn tất mà không có lỗi.
    - Thông báo này có thể được xem qua lệnh `dmesg` trên hệ thống.

---

**Tóm tắt luồng hoạt động của hàm**
1. **Lấy PID của tiến trình hiện tại** và cấp phát `ti1`.
2. **Lấy PID của tiến trình cha** và cấp phát `ti2`.
3. **Lấy PID của tiến trình tiếp theo** (bằng `next_task`) và cấp phát `ti3`.
4. **Lấy PID của tiến trình tiếp theo của tiến trình tiếp theo** và cấp phát `ti4`.
5. **Xử lý lỗi**: Nếu bất kỳ bước cấp phát nào thất bại, giải phóng tất cả các cấu trúc đã cấp phát trước đó và trả về lỗi.
6. **Thông báo thành công**: Nếu mọi thứ ổn, in thông báo và trả về `0`.

---

**Mục đích của hàm**
- **Học thuật**: Hàm này là một bài tập để học cách:
    - Quản lý bộ nhớ trong kernel (sử dụng `kmalloc` để cấp phát).
    - Truy cập thông tin tiến trình thông qua `current` và `next_task`.
    - Xử lý lỗi đúng cách trong kernel (kiểm tra con trỏ `NULL`, giải phóng bộ nhớ, trả về mã lỗi).
    - Ghi log thông tin trong kernel bằng `printk`.
- **Ứng dụng thực tế**: Module này không có chức năng thực tế trong hệ thống, mà chủ yếu để minh họa cách tương tác với danh sách tiến trình và quản lý bộ nhớ trong kernel.

---

**Mã**:
```c
ti1 = task_info_alloc(current->pid);
if (!ti1) {
    printk(KERN_ERR "Failed to allocate ti1\n");
    return -ENOMEM;
}
ti2 = task_info_alloc(current->parent->pid);
if (!ti2) {
    printk(KERN_ERR "Failed to allocate ti2\n");
    kfree(ti1);
    return -ENOMEM;
}
next_task_ptr = next_task(current);
ti3 = task_info_alloc(next_task_ptr->pid);
if (!ti3) {
    printk(KERN_ERR "Failed to allocate ti3\n");
    kfree(ti1);
    kfree(ti2);
    return -ENOMEM;
}
next_next_task_ptr = next_task(next_task_ptr);
ti4 = task_info_alloc(next_next_task_ptr->pid);
if (!ti4) {
    printk(KERN_ERR "Failed to allocate ti4\n");
    kfree(ti1);
    kfree(ti2);
    kfree(ti3);
    return -ENOMEM;
}
```

---

### **TODO 3: Hiển thị thông tin các cấu trúc `task_info`**
**Mục tiêu**: Sử dụng `printk` để hiển thị các trường `pid` và `timestamp` của bốn cấu trúc `task_info` (`ti1`, `ti2`, `ti3`, `ti4`).

**Giải thích**:
- Nhiệm vụ này được thực hiện trong hàm `memory_exit`, chạy khi module được gỡ bỏ.
- Với mỗi cấu trúc `task_info` (`ti1` đến `ti4`), mã kiểm tra xem cấu trúc có tồn tại không (tức là không phải `NULL`) để tránh truy cập con trỏ không hợp lệ.
- Hàm `printk` với mức độ `KERN_INFO` được sử dụng để in `pid` và `timestamp` theo định dạng: `tiX: pid=%d, timestamp=%lu\n`.
- **Mục đích**: Nhiệm vụ này thể hiện cách ghi log thông tin trong kernel, có thể xem qua lệnh `dmesg`. Nó cũng cho thấy cách truy cập an toàn các cấu trúc được cấp phát động.

**Mã**:
```c
if (ti1)
    printk(KERN_INFO "ti1: pid=%d, timestamp=%lu\n", ti1->pid, ti1->timestamp);
if (ti2)
    printk(KERN_INFO "ti2: pid=%d, timestamp=%lu\n", ti2->pid, ti2->timestamp);
if (ti3)
    printk(KERN_INFO "ti3: pid=%d, timestamp=%lu\n", ti3->pid, ti3->timestamp);
if (ti4)
    printk(KERN_INFO "ti4: pid=%d, timestamp=%lu\n", ti4->pid, ti4->timestamp);
```

- PID 0 là swapper (hay còn gọi là "idle task"), tiến trình đặc biệt của kernel, chạy khi không có tiến trình nào khác cần CPU. Đây là tiến trình có PID thấp nhất và luôn tồn tại.
- PID 1 là init (hoặc systemd trong các hệ thống hiện đại), tiến trình khởi tạo hệ thống, luôn là tiến trình đầu tiên được khởi động sau kernel.

---

### **TODO 4: Giải phóng bộ nhớ của các cấu trúc `task_info`**
**Mục tiêu**: Giải phóng bộ nhớ đã cấp phát cho các cấu trúc `task_info` (`ti1`, `ti2`, `ti3`, `ti4`) bằng hàm `kfree`.

**Giải thích**:
- Nhiệm vụ này cũng được thực hiện trong hàm `memory_exit`.
- Hàm `kfree` được gọi cho từng cấu trúc `task_info` để giải phóng bộ nhớ đã cấp phát bởi `kmalloc`.
- Vì `kfree` xử lý an toàn các con trỏ `NULL`, không cần kiểm tra trước khi gọi.
- **Mục đích**: Đảm bảo quản lý bộ nhớ đúng cách bằng cách giải phóng tài nguyên khi gỡ module, tránh rò rỉ bộ nhớ trong kernel.

**Mã**:
```c
kfree(ti1);
kfree(ti2);
kfree(ti3);
kfree(ti4);
```

---
