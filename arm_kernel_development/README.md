```text
 /var/lib/docker/volumes/SO2_DOCKER_VOLUME_ARM/_data/tools/labs/qemu/Makefile
```

```shell
ls arch/arm/boot/dts/
./qemu/build/qemu-system-arm -M ?

```

```text
.dtsi (Device Tree Source Include):

File "gốc" chứa các thiết lập phần cứng chung (ví dụ: cấu hình CPU, bộ nhớ, ngoại vi cơ bản).

Giống như file header (.h) trong lập trình, được include vào các file khác.

.dts (Device Tree Source):

File mô tả riêng cho board imx6ul-14x14-evk, kế thừa từ .dtsi.

Thêm/sửa các thiết lập phần cứng cụ thể (ví dụ: LED, nút bấm, kết nối trên board này).

.dtb (Device Tree Blob):

File nhị phân sinh ra khi biên dịch từ .dts + .dtsi.

Device Tree (hay Device Tree Blob - DTB) là một cấu trúc dữ liệu mô tả phần cứng của hệ thống nhúng (ARM, MIPS, PowerPC, ...) 

File nguồn: .dts (Device Tree Source) hoặc .dtsi (Include).

File nhị phân sau biên dịch: .dtb (Device Tree Blob).

Kernel Linux đọc file này để biết cách giao tiếp với phần cứng.

Khi chạy Linux trên board imx6ul-14x14-evk (hoặc Raspberry Pi, Beaglebone...), kernel sẽ đọc file .dtb 
để biết cách điều khiển phần cứng (CPU, GPIO, I2C, USB...).
```

```text
QEMU không mô phỏng chính xác tất cả các tính năng phần cứng của i.MX6UL, đặc biệt là các cơ chế DVFS phức tạp.
Trong mô phỏng imx6ul-evk hoặc imx6ul-14x14-evk, QEMU có thể chỉ hỗ trợ một tập hợp giới hạn các tần số CPU, 
và 528 MHz có thể được chọn làm tần số tối đa mặc định để đơn giản hóa mô phỏng.
Trong QEMU, mô phỏng có thể được thiết kế để phản ánh cấu hình phổ biến nhất (528 MHz) thay vì hỗ trợ toàn bộ dải tần số.
```
```shell
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies

ls /sys/bus/platform/devices/900000.sram/driver

```
```text
Địa chỉ bắt đầu: 0x00900000 , SRAM là vùng nhớ riêng biệt, không nằm trong memory (DRAM chính)

Địa chỉ kết thúc: 0x0091ffff

Kích thước: 0x0091ffff - 0x00900000 + 1 = 0x20000 (128KB)

900000.sram: Tên driver quản lý vùng nhớ này (thường là driver SRAM của kernel).

sram@900000: Tên node trong Device Tree mô tả vùng SRAM này.
```

- imx6ul-14x14-evk.dtb : Định dạng: Đây là tệp Device Tree Blob (DTB), 
dạng nhị phân được biên dịch từ tệp imx6ul-14x14-evk.dts bằng công cụ Device Tree Compiler (dtc)
Được bootloader (như U-Boot) tải và truyền cho kernel Linux khi khởi động để cung cấp thông tin cấu hình phần cứng.

#### 1. **compatible = "arm,cortex-a7";**
- **Ý nghĩa**: Xác định rằng CPU này là một lõi **ARM Cortex-A7**. Thuộc tính `compatible` giúp kernel Linux nhận diện loại CPU để tải driver hoặc module phù hợp.
- **Ứng dụng**: Kernel sẽ sử dụng thông tin này để cấu hình CPU, đảm bảo tương thích với kiến trúc ARMv7-A.
- Cortex-A7 sử dụng kiến trúc ARMv7-A, là kiến trúc 32-bit
#### 2. **device_type = "cpu";**
- **Ý nghĩa**: Chỉ định rằng node này mô tả một CPU. Đây là thuộc tính bắt buộc cho các node trong khối `cpus` để kernel nhận diện thiết bị là một bộ xử lý.

#### 3. **reg = <0>;**
- **Ý nghĩa**: Chỉ định địa chỉ của CPU trong khối `cpus`. Với `cpu@0`, đây là CPU đầu tiên (lõi 0) trong hệ thống. i.MX6 UltraLite thường là CPU đơn lõi, nên chỉ có `cpu@0`.

#### 4. **clock-frequency = <696000000>;**
- **Ý nghĩa**: Xác định tần số mặc định của CPU là **696 MHz** (696000000 Hz).
- **Lưu ý**: Đây là tần số mặc định, nhưng CPU có thể chạy ở các tần số khác được liệt kê trong `operating-points`.

