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

### TODO 1

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


 1. `request_region(...)`

```c
int request_region(unsigned long start, unsigned long n, const char *name);
```

- **`start`**: Địa chỉ bắt đầu của vùng I/O port.
- **`n`**: Số lượng cổng I/O cần yêu cầu (ở đây là `1`, vì mỗi register là một byte).
- **`name`**: Tên module hoặc thiết bị (dùng để ghi log).

**✅ Mục đích:**
- Đăng ký quyền truy cập vào một **vùng I/O port** với hệ thống kernel.
- Giúp đảm bảo rằng không có module nào khác đang sử dụng cùng địa chỉ I/O.

👉 Nếu thành công, bạn được phép đọc/ghi vào các port đó.


**⚠️ Tại sao phải dùng `request_region()`?**

Trong kernel Linux:

- Nhiều thiết bị phần cứng chia sẻ cùng một dải địa chỉ I/O.
- Bạn **không nên truy cập trực tiếp I/O port** nếu chưa yêu cầu quyền từ kernel.
- Hàm `request_region(...)` giúp:
    - Kiểm tra xem đã có module nào khác đang sử dụng chưa.
    - Gán tên cho vùng I/O để dễ debug.
    - Ngăn xung đột giữa các driver.
---

---

**🔍 1. Tại sao lại cần đăng ký hai địa chỉ?**

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

**⚠️ 2. Tại sao phải gọi `release_region(I8042_DATA_REG, 1);` khi đăng ký `I8042_STATUS_REG` thất bại?**



- Lý do phải gọi `release_region(...)` ở đây:
    - Nếu `I8042_DATA_REG` được đăng ký thành công, nhưng `I8042_STATUS_REG` thất bại → bạn đã chiếm giữ một tài nguyên mà không thể hoàn tất việc khởi tạo driver.
    - Để đảm bảo tính toàn vẹn, bạn phải **giải phóng `I8042_DATA_REG` ngay lập tức** để tránh **rò rỉ tài nguyên** (resource leak).

👉 Đây là một kỹ thuật phổ biến trong lập trình kernel:  
**"Nếu bước sau thất bại, dọn dẹp những gì đã cấp phát trước đó."**

---

**Tại sao lại không có `release_region(...)` khi đăng ký `I8042_DATA_REG` thất bại?**

Vì:
- Nếu `I8042_DATA_REG` thất bại ngay từ đầu → bạn chưa đăng ký bất kỳ vùng nào khác.
- Không có gì để giải phóng ⇒ không cần gọi `release_region()`.

### TODO 2

**✅ 1. `request_irq(...)`**


Hàm này được dùng để **đăng ký handler xử lý gián đoạn** cho một **ngắt cụ thể**, ví dụ như ngắt từ bàn phím (`I8042_KBD_IRQ`).


```c
int request_irq(unsigned int irq,
                irq_handler_t handler,
                unsigned long flags,
                const char *devname,
                void *dev_id);
```

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

**⚠️ 2. Tại sao phải dùng `IRQF_SHARED`?**

- Nhiều thiết bị có thể chia sẻ cùng một ngắt (ví dụ: PS/2 keyboard và mouse).
- Khi đó, mỗi driver đều đăng ký handler riêng và ghi nhận `dev_id`.
- Kernel sẽ gọi tất cả các handler đã đăng ký với cờ `IRQF_SHARED`.

---

**🛑 3. `free_irq(...)` là gì?**

Hàm này **giải phóng ngắt** mà bạn đã đăng ký trước đó. Thường được gọi trong hàm `module_exit()` để dọn dẹp tài nguyên.

```c
void free_irq(unsigned int irq, void *dev_id);
```

| Tham số | Ý nghĩa |
|---------|----------|
| `irq` | Số hiệu ngắt mà bạn đã đăng ký |
| `dev_id` | Con trỏ bạn đã truyền vào `request_irq(...)`, giúp kernel xác định handler nào cần hủy |


```c
free_irq(I8042_KBD_IRQ, &devs[0]);
```

