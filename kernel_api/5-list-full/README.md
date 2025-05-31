
## 🧠 3. Cấu trúc dữ liệu chính

```c
struct task_info {
    pid_t pid;
    unsigned long timestamp;
    atomic_t count;
    struct list_head list;
};
```

| Trường | Ý nghĩa                                               |
|--------|-------------------------------------------------------|
| `pid` | ID tiến trình                                         |
| `timestamp` | Thời điểm lần cuối tiến trình được thêm vào danh sách |
| `count` | Số lần tiến trình được thêm vào (biến nguyên tử), Biến nguyên tử (atomic_t) là một loại biến được thiết kế để truy cập an toàn trong môi trường đa luồng/ngắt , đảm bảo rằng thao tác đọc/ghi lên biến không bị xung đột.     |
| `list` | Node cho danh sách liên kết                           |

---

## 🔄 4. Danh sách toàn cục

```c
static struct list_head head;
```

- Biến toàn cục `head` là đầu danh sách liên kết.
- Cần gọi `INIT_LIST_HEAD(&head);` trước khi sử dụng.

---

## 🔁 5. Hàm cấp phát `task_info`

```c
static struct task_info *task_info_alloc(int pid)
{
    struct task_info *ti;

    ti = kmalloc(sizeof(*ti), GFP_KERNEL);
    if (ti == NULL)
        return NULL;
    ti->pid = pid;
    ti->timestamp = jiffies;
    atomic_set(&ti->count, 0);

    return ti;
}
```

- Dùng `kmalloc()` để cấp phát bộ nhớ.
- Khởi tạo các trường: `pid`, `timestamp`, `count`.

---

## 🔍 6. Hàm tìm kiếm tiến trình theo PID

```c
static struct task_info *task_info_find_pid(int pid)
{
    struct list_head *p;
    struct task_info *ti;

    list_for_each(p, &head) {
        ti = list_entry(p, struct task_info, list);
        if (ti->pid == pid)
            return ti;
    }

    return NULL;
}
```

- Duyệt qua từng node trong danh sách bằng `list_for_each(...)`.
- Sử dụng `list_entry(...)` để chuyển từ `list_head` về lại `task_info`.
- Nếu tìm thấy tiến trình có cùng `pid`, trả về con trỏ đến nó.

---

## 🔄 7. Hàm thêm tiến trình vào danh sách

```c
static void task_info_add_to_list(int pid)
{
    struct task_info *ti;

    ti = task_info_find_pid(pid);
    if (ti != NULL) {
        ti->timestamp = jiffies;
        atomic_inc(&ti->count);
        return;
    }

    ti = task_info_alloc(pid);
    list_add(&ti->list, &head);
}
```

- Tìm xem tiến trình có trong danh sách chưa.
- Nếu có: cập nhật `timestamp` và tăng `count`.
- Nếu không: tạo mới và thêm vào danh sách bằng `list_add(...)`.

---

## 🚀 8. Thêm tiến trình hiện tại và các tiến trình liên quan

```c
static void task_info_add_for_current(void)
{
    task_info_add_to_list(current->pid);
    task_info_add_to_list(current->parent->pid);
    task_info_add_to_list(next_task(current)->pid);
    task_info_add_to_list(next_task(next_task(current))->pid);
}
```

- Thêm 4 tiến trình vào danh sách:
    - Tiến trình hiện tại (`current`)
    - Cha của nó (`current->parent`)
    - Tiến trình kế tiếp (`next_task(current)`)
    - Tiếp theo của kế tiếp

---

## 🖨️ 9. Hàm in danh sách

```c
static void task_info_print_list(const char *msg)
{
    struct list_head *p;
    struct task_info *ti;

    pr_info("%s: [ ", msg);
    list_for_each(p, &head) {
        ti = list_entry(p, struct task_info, list);
        pr_info("(%d, %lu) ", ti->pid, ti->timestamp);
    }
    pr_info("]\n");
}
```

- In ra thông tin từng node trong danh sách dưới dạng `(pid, timestamp)`.

---

## ⏳ 10. Hàm xóa các tiến trình hết hạn

```c
static void task_info_remove_expired(void)
{
    struct list_head *p, *q;
    struct task_info *ti;

    list_for_each_safe(p, q, &head) {
        ti = list_entry(p, struct task_info, list);
        if (jiffies - ti->timestamp > 3 * HZ && atomic_read(&ti->count) < 5) {
            list_del(p);
            kfree(ti);
        }
    }
}
```

- Điều kiện để xóa:
    - Đã quá **3 giây** kể từ `timestamp`
    - Và `count < 5`
- Duyệt an toàn bằng `list_for_each_safe(...)` để tránh lỗi khi xóa node đang duyệt.

---

## 🧹 11. Hàm dọn dẹp toàn bộ danh sách

```c
static void task_info_purge_list(void)
{
    struct list_head *p, *q;
    struct task_info *ti;

    list_for_each_safe(p, q, &head) {
        ti = list_entry(p, struct task_info, list);
        list_del(p);
        kfree(ti);
    }
}
```

- Xóa tất cả các node khỏi danh sách và giải phóng bộ nhớ.

---

## 🟢 12. Hàm khởi tạo module

```c
static int list_full_init(void)
{
    INIT_LIST_HEAD(&head);

    task_info_add_for_current();
    task_info_print_list("after first add");

    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(5 * HZ);

    return 0;
}
```

- Khởi tạo danh sách.
- Thêm các tiến trình liên quan.
- Ngủ **5 giây** để chờ các tiến trình "hết hạn".

---

## 🔴 13. Hàm kết thúc module

```c
static void list_full_exit(void)
{
    struct task_info *ti;

    /* TODO 2/2: Ensure that at least one task is not deleted */
    ti = list_entry(head.next, struct task_info, list);
    atomic_set(&ti->count, 10);

    task_info_remove_expired();
    task_info_print_list("after removing expired");
    task_info_purge_list();
}
```

- Chọn **node đầu tiên** (`head.next`) và đặt `count = 10` để nó **không bị xóa**.
- Gọi `task_info_remove_expired()` để xóa các tiến trình khác.
- Cuối cùng, dọn dẹp toàn bộ danh sách.

---

## 📦 14. Đăng ký module

```c
module_init(list_full_init);
module_exit(list_full_exit);
```

