**File location**
```text
/var/lib/docker/volumes/SO2_DOCKER_VOLUME/_data/tools/labs/skels/interrupts/kbd.c
```

### IO access request
```shell
insmod skels/interrupts/kbd.ko
cat /proc/ioports | egrep "(0060|0064)"
cat /proc/ioports | grep kbd
rmmod kbd
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



### TODO 2

## ✅ 1. `request_irq(...)`

### 🧠 Mục đích:

Hàm này được dùng để **đăng ký handler xử lý gián đoạn** cho một **ngắt cụ thể**, ví dụ như ngắt từ bàn phím (`I8042_KBD_IRQ`).

---

### 🔍 Cú pháp:

```c
int request_irq(unsigned int irq,
                irq_handler_t handler,
                unsigned long flags,
                const char *devname,
                void *dev_id);
```

#### Tham số:

| Tham số | Ý nghĩa |
|---------|----------|
| `irq` | Số hiệu ngắt, ví dụ: `I8042_KBD_IRQ` thường là `1` hoặc `1` trên kiến trúc x86 |
| `handler` | Hàm xử lý gián đoạn – ví dụ: `kbd_interrupt_handler` |
| `flags` | Các cờ điều khiển loại ngắt – ví dụ: `IRQF_SHARED` nếu nhiều driver chia sẻ cùng một ngắt |
| `devname` | Tên module/driver, xuất hiện trong log |
| `dev_id` | Con trỏ dữ liệu riêng, có thể truyền vào handler khi ngắt xảy ra |

---

```c
err = request_irq(I8042_KBD_IRQ, kbd_interrupt_handler, IRQF_SHARED,
                  MODULE_NAME, &devs[0]);
```

- Đăng ký `kbd_interrupt_handler` để xử lý ngắt từ bàn phím.
- `IRQF_SHARED`: Dùng để cho phép nhiều thiết bị chia sẻ cùng một ngắt.
- `&devs[0]`: Truyền con trỏ tới cấu trúc thiết bị để handler có thể truy cập dữ liệu cần thiết.

👉 Nếu thành công → `err = 0`.  
👉 Nếu thất bại → `err < 0`, ví dụ: `-EBUSY`, `-EINVAL`.

---

## ⚠️ 2. Tại sao phải dùng `IRQF_SHARED`?

- Nhiều thiết bị có thể chia sẻ cùng một ngắt (ví dụ: PS/2 keyboard và mouse).
- Khi đó, mỗi driver đều đăng ký handler riêng và ghi nhận `dev_id`.
- Kernel sẽ gọi tất cả các handler đã đăng ký với cờ `IRQF_SHARED`.

---

## 🛑 3. `free_irq(...)` là gì?

### 🧠 Mục đích:

Hàm này **giải phóng ngắt** mà bạn đã đăng ký trước đó. Thường được gọi trong hàm `module_exit()` để dọn dẹp tài nguyên.

---

### 🔍 Cú pháp:

```c
void free_irq(unsigned int irq, void *dev_id);
```

#### Tham số:

| Tham số | Ý nghĩa |
|---------|----------|
| `irq` | Số hiệu ngắt mà bạn đã đăng ký |
| `dev_id` | Con trỏ bạn đã truyền vào `request_irq(...)`, giúp kernel xác định handler nào cần hủy |

---

### ✅ Ví dụ:

```c
free_irq(I8042_KBD_IRQ, &devs[0]);
```

👉 Giải phóng ngắt `I8042_KBD_IRQ` và thông báo rằng driver không còn muốn xử lý nữa.
free_irq(I8042_KBD_IRQ, &devs[0]) chỉ giải phóng ngắt (IRQ) đã được đăng ký trước đó bằng request_irq() với cùng dev_id (ở đây là &devs[0]). Nó không ảnh hưởng đến các driver khác đã đăng ký cùng I8042_KBD_IRQ nhưng với dev_id khác.

---

## 🔄 4. Hàm xử lý gián đoạn: `kbd_interrupt_handler`

```c
static irqreturn_t kbd_interrupt_handler(int irq, void *dev_id)
{
    pr_info("Keyboard interrupt occurred\n");
    return IRQ_NONE;
}
```

---

### 🔍 Giải thích chi tiết:

- `irq`: Số hiệu ngắt đã xảy ra.
- `dev_id`: Con trỏ bạn truyền vào khi gọi `request_irq(...)`.
- `pr_info(...)`: Ghi log vào kernel message.

---

### 📌 Trả về giá trị:

| Giá trị trả về | Ý nghĩa |
|----------------|---------|
| `IRQ_HANDLED` | Driver đã xử lý ngắt thành công |
| `IRQ_NONE`     | Không xử lý được ngắt (có thể do không phải ngắt dành cho mình) |

---


Nếu chỉ in log nhưng **không thực sự xử lý ngắt**, hãy trả về `IRQ_NONE`.  
Nếu **đã xử lý ngắt**, hãy trả về `IRQ_HANDLED`.

---

## 🧩 5. Flow tổng quát

```text
+----------------------------+
|   Module init              |
+----------------------------+
           ↓
     request_irq(...)
           ↓
        OK? → Yes
           ↓
         Lắng nghe ngắt
           ↓
       Ngắt xảy ra → Gọi kbd_interrupt_handler
           ↓
       Xử lý ngắt (in log)
           ↓
      Trả về IRQ_NONE / IRQ_HANDLED
           ↓
+----------------------------+
|   Module exit              |
+----------------------------+
           ↓
     free_irq(...) ← Giải phóng ngắt
```

**Để xem ngắt đã được đăng ký chưa**
```shell
cat /proc/interrupts
```

### Lưu ý
- To get access to the keyboard on the virtual machine boot with:
```shell
make copy
QEMU_DISPLAY=gtk make boot
```
- Nếu dùng terminal serial (putty/minicom) để nhập lệnh: Sẽ không thấy thông báo ngắt nào trong `dmesg`
