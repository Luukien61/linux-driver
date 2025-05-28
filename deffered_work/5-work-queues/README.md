Dưới đây là giải thích  **3-4-5-deferred**, 
tập trung vào **TODO 3** để sử dụng workqueue thay vì gọi trực tiếp `alloc_io()` trong `timer_handler`, 
tránh lỗi do thao tác chặn trong ngữ cảnh ngắt.

---

### Tổng quan
Mô-đun được sửa đổi để khi nhận lệnh `MY_IOCTL_TIMER_ALLOC`, thay vì gọi trực tiếp `alloc_io()` (thao tác chặn) trong `timer_handler`, 
nó lập lịch một workqueue để chạy `alloc_io()` trong ngữ cảnh tiến trình (process context), nơi thao tác chặn an toàn.
Cờ `timer_type` phân biệt giữa `TIMER_TYPE_SET` và `TIMER_TYPE_ALLOC`.

---

### Giải thích mã (chỉ tập trung vào TODO 3)

#### Cấu trúc dữ liệu
- `struct my_device_data`:
    - Thêm `struct work_struct work` để quản lý workqueue (**TODO 3**).

#### Hàm xử lý workqueue
```c
static void work_handler(struct work_struct *work)
{
    alloc_io();
}
```
- **TODO 3**: Định nghĩa `work_handler` gọi `alloc_io()`.
- Chạy trong ngữ cảnh tiến trình, an toàn cho thao tác chặn như `schedule_timeout` trong `alloc_io()`.

#### Hàm `timer_handler`
```c
static void timer_handler(struct timer_list *tl)
{
    struct my_device_data *my_data = container_of(tl, struct my_device_data, timer);
    if (!my_data) {
        pr_err("Error: my_data is NULL in timer_handler\n");
        return;
    }
    if (my_data->timer_type == TIMER_TYPE_SET) {
        pr_info("Timer fired: PID=%d, Process=%s\n", current->pid, current->comm);
    } else if (my_data->timer_type == TIMER_TYPE_ALLOC) {
        schedule_work(&my_data->work);
    }
}
```
- **TODO 3**: Khi `timer_type == TIMER_TYPE_ALLOC`, gọi `schedule_work(&my_data->work)` để lập lịch `work_handler`, 
- thay vì gọi trực tiếp `alloc_io()`.

#### Hàm `deferred_init`
```c
static int deferred_init(void)
{
    int err;
    err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME);
    if (err) return err;
    dev.timer_type = TIMER_TYPE_NONE;
    cdev_init(&dev.cdev, &my_fops);
    cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);
    timer_setup(&dev.timer, timer_handler, 0);
    INIT_WORK(&dev.work, work_handler);
    return 0;
}
```
- **TODO 3**: Khởi tạo workqueue bằng `INIT_WORK(&dev.work, work_handler)` để liên kết `work` với `work_handler`.

#### Hàm `deferred_exit`
```c
static void deferred_exit(void)
{
    pr_info("[deferred_exit] Exit module\n");
    cdev_del(&dev.cdev);
    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);
    del_timer_sync(&dev.timer);
    cancel_work_sync(&dev.work);
}
```
- **TODO 3**: Gọi `cancel_work_sync(&dev.work)` để đảm bảo không còn work nào được lập lịch khi gỡ mô-đun.

---

### Cách hoạt động
1. **Khởi tạo**: Khởi tạo timer và workqueue (`INIT_WORK`).
2. **Lệnh `MY_IOCTL_TIMER_ALLOC`**:
    - `./test a 3`: Đặt `timer_type = TIMER_TYPE_ALLOC`, lập lịch timer sau 3 giây.
    - Khi timer kích hoạt, `timer_handler` lập lịch workqueue (`schedule_work`).
    - `work_handler` chạy `alloc_io()` trong ngữ cảnh tiến trình, ngủ 5 giây và in thông báo.
3. **Dọn dẹp**: Hủy timer (`del_timer_sync`) và workqueue (`cancel_work_sync`).

---

### Sử dụng
- Tạo thiết bị: `mknod /dev/deferred c 42 0`.
- Chạy:
    - `./test a 3`: Sau 3 giây, workqueue chạy `alloc_io()`, in `"Yawn! I've been sleeping for 5 seconds."` mà không gây lỗi kernel.
    - `./test c`: Hủy timer.

---

### Kết luận
**TODO 3** được triển khai bằng cách thêm `work_struct` vào `my_device_data`, định nghĩa `work_handler` gọi `alloc_io()`, 
lập lịch workqueue trong `timer_handler`, và dọn dẹp workqueue khi gỡ mô-đun.
Sử dụng workqueue đảm bảo `alloc_io()` chạy an toàn trong ngữ cảnh tiến trình, tránh lỗi do thao tác chặn trong ngữ cảnh ngắt.
