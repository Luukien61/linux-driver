```shell
sudo rmmod mailbox_driver
make clean
make
sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 ~/Downloads/VMWARE17.priv ~/Downloads/VMWARE17.der ./mailbox_driver.ko
sudo insmod mailbox_driver.ko
sudo chmod 666 /dev/mailbox0
sudo chmod 666 /dev/mailbox1

ls -l /dev | grep mail
cat /proc/devices | grep mail
```
---

### Giải thích
1. **Kết quả từ `ls -l /dev | grep mail`**:
    - Hai thiết bị `/dev/mailbox0` và `/dev/mailbox1` là **character devices** (do ký tự `c` ở đầu dòng).
    - Cột `511, 0` và `511, 1` lần lượt là **major number** và **minor number** của các thiết bị:
        - **Major number**: `511` là số nhận diện driver (ở đây là driver `mailbox` từ module `mailbox_driver.c`).
        - **Minor number**: `0` cho `mailbox0` và `1` cho `mailbox1`, dùng để phân biệt các thiết bị khác nhau trong cùng một driver.
    - Quyền `crw-rw-rw-` cho thấy mọi người dùng đều có quyền đọc/ghi vào các thiết bị này.

2. **Kết quả từ `cat /proc/devices | grep mail`**:
    - Dòng `511 mailbox` xác nhận rằng major number `511` được gán cho driver `mailbox`. Điều này khớp với major number trong `/dev/mailbox0` và `/dev/mailbox1`.

### Kết nối với module `mailbox_driver.c`
Module `mailbox_driver.c`:
- Tạo hai character devices (`/dev/mailbox0` và `/dev/mailbox1`) với cùng major number và minor number lần lượt là `0` và `1`.
- Sử dụng `alloc_chrdev_region` để cấp phát major number động (trong trường hợp này là `511`).
- Tạo device files thông qua `device_create`, khớp với kết quả bạn thấy trong `/dev`.

