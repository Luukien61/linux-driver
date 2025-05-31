*** TODO 1
```text
head <-> A <-> B <-> head

head->next trỏ đến A.
A->next trỏ đến B.
B->next trỏ đến head.
head->prev trỏ đến B.
B->prev trỏ đến A.
A->prev trỏ đến head.

```
- `ti = task_info_alloc(pid)`: Gọi hàm có sẵn để cấp phát bộ nhớ và khởi tạo ti->pid và ti->timestamp. Nếu cấp phát thất bại, trả về NULL.
- `pr_err("Failed to allocate...")`: In thông báo lỗi nếu cấp phát thất bại, sử dụng pr_err (tương đương printk(KERN_ERR, ...)).
- `list_add_tail(&ti->list, &head)`: Thêm task_info vào cuối danh sách head. &ti->list là con trỏ tới trường list của task_info, và &head là đầu danh sách.
- Sau khi task_info_alloc hoàn thành, ti->list->prev và ti->list->next không phải là NULL mà có giá trị không xác định (undefined, còn gọi là "garbage values"). Lý do là kmalloc cấp phát bộ nhớ mà không khởi tạo nội dung, 
chứa giá trị ngẫu nhiên (garbage values) còn sót lại từ lần sử dụng trước của vùng bộ nhớ đó.

**list_for_each**

-list_for_each là một macro trong kernel Linux (linux/list.h), dùng để duyệt qua tất cả các nút trong danh sách liên kết kép vòng, bắt đầu từ nút đầu tiên sau head (head->next) và dừng trước khi quay lại head.
- p là một con trỏ kiểu struct list_head, được dùng để trỏ lần lượt đến từng nút trong danh sách.
- &head là địa chỉ của biến head, tức là đầu danh sách.
- ti = list_entry(p, struct task_info, list)

**Ý nghĩa:**

- list_entry là một macro trong linux/list.h, dùng để lấy địa chỉ của cấu trúc cha (struct task_info) từ một con trỏ struct list_head (p) nằm bên trong cấu trúc đó.
  Trong trường hợp này:
  - p là con trỏ đến trường list của một struct task_info (ví dụ: &A->list hoặc &B->list).
  - struct task_info là kiểu của cấu trúc cha.
  - list là tên trường struct list_head trong struct task_info.
  - list_entry trả về con trỏ đến struct task_info chứa p.
  
**task_info_purge_list**

- p: Con trỏ hiện tại đang duyệt.
- q: Con trỏ tạm thời lưu trữ vị trí tiếp theo, để tránh bị xóa khi đang duyệt.

**TODO 2**
- `list_for_each_safe(p, q, &head)`: Duyệt qua danh sách head một cách an toàn. Biến p trỏ đến phần tử hiện tại, q trỏ đến phần tử tiếp theo, để đảm bảo danh sách không bị hỏng khi xóa p.
- `ti = list_entry(p, struct task_info, list)`: Chuyển con trỏ p (trỏ tới list_head) thành con trỏ tới task_info bằng cách sử dụng trường list trong cấu trúc.
- `list_del(p)`: Xóa phần tử p khỏi danh sách head.
- `kfree(ti)`: Giải phóng bộ nhớ của task_info được cấp phát bởi kmalloc.



