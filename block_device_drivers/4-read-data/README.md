## Kết quả:
    root@qemux86:~/skels/block_device_drivers/4-5-relay-disk# ./test-relay-disk
    relay_disk: loading out-of-tree module taints kernel.
    First 3 bytes: 61 62 63
    read from /dev/vdb: 61 62 63 

Dưới đây là README chi tiết cho **Bài tập 4: Read data from the disk** trong Linux Kernel Labs (Block Device Drivers). README được viết cẩn thận, bao gồm giải thích mã nguồn, phân tích chương trình kiểm tra `test-relay-disk` với ký hiệu Markdown, và phần bổ sung giải thích chi tiết về code như yêu cầu trước. Tôi cũng tích hợp các bước kiểm tra đĩa `/dev/vdb` và xử lý sự cố dựa trên ngữ cảnh của bạn (đã có `disk1.img` trong `qemu/Makefile`).

---

### **README: Bài tập Driver Thiết Bị Khối - Bài 4**

#### **Tổng quan**
Bài tập 4 đọc dữ liệu trực tiếp từ đĩa vật lý `/dev/vdb` trong kernel space. 
Triển khai `open_disk`, `close_disk`, và `send_test_bio` để mở đĩa, đọc sector đầu tiên (sector 0), 
và in 3 byte đầu tiên của dữ liệu đọc được (dự kiến `61 62 63` tương ứng với "abc" do script `test-relay-disk` ghi). 
Chỉ sửa các phần `TODO 4`. Kiểm tra bằng `./test-relay-disk` trong QEMU, script sẽ ghi "abc" vào đầu `/dev/vdb` và module in dữ liệu đọc được.

#### **Giải thích mã nguồn**

##### **Tệp: `relay-disk.c`**
- **Macros**:
    - `PHYSICAL_DISK_NAME "/dev/vdb"`: Định nghĩa đường dẫn của đĩa vật lý được sử dụng trong bài tập. Đây là đĩa mà module sẽ mở và đọc dữ liệu.
    - `KERNEL_SECTOR_SIZE 512`: Kích thước của mỗi sector trên đĩa, được sử dụng để cấp phát bộ đệm và gửi yêu cầu đọc.
    - `BIO_WRITE_MESSAGE "def"`: Chuỗi dữ liệu dự kiến dùng cho Bài 5 (ghi dữ liệu), không sử dụng trong Bài 4.

- **Hàm `open_disk`**:
    - Mục đích: Mở đĩa `/dev/vdb` ở chế độ đọc/ghi độc quyền để đảm bảo không có tiến trình nào khác can thiệp.
    - Triển khai: Sử dụng `blkdev_get_by_path` để mở thiết bị khối từ đường dẫn (`name`), với các chế độ `FMODE_READ | FMODE_WRITE | FMODE_EXCL`. Tham số `THIS_MODULE` gắn module hiện tại làm chủ sở hữu (`holder`) của thiết bị, giúp kernel quản lý tài nguyên.
  ```c
  return blkdev_get_by_path(name, FMODE_READ | FMODE_WRITE | FMODE_EXCL, THIS_MODULE);
  ```

- **Hàm `close_disk`**:
    - Mục đích: Đóng đĩa đã mở để giải phóng tài nguyên.
    - Triển khai: Sử dụng `blkdev_put` để đóng `bdev`, với cùng chế độ mở (`FMODE_READ | FMODE_WRITE | FMODE_EXCL`). Điều này đảm bảo tài nguyên được giải phóng đúng cách khi module thoát.
  ```c
  blkdev_put(bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
  ```

- **Hàm `send_test_bio`**:
    - Mục đích: Tạo và gửi một `struct bio` để đọc sector đầu tiên từ đĩa, sau đó in 3 byte đầu của dữ liệu đọc được.
    - Triển khai:
        - **Tạo bio**: Sử dụng `bio_alloc(GFP_NOIO, 1)` để cấp phát một `bio` với 1 `bio_vec`. `GFP_NOIO` được dùng để tránh đệ quy I/O trong kernel.
        - **Khởi tạo bio**:
            - Gán `bi_disk` từ `bdev->bd_disk` để liên kết `bio` với đĩa.
            - Đặt `bi_opf = dir` (trong Bài 4, `dir = REQ_OP_READ` để đọc dữ liệu).
            - Đặt `bi_iter.bi_sector = 0` để đọc sector đầu tiên.
            - Thêm `page` vào `bio` bằng `bio_add_page`, với kích thước `KERNEL_SECTOR_SIZE` (512 byte).
        - **Gửi bio**: Gọi `submit_bio_wait` để gửi `bio` và chờ hoàn thành. Hàm này trả về mã lỗi nếu I/O thất bại.
        - **In dữ liệu**: Nếu `dir == REQ_OP_READ`, ánh xạ `page` bằng `kmap_atomic` để truy cập dữ liệu kernel, in 3 byte đầu bằng định dạng `%02x`, rồi giải phóng ánh xạ bằng `kunmap_atomic`.
        - **Dọn dẹp**: Gọi `bio_put` để giải phóng `bio`, và `__free_page` để giải phóng `page`.
  ```c
  bio->bi_disk = bdev->bd_disk;
  bio->bi_opf = dir;
  bio->bi_iter.bi_sector = 0;
  bio_add_page(bio, page, KERNEL_SECTOR_SIZE, 0);
  ret = submit_bio_wait(bio);
  if (dir == REQ_OP_READ) {
      buf = kmap_atomic(page);
      printk(KERN_INFO "First 3 bytes: %02x %02x %02x\n", buf[0], buf[1], buf[2]);
      kunmap_atomic(buf);
  }
  ```

