- Vùng nguyên tử là đoạn mã trong kernel mà không được phép ngắt CPU (preempt) hoặc không được phép nhường CPU (sleep) .


spin_lock(...) → bắt đầu một vùng mã nguyên tử .
Trong vùng này, không được phép nhường CPU hay ngủ .
Hàm schedule_timeout() gọi đến schedule() , tức là yêu cầu chuyển ngữ cảnh CPU .
Điều này mâu thuẫn với việc đang giữ spinlock.