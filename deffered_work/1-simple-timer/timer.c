#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timer.h>  // Thêm header cho timer
#include <linux/jiffies.h> // Thêm header cho jiffies và msecs_to_jiffies
*/
MODULE_DESCRIPTION("Simple kernel timer");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define TIMER_TIMEOUT   1

static struct timer_list timer;

static void timer_handler(struct timer_list *tl)
{
        /* TODO 1: print a message */
        pr_info("Timer fired after %d second(s)\n", TIMER_TIMEOUT);
}

static int __init timer_init(void)
{
        pr_info("[timer_init] Init module\n");

        /* TODO 1: initialize timer */
        timer_setup(&timer, timer_handler, 0);

        /* TODO 1: schedule timer for the first time */
        mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT * 1000));

        return 0;
}

static void __exit timer_exit(void)
{
        pr_info("[timer_exit] Exit module\n");

        /* TODO 1: cleanup; make sure the timer is not running after we exit */
        del_timer_sync(&timer);
}

module_init(timer_init);
module_exit(timer_exit);