# SO2 - Networking Lab (#10) - Bài 3 và Bài 4: Simple Kernel TCP Socket

## Tổng quan

Bài 3 và bài 4 tập trung vào việc phát triển một module kernel Linux sử dụng socket kernel để tạo một server TCP đơn giản. Module này lắng nghe kết nối trên cổng 60000 (loopback) và xử lý các kết nối đến.

- **Bài 3**: Tạo một socket TCP kernel, bind và listen trên cổng 60000, cho phép module hoạt động như một server cơ bản.
- **Bài 4**: Mở rộng bài 3 bằng cách chấp nhận kết nối từ client thông qua một delayed work queue, ghi log thông tin kết nối (địa chỉ và port của client).

Module được viết trong file `tcp_sock.c`, sử dụng các API socket kernel và work queue. Kết quả được kiểm tra bằng script `test-3.sh` và `test-4.sh`.

---

### **TODO 1: Tạo, bind và listen socket TCP**
**Mục tiêu tổng quát**: Khởi tạo một socket TCP kernel, bind nó vào địa chỉ loopback (127.0.0.1) trên cổng 60000, và bắt đầu lắng nghe kết nối.

**Giải thích code theo từng phần**:
1. **Khai báo socket**:
    - `static struct socket *sock = NULL;` và `static struct socket *new_sock = NULL;`: Khai báo con trỏ tới socket lắng nghe (`sock`) và socket chấp nhận kết nối (`new_sock`). `struct socket` là cấu trúc kernel đại diện cho một socket, quản lý các hoạt động mạng.
2. **Khởi tạo module (`my_tcp_sock_init`)**:
    - **Tạo socket**:
        - `err = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);`: Sử dụng `sock_create_kern` để tạo socket TCP trong namespace `init_net` (mạng mặc định). Tham số:
            - `PF_INET`: Giao thức IPv4.
            - `SOCK_STREAM`: Loại socket TCP (kết nối).
            - `IPPROTO_TCP`: Giao thức TCP.
            - `&sock`: Con trỏ để lưu socket mới.
        - Kiểm tra lỗi và in log nếu thất bại.
    - **Bind socket**:
        - `struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(MY_TCP_PORT), .sin_addr = { htonl(INADDR_LOOPBACK) } };`: Cấu trúc địa chỉ:
            - `AF_INET`: Giao thức IPv4.
            - `htons(MY_TCP_PORT)`: Chuyển cổng 60000 sang network byte order.
            - `htonl(INADDR_LOOPBACK)`: Đặt địa chỉ là 127.0.0.1 (loopback).
        - `err = sock->ops->bind(sock, (struct sockaddr *)&addr, addrlen);`: Gọi phương thức `bind` của socket để gán địa chỉ và cổng. Kiểm tra lỗi nếu thất bại.
    - **Listen socket**:
        - `err = sock->ops->listen(sock, LISTEN_BACKLOG);`: Bắt đầu lắng nghe với độ dài hàng đợi `LISTEN_BACKLOG` (5). Kiểm tra lỗi nếu thất bại.
    - **Kiến thức liên quan**: `sock->ops` là bảng phương thức (operation table) của socket, chứa các hàm như `bind`, `listen`, `accept`, được định nghĩa bởi giao thức TCP/IP trong kernel.
3. **Cleanup khi lỗi**:
    - `out_release` và `out`: Giải phóng socket bằng `sock_release(sock)` nếu có lỗi xảy ra.

**Mục đích**: Học cách tạo và cấu hình socket TCP trong kernel, sử dụng các API cơ bản như `sock_create_kern`, `bind`, và `listen`.

---

### **TODO 2: Chấp nhận kết nối và ghi log thông tin**
**Mục tiêu tổng quát**: Thực hiện chấp nhận kết nối từ client thông qua một delayed work queue, tạo socket mới cho kết nối, và ghi log địa chỉ và port của client.

**Giải thích code theo từng phần**:
1. **Khai báo work queue**:
    - `static struct delayed_work accept_work;`: Định nghĩa một delayed work để xử lý chấp nhận kết nối không đồng bộ. Work queue trong kernel cho phép thực thi công việc (như `accept_connection`) ở context riêng, tránh chặn luồng chính.
2. **Khởi tạo work queue (`my_tcp_sock_init`)**:
    - `INIT_DELAYED_WORK(&accept_work, accept_connection);`: Khởi tạo work với hàm xử lý `accept_connection`.
    - `schedule_delayed_work(&accept_work, msecs_to_jiffies(1000));`: Lên lịch chạy work sau 1 giây (1000ms), với `msecs_to_jiffies` chuyển đổi mili giây thành số tick kernel.
3. **Hàm `accept_connection`**:
    - **Tạo socket mới**:
        - `err = sock_create_lite(PF_INET, SOCK_STREAM, IPPROTO_TCP, &new_sock);`: Tạo socket mới cho kết nối chấp nhận, sử dụng `sock_create_lite` (phiên bản nhẹ hơn `sock_create_kern`). Kiểm tra lỗi và log nếu thất bại.
        - `new_sock->ops = sock->ops;`: Gán bảng phương thức của socket lắng nghe cho socket mới để đảm bảo tương thích.
    - **Chấp nhận kết nối**:
        - `err = sock->ops->accept(sock, new_sock, O_NONBLOCK, true);`: Gọi phương thức `accept` với cờ `O_NONBLOCK` (không chặn) và `true` (không khóa). Nếu trả về `-EAGAIN` (không có kết nối), thử lại tối đa 10 lần với `msleep(500)` (chờ 500ms).
        - Kiểm tra lỗi khác và log nếu thất bại.
    - **Lấy thông tin client**:
        - `err = new_sock->ops->getname(new_sock, (struct sockaddr *)&raddr, &raddrlen);`: Lấy địa chỉ client (peer address) và lưu vào `raddr`. Kiểm tra lỗi nếu thất bại.
        - `print_sock_address(raddr);`: Gọi macro in log địa chỉ và port client (dạng `%pI4:%d`, ví dụ: `127.0.0.1:60001`).
    - **Kiến thức liên quan**: Work queue (`delayed_work`) sử dụng cơ chế scheduler của kernel để xử lý công việc không đồng bộ, phù hợp cho các tác vụ như chấp nhận kết nối mà không chặn module khởi tạo. `msleep` là hàm kernel để tạm dừng thực thi trong thời gian xác định, chỉ dùng trong context có thể ngủ (sleepable context).
4. **Cleanup (`my_tcp_sock_exit`)**:
    - `cancel_delayed_work_sync(&accept_work);`: Hủy work đang chạy và chờ hoàn tất.
    - `sock_release(new_sock)` và `sock_release(sock)`: Giải phóng socket nếu tồn tại.

**Mục đích**: Học cách xử lý kết nối TCP không đồng bộ trong kernel, sử dụng work queue và API socket để chấp nhận và log thông tin client.