👉 Giải phóng ngắt `I8042_KBD_IRQ` và thông báo rằng driver không còn muốn xử lý nữa.
free_irq(I8042_KBD_IRQ, &devs[0]) chỉ giải phóng ngắt (IRQ) đã được đăng ký trước đó bằng request_irq() với cùng dev_id (ở đây là &devs[0]). Nó không ảnh hưởng đến các driver khác đã đăng ký cùng I8042_KBD_IRQ nhưng với dev_id khác.

---

**🔄 4. Hàm xử lý gián đoạn: `kbd_interrupt_handler`**

```c
static irqreturn_t kbd_interrupt_handler(int irq, void *dev_id)
{
    pr_info("Keyboard interrupt occurred\n");
    return IRQ_NONE;
}
```

---

**🔍 Giải thích chi tiết:**

- `irq`: Số hiệu ngắt đã xảy ra.
- `dev_id`: Con trỏ bạn truyền vào khi gọi `request_irq(...)`.
- `pr_info(...)`: Ghi log vào kernel message.


| Giá trị trả về | Ý nghĩa |
|----------------|---------|
| `IRQ_HANDLED` | Driver đã xử lý ngắt thành công |
| `IRQ_NONE`     | Không xử lý được ngắt (có thể do không phải ngắt dành cho mình) |

---


Nếu chỉ in log nhưng **không thực sự xử lý ngắt**, hãy trả về `IRQ_NONE`.  
Nếu **đã xử lý ngắt**, hãy trả về `IRQ_HANDLED`.

---

**🧩 5. Flow tổng quát**

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

**Lưu ý**
- To get access to the keyboard on the virtual machine boot with:
```shell
make copy
QEMU_DISPLAY=gtk make boot
dmesg
```
- Nếu dùng terminal serial (putty/minicom) để nhập lệnh: Sẽ không thấy thông báo ngắt nào trong `dmesg`

### TODO 3
 1. `SCANCODE_RELEASED_MASK = 0x80`
- Đây là **mask bit** để kiểm tra xem **phím đang được nhấn (`press`) hay nhả (`release`)**.
- Trong giao thức PS/2 (và nhiều loại bàn phím vật lý), khi một phím **được nhả**, giá trị **scancode sẽ có bit 7 (bit cao nhất) là 1**.
- Bit này thường là **bít dấu hiệu "release"**, tức là:
  - Nếu bit 7 = 0 → **key pressed**
  - Nếu bit 7 = 1 → **key released**

Ví dụ:

| Scancode | Hex | Nghĩa         |
|----------|-----|---------------|
| 0x01     | 00000001 | Phím A được nhấn |
| 0x81     | 10000001 | Phím A được nhả |

---

 2. Hàm `is_key_press(scancode)`
- Kiểm tra xem `scancode` có phải là **phím được nhấn** không.
- Dùng phép AND bit: `scancode & 0x80`.
  - Nếu kết quả khác 0 → là phím **đang được nhả**.
  - Nếu bằng 0 → là phím **đang được nhấn**.
- Hàm trả về:
  - `1` nếu là key press
  - `0` nếu là key release

---

Hàm `kbd_interrupt_handler` được gọi mỗi khi có **ngắt từ bàn phím xảy ra**, trong cả 2 trường hợp người dùng nhấn/phả phím. Hàm này:
- Đọc mã quét (scancode) từ controller.
- Xác định xem phím đang được **nhấn** hay **phả**.
- Chuyển đổi scancode thành ký tự ASCII.
- Ghi ký tự vào một **buffer vòng (circular buffer)** nếu đó là ký tự hợp lệ và phím đang được nhấn.
- `inb(port)` là một hàm hệ thống trong Linux kernel , dùng để đọc 1 byte (8 bit) từ địa chỉ I/O port .


**🔄 Flow thực thi**

```text
+-----------------------------+
|   Ngắt từ bàn phím xảy ra   |
+-----------------------------+
           ↓
     Đọc scancode từ port
           ↓
       Kiểm tra: có phải key press không?
           ↓
      Chuyển sang ký tự ASCII
           ↓
         In log debug
           ↓
       Nếu là key press & ký tự hợp lệ →
           ↓
         Ghi vào buffer với spin_lock
           ↓
     Trả về IRQ_NONE
```





### TODO 4

