# Lab Deferred Work - SO2

Hướng dẫn này ghi lại quá trình thực hiện các bài tập trong lab Deferred Work, hoàn thành vào ngày 27/05/2025.

## Thông tin chung
- **Môi trường**: Máy ảo QEMU chạy Poky (Yocto Project Reference Distro) 2.3 qemux86.
- **Thư mục mã nguồn**: `/linux/tools/labs/skels/deferred_work/`.
- **Thư mục sao lưu**: `/home/root/deferred_work_backup/` trong VM.

## Các bài đã hoàn thành

### Bài 0: Tìm hiểu lý thuyết
- **Mục tiêu**: Tìm định nghĩa của `jiffies`, `struct timer_list`, và `spin_lock_bh`.
- **Cách làm**: Sử dụng LXR Linux để tìm kiếm.
- **Kết quả**:
  - `jiffies`: Biến toàn cục trong `<linux/jiffies.h>`, lưu số tick từ khi hệ thống khởi động.
  - `struct timer_list`: Cấu trúc trong `<linux/timer.h>`, dùng để lập lịch timer.
  - `spin_lock_bh`: Hàm trong `<linux/spinlock.h>`, khóa spinlock và vô hiệu hóa soft IRQ.
- **File**: `deferred_work_notes.txt` trong thư mục sao lưu.

### Bài 1: Timer đơn giản
- **Mục tiêu**: Hiển thị thông báo sau 1 giây khi tải module.
- **Cách làm**: Sử dụng `timer_list`, lập lịch bằng `mod_timer`.
- **Kết quả**: Sau 1 giây, thông báo xuất hiện trong `dmesg`.
- **File**:
  - Mã nguồn: `1-2-timer/timer.c`.
  - Module: `timer.ko`.
  - Kết quả: `dmesg_output_1-2-timer.txt`.
- **Lưu ý**: Dùng `insmod` và `rmmod` để kiểm tra.

### Bài 2: Timer định kỳ
- **Mục tiêu**: Hiển thị thông báo mỗi 1 giây.
- **Cách làm**: Thêm `mod_timer` vào `timer_handler` để lập lịch lại.
- **Kết quả**: Thông báo lặp lại mỗi giây trong `dmesg`.
- **File**:
  - Mã nguồn: `1-2-timer/timer.c`.
  - Module: `timer.ko`.
  - Kết quả: `dmesg_output_1-2-timer_periodic.txt`.

### Bài 3: Điều khiển timer bằng ioctl
- **Mục tiêu**: Dùng `ioctl` để lập lịch và hủy timer, in PID và tên tiến trình.
- **Cách làm**: Tạo character device, xử lý `MY_IOCTL_TIMER_SET` và `MY_IOCTL_TIMER_CANCEL`.
- **Kết quả**: Timer hoạt động theo thời gian người dùng chỉ định.
- **File**:
  - Mã nguồn: `3-4-5-deferred/kernel/deferred.c`.
  - Module: `3-4-5-deferred.ko`.
  - Chương trình user: `3-4-5-deferred/user/test`.
  - Kết quả: `dmesg_output_3-4-5-deferred.txt`.


### Bài 4: Blocking Operations (Gây lỗi Blocking)
- **Mục tiêu**: Gọi `alloc_io()` trực tiếp từ `timer_handler` để gây lỗi kernel.
- **Cách làm**: Thêm xử lý cho `MY_IOCTL_TIMER_ALLOC`, gọi `alloc_io()` trong `timer_handler`.
- **Kết quả**: Kernel báo lỗi "scheduling while atomic".
- **File**:
  - Mã nguồn: `3-4-5-deferred/kernel/deferred.c`.
  - Kết quả: `dmesg_output_3-4-5-deferred_alloc.txt`.
          root@qemux86:~# /home/root/skels/deferred_work/3-4-5-deferred/user/test a 3
        [deferred_open] Device opened
        Allocate memory after 3 seconds
        [deferred_ioctl] Command: Allocate memory
        [deferred_release] Device released
        root@qemux86:~# BUG: scheduling while atomic: swapper/0/0/0x00000102
        1 lock held by swapper/0/0:

### Bài 5: Workqueues (Sử dụng Workqueue để tránh lỗi)
- **Mục tiêu**: Dùng workqueue để trì hoãn `alloc_io()`, tránh lỗi blocking.
- **Cách làm**: Thêm `work_struct`, lập lịch bằng `schedule_work`.
- **Kết quả**: `alloc_io()` chạy mà không gây lỗi.
- **File**:
  - Mã nguồn: `3-4-5-deferred/kernel/deferred.c`.
  - Kết quả: `dmesg_output_3-4-5-deferred_workqueue.txt`.
        root@qemux86:~# /home/root/skels/deferred_work/3-4-5-deferred/user/test a 3
        [deferred_open] Device opened                                                   
        Allocate memory after 3 seconds                                                 
        [deferred_ioctl] Command: Allocate memory                                       
        [deferred_release] Device released                                              
        root@qemux86:~# Yawn! I've been sleeping for 5 seconds.   