- **Hàm `relay_init`**:
    - Gọi `open_disk` để mở `/dev/vdb` và lưu vào `phys_bdev`.
    - Gọi `send_test_bio` với `REQ_OP_READ` để đọc sector 0.
    - Giữ nguyên logic kiểm tra lỗi (`phys_bdev == NULL`).

- **Hàm `relay_exit`**:
    - Gọi `close_disk` để đóng `phys_bdev`.
    - Bỏ qua `TODO 5` (thuộc Bài 5).

- **Giữ nguyên**:
    - Các phần `TODO 5` (dành cho Bài 5, liên quan đến ghi dữ liệu).
    - Các include và định nghĩa module (`MODULE_AUTHOR`, `MODULE_LICENSE`, v.v.).

##### **Chức năng**
- **Mở và đóng đĩa**: Quản lý truy cập độc quyền vào `/dev/vdb` để đọc dữ liệu an toàn.
- **Đọc sector**: Tạo `bio` để đọc sector 0, in 3 byte đầu tiên của dữ liệu.
- **Kết quả kiểm tra**: Script `test-relay-disk` ghi "abc" vào đầu `/dev/vdb`, module đọc và in `61 62 63` (giá trị hex của "abc").

#### **Phân tích chương trình kiểm tra**

##### **Tệp: `test-relay-disk`**

###### **Chức năng chính**
- **Nạp module**:
    - Chạy `insmod ../kernel/relay-disk.ko` để nạp module kernel.
- **Tạo node thiết bị**:
    - Chạy `mknod /dev/myblock b 240 0` nếu node chưa tồn tại (dùng cho các bài trước, không cần thiết cho Bài 4).
- **Ghi dữ liệu**:
    - Mở `/dev/vdb` và ghi chuỗi "abc" (3 byte) vào sector đầu tiên bằng `write`.
    - Đồng bộ bằng `fsync` để đảm bảo dữ liệu được ghi.
- **Kiểm tra**:
    - Module tự động đọc sector 0 khi được nạp (trong `relay_init`) và in 3 byte đầu.
- **Gỡ module**:
    - Chạy `rmmod relay` sau khi kiểm tra.

###### **Các macro quan trọng**
- `PHYSICAL_DISK_NAME "/dev/vdb"`: Đĩa vật lý mà script ghi dữ liệu.
- `KERNEL_SECTOR_SIZE 512`: Kích thước sector, khớp với module.

###### **Điểm đáng chú ý**
- **Ghi dữ liệu**:
    - Script đảm bảo "abc" được ghi vào sector 0 của `/dev/vdb` trước khi module đọc, tạo điều kiện để kiểm tra.
- **Xử lý lỗi**:
    - Kiểm tra lỗi khi mở `/dev/vdb` (`open`) và ghi (`write`), in `errno` nếu thất bại.
    - Thoát với `EXIT_FAILURE` nếu `insmod` không thành công.
- **Tích hợp**:
    - Script giả định `relay-disk.ko` nằm trong `../kernel/` và `/dev/vdb` tồn tại trong hệ thống.

#### **Hướng dẫn biên dịch và kiểm tra**

##### **1. Kiểm tra đĩa**
- Mở `qemu/Makefile`, kiểm tra biến `QEMU_OPTS`. Đảm bảo có ít nhất một đĩa phụ ngoài đĩa gốc:
  ```makefile
  -drive file=disk1.img,if=virtio,format=raw
  ```
- Để khớp với hướng dẫn, thêm một đĩa mới:
  ```makefile
  -drive file=qemu/mydisk.img,if=virtio,format=raw
  ```
    - Ví dụ `QEMU_OPTS` sau khi thêm:
      ```makefile
      QEMU_OPTS = ... -drive file=disk1.img,if=virtio,format=raw -drive file=qemu/mydisk.img,if=virtio,format=raw ...
      ```
- Tạo tệp đĩa nếu chưa có:
  ```bash
  cd /linux/tools/labs
  dd if=/dev/zero of=qemu/mydisk.img bs=1024 count=1
  ```
