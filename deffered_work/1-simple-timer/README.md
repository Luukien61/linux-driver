🌟 Lab Deferred Work - Bài 1: Timer đơn giản

ℹ️ Thông tin chung
Thư mục mã nguồn: /linux/tools/labs/skels/deferred_work/1-2-timer/.  

🎯 Mục tiêu
Tạo một kernel module sử dụng timer để:

In một thông báo ra sau 1 giây kể từ khi module được tải vào kernel.


🛠️ Cách làm

Sử dụng struct timer_list để tạo một timer đơn giản.  
Khởi tạo timer bằng timer_setup và lập lịch bằng mod_timer với thời gian trì hoãn 1 giây.  
Hủy timer an toàn bằng del_timer_sync khi gỡ module để tránh lỗi.


✅ Kết quả
Sau khi tải module, thông báo xuất hiện trong dmesg sau đúng 1 giây.  
root@qemux86:~# insmod /home/root/skels/deferred_work/1-2-timer/timer.ko
[timer_init] Init module
[timer_handler] Timer fired after 1 second(s)
root@qemux86:~# rmmod timer
[timer_exit] Exit module


💻 Giải thích mã nguồn
File: timer.c
Dưới đây là mã nguồn hoàn chỉnh với giải thích chi tiết:
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

MODULE_DESCRIPTION("Simple kernel timer");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define TIMER_TIMEOUT   1

static struct timer_list timer;

static void timer_handler(struct timer_list *tl)
{
pr_info("Timer fired after %d second(s)\n", TIMER_TIMEOUT);
}

static int __init timer_init(void)
{
pr_info("[timer_init] Init module\n");
timer_setup(&timer, timer_handler, 0);
mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000));
return 0;
}

static void __exit timer_exit(void)
{
pr_info("[timer_exit] Exit module\n");
del_timer_sync(&timer);
}

module_init(timer_init);
module_exit(timer_exit);

Giải thích chi tiết

📚 Headers:

#include <linux/kernel.h>: Cung cấp pr_info để in thông báo ra dmesg.
#include <linux/init.h>: Định nghĩa macro __init và __exit cho hàm khởi tạo/thoát.
#include <linux/module.h>: Cung cấp module_init, module_exit để tạo kernel module.
#include <linux/sched.h>: Hỗ trợ thông tin tiến trình (dù bài này không dùng current).
#include <linux/timer.h>: Cung cấp struct timer_list, timer_setup, mod_timer, del_timer_sync.
#include <linux/jiffies.h>: Cung cấp jiffies và msecs_to_jiffies để xử lý thời gian.


🔖 Metadata:

MODULE_DESCRIPTION("Simple kernel timer"): Mô tả chức năng module.
MODULE_AUTHOR("SO2"): Tác giả (có thể là tên lớp hoặc nhóm).
MODULE_LICENSE("GPL"): Giấy phép GPL, đảm bảo kernel chấp nhận module.


⚙️ Hằng số và biến:

#define TIMER_TIMEOUT 1: Thời gian trì hoãn của timer là 1 giây.
static struct timer_list timer: Biến toàn cục để quản lý timer.


⏰ Hàm timer_handler:

Hàm callback, gọi khi timer hết hạn.
struct timer_list *tl: Con trỏ đến timer (ở đây là timer).
pr_info("Timer fired after %d second(s)\n", TIMER_TIMEOUT): In thông báo ra dmesg.


🚀 Hàm timer_init:

Gọi khi module được tải (insmod).
pr_info("[timer_init] Init module\n"): Xác nhận module được tải.
timer_setup(&timer, timer_handler, 0): Khởi tạo timer với callback timer_handler.
mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000)):
Lập lịch timer chạy sau 1 giây.
jiffies: Số tick hiện tại.
msecs_to_jiffies(1000): Chuyển 1 giây thành số tick.


return 0: Khởi tạo thành công.


🛑 Hàm timer_exit:

Gọi khi module được gỡ (rmmod).
pr_info("[timer_exit] Exit module\n"): Xác nhận module được gỡ.
del_timer_sync(&timer): Hủy timer an toàn, tránh lỗi kernel panic.


🔗 Đăng ký hàm:

module_init(timer_init): Chạy timer_init khi tải module.
module_exit(timer_exit): Chạy timer_exit khi gỡ module.




🔍 Thay đổi so với file gốc

File gốc có thể chỉ là khung mã với các TODO chưa hoàn chỉnh.
Thay đổi:
Thêm khai báo struct timer_list timer.
Thêm hàm timer_handler để in thông báo.
Thêm logic khởi tạo và lập lịch timer trong timer_init.
Thêm logic hủy timer trong timer_exit.




🖥️ Các lệnh đã sử dụng



Lệnh
Mô tả



cd /linux/tools/labs
Chuyển đến thư mục lab.


make build
Biên dịch mã nguồn thành module .ko.


make console
Khởi động máy ảo QEMU.


insmod .../timer.ko
Tải module vào kernel.


dmesg
Xem log kernel để kiểm tra kết quả.


rmmod timer
Gỡ module khỏi kernel.



🛠️ Khắc phục sự cố

Không thấy file .ko:Kiểm tra lệnh make build đã chạy thành công và thư mục CIFS được mount đúng (mount | grep skels).
Không thấy thông báo trong dmesg:Đảm bảo module được tải (lsmod) và chờ 1 giây để timer chạy.
Hiệu suất chậm:Do CPU không hỗ trợ KVM, cần kiên nhẫn khi chạy VM.


📝 Ghi chú

Sao lưu: File đã được lưu trong /home/root/deferred_work_backup/:
Mã nguồn: 1-2-timer/timer.c.
Module: timer.ko.
Kết quả: dmesg_output_1-2-timer.txt.


Giải thích cho thầy:
Timer trong kernel: Dùng để trì hoãn công việc, hữu ích cho lập lịch hoặc timeout.
Tầm quan trọng của del_timer_sync: Tránh lỗi kernel panic nếu timer chạy sau khi gỡ module.
Ứng dụng thực tế: Timer thường dùng trong driver để kiểm tra định kỳ hoặc xử lý ngắt.


✅ Đã hoàn thành Bài 1.

