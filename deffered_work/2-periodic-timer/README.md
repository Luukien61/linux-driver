
### Tổng quan
Mô-đun kernel này tạo một bộ đếm thời gian (timer) định kỳ gọi hàm xử lý (`timer_handler`) mỗi `TIMER_TIMEOUT` giây (được định nghĩa là 1 giây trong mã).
Bộ đếm thời gian được khởi tạo khi mô-đun được nạp, được lập lịch lại định kỳ, và được dọn dẹp khi mô-đun được gỡ bỏ.

### Giải thích mã

#### Các thành phần chính
1. **Các tệp tiêu đề (Header Files)**:
    - `<linux/kernel.h>`: Cung cấp các hàm kernel cốt lõi như `pr_info` để ghi log.
    - `<linux/init.h>`: Cung cấp macro cho việc khởi tạo (`__init`) và dọn dẹp (`__exit`) mô-đun.
    - `<linux/module.h>`: Cung cấp các macro liên quan đến mô-đun như `MODULE_LICENSE` và `module_init`.
    - `<linux/sched.h>`: Cung cấp các định nghĩa liên quan đến lập lịch, thường được yêu cầu gián tiếp bởi các API kernel.
    - `<linux/timer.h>`: Cung cấp các hàm và cấu trúc để làm việc với bộ đếm thời gian kernel.
    - `<linux/jiffies.h>`: Cung cấp `jiffies` (biến đếm thời gian kernel) và các hàm như `msecs_to_jiffies` để chuyển đổi thời gian.

3. **Định nghĩa hằng số**:
    - `#define TIMER_TIMEOUT 1`: Định nghĩa khoảng thời gian (1 giây) giữa các lần kích hoạt bộ đếm thời gian.

4. **Biến tĩnh**:
    - `static struct timer_list timer`: Khai báo một cấu trúc `timer_list` để quản lý bộ đếm thời gian.

#### Hàm xử lý bộ đếm thời gian (`timer_handler`)
```c
static void timer_handler(struct timer_list *tl)
{
    pr_info("Timer fired after %d second(s)\n", TIMER_TIMEOUT);
    mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000));
}
```
- **Chức năng**: Hàm này được gọi mỗi khi bộ đếm thời gian hết hạn.
- **Chi tiết**:
    - `pr_info("Timer fired after %d second(s)\n", TIMER_TIMEOUT)`: In một thông báo vào log kernel, thông báo rằng bộ đếm thời gian đã kích hoạt sau `TIMER_TIMEOUT` giây.
    - `mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000))`: Lập lịch lại bộ đếm thời gian để kích hoạt sau `TIMER_TIMEOUT` giây tiếp theo.
        - `jiffies`: Biến kernel đếm số lần ngắt đồng hồ (timer ticks) kể từ khi hệ thống khởi động.
        - `msecs_to_jiffies(TIMER_TIMEOUT * 1000)`: Chuyển đổi `TIMER_TIMEOUT` giây thành số `jiffies` (1 giây = 1000 mili giây).
        - `mod_timer`: Cập nhật hoặc lập lịch bộ đếm thời gian để kích hoạt vào thời điểm được chỉ định (ở đây là `jiffies` hiện tại cộng thêm thời gian chờ).
    - **TODO 2**: Dòng `mod_timer` là phần được thêm để hoàn thành yêu cầu TODO 2, đảm bảo bộ đếm thời gian được lập lịch lại định kỳ, làm cho nó hoạt động như một bộ đếm thời gian định kỳ.

#### Hàm khởi tạo mô-đun (`timer_init`)
```c
static int __init timer_init(void)
{
    pr_info("[timer_init] Init module\n");
    timer_setup(&timer, timer_handler, 0);
    mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000));
    return 0;
}
```
- **Chức năng**: Được gọi khi mô-đun được nạp vào kernel.
- **Chi tiết**:
    - `pr_info("[timer_init] Init module\n")`: In thông báo khởi tạo mô-đun.
    - `timer_setup(&timer, timer_handler, 0)`: Khởi tạo bộ đếm thời gian, liên kết nó với hàm `timer_handler`. Tham số thứ ba (0) chỉ định không sử dụng cờ đặc biệt.
    - `mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000))`: Lập lịch bộ đếm thời gian để kích hoạt lần đầu tiên sau `TIMER_TIMEOUT` giây.
    - `return 0`: Báo hiệu khởi tạo thành công.

#### Hàm dọn dẹp mô-đun (`timer_exit`)
```c
static void __exit timer_exit(void)
{
    pr_info("[timer_exit] Exit module\n");
    del_timer_sync(&timer);
}
```
- **Chức năng**: Được gọi khi mô-đun được gỡ bỏ khỏi kernel.
- **Chi tiết**:
    - `pr_info("[timer_exit] Exit module\n")`: In thông báo gỡ bỏ mô-đun.
    - `del_timer_sync(&timer)`: Xóa bộ đếm thời gian và đảm bảo nó không còn chạy.
    - Hàm `del_timer_sync` chờ đợi nếu bộ đếm thời gian đang được xử lý trên CPU khác, đảm bảo an toàn khi gỡ mô-đun.

#### Macro đăng ký mô-đun
```c
module_init(timer_init);
module_exit(timer_exit);
```
- `module_init(timer_init)`: Chỉ định `timer_init` là hàm khởi tạo khi mô-đun được nạp.
- `module_exit(timer_exit)`: Chỉ định `timer_exit` là hàm dọn dẹp khi mô-đun được gỡ bỏ.

### Tóm tắt cách hoạt động
1. Khi mô-đun được nạp (`insmod`):
    - Hàm `timer_init` khởi tạo bộ đếm thời gian và lập lịch cho nó kích hoạt lần đầu sau 1 giây.
2. Mỗi khi bộ đếm thời gian hết hạn:
    - Hàm `timer_handler` được gọi, in một thông báo và lập lịch lại bộ đếm thời gian cho lần kích hoạt tiếp theo (sau 1 giây nữa).
3. Khi mô-đun được gỡ bỏ (`rmmod`):
    - Hàm `timer_exit` xóa bộ đếm thời gian và đảm bảo không còn hoạt động nào đang chạy.

### Giải thích TODO 2
Yêu cầu TODO 2 là sửa đổi mã để bộ đếm thời gian kích hoạt định kỳ mỗi `TIMER_TIMEOUT` giây. Điều này được thực hiện bằng cách thêm dòng:
```c
mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000));
```
trong hàm `timer_handler`. Dòng này đảm bảo rằng sau mỗi lần bộ đếm thời gian kích hoạt, nó được lập lịch lại để chạy lại sau `TIMER_TIMEOUT` giây, 
tạo ra một chu kỳ lặp vô hạn cho đến khi mô-đun được gỡ bỏ.

### Kết quả
Mô-đun sẽ in thông báo `"Timer fired after 1 second(s)"` vào log kernel mỗi giây, bắt đầu từ khi mô-đun được nạp, cho đến khi nó được gỡ bỏ.
Điều này chứng minh rằng bộ đếm thời gian hoạt động định kỳ như yêu cầu.