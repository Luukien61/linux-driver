
### **README: Tổng kết 6 Bài tập Driver Thiết Bị Khối**

#### **Thông tin tổng quan**
- **Mục tiêu**: Tổng kết 6 bài tập trong Linux Kernel Labs (Block Device Drivers), 
- bao gồm thiết kế và triển khai driver thiết bị khối trong kernel Linux, từ cơ bản (Bài 1-3) đến nâng cao (Bài 4-6),
- với trọng tâm là làm việc với đĩa `/dev/vdb` trong QEMU.
- **Môi trường**: Sử dụng QEMU với đĩa ảo (`disk1.img` ánh xạ thành `/dev/vdb`), biên dịch bằng `make`, 
- và kiểm tra với các script như `test-relay-disk`.

#### **Tổng quan các bài tập**

##### **Bài 1: Tạo một driver thiết bị khối cơ bản**
- **Mục tiêu**: Tạo một driver cơ bản cho thiết bị khối, đăng ký với kernel và cung cấp giao diện cơ bản.
- **Chức năng chính**:
    - Đăng ký một thiết bị khối bằng `register_blkdev`.
    - Tạo node thiết bị (ví dụ: `/dev/myblock`) bằng `mknod`.
- **File chính**: `block-dev.c`.
- **Kiểm tra**: Sử dụng script `test-block-dev` để nạp module và kiểm tra node thiết bị.

##### **Bài 2: Thêm yêu cầu đọc cơ bản**
- **Mục tiêu**: Mở rộng driver để hỗ trợ đọc dữ liệu từ bộ đệm RAM ảo (RAM disk).
- **Chức năng chính**:
    - Triển khai hàm `make_request` để xử lý yêu cầu đọc từ sector.
    - Sử dụng `submit_bio` để gửi yêu cầu đọc.
- **File chính**: `block-dev.c`.
- **Kiểm tra**: Script `test-block-dev` đọc dữ liệu từ `/dev/myblock`.

##### **Bài 3: Thêm yêu cầu ghi cơ bản**
- **Mục tiêu**: Thêm hỗ trợ ghi dữ liệu vào RAM disk.
- **Chức năng chính**:
    - Mở rộng `make_request` để xử lý cả đọc và ghi.
    - Lưu dữ liệu vào bộ đệm RAM khi nhận yêu cầu ghi.
- **File chính**: `block-dev.c`.
- **Kiểm tra**: Script `test-block-dev` ghi và đọc dữ liệu để xác nhận.

##### **Bài 4: Đọc dữ liệu từ đĩa**
- **Mục tiêu**: Đọc dữ liệu trực tiếp từ đĩa `/dev/vdb` bằng `struct bio`.
- **Chức năng chính**:
    - Triển khai `open_disk` để mở `/dev/vdb` với `blkdev_get_by_path`.
    - Triển khai `send_test_bio` để đọc sector 0, in 3 byte đầu.
- **File chính**: `relay-disk.c`.
- **Kiểm tra**: Sử dụng `./skels/block_device_drivers/4-5-relay/test-relay-disk`.

##### **Bài 5: Ghi dữ liệu vào đĩa**
- **Mục tiêu**: Ghi dữ liệu (`BIO_WRITE_MESSAGE` "def") vào `/dev/vdb` bằng `struct bio`.
- **Chức năng chính**:
    - Mở rộng `send_test_bio` để ghi "def" khi gỡ module.
    - Gọi `REQ_OP_READ` trong `relay_init`, `REQ_OP_WRITE` trong `relay_exit`.
- **File chính**: `relay-disk.c`.
- **Kiểm tra**: Sử dụng `./skels/block_device_drivers/4-5-relay/test-relay-disk`.

##### **Bài 6: Tích hợp và tối ưu hóa**
- **Mục tiêu**: Tích hợp các chức năng trước, tối ưu hiệu suất hoặc thêm tính năng nâng cao (ví dụ: quản lý queue, đồng bộ I/O).
- **Chức năng chính**:
    - Sử dụng `request_queue` để quản lý yêu cầu.
    - Thêm cơ chế đồng bộ (ví dụ: `blk_queue_flush`).
