Dưới đây là giải thích  cho việc sửa đổi mô-đun kernel Linux trong nhiệm vụ **3-4-5-deferred**,
tập trung vào **TODO 2** để xử lý lệnh `MY_IOCTL_TIMER_ALLOC` và gọi hàm `alloc_io()` trong `timer_handler`. 
Mục tiêu là kiểm tra tác động của thao tác chặn (blocking operation) trong hàm xử lý timer.

---

### Tổng quan
Mô-đun bổ sung lệnh `MY_IOCTL_TIMER_ALLOC` để lập lịch timer, khi kích hoạt sẽ gọi `alloc_io()` (mô phỏng thao tác chặn bằng cách ngủ 5 giây). Cờ `timer_type` được sử dụng để phân biệt giữa `TIMER_TYPE_SET` (in PID và tên tiến trình) và `TIMER_TYPE_ALLOC` (gọi `alloc_io()`).

---

### Giải thích mã (chỉ tập trung vào TODO 2)

#### Cấu trúc dữ liệu
- `struct my_device_data`:
    - Đã có `timer_type` (cờ trạng thái, `int`) để theo dõi loại timer (`TIMER_TYPE_NONE`, `TIMER_TYPE_SET`, `TIMER_TYPE_ALLOC`).

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
    }
    else if (my_data->timer_type == TIMER_TYPE_ALLOC) {
        alloc_io();
    }
}
```
- **TODO 2**: Thêm kiểm tra `timer_type == TIMER_TYPE_ALLOC` và gọi `alloc_io()`.
- **Hàm `alloc_io`**:
    - Đặt trạng thái tiến trình thành `TASK_INTERRUPTIBLE` và ngủ 5 giây (`schedule_timeout(5 * HZ)`).
    - In thông báo: `"Yawn! I've been sleeping for 5 seconds."`.
- **Vấn đề**: Gọi `alloc_io()` trong `timer_handler` là không an toàn vì timer chạy trong ngữ cảnh ngắt (interrupt context).
- Thao tác chặn như `schedule_timeout` có thể gây lỗi kernel (panic), vì ngữ cảnh ngắt không cho phép ngủ.

#### Hàm `deferred_ioctl`
```c
static long deferred_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_device_data *my_data = (struct my_device_data*) file->private_data;
    switch (cmd) {
    case MY_IOCTL_TIMER_SET:
        my_data->timer_type = TIMER_TYPE_SET;
        mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(arg * 1000));
        break;
    case MY_IOCTL_TIMER_CANCEL:
        del_timer_sync(&my_data->timer);
        break;
    case MY_IOCTL_TIMER_ALLOC:
        my_data->timer_type = TIMER_TYPE_ALLOC;
        mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(arg * 1000));
        break;
    default:
        return -ENOTTY;
    }
    return 0;
}
```
- **TODO 2**: Thêm trường hợp `MY_IOCTL_TIMER_ALLOC`:
    - Đặt `timer_type = TIMER_TYPE_ALLOC`.
    - Lập lịch timer chạy sau `arg` giây bằng `mod_timer`.

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
    return 0;
}
```
- **TODO 2**: Khởi tạo `timer_type = TIMER_TYPE_NONE` để đảm bảo timer không ở trạng thái nào khi khởi tạo.

---

### Cách hoạt động
1. **Khởi tạo**: `timer_type` được đặt thành `TIMER_TYPE_NONE`.
2. **Lệnh `MY_IOCTL_TIMER_ALLOC`**:
    - `./test a 3`: Đặt `timer_type = TIMER_TYPE_ALLOC` và lập lịch timer chạy sau 3 giây.
    - Khi timer kích hoạt, `timer_handler` gọi `alloc_io()`, ngủ 5 giây và in thông báo.
3. **Hậu quả**:
    - Vì `alloc_io()` gây chặn trong ngữ cảnh ngắt, kernel có thể gặp lỗi (panic) hoặc hành vi không xác định.
    - Điều này chứng minh rằng các thao tác chặn không nên được gọi trong `timer_handler`.

---

### Sử dụng
- Tạo thiết bị: `mknod /dev/deferred c 42 0`.
- Chạy:
    - `./test s 3`: Timer in PID và tên tiến trình sau 3 giây.
    - `./test a 3`: Timer gọi `alloc_io()` sau 3 giây (có thể gây lỗi).
    - `./test c`: Hủy timer.

---

### Kết luận
**TODO 2** được triển khai bằng cách thêm trường hợp `TIMER_TYPE_ALLOC` trong `timer_handler` để gọi `alloc_io()`
và xử lý `MY_IOCTL_TIMER_ALLOC` trong `deferred_ioctl`. Tuy nhiên, việc gọi `alloc_io()` trong ngữ cảnh ngắt là không an toàn,
thường dẫn đến lỗi kernel, minh họa nguy cơ của thao tác chặn trong timer.