- `flags`: Dùng để lưu trạng thái gián đoạn khi tắt gián đoạn (`spin_lock_irqsave(...)`)
- `ret`: Biến trả về, ban đầu là `false` (nghĩa là chưa lấy được ký tự nào)

---

**Tắt gián đoạn và khóa buffer**

```c
spin_lock_irqsave(&data->lock, flags);
```

- `spin_lock_irqsave(...)`:
  - Khóa spinlock.
  - Tạm dừng gián đoạn (interrupts) để tránh xung đột với handler ngắt đang thêm dữ liệu.
- Giữ nguyên tính toàn vẹn dữ liệu khi truy cập chia sẻ giữa nhiều thread/ngắt.

**Vòng `for`
- thực hiện đọc từng ký tự từ con trỏ data vào mảng, sau mỗi lần đọc 1 ký tự sẽ update index lên 1, giảm count đi 1.
**Giải phóng khóa và khôi phục gián đoạn sau khi đọc xong**

```c
spin_unlock_irqrestore(&data->lock, flags);
```

- Mở khóa spinlock.
- Khôi phục trạng thái gián đoạn trước đó.

**Hàm `copy_to_user(user_buffer, local_buf, read_bytes)`
- thực hiện copy dữ liệu từ local_buf sang user space user_buffer. Trả về 0 nếu thành công.
- vì hàm này không thể được thực hiện khi đang lock nên cần đọc hết dữ liệu rồi mới copy.

**⚠️ 6. Tại sao phải dùng `spin_lock_irqsave()`?**

- Vì buffer có thể bị **viết đồng thời** bởi handler ngắt.
- Bạn cần đảm bảo **truy cập độc quyền** đến buffer.
- `spin_lock_irqsave(...)` vừa khóa buffer, vừa tắt gián đoạn → an toàn cho vùng mã tới hạn (critical section).

---

**🎯 10. Tại sao phải dùng buffer vòng?**

- Để **giữ lại lịch sử input** từ bàn phím.
- Cho phép ứng dụng đọc ký tự **không trực tiếp từ handler ngắt**.
- Buffer vòng giúp **tối ưu bộ nhớ** và dễ quản lý hơn so với mảng cố định.



Hàm `spin_lock_irqsave(&data->lock, flags);` thực hiện hai việc rất quan trọng trong môi trường kernel (nhân Linux), đặc biệt khi làm việc với **driver** hoặc **critical section (vùng quan trọng)**:

---

**Ý nghĩa của `spin_lock_irqsave(&lock, flags)`**

1. **Khoá spinlock (`data->lock`)** để đảm bảo chỉ một CPU hoặc một luồng trong kernel truy cập vào tài nguyên tại một thời điểm → giúp tránh race condition (điều kiện tranh chấp).
2. **Tắt local interrupt (IRQ)** và lưu cờ trạng thái IRQ hiện tại vào biến `flags`. Việc tắt IRQ đảm bảo rằng ngắt sẽ không làm gián đoạn vùng đang được bảo vệ bởi spinlock.

---

**🔄 Khi nào dùng `spin_lock_irqsave` thay vì `spin_lock`?**

* Khi **vùng bảo vệ có thể bị gián đoạn bởi ngắt**, ví dụ: bạn đang ở trong ngữ cảnh có thể bị ngắt (interruptible context).
* Nếu không tắt IRQ, một **interrupt handler** có thể cũng cố gắng lấy cùng spinlock đó, gây **deadlock (treo vĩnh viễn)**.
---

* `flags` là biến `unsigned long` dùng để lưu trạng thái IRQ trước khi bị tắt.
* Sau khi xong việc với tài nguyên được bảo vệ, bạn cần gọi:

```c
spin_unlock_irqrestore(&data->lock, flags);
```

Để:

* **Giải phóng spinlock**
* **Khôi phục trạng thái IRQ** trước đó.

---

| Hàm                      | Chức năng                                     |
| ------------------------ | --------------------------------------------- |
| `spin_lock_irqsave`      | Khoá spinlock và tắt ngắt, lưu trạng thái IRQ |
| `spin_unlock_irqrestore` | Mở khoá và khôi phục trạng thái IRQ trước đó  |