#### 5. **clock-latency = <61036>; /* two CLK32 periods */**
- **Ý nghĩa**: Chỉ định độ trễ (latency) khi chuyển đổi giữa các trạng thái tần số hoặc nguồn điện, tính bằng nanosecond. Giá trị 61036 ns tương ứng với hai chu kỳ của **CLK32** (thường là xung nhịp 32 kHz).
- **Ứng dụng**: Kernel sử dụng thông tin này để quản lý chuyển đổi tần số (CPUFreq) hoặc trạng thái tiết kiệm năng lượng (CPUIdle).

#### 6. **#cooling-cells = <2>;**
- **Ý nghĩa**: Chỉ định rằng CPU này hỗ trợ cơ chế làm mát (thermal cooling) với 2 giá trị (cells) trong mô tả làm mát.
- **Ứng dụng**: Được sử dụng bởi hệ thống quản lý nhiệt (thermal framework) để điều chỉnh tần số CPU hoặc trạng thái nguồn khi nhiệt độ vượt ngưỡng, nhằm tránh quá nhiệt.

#### 7. **operating-points**
```text
operating-points = <
    /* kHz   uV */
    696000  1275000
    528000  1175000
    396000  1025000
    198000  950000
>;
```
- **Ý nghĩa**: Liệt kê các điểm hoạt động (operating points) của CPU, bao gồm tần số (kHz) và điện áp (microvolt, uV) tương ứng:
    - **696 MHz** với điện áp **1.275V**.
    - **528 MHz** với điện áp **1.175V**.
    - **396 MHz** với điện áp **1.025V**.
    - **198 MHz** với điện áp **0.95V**.
- **Ứng dụng**: Kernel sử dụng thông tin này để điều chỉnh tần số CPU (qua CPUFreq) dựa trên nhu cầu hiệu suất hoặc tiết kiệm năng lượng. Ví dụ:
    - Tần số tối đa: **696 MHz**.
    - Tần số tối thiểu: **198 MHz**.

#### 8. **fsl,soc-operating-points**
```text
fsl,soc-operating-points = <
    /* KHz   uV */
    696000  1275000
    528000  1175000
    396000  1175000
    198000  1175000
>;
```
- **Ý nghĩa**: Đây là thuộc tính riêng của NXP (Freescale), mô tả các điểm hoạt động cho **SoC** (khác với `operating-points` dành cho CPU). Điện áp của SoC ở các tần số thấp hơn (396 MHz và 198 MHz) được giữ ở mức **1.175V**.
- **Ứng dụng**: Được sử dụng để cấu hình điện áp cho toàn bộ SoC, đảm bảo hoạt động ổn định ở các tần số khác nhau.


- **Ý nghĩa**: Đặt tên cho các nguồn xung nhịp được liệt kê trong `clocks`, giúp kernel và driver ánh xạ đúng các nguồn xung nhịp.
- **Ứng dụng**: Đảm bảo kernel biết cách sử dụng từng nguồn xung nhịp cho các mục đích cụ thể (ví dụ: `arm` cho CPU, `pll2_bus` cho bus hệ thống).

#### 11. **arm-supply = <®_arm>;**
- **Ý nghĩa**: Tham chiếu đến bộ điều chỉnh điện áp (regulator) cung cấp nguồn cho CPU (lõi ARM). `®_arm` là một phandle (tham chiếu) đến node regulator trong Device Tree.
- **Ứng dụng**: Kernel sử dụng thông tin này để điều chỉnh điện áp CPU dựa trên tần số (theo `operating-points`).

#### 12. **soc-supply = <®_soc>;**
- **Ý nghĩa**: Tham chiếu đến bộ điều chỉnh điện áp cung cấp nguồn cho SoC.
- **Ứng dụng**: Đảm bảo nguồn điện cho toàn bộ SoC được cấu hình đúng (theo `fsl,soc-operating-points`).

#### 13. **nvmem-cells = <&cpu_speed_grade>;**
- **Ý nghĩa**: Tham chiếu đến một ô dữ liệu trong bộ nhớ không khả biến (NVMEM, như eFUSE), chứa thông tin về **speed grade** của CPU (mức tốc độ tối đa được nhà sản xuất đảm bảo).
- **Ứng dụng**: Kernel đọc giá trị này để xác định khả năng tối đa của CPU, ví dụ: liệu CPU có thể chạy ổn định ở 696 MHz hay không.

#### 14. **nvmem-cell-names = "speed_grade";**
- **Ý nghĩa**: Đặt tên cho ô dữ liệu NVMEM được tham chiếu, ở đây là `speed_grade`.
- **Ứng dụng**: Giúp kernel truy cập đúng ô dữ liệu trong NVMEM.