- **File chính**: `block-dev.c` hoặc `relay-disk.c` (tùy triển khai).
- **Kiểm tra**: Sử dụng script `test-block-dev` hoặc `test-relay-disk` với dữ liệu lớn.

#### **Giải thích mã nguồn tổng quát**

##### **Tệp: `block-dev.c` (Bài 1-3, 6)**
- **Macros**:
    - `NR_SECTORS 128`: Số sector của RAM disk.
    - `SECTOR_SIZE 512`: Kích thước mỗi sector.
    - `DEVICE_NAME "/dev/myblock"`: Node thiết bị.
- **Hàm quan trọng**:
    - `block_dev_init`: Đăng ký thiết bị, khởi tạo RAM disk.
    - `make_request`: Xử lý đọc/ghi từ/to RAM disk.
    - `block_dev_exit`: Gỡ đăng ký thiết bị.
- **Chức năng**: Quản lý RAM disk cơ bản, mở rộng qua các bài.

##### **Tệp: `relay-disk.c` (Bài 4-5)**
- **Macros**:
    - `PHYSICAL_DISK_NAME "/dev/vdb"`: Đĩa vật lý.
    - `KERNEL_SECTOR_SIZE 512`: Kích thước sector.
    - `BIO_WRITE_MESSAGE "def"`: Dữ liệu ghi (Bài 5).
- **Hàm quan trọng**:
    - `open_disk`: Mở `/dev/vdb` bằng `blkdev_get_by_path`.
    - `close_disk`: Đóng đĩa bằng `blkdev_put`.
    - `send_test_bio`:
        - Đọc sector 0, in 3 byte đầu (Bài 4).
        - Ghi "def" khi gỡ (Bài 5).
- **Chức năng**: Đọc/ghi trực tiếp trên đĩa vật lý.

#### **Phân tích chương trình kiểm tra**

##### **Tệp: `test-block-dev` (Bài 1-3, 6)**
###### **Chức năng chính**
- **Nạp module**: `insmod ../kernel/block-dev.ko`.
- **Tạo node**: `mknod /dev/myblock b 240 0`.
- **Kiểm tra đọc/ghi**: Mở `/dev/myblock`, ghi/đọc dữ liệu, so sánh bằng `memcmp`.
- **Gỡ module**: `rmmod block-dev`.

###### **Điểm đáng chú ý**
- Đảm bảo đồng bộ bằng `fsync`.
- Kiểm tra lỗi `open`, `write`, in `errno`.

##### **Tệp: `test-relay-disk` (Bài 4-5)**
###### **Chức năng chính**
- **Nạp module**: `insmod ../kernel/relay-disk.ko`.
- **Ghi dữ liệu**: Ghi "abc" vào sector 0 của `/dev/vdb`.
- **Kiểm tra**:
    - Bài 4: Module đọc, in `61 62 63`.
    - Bài 5: Module ghi "def" khi gỡ, script đọc lại, in `64 65 66`.
- **Gỡ module**: `rmmod relay`.

###### **Các macro quan trọng**
- `PHYSICAL_DISK_NAME "/dev/vdb"`: Đĩa vật lý.
- `KERNEL_SECTOR_SIZE 512`: Kích thước sector.

###### **Điểm đáng chú ý**
- Ghi "abc" ban đầu, ghi đè "def" ở Bài 5.
- Xử lý lỗi `open`, `write`, in `errno`.

#### **Hướng dẫn biên dịch và kiểm tra**

##### **1. Kiểm tra đĩa**
- Mở `qemu/Makefile`, xác nhận `QEMU_OPTS` có:
  ```makefile
  -drive file=disk1.img,if=virtio,format=raw
  ```
- Đĩa `disk1.img` ánh xạ thành `/dev/vdb`, dùng cho Bài 4-5.
- Kiểm tra trong QEMU:
  ```bash
  make console
  lsblk
  ```
    - Mong đợi:
      ```
      NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
      vda    254:0    0  512M  0 disk /
      vdb    254:16   0    1M  0 disk
      ```