- Khởi động QEMU và kiểm tra:
  ```bash
  make console
  lsblk
  ```
    - Mong đợi thấy `/dev/vdb` (hoặc `/dev/vdc` nếu có nhiều đĩa). Ví dụ:
      ```
      NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
      vda    254:0    0  512M  0 disk /
      vdb    254:16   0    1M  0 disk
      vdc    254:32   0    1M  0 disk
      ```
    - Đảm bảo `/dev/vdb` tương ứng với `qemu/mydisk.img` bằng cách điều chỉnh thứ tự trong `QEMU_OPTS` nếu cần.

##### **2. Biên dịch**
- Trong thư mục `/linux/tools/labs`, chạy:
  ```bash
  make build
  ```

##### **3. Sao chép file kiểm tra**
- Sao chép các file cần thiết vào QEMU:
  ```bash
  make copy
  ```
- Đảm bảo script `test-relay-disk` có quyền thực thi:
  ```bash
  chmod +x /home/root/skels/block_device_drivers/4-5-relay/user/test-relay-disk
  ```

##### **4. Kiểm tra trong QEMU**
- Khởi động QEMU:
  ```bash
  make console
  ```
- Vào thư mục:
  ```bash
  cd /skels/block_device_drivers/4-5-relay/user
  ```
- Chạy script kiểm tra:
  ```bash
  ./test-relay-disk
  ```
- Xem log kernel:
  ```bash
  cat /proc/kmsg
  ```
    - **Kết quả mong đợi**:
        - Script ghi "abc" vào đầu `/dev/vdb`.
        - Module in:
          ```
          First 3 bytes: 61 62 63
          ```
- Gỡ module:
  ```bash
  rmmod relay
  ```

##### **5. Xử lý sự cố**
- **Không thấy `/dev/vdb`**:
    - Kiểm tra `qemu/Makefile`, đảm bảo `QEMU_OPTS` có tùy chọn đĩa phụ.
    - Xác nhận `qemu/mydisk.img` tồn tại:
      ```bash
      ls -l /linux/tools/labs/qemu/mydisk.img
      ```
    - Chạy `lsblk` trong QEMU để kiểm tra.
- **Lỗi `submit_bio_wait`**:
    - Kiểm tra `bio_alloc` và `bio_add_page` có tham số đúng.
    - Đảm bảo `page` được cấp phát thành công.
- **Dữ liệu in sai**:
    - Xác nhận `test-relay-disk` ghi "abc" vào `/dev/vdb`:
      ```bash
      hexdump -C /dev/vdb | head
      ```
        - Mong đợi: `61 62 63` ở đầu.
    - Kiểm tra `kmap_atomic` và logic in trong `send_test_bio`.
- **Script không chạy**:
    - Đảm bảo `test-relay-disk` có quyền thực thi.
    - Kiểm tra `relay-disk.ko` đã biên dịch và sao chép:
      ```bash
      ls -l /home/root/skels/block_device_drivers/4-5-relay/kernel/relay-disk.ko
      ```

#### **Phần bổ sung**

**Giải thích bổ sung về mã nguồn**:
- Hàm `open_disk` sử dụng `blkdev_get_by_path` để mở `/dev/vdb` với `FMODE_EXCL`, 
- đảm bảo truy cập độc quyền và ngăn các tiến trình khác can thiệp trong suốt thời gian module hoạt động. 
- `THIS_MODULE` gắn module hiện tại làm chủ sở hữu, hỗ trợ kernel quản lý tài nguyên thiết bị một cách an toàn và hiệu quả.
- Trong `send_test_bio`, việc sử dụng `GFP_NOIO` khi cấp phát `bio` và `page` giúp tránh các vấn đề đệ quy I/O trong kernel,
- đặc biệt khi xử lý block I/O. Thiết lập `bi_iter.bi_sector = 0` đảm bảo đọc đúng sector đầu tiên, nơi `test-relay-disk` ghi "abc". 
- Hàm `submit_bio_wait` đồng bộ hóa thao tác I/O, chờ cho đến khi đọc hoàn tất trước khi truy cập dữ liệu, giảm nguy cơ lỗi thời gian.
- Ánh xạ `page` bằng `kmap_atomic` cung cấp một cách an toàn để truy cập bộ đệm kernel, 
- đặc biệt trên các kiến trúc cần ánh xạ bộ nhớ cao (high memory). Định dạng `%02x` đảm bảo in giá trị hex chính xác (`61 62 63` cho "abc"). 
- Việc kiểm tra `dir == REQ_OP_READ` trước khi in giúp hàm linh hoạt cho cả đọc và ghi (dùng trong Bài 5).
- Kiểm tra lặp lại bằng `./test-relay-disk` xác nhận tính đúng đắn của module, dù có thể gặp lỗi flush không ảnh hưởng đến kết quả chính.