### Tại sao max frequency lại không khớp với khai báo?
```text
speed-grade@10 là một eFuse/OTP (One-Time Programmable) register mà manufacturer sử dụng để bin CPU theo performance level.
Đây là giới hạn phần cứng do nhà sản xuất (NXP) thiết lập, dựa trên chất lượng chip (binning).
  Speed Grade hoạt động như thế nào:
1. Manufacturer binning process:

Khi sản xuất, mỗi chip được test với các frequency khác nhau
Những chip pass test ở 696MHz → được mark là grade cao
Những chip chỉ stable ở 528MHz → được mark là grade thấp hơn
Grade được burn vào eFuse register tại offset 0x10

2. Kernel đọc speed grade:
   c// Trong driver, kernel sẽ đọc:
   speed_grade = readl(fuse_base + 0x10);
   switch (speed_grade) {
   case 0: max_freq = 696000; break;  // High grade
   case 1: max_freq = 528000; break;  // Medium grade  
   case 2: max_freq = 396000; break;  // Low grade
   // ...
   }
```
```shell
grep -r "fuse" /proc/device-tree/
```

Kiểm tra node efuse@21bc000:

```shell
ls /proc/device-tree/soc/bus@2100000/efuse@21bc000/
hexdump -C /proc/device-tree/soc/bus@2100000/efuse@21bc000/cpu_speed_grade
```
Nếu không có dữ liệu, QEMU không mô phỏng eFUSE, dẫn đến kernel chọn tần số an toàn (528 MHz).

**Đầu ra: Nội dung của `hexdump -C /sys/bus/nvmem/devices/*/nvmem` (eFUSE) chỉ chứa toàn các byte 0x00 (giá trị rỗng) từ địa chỉ 0x00000000 đến 0x00000204.**

- Trong môi trường QEMU, eFUSE không được mô phỏng đầy đủ, dẫn đến dữ liệu trong `/sys/bus/nvmem/devices/*/nvmem` không chứa giá trị speed grade thực tế (giá trị xác định tần số tối đa của CPU).
- Trong node cpu@0 của Device Tree, bạn có `nvmem-cells = <&cpu_speed_grade>`, cho thấy kernel cố gắng đọc giá trị speed grade từ eFUSE để xác định tần số tối đa. Vì eFUSE trả về giá trị rỗng (0x00), kernel có thể mặc định chọn tần số an toàn là 528 MHz thay vì 696 MHz (theo operating-points trong imx6ul.dtsi).
- Kết luận: QEMU không cung cấp giá trị speed grade hợp lệ, khiến kernel giới hạn tần số tối đa ở 528 MHz.

**Trong môi trường QEMU**, **eFUSE không được mô phỏng đầy đủ**, hoặc thậm chí **không được mô phỏng** trong phần lớn cấu hình và bo mạch ảo.

### 1. **eFUSE là phần cứng vật lý một lần ghi (OTP – One Time Programmable)**

* eFUSE là các bit vật lý trên chip, **chỉ có thể ghi một lần duy nhất**, dùng để lưu:

    * Khóa bảo mật (encryption key)
    * Cấu hình boot
    * Tùy chỉnh chip ID, MAC, hay vùng cấm truy cập
* Việc mô phỏng chính xác **“ghi một lần – không thể thay đổi”** là **khó và ít cần thiết** trong môi trường ảo.

---

### 2. **QEMU ưu tiên mô phỏng CPU, RAM, ngoại vi phổ biến**

* QEMU tập trung mô phỏng:

    * CPU (ARM, x86, RISC-V…)
    * RAM, GPIO, UART, SPI, I2C, MMIO
    * Một số thiết bị đặc biệt (timer, interrupt controller, NIC…)
* Các phần tử hiếm dùng, ít tài liệu công khai, hoặc quá gắn với phần cứng cụ thể (như eFUSE) thì:

    * ❌ Không mô phỏng
    * ✅ Hoặc chỉ mô phỏng ở mức "stub" – có địa chỉ nhưng không hoạt động thật

---

### 3. **Bảo mật và OEM – eFUSE thường mang tính độc quyền**

* Nhiều SoC (Qualcomm, NXP, ESP32, i.MX, v.v.) có eFUSE thiết kế riêng theo từng hãng.
* Cấu trúc eFUSE, cách ghi, bảo vệ... thường là tài liệu **NDA hoặc không công khai**.
* QEMU không thể mô phỏng chuẩn nếu thiếu thông tin chi tiết.

---

| Đặc điểm                     | Mô phỏng eFUSE trong QEMU                                                   |
| ---------------------------- | --------------------------------------------------------------------------- |
| Tính khả thi kỹ thuật        | Khó – eFUSE là OTP vật lý                                                   |
| Có tài liệu công khai không? | Ít hoặc độc quyền                                                           |
| Có được QEMU mô phỏng?       | ❌ Phần lớn là không<br>✅ Một số stub (nếu có) chỉ dùng để kiểm tra truy cập |


