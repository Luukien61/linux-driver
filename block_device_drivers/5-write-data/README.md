## Lệnh sửa code: 
 - vi /linux/tools/labs/skels/block_device_drivers/4-5-relay-disk/relay-disk.c
## Thực thi test-relay-disk:
 -  ./skels/block_device_drivers/4-5-relay/test-relay-disk
## Kết quả:
    root@qemux86:~/skels/block_device_drivers/4-5-relay-disk# ./test-relay-disk
    relay_disk: loading out-of-tree module taints kernel.
    First 3 bytes: 61 62 63
    read from /dev/vdb: 64 65 66

---

### **README: Bài tập Driver Thiết Bị Khối - Bài 5**

#### **Tổng quan**
Bài tập 5 mở rộng từ Bài 4, ghi dữ liệu vào đĩa `/dev/vdb` bằng `struct bio`. 
Triển khai `TODO 5` để ghi `BIO_WRITE_MESSAGE` ("def") vào sector 0 khi gỡ module, và đọc dữ liệu khi nạp module. 
Sử dụng `REQ_OP_READ` trong `relay_init` và `REQ_OP_WRITE` trong `relay_exit`. 
Kiểm tra bằng `./skels/block_device_drivers/4-5-relay/test-relay-disk`, script sẽ đọc dữ liệu và in:
```
read from /dev/vdb: 64 65 66
```

#### **Giải thích mã nguồn**

##### **Tệp: `relay-disk.c`**
- **Macros**:
    - `PHYSICAL_DISK_NAME "/dev/vdb"`: Đường dẫn của đĩa vật lý, ánh xạ từ `disk1.img` trong `qemu/Makefile`.
    - `KERNEL_SECTOR_SIZE 512`: Kích thước mỗi sector, dùng để cấp phát bộ đệm và gửi yêu cầu đọc/ghi.
    - `BIO_WRITE_MESSAGE "def"`: Chuỗi dữ liệu ("def") được ghi vào đĩa.

- **Hàm `send_test_bio`**:
    - Đã triển khai ở Bài 4 cho đọc (`REQ_OP_READ`).
    - Thêm logic ghi (`REQ_OP_WRITE`):
        - Nếu `dir == REQ_OP_WRITE`, ánh xạ `page` bằng `kmap_atomic`.
        - Sao chép `BIO_WRITE_MESSAGE` vào bộ đệm bằng `memcpy`.
        - Giải phóng ánh xạ bằng `kunmap_atomic`.
  ```c
  if (dir == REQ_OP_WRITE) {
      buf = kmap_atomic(page);
      memcpy(buf, BIO_WRITE_MESSAGE, strlen(BIO_WRITE_MESSAGE));
      kunmap_atomic(buf);
  }
  ```

- **Hàm `relay_init`**:
    - Gọi `send_test_bio` với `REQ_OP_READ` để đọc dữ liệu khi nạp module.
  ```c
  send_test_bio(phys_bdev, REQ_OP_READ);
  ```

- **Hàm `relay_exit`**:
    - Gọi `send_test_bio` với `REQ_OP_WRITE` để ghi `BIO_WRITE_MESSAGE` khi gỡ module.
  ```c
  send_test_bio(phys_bdev, REQ_OP_WRITE);
  ```

- **Giữ nguyên từ Bài 4**:
    - `open_disk`, `close_disk`.
    - Logic đọc và in trong `send_test_bio`.

##### **Chức năng**
- **Đọc dữ liệu**: Khi nạp module, đọc sector 0, in 3 byte đầu (`61 62 63` từ "abc" do script ghi ban đầu).
- **Ghi dữ liệu**: Khi gỡ module, ghi "def" vào sector 0, đè lên dữ liệu cũ.
- **Kết quả**: Script đọc lại và in `64 65 66` (hex của "def").

#### **Phân tích chương trình kiểm tra**

##### **Tệp: `test-relay-disk`**

###### **Chức năng chính**
- **Nạp module**:
    - Chạy `insmod ../kernel/relay-disk.ko`.
- **Tạo node thiết bị**:
    - Chạy `mknod /dev/myblock b 240 0` (không dùng trong Bài 5).
- **Ghi dữ liệu**:
    - Ghi "abc" vào sector 0 của `/dev/vdb` bằng `write`.
    - Đồng bộ bằng `fsync`.
