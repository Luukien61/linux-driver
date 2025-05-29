**File location**
```text
/var/lib/docker/volumes/SO2_DOCKER_VOLUME/_data/tools/labs/skels/interrupts/kbd.c
```


```c
if (!request_region(I8042_DATA_REG, 1, MODULE_NAME)) {
    pr_err("failed to request I8042_DATA_REG\n");
    err = -EBUSY;
    goto out_unregister;
}

if (!request_region(I8042_STATUS_REG, 1, MODULE_NAME)) {
    pr_err("failed to request I8042_STATUS_REG\n");
    err = -EBUSY;
    release_region(I8042_DATA_REG, 1);
    goto out_unregister;
}
```

---

## 🧠 Giải thích từng hàm

### 1. `request_region(...)`

#### ✅ Định nghĩa:

```c
int request_region(unsigned long start, unsigned long n, const char *name);
```

- **`start`**: Địa chỉ bắt đầu của vùng I/O port.
- **`n`**: Số lượng cổng I/O cần yêu cầu (ở đây là `1`, vì mỗi register là một byte).
- **`name`**: Tên module hoặc thiết bị (dùng để ghi log).

#### ✅ Mục đích:
- Đăng ký quyền truy cập vào một **vùng I/O port** với hệ thống kernel.
- Giúp đảm bảo rằng không có module nào khác đang sử dụng cùng địa chỉ I/O.

👉 Nếu thành công, bạn được phép đọc/ghi vào các port đó.

#### ❗ Trả về:
- **Không NULL** nếu thành công.
- **NULL** nếu đã có ai đó đăng ký trước đó → lỗi `-EBUSY`.

---

## ⚠️ Tại sao phải dùng `request_region()`?

Trong kernel Linux:

- Nhiều thiết bị phần cứng chia sẻ cùng một dải địa chỉ I/O.
- Bạn **không nên truy cập trực tiếp I/O port** nếu chưa yêu cầu quyền từ kernel.
- Hàm `request_region(...)` giúp:
    - Kiểm tra xem đã có module nào khác đang sử dụng chưa.
    - Gán tên cho vùng I/O để dễ debug.
    - Ngăn xung đột giữa các driver.
---

---

## 🔍 1. Tại sao lại cần đăng ký **hai địa chỉ**?

### 🔧 Các địa chỉ I/O:

```c
#define I8042_STATUS_REG    0x64  // Trạng thái / lệnh
#define I8042_DATA_REG      0x60  // Dữ liệu giao tiếp với bàn phím hoặc chuột
```

- `I8042_STATUS_REG`:
    - Cho phép kiểm tra trạng thái thiết bị (ví dụ: có dữ liệu sẵn sàng không).
    - Gửi lệnh điều khiển đến controller.
- `I8042_DATA_REG`:
    - Đọc/gửi dữ liệu thực tế từ/to bàn phím hoặc chuột.

👉 Vì driver cần sử dụng **cả hai register**, nên phải yêu cầu quyền truy cập vào **cả hai địa chỉ**.

---

## ⚠️ 2. Tại sao phải gọi `release_region(I8042_DATA_REG, 1);` khi đăng ký `I8042_STATUS_REG` thất bại?



### ✅ Lý do phải gọi `release_region(...)` ở đây:

- Nếu `I8042_DATA_REG` được đăng ký thành công, nhưng `I8042_STATUS_REG` thất bại → bạn đã chiếm giữ một tài nguyên mà không thể hoàn tất việc khởi tạo driver.
- Để đảm bảo tính toàn vẹn, bạn phải **giải phóng `I8042_DATA_REG` ngay lập tức** để tránh **rò rỉ tài nguyên** (resource leak).

👉 Đây là một kỹ thuật phổ biến trong lập trình kernel:  
**"Nếu bước sau thất bại, dọn dẹp những gì đã cấp phát trước đó."**

---

## ❓ Tại sao lại không có `release_region(...)` khi đăng ký `I8042_DATA_REG` thất bại?

Vì:
- Nếu `I8042_DATA_REG` thất bại ngay từ đầu → bạn chưa đăng ký bất kỳ vùng nào khác.
- Không có gì để giải phóng ⇒ không cần gọi `release_region()`.