**To test**
```shell
mknod /dev/kbd c 42 0
cat /dev/kbd
```
- Lệnh `cat /dev/kbd` sẽ thực hiện:
 - Gọi `open("/dev/kbd", O_RDONLY)` → kernel gọi `kbd_open()`

### TODO 5

```shell
mknod /dev/kbd c 42 0
cat /dev/kbd
echo "clear" > /dev/kbd
```

```c
static void reset_buffer(struct kbd *data)
{
    unsigned long flags;

    spin_lock_irqsave(&data->lock, flags);

    data->put_idx = 0;
    data->get_idx = 0;
    data->count = 0;

    spin_unlock_irqrestore(&data->lock, flags);
}
```

| Dòng | Giải thích |
|------|------------|
| `unsigned long flags;` | Biến lưu trữ trạng thái gián đoạn trước khi tắt |
| `spin_lock_irqsave(...)` | Khóa spinlock và tắt gián đoạn để đảm bảo an toàn cho vùng mã tới hạn |
| `data->put_idx = 0;` | Đặt lại chỉ số viết về vị trí bắt đầu của buffer |
| `data->get_idx = 0;` | Đặt lại chỉ số đọc về vị trí bắt đầu của buffer |
| `data->count = 0;` | Xóa đếm số lượng ký tự đang có trong buffer |
| `spin_unlock_irqrestore(...)` | Mở khóa spinlock và khôi phục trạng thái gián đoạn |

👉 Kết quả: Buffer được "làm sạch", như mới khởi tạo.

---

**📌 2. Hàm `kbd_write(...)`**


**Lấy dữ liệu thiết bị từ `file->private_data`**


- `file->private_data` là con trỏ đã được gán trong hàm `open()`.
- nó trỏ đến cấu trúc `struct kbd`, chứa buffer và các biến liên quan.


```c
if (size < 5)
    return -EINVAL;
```

- Chỉ chấp nhận lệnh có ít nhất 5 byte → vì `"clear"` có 5 ký tự.
- Nếu nhỏ hơn 5 → trả về lỗi `-EINVAL`.

**Sao chép lệnh từ không gian người dùng**

```c
err = copy_from_user(cmd, user_buffer, size);
```

- `copy_from_user(...)` là hàm an toàn để sao chép dữ liệu từ không gian người dùng sang kernel.
- Nếu có lỗi (ví dụ địa chỉ không hợp lệ), trả về `-EFAULT`.

**Thêm ký tự kết thúc chuỗi**
-` echo "clear"` gửi 6 bytes (bao gồm cả ký tự newline `\n`). 

```c
cmd[size-1] = '\0';
```

- `cmd` có kích thước 6 byte → `cmd[5]` là vị trí cuối cùng.
- Gán `\0` để biến `cmd` trở thành một **chuỗi null-terminated string**.

**So sánh với lệnh `"clear"`**

```c
if (strncmp(cmd, "clear", 5) == 0)
```

- Kiểm tra xem người dùng có gửi lệnh `"clear"` hay không.
- Nếu có → gọi `reset_buffer(data);` để xóa buffer.
- `return size`: Điều này báo cho kernel rằng tất cả dữ liệu đã được xử lý thành công

```text
+----------------------------+
|   Người dùng gọi write(...)|
+----------------------------+
           ↓
      Kiểm tra size >= 5?
         ┌─── No → return -EINVAL
         ↓ Yes
     Sao chép 5 byte từ user
           ↓
       Gán \0 vào cmd[5]
           ↓
      So sánh với "clear"
         ┌─── No → return -EINVAL
         ↓ Yes
     Gọi reset_buffer()
           ↓
       Trả về size
```
```text
I8042_STATUS_REG = 0x65
I8042_DATA_REG = 0x61

Nhưng thực tế, I8042 keyboard controller sử dụng:

Status register: 0x64
Data register: 0x60
```

```shell
mknod /dev/kbd c 42 0
cat /proc/devices
insmod skels/interrupts/kbd.ko
cat /dev/kbd
echo "clear" > /dev/kbd
rmmod skels/interrupts/kbd.ko
```

**kfifo**
```shell
sudo cp interrupts/kbd_kfifo.c /var/lib/docker/volumes/SO2_DOCKER_VOLUME/_data/tools/labs/skels/interrupts/kbd.c
```