- **Kiểm tra**:
    - Module đọc và in `61 62 63` khi nạp.
    - Module ghi "def" khi gỡ.
    - Script đọc lại `/dev/vdb` và in `64 65 66`.
- **Gỡ module**:
    - Chạy `rmmod relay`.

###### **Các macro quan trọng**
- `PHYSICAL_DISK_NAME "/dev/vdb"`: Đĩa vật lý.
- `KERNEL_SECTOR_SIZE 512`: Kích thước sector.

###### **Điểm đáng chú ý**
- **Ghi dữ liệu**:
    - Script ghi "abc" ban đầu, module ghi đè "def" khi gỡ.
- **Xử lý lỗi**:
    - Kiểm tra lỗi `open`, `write`, in `errno`.
    - Thoát với `EXIT_FAILURE` nếu `insmod` thất bại.
- **Tích hợp**:
    - Script yêu cầu `relay-disk.ko` ở `../kernel/` và `/dev/vdb`.

#### **Hướng dẫn biên dịch và kiểm tra**

##### **1. Kiểm tra đĩa**
- Mở `qemu/Makefile`, xác nhận `QEMU_OPTS` có:
  ```makefile
  -drive file=disk1.img,if=virtio,format=raw
  ```
- Đĩa `disk1.img` ánh xạ thành `/dev/vdb`, không cần thêm đĩa mới.
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
- Chỉnh sửa file `relay-disk.c`:
  ```bash
  vi /linux/tools/labs/skels/block_device_drivers/4-5-relay-disk/relay-disk.c
  ```
    - Thêm logic `TODO 5` như mô tả trong mã nguồn.

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
  ```bash
  chmod +x /home/root/skels/block_device_drivers/4-5-relay/test-relay-disk
  ```

##### **5. Kiểm tra trong QEMU**
- Khởi động QEMU:
  ```bash
  make console
  ```
- Vào thư mục:
  ```bash
  cd /home/root/skels/block_device_drivers/4-5-relay-disk
  ```
- Chạy script kiểm tra:
  ```bash
  ./test-relay-disk
  ```
- **Kết quả mong đợi**:
    - Script in:
      ```
      relay_disk: loading out-of-tree module taints kernel.
      First 3 bytes: 61 62 63
      read from /dev/vdb: 64 65 66
      ```
    - Gỡ module:
      ```bash
      rmmod relay
      ```

##### **6. Xử lý sự cố**
- **Script không in `64 65 66`**:
    - Kiểm tra `/dev/vdb`:
      ```bash
      hexdump -C /dev/vdb | head
      ```
        - Mong đợi: `64 65 66`.
    - Xác nhận `send_test_bio` ghi `BIO_WRITE_MESSAGE`.
- **Lỗi `submit_bio_wait`**:
    - Kiểm tra `bio_alloc`, `bio_add_page`.
- **Không thấy `/dev/vdb`**:
    - Xác nhận `qemu/Makefile` có `disk1.img`.
    - Chạy `lsblk`.

#### **Phần bổ sung**

**Giải thích bổ sung về mã nguồn**:
- Trong `send_test_bio`, logic ghi được thực hiện khi `dir == REQ_OP_WRITE`, 
- sử dụng `kmap_atomic` và `kunmap_atomic` để ánh xạ và giải phóng bộ đệm `page` một cách an toàn. 
- `memcpy` sao chép `BIO_WRITE_MESSAGE` ("def") vào sector 0, ghi đè dữ liệu "abc" ban đầu, dẫn đến kết quả `64 65 66` khi script đọc lại.
- Điều này chứng tỏ module đã ghi thành công.
- Hàm `relay_init` gọi `send_test_bio` với `REQ_OP_READ`, đọc dữ liệu ban đầu ("abc") và in `61 62 63`, phù hợp với luồng kiểm tra của script.
- `relay_exit` gọi với `REQ_OP_WRITE`, ghi "def" trước khi gỡ module, đảm bảo dữ liệu mới được lưu vào đĩa để script kiểm tra sau đó.
- Việc sử dụng `GFP_NOIO` trong `bio_alloc` và `alloc_page` tiếp tục tránh đệ quy I/O, đảm bảo tính ổn định khi xử lý block I/O.
- Kết quả kiểm tra bằng `./test-relay-disk` phản ánh rõ ràng hai giai đoạn: đọc ban đầu (in `61 62 63`)
- và ghi thành công (in `64 65 66` sau khi gỡ module).

