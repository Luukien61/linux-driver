### 🧩 Mục tiêu của driver:
- Đăng ký một **platform driver**.
- Khi kernel tìm thấy một thiết bị tương ứng với driver này trong Device Tree (dựa vào thuộc tính `compatible`), nó sẽ gọi hàm `.probe`.
- Driver này không thực hiện bất kỳ tác vụ cụ thể nào, chỉ in ra log khi probe và remove.

---

## 🔍 Phân tích từng phần:

### 2. **Bảng tương thích (Device Tree matching table)**
```c
static const struct of_device_id simple_device_ids[] = {
    { .compatible = "so2,simple-device-v1" },
    { .compatible = "so2,simple-device-v2" },
    { /* sentinel */ }
};
```
- Đây là bảng các chuỗi `compatible` mà driver này hỗ trợ.
- Kernel sẽ so khớp các nút trong Device Tree có `compatible` giống với một trong hai giá trị này thì sẽ gọi `probe` của driver.
- Kết thúc bằng một mục trống `{}` như một dấu hiệu kết thúc (sentinel).

---

### 3. **Hàm probe**
```c
static int simple_probe(struct platform_device *pdev)
{
    pr_info("simple_probe() %pOF\n", pdev->dev.of_node);

    return 0;
}
```
- Hàm này được gọi khi kernel phát hiện một thiết bị phù hợp và gắn driver này vào thiết bị đó.
- `pr_info()` là macro dùng để in thông báo ra log kernel.
- `%pOF` là định dạng đặc biệt để in đường dẫn của node Device Tree liên quan đến thiết bị.

---

### 4. **Hàm remove**
```c
static int simple_remove(struct platform_device *pdev)
{
    pr_info("simple_remove()\n");

    return 0;
}
```
- Được gọi khi thiết bị bị gỡ bỏ khỏi hệ thống hoặc module bị unload.
- Cũng chỉ in ra thông báo.

---

### 5. **Cấu trúc platform_driver**
```c
struct platform_driver simple_driver = {
    .probe	= simple_probe,
    .remove	= simple_remove,
    .driver = {
        .name = "simple_driver",
        .of_match_table = simple_device_ids,
    },
};
```
- Cấu trúc chính mô tả một `platform driver`.
- Chỉ định các hàm xử lý `probe`, `remove`.
- Thiết lập tên driver và bảng match dựa trên Device Tree (`of_match_table`).

---

### 6. **Khởi tạo và hủy module**
```c
static int simple_init(void)
{
    pr_info("Simple driver init!\n");

    return platform_driver_register(&simple_driver);
}

static void simple_exit(void)
{
    pr_info("Simple driver exit\n");

    platform_driver_unregister(&simple_driver);
}

module_init(simple_init);
module_exit(simple_exit);
```
- `simple_init`: Được gọi khi module được tải vào kernel (`insmod`). Đăng ký driver với hệ thống platform.
- `simple_exit`: Được gọi khi module bị gỡ ra (`rmmod`). Hủy đăng ký driver.
- `module_init()` và `module_exit()` là macro để khai báo điểm vào/ra của module.

---

## ✅ Cách dùng

### Trong Device Tree:

```dts
my_simple_device: simple-device@12340000 {
    compatible = "so2,simple-device-v1";
    reg = <0x12340000 0x1000>;
};
```

### Tải và kiểm tra module:
```bash
sudo insmod simple_driver.ko
dmesg | grep "simple"
```

Sẽ thấy dòng:
```
[timestamp] Simple driver init!
[timestamp] simple_probe() /soc/simple-device@12340000
```

Khi gỡ module:
```bash
sudo rmmod simple_driver
dmesg | grep "simple"
```

Sẽ thấy:
```
[timestamp] simple_remove()
[timestamp] Simple driver exit
```
---

## 🔍 1. `struct of_device_id simple_device_ids[]`

### 📌 Mục đích:
Cấu trúc này được dùng để xác định các thiết bị mà driver có thể hỗ trợ, dựa trên thông tin từ **Device Tree (DTS)** — cụ thể là trường `compatible`.

Trong hệ thống nhúng ARM/Linux, **Device Tree** mô tả phần cứng cho kernel. Mỗi thiết bị trong Device Tree có một hoặc nhiều chuỗi `compatible` mô tả loại thiết bị đó.

Driver sử dụng bảng `of_device_id` để so khớp với các nút thiết bị trong Device Tree.

