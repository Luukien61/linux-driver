```shell
sudo cp device_drivers/register_unregister/so2_cdev.c  /var/lib/docker/volumes/SO2_DOCKER_VOLUME/_data/tools/labs/skels/device_drivers/kernel/so2_cdev.c
```

```text
MY_MAJOR là số major mà bạn chọn để đăng ký cho driver của mình. 
MY_MINOR là số minor đầu tiên mà bạn muốn đăng ký cho driver. Là chỉ số để phân biệt giữa các thiết bị cùng thuộc một driver.
```

**Create /dev/so2_cdev character device node using mknod.**
```shell
mknod /dev/so2_cdev c 42 0
```
Trong trường hợp này: 
 - `<tên_thiết_bị>` là so2_cdev.
 - `c` chỉ định đây là character device.
 - `<major>` là 42 (được định nghĩa bởi MY_MAJOR trong code).
 - `<minor>` là 0 (được định nghĩa bởi MY_MINOR)

**Load the module into the kernel**
```shell
insmod so2_cdev.ko
```

**And see character devices in /proc/devices:**
```shell
cat /proc/devices | less
```

**Unload the kernel module**
```shell
rmmod so2_cdev
rm /dev/so2_cdev 
```


### Code

#### TODO 2

**Hàm `so2_cdev_init`**
- register_chrdev_region() chỉ đăng ký vùng major/minor number — không làm gì hơn.
- cdev_init(...) : Gán các phương thức (file operations) như open, read, write, ioctl vào struct cdev.
- cdev_add(...) : Thêm struct cdev đã được khởi tạo vào hệ thống kernel, đồng thời gán nó với major/minor number cụ thể .
- Số 1 trong hàm cdev_add(..., ..., 1); cho biết có bao nhiêu minor number liên tiếp được cấp phát cho thiết bị này.


**Hàm `atomic_cmpxchg(&data->access, 0, 1)`**
- Nếu data->access hiện đang là 0, thì đặt nó thành 1 và trả về giá trị cũ.”
  Nếu giá trị trả về là 0, tức là thành công → ta vừa "chiếm quyền" mở thiết bị.
  Nếu giá trị trả về là khác 0, tức là thất bại → thiết bị đã được mở trước đó.


`container_of(inode->i_cdev, struct so2_device_data, cdev)`
- inode->i_cdev: Là con trỏ đến struct cdev, được thêm vào kernel bằng hàm cdev_add().
- struct so2_device_data: Là cấu trúc chứa toàn bộ thông tin về thiết bị.
- cdev: Là tên của trường trong struct so2_device_data mà bạn đã khai báo.
- Từ con trỏ cdev, hàm container_of(...) sẽ tìm lại địa chỉ của toàn bộ struct so2_device_data chứa nó.
Tức là inode->i_cdev là cấu trúc tương đương 1 field cdev trong struct so2_device_data mà bạn muốn truy ngược.
- container_of(ptr, type, member)
  Nghĩa là: cho con trỏ ptr trỏ tới member bên trong kiểu type, hãy tính toán ra con trỏ đến struct type.


### TODO 4

Hàm `so2_cdev_read()` là **hàm xử lý đọc dữ liệu từ thiết bị** khi người dùng gọi `read()` từ user space (`cat /dev/so2_cdev`).


```c
struct so2_device_data *data =
    (struct so2_device_data *) file->private_data;
```

* `file->private_data` chứa con trỏ đến cấu trúc thiết bị `so2_device_data`, được gán trước đó trong `open()`. Trong `open()` thì `data` là 1 `so2_device_data`
* `data`  chứa thông tin như `buffer`, `access`,

---

```c
to_read = min(size, (size_t)(BUFSIZ - *offset));
```

* Xác định số byte thực sự sẽ đọc:

    * `size` là yêu cầu từ user.
    * `BUFSIZ - *offset` là số byte còn lại từ vị trí offset.
    * `min()` để không đọc vượt quá buffer.

---

```c
if (to_read <= 0)
    return 0;
```

* Nếu không còn gì để đọc (offset vượt giới hạn), trả về `0` để báo là đã đọc hết (EOF).

---
### TODO 5

```shell
echo "arpeggio"> /dev/so2_cdev
cat /dev/so2_cdev
```

```c
if (copy_to_user(user_buffer, data->buffer + *offset, to_read)) {
    pr_info("%s: copy_to_user failed\n", MODULE_NAME);
    return -EFAULT;
}
```


* Dùng `copy_to_user()` để sao chép dữ liệu từ kernel space sang user space.
* `copy_to_user()`: Hàm kernel để sao chép dữ liệu từ không gian kernel sang không gian user .
  - `user_buffer`: Địa chỉ nơi người dùng muốn nhận dữ liệu.
  - `data->buffer + *offset`: Bắt đầu đọc từ vị trí offset trong buffer nội bộ.
  - `to_read`: Số byte cần sao chép.
* Nếu sao chép thất bại → trả về lỗi `-EFAULT` (bad address).

---

```c
*offset += to_read;
return to_read;
```

* Cập nhật `offset` để biết lần sau sẽ đọc từ đâu.
* Trả về số byte đã đọc thành công.

### TODO 6

---


```c
static long so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
```

- Hàm này được gọi khi người dùng giao tiếp với thiết bị đặc biệt bằng lệnh **`ioctl()`** từ không gian user-space.
- Trong ví dụ này, nếu người dùng gửi lệnh `MY_IOCTL_PRINT`, module sẽ in ra một thông báo xác định (được lưu trong `IOCTL_MESSAGE`).


### 1. Lấy dữ liệu thiết bị

