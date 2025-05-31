# Memory Mapping Lab - kmmap.c

## Mô tả

Module kmmap.c là một phần của bài lab về memory mapping trong Linux kernel, thuộc Exercise #1. Module này tập trung vào việc sử dụng bộ nhớ được cấp phát bằng kmalloc để ánh xạ sang không gian địa chỉ của user space. Các chức năng chính bao gồm:
1. **Memory Mapping**:
- Cấp phát vùng bộ nhớ kernel liên tục vật lý với kích thước `NPAGES + 2` trang (mặc định `NPAGES = 16`, mỗi trang 4KB).
- Căn chỉnh vùng bộ nhớ đến ranh giới trang và đánh dấu các trang là "reserved" để ngăn swap.
- Khởi tạo 4 byte đầu tiên của mỗi trang với `0xaa, 0xbb, 0xcc, 0xdd` để khớp với kiểm tra trong `mmap-test.c`.
- Triển khai hàm `mmap` để ánh xạ vùng bộ nhớ kernel sang user space bằng `remap_pfn_range`.

2. **Read/Write Operations**:
    - Cung cấp các hàm `read` và `write` để sao chép dữ liệu giữa vùng bộ nhớ kernel và user space.
    - Hàm `read` đặt lại offset về 0 để đảm bảo đọc từ đầu vùng bộ nhớ, khắc phục lỗi trả về `0x0` trong kiểm tra.
    - Đảm bảo kích thước đọc/ghi không vượt quá vùng bộ nhớ ánh xạ (`NPAGES * PAGE_SIZE`).

3. **Procfs Integration**:
    - Tạo mục `/proc/my-proc-entry` để hiển thị tổng số vùng ánh xạ bộ nhớ (VMAs) của tiến trình hiện tại.
    - Duyệt qua danh sách VMA của tiến trình và ghi số lượng dưới dạng số nguyên (`%lu`) để tương thích với `mmap-test.c`.

## Yêu cầu

- **Môi trường**: Hệ thống Linux với kernel >= 5.8, chạy trong QEMU hoặc máy thật (ví dụ: Ubuntu 22.04).
- **Công cụ**: `gcc`, `make`, kernel headers tương ứng.
- **Tệp thiết bị**: Tạo tệp thiết bị `/dev/mymmap` với major number `42`.
- **Chương trình kiểm tra**: Sử dụng `mmap-test.c` để:
    - Mở `/dev/mymmap` và gọi `mmap` để ánh xạ bộ nhớ.
    - Kiểm tra đọc/ghi dữ liệu (`0xa0, 0xb0, 0xc0, 0xd0`) vào vùng bộ nhớ ánh xạ.
    - Đọc `/proc/my-proc-entry` để kiểm tra số lượng VMA.

## Cách sử dụng

### 1. Biên dịch module và chương trình kiểm tra
- Đảm bảo thư mục lab chứa các file `kmmap.c`, `mmap-test.c`, và `mmap-test.h`.
- Biên dịch module kernel:
  ```bash
  make build
  ```
- Vào máy ảo QEMU:
  ```bash
  make console
  ```
- Di chuyen đến thư mục chứa mã nguồn để nạp module:
  ```bash
  cd ~/skels/memory_mapping/kmmap
  ```

### 2. Nạp module
- Nạp module vào kernel:
  ```bash
  sudo insmod kmmap.ko
  ```

### 4. Kiểm tra ánh xạ bộ nhớ
Chạy chương trình `mmap-test` với các tham số để kiểm tra các tính năng:
  ```bash
  ./mmap-test 1
  ```
- Kiểm tra 4 byte đầu tiên của mỗi trang (`0xaa, 0xbb, 0xcc, 0xdd`).
    - **Kết quả mong đợi**:
      ```
      matched
      ...
      (16 dòng "matched")
      ```

- **Kiểm tra đọc/ghi** (test = 2):
  ```bash
  ./mmap-test 2
  ```
    - Ghi dữ liệu mẫu (`0xa0, 0xb0, 0xc0, 0xd0`) vào vùng bộ nhớ ánh xạ và kiểm tra nội dung qua `write` và `read`.
    - **Kết quả mong đợi**:
      ```
      matched
      ...
      Write test ...
      matched
      ...
      Read test ...
      matched
      ...
      (16 dòng "matched" cho tất cả)
      ```

- **Kiểm tra số lượng VMA** (test = 3):
  ```bash
  ./mmap-test 3
  ```
    - Hiển thị số VMA trước và sau khi ánh xạ thêm 10MB bộ nhớ ẩn danh.
    - Giữ tiến trình chạy trong 30 giây để kiểm tra.
    - **Kết quả mong đợi**:
      ```
      (Như test = 2, cộng thêm)
      Memory usage: <số, ví dụ 11>
      Memory usage: <số + 1, ví dụ 12>
      mmaped :0 MB
      ```

- Xem số VMA:
  ```bash
  cat /proc/my-proc-entry
  ```

### 4. Gỡ module
- Gỡ module sau khi kiểm tra:
  ```bash
  rmmod kmmap
  ```
- Xóa tệp thiết bị:
  ```bash
  rm /dev/mymmap
  ```

## Lưu ý
- Đảm bảo kernel headers khớp với phiên bản kernel đang chạy.
- Module sử dụng `kmalloc`, phù hợp cho cả hệ thống 32-bit và 64-bit, không yêu cầu xử lý đặc biệt cho HIGHMEM.

## Tệp liên quan
- `kmmap.c`: Module kernel triển khai ánh xạ bộ nhớ bằng `kmalloc` và giao diện `/proc/my-proc-entry`.
- `mmap-test.c`: Chương trình user space để kiểm tra ánh xạ, đọc/ghi, và thông tin VMA.
- `mmap-test.h`: Header chứa các định nghĩa cần thiết cho `mmap-test.c`.