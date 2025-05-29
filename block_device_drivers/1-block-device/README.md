
**Bài tập 1: Block Device** trong Linux Kernel Labs. README chỉ sửa các phần `TODO 1` trong `ram-disk.c`

---

# README: Bài tập Driver Thiết bị Khối - Bài 1

## Tổng quan
Bài tập 1 tạo module hạt nhân Linux để đăng ký và hủy đăng ký thiết bị khối với số chính 240, tên `mybdev`.
Sử dụng `register_blkdev` và `unregister_blkdev` trong `ram-disk.c` từ thư mục `skels/block_device_drivers/1-2-3-6-ram-disk/kernel`.
Chỉ sửa `TODO 1`

## Giải thích mã nguồn

### Tệp: `ram-disk.c`
- **Macros**:
    - `MY_BLOCK_MAJOR 240`: Số chính của thiết bị.
    - `MY_BLKDEV_NAME "mybdev"`: Tên thiết bị.

- **Hàm `my_block_init`**:
    - Đăng ký thiết bị khối với `register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME)`.
    - Kiểm tra lỗi (`err < 0`), in thông báo lỗi với `printk(KERN_ERR, ...)` và trả về `-EBUSY` nếu thất bại.
  ```c
  static int __init my_block_init(void)
  {
      int err = 0;
      /* TODO 1: register block device */
      err = register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
      if (err < 0) {
          printk(KERN_ERR "unable to register mybdev block device\n");
          return -EBUSY;
      }
      /* TODO 2: create block device using create_block_device */
      return 0;
  out:
      /* TODO 2: unregister block device in case of an error */
      return err;
  }
  ```

- **Hàm `my_block_exit`**:
    - Hủy đăng ký thiết bị với `unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME)`.
  ```c
  static void __exit my_block_exit(void)
  {
      /* TODO 2: cleanup block device using delete_block_device */
      /* TODO 1: unregister block device */
      unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
  }
  ```

- **Giữ nguyên**: Các `TODO 2`, `TODO 3`, `TODO 6`, và toàn bộ mã khác.

### Chức năng
- **Đăng ký**: Tạo mục `240 mybdev` trong `/proc/devices`.
- **Hủy đăng ký**: Xóa mục `mybdev` khi gỡ module.
- **Xử lý lỗi**: Kiểm tra lỗi đăng ký, trả về `-EBUSY` nếu số chính đã dùng.

## Hướng dẫn biên dịch và kiểm tra

### 1. Biên dịch
- Trong `linux/tools/labs`, chạy:
  ```bash
  make build
  ```
- Tạo `ram-disk.ko` trong `skels/block_device_drivers/1-2-3-6-ram-disk`.

### 2. Kiểm tra trong QEMU
- Khởi động QEMU:
  ```bash
  make console
  ```
- Vào thư mục module:
  ```bash
  cd /home/root/skels/block_device_drivers/1-2-3-6-ram-disk
  ```
- Nạp module:
  ```bash
  insmod ram-disk.ko
  ```
- Kiểm tra đăng ký:
  ```bash
  cat /proc/devices | grep mybdev
  ```
    - **Kết quả**: `240 mybdev`
- Gỡ module:
  ```bash
  rmmod ram-disk
  ```
- Kiểm tra hủy đăng ký:
  ```bash
  cat /proc/devices | grep mybdev
  ```
    - **Kết quả**: Không có đầu ra.

### 3. Kiểm tra số chính 7
- Sửa `MY_BLOCK_MAJOR` thành 7:
  ```c
  #define MY_BLOCK_MAJOR 7
  ```
- Biên dịch lại:
  ```bash
  make build
  ```
- Nạp module:
  ```bash
  insmod ram-disk.ko
  ```
- Kiểm tra `/proc/devices`:
  ```bash
  cat /proc/devices | grep mybdev
  ```
    - **Kết quả**: Không có đầu ra (do lỗi vì số 7 đã được dùng).
- Khôi phục `MY_BLOCK_MAJOR` về 240:
  ```c
  #define MY_BLOCK_MAJOR 240
  ```
- Biên dịch và kiểm tra lại.

### 4. Xử lý sự cố
- Không thấy `mybdev`: Kiểm tra `register_blkdev`, đảm bảo `MY_BLOCK_MAJOR` là 240.
- `insmod` thất bại: Kiểm tra cú pháp, phiên bản hạt nhân (`uname -r`).
- Không gỡ module: Xem `lsmod | grep ram_disk`.



