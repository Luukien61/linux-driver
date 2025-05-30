## Kết quả:
    root@qemux86:~# insmod /home/root/skels/filesystems/myfs/myfs.ko                
    myfs: loading out-of-tree module taints kernel.                                 
    root@qemux86:~# ls                                                              
    deferred_work_backup  skels                                                     
    root@qemux86:~# mkdir -p /mnt/myfs                                              
    root@qemux86:~# ls                                                              
    deferred_work_backup  skels                                                     
    root@qemux86:~# mount -t myfs none /mnt/myfs                                    
    root inode has 2 link(s)                                                        
    root@qemux86:~# cat /proc/mounts                                                
    //10.0.2.1/rootfs / cifs rw,relatime,vers=1.0,cache=strict,username=dummy,uid=00
    proc /proc proc rw,relatime 0 0                                                 
    sysfs /sys sysfs rw,relatime 0 0                                                
    debugfs /sys/kernel/debug debugfs rw,relatime 0 0                               
    configfs /sys/kernel/config configfs rw,relatime 0 0                            
    devtmpfs /dev devtmpfs rw,relatime,size=247876k,nr_inodes=61969,mode=755 0 0    
    tmpfs /run tmpfs rw,nosuid,nodev,mode=755 0 0                                   
    tmpfs /var/volatile tmpfs rw,relatime 0 0                                       
    //10.0.2.1/skels /home/root/skels cifs rw,relatime,vers=3.1.1,cache=strict,user0
    devpts /dev/pts devpts rw,relatime,gid=5,mode=620,ptmxmode=000 0 0              
    none /mnt/myfs myfs rw,relatime 0 0                                             
    root@qemux86:~# ls -di /mnt/myfs                                                
    4714 /mnt/myfs                                                               
    root@qemux86:~# stat -f /mnt/myfs                                               
    File: "/mnt/myfs"                                                             
    ID: 0        Namelen: 255     Type: UNKNOWN                                 
    Block size: 4096                                                                
    Blocks: Total: 0          Free: 0          Available: 0                         
    Inodes: Total: 0          Free: 0                                               
    root@qemux86:~# random: crng init done                                          
    ls -la /mnt/myfs                                                                
    drwxr-xr-x    2 root     root             0 May 30 01:02 .                      
    drwxr-xr-x    3 root     root             0 May 29 17:08 ..                     
    root@qemux86:~# touch /mnt/myfs/a.txt                                           
    touch: /mnt/myfs/a.txt: Permission denied                                       
    root@qemux86:~# umount /mnt/myfs                                                
    root@qemux86:~# rmmod myfs  



### **README - Cách làm Bài 4 MyFS**


## Cách làm

### Mô tả
Bài 4 yêu cầu kiểm tra việc gắn (mount) và tháo (unmount) hệ thống tệp `myfs` 
bằng mã nguồn từ bài 3 (`/linux/tools/labs/skels/filesystems/myfs/myfs.c`). Không cần sửa mã, chỉ thực hiện các bước kiểm tra:

- Gắn `myfs` vào `/mnt/myfs` và xác nhận qua `/proc/mounts`.
- Kiểm tra số inode của `/mnt/myfs`, thống kê hệ thống tệp, và nội dung thư mục.
- Thử tạo tệp (sẽ thất bại do chưa triển khai `myfs_create`).
- (Tùy chọn) Chạy script `test-myfs.sh` để tự động hóa kiểm tra.

Mã từ bài 3 (TODO 1, 2, 3) đủ để mount thành công và đáp ứng yêu cầu bài 4.

### Bước 1: Biên dịch module

2. Xóa tệp cũ:
   ```bash
   cd /linux/tools/labs/skels
   make clean
   ```
3. Biên dịch:
   ```bash
   make build
   ```
4. Kiểm tra `myfs.ko`:
   ```bash
   ls -l /linux/tools/labs/skels/filesystems/myfs/*.ko
   ```

### Bước 2: Chuẩn bị VM
1. Cấp quyền cho script kiểm tra (nếu dùng):
   ```bash
   chmod +x /linux/tools/labs/skels/filesystems/myfs/test-myfs.sh
   ```
2. Sao chép tệp vào VM:
   ```bash
   cd /linux/tools/labs
   make copy
   ```
3. Chạy VM:
   ```bash
   make console
   ```

### Bước 3: Kiểm tra mount và unmount
1. Nạp module:
   ```bash
   insmod /home/root/skels/filesystems/myfs/myfs.ko
   ```
2. Tạo điểm gắn:
   ```bash
   mkdir -p /mnt/myfs
   ```
3. Gắn hệ thống tệp:
   ```bash
   mount -t myfs none /mnt/myfs
   ```
    - Kết quả: Thấy `root inode has 2 link(s)` trong `dmesg`.
4. Kiểm tra mount:
   ```bash
   cat /proc/mounts
   ```
    - Tìm: `none /mnt/myfs myfs rw,relatime 0 0`.
5. Kiểm tra số inode:
   ```bash
   ls -di /mnt/myfs
   ```
    - Kết quả: Inode ~4714 (do `get_next_ino` cấp số tăng dần).
6. Kiểm tra thống kê:
   ```bash
   stat -f /mnt/myfs
   ```
    - Hiển thị: Block size 4096, tổng khối/inode là 0 (do dùng `simple_statfs`).
7. Kiểm tra nội dung và thử tạo tệp:
   ```bash
   ls -la /mnt/myfs
   ```
    - Thấy: `.` và `..` với quyền `drwxr-xr-x`.
   ```bash
   touch /mnt/myfs/a.txt
   ```
    - Lỗi: `Permission denied` (do chưa có `myfs_create`).
8. Tháo hệ thống tệp:
   ```bash
   umount /mnt/myfs
   ```
9. Gỡ module:
   ```bash
   rmmod myfs
   ```

### Bước 4: (Tùy chọn) Chạy script kiểm tra
1. Chạy script:
   ```bash
   /home/root/skels/filesystems/myfs/test-myfs.sh
   ```
    - Script tự động kiểm tra mount, inode, thống kê, thử tạo tệp, và tháo module.

### Giải thích kết quả
- **Số inode (~4714)**: Trong `myfs_get_inode`, `inode->i_ino = get_next_ino()` cấp số inode tăng dần. 
- Số 4714 là do kernel đã dùng nhiều inode trước đó, bình thường.
- **Thống kê**: Thông tin tối thiểu (khối/inode = 0) vì `myfs_super_ops` dùng `simple_statfs`.
- **Tạo tệp thất bại**: Đúng vì `myfs_create` (TODO 5) chưa được triển khai.
- **Số liên kết**: `root inode has 2 link(s)` xác nhận `set_nlink(inode, 2)` trong bài 3 hoạt động.


### **Bước tiếp theo**

- **TODO 5 và 6**:
    - **TODO 5**: Triển khai `myfs_mknod`, `myfs_create`, `myfs_mkdir` để hỗ trợ tạo tệp/thư mục (giải quyết lỗi `touch`).
    - **TODO 6**: Điền `myfs_file_operations`, `myfs_aops` cho thao tác tệp.

