# SO2 - Networking Lab (#10) - Bài 5: Simple Kernel UDP Socket

## Tổng quan

Bài 5 tập trung vào việc phát triển một module kernel Linux sử dụng socket kernel để tạo một client UDP đơn giản. Module này gửi một thông điệp UDP tới địa chỉ loopback (127.0.0.1) trên cổng 60001.

- **Bài 5**: Tạo một socket UDP kernel, bind nó vào cổng 60000 (loopback), và gửi thông điệp "kernelsocket\n" tới cổng 60001 (loopback) thông qua hàm `kernel_sendmsg`.

Module được viết trong file `udp_sock.c`, sử dụng các API socket kernel. Kết quả được kiểm tra bằng script `test5.sh`.

---

### **TODO 1: Tạo, bind và gửi thông điệp qua socket UDP**
**Mục tiêu tổng quát**: Khởi tạo một socket UDP kernel, bind nó vào địa chỉ loopback (127.0.0.1) trên cổng 60000, và gửi thông điệp "kernelsocket\n" tới cổng 60001 (loopback) thông qua hàm `my_udp_msgsend`.

**Giải thích code theo từng phần**:
1. **Khai báo socket**:
    - `static struct socket *sock;`: Khai báo con trỏ tới socket UDP. `struct socket` là cấu trúc kernel đại diện cho một socket, quản lý các hoạt động mạng.
2. **Khởi tạo module (`my_udp_sock_init`)**:
    - **Tạo socket**:
        - `err = sock_create_kern(&init_net, PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);`: Sử dụng `sock_create_kern` để tạo socket UDP trong namespace `init_net` (mạng mặc định). Tham số:
            - `PF_INET`: Giao thức IPv4.
            - `SOCK_DGRAM`: Loại socket UDP (datagram, không kết nối).
            - `IPPROTO_UDP`: Giao thức UDP.
            - `&sock`: Con trỏ để lưu socket mới.
        - Kiểm tra lỗi và in log nếu thất bại.
    - **Bind socket**:
        - `struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(MY_UDP_LOCAL_PORT), .sin_addr = { htonl(INADDR_LOOPBACK) } };`: Cấu trúc địa chỉ:
            - `AF_INET`: Giao thức IPv4.
            - `htons(MY_UDP_LOCAL_PORT)`: Chuyển cổng 60000 sang network byte order.
            - `htonl(INADDR_LOOPBACK)`: Đặt địa chỉ là 127.0.0.1 (loopback).
        - `err = sock->ops->bind(sock, (struct sockaddr *)&addr, addrlen);`: Gọi phương thức `bind` để gán địa chỉ và cổng. Kiểm tra lỗi nếu thất bại.
    - **Gửi thông điệp**:
        - `err = my_udp_msgsend(sock);`: Gọi hàm `my_udp_msgsend` để gửi thông điệp "kernelsocket\n". Kiểm tra lỗi nếu thất bại.
    - **Cleanup khi lỗi**:
        - `out_release` và `out`: Giải phóng socket bằng `sock_release(sock)` nếu có lỗi xảy ra.
3. **Hàm `my_udp_msgsend`**:
    - **Cấu hình địa chỉ đích**:
        - `struct sockaddr_in raddr = { .sin_family = AF_INET, .sin_port = htons(MY_UDP_REMOTE_PORT), .sin_addr = { htonl(INADDR_LOOPBACK) } };`: Đặt địa chỉ đích là `127.0.0.1:60001`.
    - **Xây dựng thông điệp**:
        - `struct msghdr msg;`: Cấu trúc `msghdr` chứa metadata của thông điệp:
            - `msg.msg_name = &raddr;`: Địa chỉ đích.
            - `msg.msg_namelen = raddrlen;`: Độ dài địa chỉ.
            - `msg.msg_flags = 0;`: Không dùng cờ đặc biệt.
            - `msg.msg_control = NULL;` và `msg.msg_controllen = 0;`: Không dùng control message.
        - `struct iovec iov;`: Cấu trúc `iovec` chứa dữ liệu:
            - `iov.iov_base = buffer;`: Con trỏ tới thông điệp "kernelsocket\n".
            - `iov.iov_len = len;`: Độ dài thông điệp (bao gồm ký tự null).
    - **Gửi thông điệp**:
        - `struct kvec vec = { .iov_base = iov.iov_base, .iov_len = iov.iov_len };`: Chuyển `iovec` thành `kvec` (yêu cầu bởi `kernel_sendmsg`).
        - `int ret = kernel_sendmsg(s, &msg, &vec, 1, len);`: Gửi thông điệp qua socket với:
            - `s`: Socket UDP.
            - `&msg`: Metadata thông điệp.
            - `&vec`: Dữ liệu (1 vector).
            - `1`: Số vector.
            - `len`: Độ dài thông điệp.
        - Kiểm tra lỗi và in log nếu thất bại.
    - **Kiến thức liên quan**: `kernel_sendmsg` là API kernel để gửi dữ liệu qua socket, tương tự `sendmsg` ở user-space nhưng được tối ưu cho kernel-space. UDP không cần kết nối trước (connectionless), nên thông điệp được gửi trực tiếp tới địa chỉ đích.
4. **Thoát module (`my_udp_sock_exit`)**:
    - `if (sock) { sock_release(sock); sock = NULL; }`: Giải phóng socket khi gỡ module để tránh rò rỉ tài nguyên.

**Mục đích**: Học cách tạo và sử dụng socket UDP trong kernel, gửi dữ liệu qua `kernel_sendmsg`, và quản lý tài nguyên socket.

---

## Kết quả

- **Bài 5** (Kiểm tra với `test5.sh`):
    - **Script `test5.sh`**:
        - `../netcat -l -u -p 60001 &`: Khởi động `netcat` để lắng nghe UDP trên `127.0.0.1:60001`, chạy nền.
        - `pid=$!`: Lưu PID của `netcat`.
        - `sleep 1`: Chờ `netcat` sẵn sàng lắng nghe.
        - `insmod udp_sock.ko`: Cài module, module sẽ gửi thông điệp "kernelsocket\n" tới `127.0.0.1:60001`.
        - `rmmod udp_sock`: Gỡ module.
        - `kill $pid 2>/dev/null`: Kết thúc `netcat`.
