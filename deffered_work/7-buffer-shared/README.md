Dưới đây là giải thích  **7 - Buffer shared between timer and process**,
tập trung vào **TODO 4** để triển khai giám sát danh sách tiến trình bằng timer định kỳ, 
sử dụng spinlock để đồng bộ hóa truy cập danh sách giữa timer và ngữ cảnh tiến trình.

---

### Tổng quan
Mô-đun bổ sung lệnh `MY_IOCTL_TIMER_MON` để thêm tiến trình vào danh sách giám sát (`mon_list`) 
và sử dụng timer định kỳ (1 giây) để kiểm tra trạng thái tiến trình. Nếu tiến trình kết thúc (`TASK_DEAD`), 
in thông tin và xóa khỏi danh sách. Spinlock (`lock`) bảo vệ truy cập danh sách, đảm bảo an toàn trong ngữ cảnh ngắt (timer) và tiến trình (ioctl).

---

### Giải thích mã (chỉ tập trung vào TODO 4)

#### Cấu trúc dữ liệu
- `struct my_device_data`:
    - Thêm `struct list_head mon_list`: Danh sách liên kết đôi để lưu các `mon_proc` (TODO 4).
    - Thêm `spinlock_t lock`: Spinlock bảo vệ `mon_list` (TODO 4).

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
    } else if (my_data->timer_type == TIMER_TYPE_MON) {
        struct mon_proc *mp, *tmp;
        unsigned long flags;
        spin_lock_irqsave(&my_data->lock, flags);
        list_for_each_entry_safe(mp, tmp, &my_data->mon_list, list) {
            if (mp->task->state == TASK_DEAD) {
                pr_info("Process terminated: PID=%d, Name=%s\n", mp->task->pid, mp->task->comm);
                list_del(&mp->list);
                put_task_struct(mp->task);
                kfree(mp);
            }
        }
        spin_unlock_irqrestore(&my_data->lock, flags);
        mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(1000));
    }
}
```
- **TODO 4**: Thêm xử lý cho `TIMER_TYPE_MON`:
    - Sử dụng `spin_lock_irqsave` để khóa `mon_list` và tắt ngắt cục bộ (an toàn trong ngữ cảnh ngắt của timer).
    - Duyệt danh sách bằng `list_for_each_entry_safe` (an toàn khi xóa phần tử).
    - Nếu `mp->task->state == TASK_DEAD`, in PID và tên tiến trình, xóa `mon_proc` khỏi danh sách (`list_del`), 
    - giảm bộ đếm tham chiếu (`put_task_struct`), và giải phóng bộ nhớ (`kfree`).
    - Lập lịch lại timer chạy sau 1 giây (`mod_timer`).

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
        my_data->timer_type = TIMER_TYPE_NONE;
        del_timer_sync(&my_data->timer);
        cancel_work_sync(&my_data->work);
        break;
    case MY_IOCTL_TIMER_ALLOC:
        my_data->timer_type = TIMER_TYPE_ALLOC;
        mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(arg * 1000));
        break;
    case MY_IOCTL_TIMER_MON:
        {
            struct mon_proc *mp;
            unsigned long flags;
            my_data->timer_type = TIMER_TYPE_MON;
            mp = get_proc(arg);
            if (IS_ERR(mp))
                return PTR_ERR(mp);
            spin_lock_irqsave(&my_data->lock, flags);
            list_add_tail(&mp->list, &my_data->mon_list);
            spin_unlock_irqrestore(&my_data->lock, flags);
            mod_timer(&my_data->timer, jiffies + msecs_to_jiffies(1000));
        }
        break;
    default:
        return -ENOTTY;
    }
    return 0;
}
```
- **TODO 4**: Thêm trường hợp `MY_IOCTL_TIMER_MON`:
    - Đặt `timer_type = TIMER_TYPE_MON`.
    - Gọi `get_proc(arg)` để lấy `task_struct` từ PID và tạo `mon_proc`.
    - Dùng `spin_lock_irqsave` để khóa `mon_list`, thêm `mon_proc` vào danh sách (`list_add_tail`), rồi mở khóa.
    - Lập lịch timer chạy sau 1 giây.

#### Hàm `deferred_init`
```c
static int deferred_init(void)
{
    int err;
    pr_info("[deferred_init] Init module\n");
    err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME);
    if (err) return err;
    dev.timer_type = TIMER_TYPE_NONE;
    cdev_init(&dev.cdev, &my_fops);
    cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);
    timer_setup(&dev.timer, timer_handler, 0);
    INIT_WORK(&dev.work, work_handler);
    spin_lock_init(&dev.lock);
    INIT_LIST_HEAD(&dev.mon_list);
    return 0;
}
```
- **TODO 4**: Khởi tạo spinlock bằng `spin_lock_init(&dev.lock)` và danh sách bằng `INIT_LIST_HEAD(&dev.mon_list)`.

#### Hàm `deferred_exit`
```c
static void deferred_exit(void)
{
    struct mon_proc *mp, *tmp;
    unsigned long flags;
    pr_info("[deferred_exit] Exit module\n");
    cdev_del(&dev.cdev);
    unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);
    del_timer_sync(&dev.timer);
    cancel_work_sync(&dev.work);
    spin_lock_irqsave(&dev.lock, flags);
    list_for_each_entry_safe(mp, tmp, &dev.mon_list, list) {
        list_del(&mp->list);
        put_task_struct(mp->task);
        kfree(mp);
    }
    spin_unlock_irqrestore(&dev.lock, flags);
}
```
- **TODO 4**: Dọn dẹp `mon_list`:
    - Khóa danh sách bằng `spin_lock_irqsave`.
    - Duyệt và xóa từng `mon_proc`, giảm bộ đếm tham chiếu (`put_task_struct`), và giải phóng bộ nhớ (`kfree`).
    - Mở khóa sau khi dọn dẹp.

---

### Cách hoạt động
1. **Khởi tạo**: Khởi tạo spinlock, danh sách `mon_list`, timer, và workqueue.
2. **Lệnh `MY_IOCTL_TIMER_MON`**:
    - `./test t <pid>`: Thêm tiến trình (PID) vào `mon_list`, đặt `timer_type = TIMER_TYPE_MON`, lập lịch timer định kỳ (1 giây).
    - Timer kiểm tra danh sách, in thông tin và xóa tiến trình nếu `TASK_DEAD`.
3. **Dọn dẹp**: Hủy timer, workqueue, và giải phóng toàn bộ `mon_proc` trong `mon_list`.

---

### Sử dụng
- Tạo thiết bị: `mknod /dev/deferred c 42 0`.
- Chạy:
    - `./test t <pid>`: Giám sát tiến trình với PID, timer kiểm tra mỗi giây.
    - `./test c`: Hủy timer và workqueue.
- Kết quả: Khi tiến trình kết thúc, log in: `Process terminated: PID=<pid>, Name=<name>`.

---

### Kết luận
**TODO 4** được triển khai bằng cách thêm danh sách `mon_list` và spinlock `lock`, 
xử lý `MY_IOCTL_TIMER_MON` để thêm tiến trình, và kiểm tra trạng thái định kỳ trong `timer_handler`. 
Spinlock đảm bảo đồng bộ hóa an toàn giữa timer (ngữ cảnh ngắt) và ioctl (ngữ cảnh tiến trình).
Danh sách được dọn dẹp đúng cách khi gỡ mô-đun.