### 1. **Kích thước của System RAM**
Node `memory@80000000`:
```dts
memory@80000000 {
    device_type = "memory";
    reg = <0x80000000 0x20000000>;
};
```

#### **Giải thích**:
- Thuộc tính `reg` mô tả vùng bộ nhớ cho RAM, gồm hai giá trị:
    - `0x80000000`: Địa chỉ bắt đầu của vùng RAM (địa chỉ vật lý).
    - `0x20000000`: Kích thước của vùng RAM (theo byte, ở dạng thập lục phân).

#### **Tính toán kích thước**:
- Giá trị `0x20000000` (hex) = **512 MB** (tính toán: `0x20000000` = 536,870,912 byte = 512 × 1024 × 1024 byte = 512 MB).
- Vì vậy, kích thước của **System RAM** là **512 MB**.

---

### 2. **Kích thước của I/O space cho thiết bị GPIO (`gpio1`)**
Node `gpio1: gpio@209c000`:
```dts
gpio1: gpio@209c000 {
    compatible = "fsl,imx6ul-gpio", "fsl,imx35-gpio";
    reg = <0x0209c000 0x4000>;
    interrupts = <GIC_SPI 66 IRQ_TYPE_LEVEL_HIGH>,
                 <GIC_SPI 67 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clks IMX6UL_CLK_GPIO1>;
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
    gpio-ranges = <&iomuxc 0 23 10>, <&iomuxc 10 17 6>,
                  <&iomuxc 16 33 16>;
};
```
```text
Chân đa năng có thể cấu hình thành ngõ vào hoặc ngõ ra tín hiệu số.

Công dụng:

Điều khiển LED, nút nhấn.

Giao tiếp với cảm biến đơn giản (ví dụ: DHT11).

Tạo xung PWM (điều khiển servo, motor).
```
#### **Giải thích**:
- Thuộc tính `reg` mô tả vùng I/O space cho thiết bị GPIO:
    - `0x0209c000`: Địa chỉ bắt đầu của vùng I/O.
    - `0x4000`: Kích thước của vùng I/O (theo byte, ở dạng thập lục phân).

#### **Tính toán kích thước**:
- Giá trị `0x4000` (hex) = **16 KB** (tính toán: `0x4000` = 16,384 byte = 16 × 1024 byte = 16 KB).
- Vì vậy, kích thước của **I/O space** cho thiết bị `gpio1` là **16 KB**.

---

### 3. **Kích thước của I/O space cho thiết bị I2C (`i2c1`)**
Node `i2c1: i2c@21a0000`:
```dts
i2c1: i2c@21a0000 {
    #address-cells = <1>;
    #size-cells = <0>;
    compatible = "fsl,imx6ul-i2c", "fsl,imx21-i2c";
    reg = <0x021a0000 0x4000>;
    interrupts = <GIC_SPI 36 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clks IMX6UL_CLK_I2C1>;
    status = "disabled";
};
```
```text
Giao tiếp nối tiếp tốc độ thấp, 2 dây (SCL + SDA).

Công dụng:

Kết nối cảm biến (ví dụ: nhiệt độ LM75).

Giao tiếp với màn hình OLED, EEPROM.

Đọc/ghi dữ liệu từ IC ngoại vi.
```

#### **Giải thích**:
- Thuộc tính `reg` mô tả vùng I/O space cho thiết bị I2C:
    - `0x021a0000`: Địa chỉ bắt đầu của vùng I/O.
    - `0x4000`: Kích thước của vùng I/O (theo byte, ở dạng thập lục phân).
- Lưu ý: Thuộc tính `status = "disabled"` cho biết thiết bị I2C này hiện không được kích hoạt trong hệ thống. Tuy nhiên, điều này không ảnh hưởng đến kích thước vùng I/O được định nghĩa.

#### **Tính toán kích thước**:
- Giá trị `0x4000` (hex) = **16 KB** (tính toán: `0x4000` = 16,384 byte = 16 × 1024 byte = 16 KB).
- Vì vậy, kích thước của **I/O space** cho thiết bị `i2c1` là **16 KB**.

---

### **Tóm tắt câu trả lời**
1. **Kích thước của System RAM** (từ node `memory@80000000`): **512 MB**.
2. **Kích thước của I/O space cho thiết bị GPIO (`gpio1`)**: **16 KB**.
3. **Kích thước của I/O space cho thiết bị I2C (`i2c1`)**: **16 KB**.


```shell
sudo find /var/lib/docker/volumes/SO2_DOCKER_VOLUME_ARM/_data -name imx6ul.dtsi 
sudo cp arm_kernel_development/5-simple-driver/imx6ul.dtsi /var/lib/docker/volumes/SO2_DOCKER_VOLUME_ARM/_data/arch/arm/boot/dts/imx6ul.dtsi
/linux: ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make -j8
ls -R /sys/firmware/devicetree/base/
```