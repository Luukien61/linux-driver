# SO2 - Networking Lab (#10) - Bài 1 và Bài 2: Simple Netfilter Module

## Tổng quan

Bài 1 và bài 2 thuộc Networking tập trung vào việc phát triển một module kernel Linux sử dụng **Netfilter** để giám sát và lọc các gói tin mạng, đặc biệt là các gói TCP SYN (bắt đầu kết nối TCP). Module này kết hợp với một thiết bị character device để cho phép người dùng giao tiếp với kernel space thông qua `ioctl`.

- **Bài 1**: Xây dựng một module Netfilter cơ bản để phát hiện và ghi log tất cả các gói SYN đi ra từ máy, hiển thị thông tin về địa chỉ nguồn và port nguồn.
- **Bài 2**: Mở rộng module từ bài 1, cho phép người dùng thiết lập một địa chỉ IP đích để lọc, chỉ ghi log các gói SYN gửi đến địa chỉ đó.

Module được viết trong file `filter.c` với header `filter.h` định nghĩa các hằng số quan trọng.

---

### **TODO 1: Định nghĩa và đăng ký Netfilter hook**
**Mục tiêu tổng quát**: Thiết lập và đăng ký một Netfilter hook tại điểm `NF_INET_LOCAL_OUT` để bắt các gói tin IPv4, xử lý chúng trong hàm `hook_func`, đồng thời tích hợp một thiết bị character device cơ bản.

**Giải thích code theo từng phần**:
1. **Khai báo cấu trúc `nf_hook_ops`**:
  - `static struct nf_hook_ops nfho;` định nghĩa một biến tĩnh để lưu thông tin hook. Cấu trúc này là nền tảng để Netfilter biết cách can thiệp vào luồng gói tin.
2. **Khởi tạo module (`my_hook_init`)**:
  - `register_chrdev_region(MKDEV(MY_MAJOR, 0), 1, MY_DEVICE)`: Cấp phát major number 42 (định nghĩa trong `filter.h`) và tên thiết bị `"filter"` cho giao diện character device, trả về lỗi nếu thất bại.
  - `atomic_set(&ioctl_set, 0)` và `ioctl_set_addr = 0`: Khởi tạo trạng thái lọc (chưa thiết lập địa chỉ).
  - `cdev_init(&my_cdev, &my_fops)` và `cdev_add(&my_cdev, MKDEV(MY_MAJOR, 0), 1)`: Khởi tạo và thêm thiết bị character vào hệ thống.
  - Cấu hình `nfho`:
    - `nfho.hook = hook_func`: Gán hàm xử lý gói tin.
    - `nfho.hooknum = NF_INET_LOCAL_OUT`: Hook tại điểm gói tin đi ra từ máy.
    - `nfho.pf = PF_INET`: Chỉ định IPv4.
    - `nfho.priority = NF_IP_PRI_FIRST`: Đặt ưu tiên cao nhất.
    - `nf_register_net_hook(&init_net, &nfho)`: Đăng ký hook vào namespace mạng mặc định.
3. **Thoát module (`my_hook_exit`)**:
  - `nf_unregister_net_hook(&init_net, &nfho)`: Hủy đăng ký hook để giải phóng tài nguyên.
  - `cdev_del(&my_cdev)` và `unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1)`: Xóa thiết bị và giải phóng major number.
4. **Hàm xử lý `hook_func`**:
  - `iph = ip_hdr(skb)`: Trích xuất header IP, kiểm tra không NULL để tránh lỗi.
  - `if (iph->protocol == IPPROTO_TCP)`: Kiểm tra giao thức TCP, lấy header TCP (`tcph = tcp_hdr(skb)`).
  - `if (tcph->syn && !tcph->ack)`: Xác định gói SYN (khởi đầu kết nối TCP).
  - `if (!atomic_read(&ioctl_set) || test_daddr(iph->daddr))`: Kiểm tra điều kiện log (chưa lọc hoặc địa chỉ khớp).
  - `printk(LOG_LEVEL "TCP connection initiated from %pI4:%d\n", &iph->saddr, ntohs(tcph->source))`: Ghi log địa chỉ nguồn và port nguồn.
  - `return NF_ACCEPT`: Cho phép gói tin tiếp tục.

**Mục đích**: Học cách sử dụng Netfilter để giám sát gói tin, quản lý thiết bị character, và đảm bảo cleanup tài nguyên.

---

### **TODO 2: Lọc gói SYN theo địa chỉ đích**
**Mục tiêu tổng quát**: Thêm chức năng lọc gói SYN dựa trên địa chỉ IP đích được thiết lập qua `ioctl`, tích hợp với logic trong `hook_func`.

**Giải thích code theo từng phần**:
1. **Khai báo biến lọc**:
  - `static atomic_t ioctl_set`: Biến atomic để đánh dấu trạng thái lọc (0: chưa thiết lập, 1: đã thiết lập), đảm bảo an toàn luồng.
  - `static unsigned int ioctl_set_addr`: Lưu địa chỉ IP đích (kiểu `unsigned int`) được thiết lập từ user-space.
2. **Hàm `test_daddr`**:
  - `int ret = 0;`: Khởi tạo kết quả mặc định.
  - `if (atomic_read(&ioctl_set) && (ioctl_set_addr == dst_addr)) ret = 1;`: Kiểm tra trạng thái lọc và so sánh địa chỉ, trả về 1 nếu khớp.
  - `return ret;`: Trả về kết quả để `hook_func` sử dụng.
  - **Mục đích**: Xác định xem gói tin có cần log dựa trên địa chỉ đích.
3. **Xử lý `ioctl` trong `my_ioctl`**:
  - `case MY_IOCTL_FILTER_ADDRESS`: Xử lý lệnh từ `filter.h` (`_IOW('k', 1, unsigned int)`).
  - `if (copy_from_user(&ioctl_set_addr, (unsigned int *)arg, sizeof(unsigned int))) return -EFAULT;`: Sao chép địa chỉ IP từ user-space, trả lỗi nếu thất bại.
  - `atomic_set(&ioctl_set, 1);`: Đánh dấu địa chỉ đã được thiết lập.
  - `default: return -ENOTTY;`: Xử lý lệnh không hợp lệ.
  - **Mục đích**: Cung cấp giao diện để user-space gửi địa chỉ IP.
4. **File operations (`my_fops`)**:
  - `.open = my_open` và `.release = my_close`: Hàm rỗng, chỉ trả về 0 (chưa sử dụng).
  - `.unlocked_ioctl = my_ioctl`: Gán hàm xử lý lệnh `ioctl`.
  - **Mục đích**: Định nghĩa giao diện character device.

**Mục đích**: Học cách giao tiếp user-kernel qua `ioctl`, triển khai logic lọc, và tích hợp với Netfilter.