---

### 🧱 Cú pháp:
```c
static const struct of_device_id simple_device_ids[] = {
    { .compatible = "so2,simple-device-v1" },
    { .compatible = "so2,simple-device-v2" },
    { /* sentinel */ }
};
```

- `const`: Bảng này không thay đổi sau khi biên dịch.
- `struct of_device_id`: Một cấu trúc trong Linux kernel chứa các trường để so khớp thiết bị.

---

### 📚 Thành phần của `struct of_device_id`
Một số trường thường dùng trong `struct of_device_id`:

| Trường | Mô tả |
|--------|-------|
| `.name` | Tên thiết bị (ít dùng hơn) |
| `.type` | Kiểu thiết bị (ít dùng hơn) |
| `.compatible` | Chuỗi `compatible` trong Device Tree — hay dùng nhất |
| `.data` | Dữ liệu tùy chọn, ví dụ: con trỏ tới dữ liệu riêng của driver |


---

### 🔍 Ví dụ trong Device Tree:
Giả sử bạn có đoạn DTS như sau:
```dts
simple_dev: simple-device@10000000 {
    compatible = "so2,simple-device-v1";
    reg = <0x10000000 0x1000>;
};
```

Khi kernel khởi động:
- Nó thấy nút `simple_dev` có `compatible = "so2,simple-device-v1"`.
- So khớp với bảng `simple_device_ids`, thấy có mục khớp → gọi hàm `probe()` của driver.

---

### 🧠 Lưu ý:
- Kết thúc mảng bằng `{}` để đánh dấu kết thúc — tương tự như cách kết thúc chuỗi NULL trong C.
- Nếu không có dòng cuối cùng này, kernel có thể đọc vượt ra ngoài mảng gây lỗi nghiêm trọng.

---

## 🔧 2. `struct platform_driver simple_driver`

### 📌 Mục đích:
Đây là cấu trúc chính mô tả một **platform driver** trong Linux kernel. Platform driver là kiểu driver dùng cho các thiết bị tích hợp sẵn trên SoC (System on Chip), không phải thiết bị ngoại vi cắm nóng như USB.

---

### 🧱 Cú pháp:
```c
struct platform_driver simple_driver = {
    .probe	= simple_probe,
    .remove	= simple_remove,
    .driver = {
        .name = "simple_driver",
        .of_match_table = simple_device_ids,
    },
};
```

---

### 📦 Các thành phần chính:

#### a. `.probe`
- Hàm được gọi khi kernel gắn driver vào một thiết bị phù hợp.
- Thường dùng để:
    - Mapping vùng nhớ vật lý sang ảo (`ioremap`)
    - Đăng ký ngắt (`request_irq`)
    - Khởi tạo phần cứng
    - Đăng ký giao diện người dùng (ví dụ: device file `/dev/...`)

Ví dụ:
```c
static int simple_probe(struct platform_device *pdev)
{
    pr_info("simple_probe() %pOF\n", pdev->dev.of_node);
    return 0;
}
```

Hàm trả về `0` nếu thành công, giá trị âm nếu lỗi (vd: `-ENOMEM`).

---

#### b. `.remove`
- Gọi khi thiết bị bị gỡ bỏ hoặc module bị unload.
- Có trách nhiệm dọn dẹp tài nguyên đã cấp phát ở `probe`.

Ví dụ:
```c
static int simple_remove(struct platform_device *pdev)
{
    pr_info("simple_remove()\n");
    return 0;
}
```

---

#### c. `.driver.name`
- Tên của driver, xuất hiện trong sysfs tại `/sys/bus/platform/drivers/`.

---

#### d. `.driver.of_match_table`
- Trỏ đến bảng `of_device_id` đã định nghĩa trước đó.
- Giúp kernel biết driver này hỗ trợ những thiết bị nào trong Device Tree.

Nếu không có `.of_match_table`, kernel sẽ cố gắng so khớp theo tên thiết bị (`.name`) — nhưng điều này ít phổ biến và không nên dùng.

---

## 🧩 Tóm tắt mối liên hệ

| Thành phần | Mô tả |
|-----------|-------|
| `of_device_id[]` | Danh sách các chuỗi `compatible` mà driver hỗ trợ |
| `platform_driver` | Chứa logic xử lý (`probe`, `remove`) và thông tin driver |
| `device tree` | Mô tả phần cứng thực tế, chứa `compatible` để kernel match driver |
| `probe()` | Được gọi khi match thành công |
| `remove()` | Được gọi khi driver/module bị unload |