```c
struct so2_device_data *data =
    (struct so2_device_data *) file->private_data;
```

- `file->private_data` là con trỏ đến cấu trúc `so2_device_data`, đã được gán trong hàm `open()`.
- Đây là nơi lưu trữ các biến như `buffer`, `access`, v.v., liên quan đến thiết bị hiện tại.

---

### 2. Biến trả về

```c
int ret = 0;
```

- `ret` là giá trị trả về cho người dùng. Thường là:
    - `0`: Thành công
    - `-EINVAL`: Lệnh không hợp lệ

---

### 3. Xử lý lệnh `ioctl`

```c
switch (cmd) {
case MY_IOCTL_PRINT:
    pr_info("%s: %s", MODULE_NAME, IOCTL_MESSAGE);
    break;
default:
    ret = -EINVAL;
}
```

#### ✅ `MY_IOCTL_PRINT` là gì?

- Là một **lệnh tự định nghĩa**, thường được khai báo như sau:

```c
#define MY_IOCTL_PRINT _IO('k', 0) // 'k' là ký hiệu riêng, 0 là mã lệnh
```

- Khi người dùng gọi `ioctl(fd, MY_IOCTL_PRINT, ...)` từ không gian user-space, hệ thống sẽ chuyển tới đây và thực thi khối `case`.

#### ✅ `pr_info(...)` là gì?

- Là hàm in log trong kernel, tương đương với `printk(KERN_INFO ...)`.
- Dùng để in thông tin lên `/var/log/kern.log` hoặc xem qua `dmesg`.

👉 Trong ví dụ này, nó sẽ in ra nội dung của `IOCTL_MESSAGE`, ví dụ: `"Hello ioctl\n"`.

---

### 4. Trả về kết quả

```c
return ret;
```

- Nếu lệnh hợp lệ → `ret = 0`
- Nếu không nhận được lệnh nào phù hợp → `ret = -EINVAL` (Invalid argument)


## Flow tổng quát của hàm

```text
+-----------------------------+
|     ioctl() được gọi        |
+-----------------------------+
           ↓
      Nhận cmd và arg
           ↓
       switch (cmd)
           ↓
          OK? → In message
           ↓
         return 0
           ↓
       Không hợp lệ → return -EINVAL
```

**MY_IOCTL_SET_BUFFER**

```c
ioctl(fd, MY_IOCTL_SET_BUFFER, buffer);
```
- Tham số thứ ba `(buffer)` được truyền vào kernel dưới dạng một giá trị unsigned long, được lưu trong tham số `arg` của hàm `so2_cdev_ioctl`
- arg chứa giá trị số của địa chỉ con trỏ từ userspace (ví dụ: 0x7fff12345678).
- Để sử dụng giá trị này như một con trỏ trong kernel, chúng ta phải ép kiểu unsigned long thành một con trỏ kiểu char *. Do đó, user_buffer = (char *)arg chuyển đổi giá trị số trong arg thành một con trỏ char * trỏ đến địa chỉ trong userspace.
- user_buffer bây giờ là một con trỏ userspace, không phải kernel space. Nó trỏ đến bộ nhớ trong chương trình userspace (vùng nhớ của buffer trong chương trình C).

- `wait_event_interruptible`: Đây là một macro trong kernel Linux dùng để đưa process hiện tại vào trạng thái chờ (sleep) trên một hàng đợi chờ `(wait_queue_head_t queue)` cho đến khi một điều kiện được thỏa mãn.
- Tham số:
   - `data->queue`: Là một wait_queue_head_t được định nghĩa trong `struct so2_device_data`, dùng để quản lý các process đang chờ.
   - `!data->is_blocked`: Là điều kiện để thoát khỏi trạng thái chờ. Process sẽ tiếp tục chạy khi data->is_blocked trở thành 0 (tức là !data->is_blocked là true).
- Biểu thức (void __user *)arg trong lập trình Linux kernel – đặc biệt khi xử lý ioctl – là một ép kiểu (cast). 
Nó chuyển đổi đối số arg (thường là một unsigned long) sang một con trỏ đến vùng nhớ không gian người dùng (user space). 
- __user: là một macro dùng để đánh dấu rằng con trỏ này trỏ tới vùng nhớ thuộc user space, chứ không phải kernel space.

**What are the flags used to open the file when running cat /dev/so2_dev**

```bash
cat /dev/so2_dev
```

lệnh `cat` sẽ mở file thiết bị `/dev/so2_dev` bằng lời gọi hệ thống `open()` với **cờ** (flags):

```c
O_RDONLY
```

### ✅ Ý nghĩa:

| Cờ (`flag`)               | Mô tả                                |
| ------------------------- | ------------------------------------ |
| `O_RDONLY`                | Mở file chỉ để đọc (read-only).      |
| **Không có** `O_NONBLOCK` | Tức là ở chế độ **chặn** (blocking). |

---

### 🧠 Điều đó có nghĩa là gì?

* Nếu trong driver, bạn kiểm tra thấy `data_size == 0` và **không có** `O_NONBLOCK`, thì hàm `read()` sẽ **chờ** (`wait_event_interruptible(...)`) đến khi có dữ liệu.
* Nếu bạn mở với `O_NONBLOCK`, thì sẽ không chờ mà trả về lỗi `-EAGAIN`.


- `file->f_flags`: là cờ (flags) được gán khi user-space gọi open() trên file thiết bị. Nó chứa các cờ như O_RDONLY, O_WRONLY, O_NONBLOCK, v.v.
- `& O_NONBLOCK`: là phép toán AND bit, dùng để kiểm tra xem cờ O_NONBLOCK có được thiết lập trong f_flags hay không.