### Bài 6: Kernel Thread
- **Mục tiêu**: Tạo kernel thread hiển thị PID và tên mỗi giây.
- **Cách làm**: Dùng `kthread_run` để tạo thread, `msleep` để lặp.
- **Kết quả**: Thread in thông tin định kỳ trong `dmesg`.
- **File**:
  - Mã nguồn: `6-kthread/kthread.c`.
        Giải thích mã
        Headers:
        Thêm #include <linux/wait.h> để sử dụng wait queues (wait_event, wake_up).
        Biến toàn cục:
        thread: Lưu con trỏ đến kernel thread để quản lý.
        Hàm my_thread_f:
        wait_event(wq_stop_thread, atomic_read(&flag_stop_thread) == 1): Thread ngủ trên wait queue wq_stop_thread cho đến khi flag_stop_thread được đặt thành 1 (tín hiệu dừng từ kthread_exit).
        atomic_set(&flag_thread_terminated, 1): Đặt cờ để báo hiệu thread đã kết thúc.
        wake_up(&wq_thread_terminated): Đánh thức module (đang chờ trong kthread_exit) để tiếp tục.
        Hàm kthread_init:
        init_waitqueue_head(&wq_stop_thread) và init_waitqueue_head(&wq_thread_terminated): Khởi tạo wait queues.
        atomic_set(&flag_stop_thread, 0) và atomic_set(&flag_thread_terminated, 0): Đặt giá trị ban đầu cho các cờ.
        kthread_run: Tạo và chạy kernel thread với tên "mykthread%d".
        Hàm kthread_exit:
        atomic_set(&flag_stop_thread, 1) và wake_up(&wq_stop_thread): Đặt cờ và đánh thức thread để thoát.
        wait_event(wq_thread_terminated, atomic_read(&flag_thread_terminated) == 1): Module chờ thread kết thúc (khi flag_thread_terminated được đặt thành 1).                                  
  - Module: `6-kthread.ko`.
  - Kết quả: `dmesg_output_6-kthread.txt`.
        root@qemux86:~# insmod /home/root/skels/deferred_work/6-kthread/kthread.ko      
        kthread: loading out-of-tree module taints kernel.                              
        [kthread_init] Init module                                                      
        [my_thread_f] Current process id is 247 (mykthread0)

### Bài 7: Giám sát danh sách tiến trình
- **Mục tiêu**: Dùng timer để giám sát danh sách tiến trình, dọn dẹp khi tiến trình chết.
- **Cách làm**: Thêm `mon_list`, `spin_lock`, xử lý `MY_IOCTL_TIMER_MON`.
- **Kết quả**: Phát hiện và dọn dẹp tiến trình đã chết.
- **File**:
  - Mã nguồn: `3-4-5-deferred/kernel/deferred.c`.
        Giải thích mã
        Thêm headers:
        #include <linux/list.h> và #include <linux/spinlock.h> để sử dụng danh sách liên kết và spinlock.
        Cấu trúc my_device_data:
        Thêm struct list_head mon_list để lưu danh sách tiến trình được giám sát.
        Thêm spinlock_t lock để bảo vệ danh sách khỏi truy cập đồng thời.
        Hàm timer_handler:
        Khi timer_type == TIMER_TYPE_MON, timer chạy định kỳ mỗi giây (mod_timer với 1000ms).
        Dùng spin_lock_irqsave/spin_unlock_irqrestore để khóa danh sách.
        Kiểm tra trạng thái tiến trình (mp->task->state == TASK_DEAD), in thông báo khi tiến trình chết, và dọn dẹp (xóa khỏi danh sách, giải phóng bộ nhớ).
        Hàm deferred_ioctl:
        Thêm xử lý cho MY_IOCTL_TIMER_MON:
        Gọi get_proc để lấy thông tin tiến trình dựa trên PID (truyền qua arg).
        Thêm tiến trình vào mon_list với list_add_tail.
        Đặt timer_type = TIMER_TYPE_MON và lập lịch timer.
        Hàm deferred_init:
        spin_lock_init(&dev.lock): Khởi tạo spinlock.
        INIT_LIST_HEAD(&dev.mon_list): Khởi tạo danh sách liên kết.
        Hàm deferred_exit:
        Dọn dẹp toàn bộ danh sách mon_list, giải phóng bộ nhớ cho từng tiến trình.
  - Kết quả: `dmesg_output_3-4-5-deferred_mon.txt`.
        root@qemux86:~# /home/root/skels/deferred_work/3-4-5-deferred/user/test p 226
        [deferred_open] Device opened                                                   
        Monitor PID 226.                                                                
        [deferred_ioctl] Command: Monitor pid                                           
        [deferred_release] Device released   

## Cách biên dịch và chạy
- **Biên dịch**: `make build` trong `/linux/tools/labs/`.
- **Chạy VM**: `make console`, đăng nhập bằng `root`.
- **Tải module**: `insmod <module.ko>`.
- **Gỡ module**: `rmmod <module>`.

## Khắc phục sự cố
- **Không thấy file `.ko`**: Kiểm tra `make build` và CIFS mount (`mount | grep skels`).
- **Lỗi `ioctl`**: Kiểm tra `/dev/deferred` bằng `ls /dev/deferred`.
- **Hiệu suất chậm**: Do CPU không hỗ trợ KVM, cần kiên nhẫn.

## Ghi chú
- Tất cả file đã được sao lưu trong `/home/root/deferred_work_backup/`.
- Đã hoàn thành toàn bộ 7 bài tập.

---