Dưới đây là giải thích  **3-4-5-deferred**, 
tập trung vào **TODO 1** (triển khai bộ đếm thời gian) và cách mô-đun điều khiển timer qua `ioctl`.

---

### Tổng quan
Mô-đun kernel tạo thiết bị ký tự **deferred**, cho phép dùng `ioctl` từ user space để:
- **MY_IOCTL_TIMER_SET**: Lập lịch timer chạy một lần sau `N` giây, hiển thị PID và tên tiến trình.
- **MY_IOCTL_TIMER_CANCEL**: Hủy timer.

---

### Giải thích mã

#### Thành phần chính
1. **Header Files**:
    - Cung cấp hàm kernel (`pr_info`), hỗ trợ mô-đun, thiết bị ký tự, timer (`timer.h`, `jiffies.h`), và tiến trình (`sched.h`).
    - `../include/deferred.h`: Chứa định nghĩa `ioctl` (`MY_IOCTL_TIMER_SET`, `MY_IOCTL_TIMER_CANCEL`).

2. **Cấu trúc dữ liệu**:
    - `struct my_device_data`: Chứa `cdev` (thiết bị ký tự), `timer` (bộ đếm thời gian, TODO 1), `timer_type` (cờ trạng thái).

3. **Hàm `timer_handler`**:
```c
static void timer_handler(struct timer_list *tl)
{
    struct my_device_data *my_data = container_of(tl, struct my_device_data, timer);
    if (my_data->timer_type == TIMER_TYPE_SET) {
        pr_info("Timer fired: PID=%d, Process=%s\n", current->pid, current->comm);
    }
}
```
- Gọi khi timer hết hạn, in PID (`current->pid`) và tên tiến trình (`current->comm`, thường là `swapper/0`) nếu `timer_type == TIMER_TYPE_SET`.
- **TODO 1**: Hiển thị thông tin tiến trình.

4. **Hàm `deferred_ioctl`**:
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
    default:
        return -ENOTTY;
    }
    return 0;
}
```
- Xử lý lệnh `ioctl`:
    - `MY_IOCTL_TIMER_SET`: Đặt `timer_type` và lập lịch timer chạy sau `arg` giây (TODO 1).
    - `MY_IOCTL_TIMER_CANCEL`: Hủy timer (TODO 1).
    - Trả về `-ENOTTY` nếu lệnh không hợp lệ.

5. **Hàm `deferred_init`**:
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
- Đăng ký thiết bị, khởi tạo `timer_type`, thiết bị ký tự, và timer với `timer_setup` (TODO 1).

6. **Hàm `deferred_exit`**:
```c
static void deferred_exit(void)
{
    cdev_del(&dev.cdev);
    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);
    del_timer_sync(&dev.timer);
}
```
- Dọn dẹp thiết bị và hủy timer (TODO 1).

---

### Cách hoạt động
1. **Khởi tạo**: Đăng ký thiết bị, khởi tạo timer (`timer_setup`).
2. **Ioctl**:
    - `./test s 3`: Lập lịch timer chạy sau 3 giây, in PID và tên tiến trình (thường `swapper/0`, PID 0).
    - `./test c`: Hủy timer.
3. **Dọn dẹp**: Hủy timer và giải phóng tài nguyên khi gỡ mô-đun.

---

### TODO 1
- **Khởi tạo timer**: `timer_setup(&dev.timer, timer_handler, 0)` trong `deferred_init`.
- **Lập lịch timer**: `mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(arg * 1000))` trong `ioctl` (`MY_IOCTL_TIMER_SET`).
- **Hủy timer**: `del_timer_sync` trong `ioctl` (`MY_IOCTL_TIMER_CANCEL`) và `deferred_exit`.
- **Xử lý timer**: In PID và tên tiến trình trong `timer_handler`.

---

### Kết luận
Mô-đun tạo thiết bị ký tự điều khiển timer qua `ioctl`. `MY_IOCTL_TIMER_SET` lập lịch timer chạy một lần, 
hiển thị thông tin tiến trình. `MY_IOCTL_TIMER_CANCEL` hủy timer. **TODO 1** được triển khai đầy đủ, đảm bảo timer hoạt động như yêu cầu.