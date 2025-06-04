#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Sleep while atomic");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

static int sched_spin_init(void)
{
        spinlock_t lock; // spinlock là một cơ chế bảo vệ vùng găng (vùng nguyên tử)

        spin_lock_init(&lock); // khởi tạo biến lock

        /* TODO 0: Use spin_lock to aquire the lock */
        spin_lock(&lock); // cpu chiếm spinlock(vùng nguyên tử), nếu vùng găng đã được lock, sẽ chờ cho đến khi được release

        set_current_state(TASK_INTERRUPTIBLE); // báo rằng task hiện tại có thể sleep cho task khác chạy
        /* Try to sleep for 5 seconds. */
        schedule_timeout(5 * HZ); // sleep trong 5s

        /* TODO 0: Use spin_unlock to release the lock */
        spin_unlock(&lock); // release lock, so other thread or cpu can accquires the lock

        return 0;
}

static void sched_spin_exit(void)
{
}

module_init(sched_spin_init);
module_exit(sched_spin_exit);