---

## ✅ Ví dụ tổng quan

Bạn có thể tưởng tượng quá trình hoạt động như sau:

1. Bạn thêm một thiết bị vào Device Tree:
   ```dts
   mydevice: simple-device@10000000 {
       compatible = "so2,simple-device-v1";
       reg = <0x10000000 0x1000>;
   };
   ```
**@10000000 — Địa chỉ cơ sở (base address)**
   - Là địa chỉ vật lý trên bus mà thiết bị này chiếm giữ trong bộ nhớ.
   - Thường dùng để truy cập các thanh ghi phần cứng qua memory-mapped I/O .
   - reg mô tả vùng địa chỉ mà thiết bị sử dụng:
     - 0x10000000: địa chỉ bắt đầu
     - 0x1000: kích thước vùng (4KB)


2. Khi kernel boot:
    - Đọc Device Tree.
    - Tìm thấy `mydevice` với `compatible = "so2,simple-device-v1"`.
    - So khớp với `simple_device_ids[]` trong driver.
    - Gọi `simple_probe()` để gắn driver vào thiết bị.

3. Khi bạn chạy `insmod simple_driver.ko`:
    - Module được tải vào kernel.
    - Driver được đăng ký qua `platform_driver_register(&simple_driver);`
    - Nếu thiết bị đã tồn tại trong DT, `probe()` sẽ được gọi ngay lập tức.

4. Khi bạn chạy `rmmod simple_driver`:
    - `simple_remove()` được gọi.
    - Driver bị hủy đăng ký khỏi hệ thống.

**Lưu ý**
1. Đây là 1 `platform device`
```text
Vì đây là một platform device , nó không phải là thiết bị người dùng (user-space) truy cập trực tiếp qua /dev/... như các thiết bị chữ (character device).
Việc tạo file thiết bị bằng mknod chỉ cần thiết nếu bạn xây dựng character device hoặc block device .
1. Platform Device là gì?
simple-device@10000000 trong Device Tree là một platform device — mô tả phần cứng tích hợp trên SoC (ví dụ: thanh ghi, timer, gpio controller,...).
Trình điều khiển (simple_driver) được gắn vào platform bus, không phải character hoặc misc device.
👉 Đây là driver nằm ở cấp độ kernel , dùng để tương tác với phần cứng, nhưng chưa cung cấp giao diện người dùng (user-space) .

```
2. Kernel map device bằng `compatible`
```text
Sau khi khởi động, kernel tìm kiếm `compatible` của divice để biết driver nào khớp cho device này.
nếu tìm thành công thì device được đăng ký thành công với tên khai báo.
```


---

### **1. Tại sao không cần khai báo `reg` mà vẫn hoạt động?**
- **Lý do chính**: Không phải tất cả platform devices đều cần truy cập vùng nhớ vật lý (memory-mapped I/O).
- Trong trường hợp này:
    - Driver chỉ cần **nhận biết sự tồn tại** của device thông qua compatible string
    - Không cần truy cập thanh ghi (registers) hoặc vùng nhớ cụ thể
    - Mục đích chỉ là demo cơ chế **match device-probe** bằng device tree

---

### **2. Khi nào CẦN khai báo `reg` property?**
Bạn phải khai báo `reg` khi:
```dts
device@addr {
    compatible = "vendor,device";
    reg = <0x12345678 0x1000>; // Địa chỉ + kích thước
};
```
Khi:
- Device có memory-mapped registers cần truy cập
- Driver sử dụng `platform_get_resource()` hoặc `devm_ioremap_resource()` để ánh xạ vùng nhớ
- Ví dụ: UART, GPIO controller, DMA controller...

---

### **3. Tại sao ví dụ của bạn vẫn hoạt động?**
Khi không có `reg`:
1. Kernel vẫn tạo platform device từ DT node
2. Driver match thành công nhờ `compatible` string
3. Hàm `probe()` vẫn được gọi, nhưng:
   ```c
   // Trong probe():
   struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   // res sẽ là NULL vì không có reg property
   ```

## In order to rebuild the kernel with a new `imx6ul.dtsi` file:
```shell
# modules build
tools/labs $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make build
# modules copy
tools/labs $ ARCH=arm make copy
# kernel build
/linux $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make -j8
```