##### **2. Sửa mã nguồn**
- Chỉnh sửa file:
    - Bài 1-3, 6: `vi /linux/tools/labs/skels/block_device_drivers/block-dev.c`.
    - Bài 4-5: `vi /linux/tools/labs/skels/block_device_drivers/4-5-relay-disk/relay-disk.c`.

##### **3. Biên dịch**
```bash
cd /linux/tools/labs
make build
```

##### **4. Sao chép**
```bash
make copy
```
- Đảm bảo quyền thực thi:
    - Bài 1-3, 6: `chmod +x /home/root/skels/block_device_drivers/test-block-dev`.
    - Bài 4-5: `chmod +x /home/root/skels/block_device_drivers/4-5-relay/test-relay-disk`.

##### **5. Kiểm tra trong QEMU**
- Khởi động QEMU:
  ```bash
  make console
  ```
- Vào thư mục:
    - Bài 1-3, 6: `cd /home/root/skels/block_device_drivers`.
    - Bài 4-5: `cd /home/root/skels/block_device_drivers/4-5-relay-disk`.
- Chạy kiểm tra:
    - Bài 1-3, 6: `./test-block-dev`.
    - Bài 4-5: `./test-relay-disk`.
- **Kết quả Bài 4-5**:
  ```
  relay_disk: loading out-of-tree module taints kernel.
  First 3 bytes: 61 62 63
  read from /dev/vdb: 64 65 66
  ```
- Gỡ module:
  ```bash
  rmmod block-dev
  ```
  hoặc
  ```bash
  rmmod relay
  ```

##### **6. Xử lý sự cố**
- **Không thấy `/dev/vdb`**:
    - Kiểm tra `qemu/Makefile` có `disk1.img`.
    - Chạy `lsblk`.
- **Dữ liệu sai**:
    - Kiểm tra `/dev/vdb`:
      ```bash
      hexdump -C /dev/vdb | head
      ```
    - Xác nhận `send_test_bio` hoặc `make_request`.
- **Script lỗi**:
    - Đảm bảo quyền thực thi và file `.ko` tồn tại.

#### **Phần bổ sung**

**Giải thích bổ sung về mã nguồn**:
- **Bài 1-3**: 
- `block-dev.c` khởi tạo RAM disk với `register_blkdev`, sử dụng `make_request` để xử lý đọc/ghi. 
- `GFP_NOIO` trong cấp phát bộ đệm tránh đệ quy I/O, đảm bảo hiệu suất khi làm việc với RAM disk.
- Đồng bộ bằng `fsync` trong script giúp dữ liệu nhất quán.
- **Bài 4-5**:
- `relay-disk.c` chuyển sang làm việc với đĩa vật lý `/dev/vdb`. `blkdev_get_by_path` với `FMODE_EXCL` đảm bảo truy cập độc quyền,
- `THIS_MODULE` quản lý tài nguyên. `send_test_bio` sử dụng `bio_alloc` và `submit_bio_wait` để đọc/ghi sector 0.
- `kmap_atomic` và `kunmap_atomic` đảm bảo an toàn khi truy cập bộ đệm, với `memcpy` ghi "def" trong Bài 5.
- Kết quả `61 62 63` (đọc "abc") và `64 65 66` (đọc "def") phản ánh luồng đọc-ghi thành công.
- **Bài 6**: 
- Tích hợp `request_queue` và `blk_queue_flush` tối ưu hóa hiệu suất, quản lý queue I/O để xử lý nhiều yêu cầu đồng thời, nâng cao độ tin cậy.

**Lưu ý quan trọng**:
- Đĩa `disk1.img` (ánh xạ thành `/dev/vdb`) đủ cho Bài 4-5, không cần thêm `qemu/mydisk.img` nếu đã hoạt động đúng.
- Kiểm tra lặp lại với `./test-relay-disk` xác nhận tính nhất quán của dữ liệu trên đĩa.

