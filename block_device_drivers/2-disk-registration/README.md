## Kết quả
    root@qemux86:~/skels/block_device_drivers/1-2-3-6-ram-disk/kernel# insmod ram-di
    sk.ko                                                                           
    ram_disk: loading out-of-tree module taints kernel.                             
    root@qemux86:~/skels/block_device_drivers/1-2-3-6-ram-disk/kernel# random: crnge
    
    root@qemux86:~/skels/block_device_drivers/1-2-3-6-ram-disk/kernel# ls /dev/myblo
    ck                                                                              
    /dev/myblock                                                                    
    root@qemux86:~/skels/block_device_drivers/1-2-3-6-ram-disk/kernel# echo "abc" >
    /dev/myblock                                                                    
    request received: sector=0, total_size=4096, bio_size=4096, dir=0               
    request received: sector=0, total_size=4096, bio_size=4096, dir=1   


## Tổng quan
Bài tập 2 mở rộng module từ Bài 1 để thêm đĩa và xử lý yêu cầu cơ bản. 
Sử dụng `create_block_device` và `delete_block_device` để quản lý đĩa, và triển khai `my_block_request` 
để in thông tin yêu cầu (sector, kích thước, hướng). Chỉ sửa các phần `TODO 2` trong `ram-disk.c`, giữ nguyên bình luận và mã khác.

## Giải thích mã nguồn

### Tệp: `ram-disk.c`
- **Macros**:
    - `MY_BLOCK_MAJOR 240`: Số chính.
    - `MY_BLKDEV_NAME "mybdev"`: Tên thiết bị.
    - `MY_BLOCK_MINORS 1`: Số minor.
    - `NR_SECTORS 128`: Dung lượng đĩa.

- **Cấu trúc `my_block_dev`**:
    - Chứa `tag_set`, `queue`, `gd`, `data`, `size` cho thiết bị và đĩa.

- **Hàm `my_block_request`**:
    - Lấy yêu cầu từ `bd->rq`.
    - Bắt đầu với `blk_mq_start_request`.
    - Kiểm tra `blk_rq_is_passthrough`: Nếu đúng, in thông báo, kết thúc với lỗi.
    - In sector (`blk_rq_pos`), kích thước tổng (`blk_rq_bytes`), kích thước `bio` (`blk_rq_cur_bytes`), hướng (`rq_data_dir`).
    - Kết thúc với `blk_mq_end_request`.
  ```c
  static blk_status_t my_block_request(struct blk_mq_hw_ctx *hctx,
				     const struct blk_mq_queue_data *bd)
  {
      struct request *rq;
      struct my_block_dev *dev = hctx->queue->queuedata;
      rq = bd->rq;
      blk_mq_start_request(rq);
      if (blk_rq_is_passthrough(rq)) {
          printk(KERN_NOTICE "Skip non-fs request\n");
          blk_mq_end_request(rq, BLK_STS_IOERR);
          return BLK_STS_OK;
      }
      printk(KERN_INFO "request received: sector=%llu, total_size=%u, bio_size=%u, dir=%d\n",
             (unsigned long long)blk_rq_pos(rq),
             blk_rq_bytes(rq),
             blk_rq_cur_bytes(rq),
             rq_data_dir(rq));
      blk_mq_end_request(rq, BLK_STS_OK);
  out:
      return BLK_STS_OK;
  }
  ```

- **Hàm `delete_block_device`**:
    - Dọn dẹp `gendisk`, hàng đợi, tag set, và bộ nhớ.
  ```c
  static void delete_block_device(struct my_block_dev *dev)
  {
      if (dev->gd) {
          del_gendisk(dev->gd);
          put_disk(dev->gd);
      }
      if (dev->queue)
          blk_cleanup_queue(dev->queue);
      if (dev->tag_set.tags)
          blk_mq_free_tag_set(&dev->tag_set);
      if (dev->data)
          vfree(dev->data);
  }
  ```

- **Hàm `my_block_init`**:
    - Gọi `create_block_device(&g_dev)`.
    - Nếu lỗi, hủy đăng ký.
  ```c
  static int __init my_block_init(void)
  {
      int err = 0;
      err = register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
      if (err < 0) {
          printk(KERN_ERR "unable to register mybdev block device\n");
          return -EBUSY;
      }
      err = create_block_device(&g_dev);
      if (err)
          goto out;
      return 0;
  out:
      unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
      return err;
  }
  ```

- **Hàm `my_block_exit`**:
    - Gọi `delete_block_device(&g_dev)`.
  ```c
  static void __exit my_block_exit(void)
  {
      delete_block_device(&g_dev);
      unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
  }
  ```

- **Giữ nguyên**: `TODO 1`, `TODO 3`, `TODO 6`, và mã khác.

### Chức năng
- **Thêm đĩa**: `create_block_device` khởi tạo `gendisk`, hàng đợi, và thêm đĩa.
- **Xử lý yêu cầu**: `my_block_request` in thông tin yêu cầu và kết thúc.
- **Dọn dẹp**: `delete_block_device` xóa đĩa và tài nguyên.

## Hướng dẫn biên dịch và kiểm tra

### 1. Biên dịch
- Chạy:
  ```bash
  make build
  ```

### 2. Kiểm tra trong QEMU
- Khởi động QEMU:
  ```bash
  make console
  ```
- Vào thư mục:
  ```bash
  cd /home/root/skels/block_device_drivers/1-2-3-6-ram-disk/kernel
  ```
- Nạp module:
  ```bash
  insmod ram-disk.ko
  ```
- Kiểm tra `/dev/myblock`:
  ```bash
  ls /dev/myblock
  ```
    - Nếu không có, tạo:
      ```bash
      mknod /dev/myblock b 240 0
      ```
- Sinh yêu cầu ghi:
  ```bash
  echo "abc" > /dev/myblock
  ```
- Xem log:
  ```bash
  cat /proc/kmsg
  ```
    - **Kết quả**: Thấy thông tin yêu cầu (sector, kích thước, hướng).
- Gỡ module:
  ```bash
  rmmod ram-disk
  ```

### 3. Xử lý sự cố
- Không thấy `/dev/myblock`: Tạo với `mknod`.
- Không thấy log yêu cầu: Kiểm tra `my_block_request`.
- Module không nạp: Kiểm tra `create_block_device`.

## Tiếp theo
Bài 2 hoàn thành. Bài 3 sẽ tạo RAM disk (`TODO 3`). Liên hệ nếu cần hỗ trợ!

---

Nếu bạn cần thêm hỗ trợ (ví dụ: kiểm tra lỗi khác hoặc Bài 3), hãy báo nhé!