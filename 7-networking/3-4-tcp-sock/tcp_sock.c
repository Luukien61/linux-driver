/*
 * SO2 - Networking Lab (#10)
 *
 * Exercise #3, #4: simple kernel TCP socket
 *
 * Code skeleton.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/fs.h>
#include <net/sock.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

MODULE_DESCRIPTION("Simple kernel TCP socket");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL		KERN_ALERT
#define MY_TCP_PORT		60000
#define LISTEN_BACKLOG		5

#define ON			1
#define OFF			0
#define DEBUG			ON

#if DEBUG == ON
#define LOG(s)					\
	do {					\
		printk(KERN_DEBUG s "\n");	\
	} while (0)
#else
#define LOG(s)					\
	do {} while (0)
#endif

#define print_sock_address(addr)		\
	do {					\
		printk(LOG_LEVEL "connection established to "	\
				"%pI4:%d\n",	 		\
				&addr.sin_addr.s_addr,		\
				ntohs(addr.sin_port));		\
	} while (0)

static struct socket *sock = NULL;   /* listening (server) socket */
static struct socket *new_sock = NULL; /* communication socket */
static struct delayed_work accept_work;

static void accept_connection(struct work_struct *work)
{
	int err;
	int retries = 10;
	struct sockaddr_in raddr;
	int raddrlen = sizeof(raddr);

	/* Create new socket for the accepted connection */
	err = sock_create_lite(PF_INET, SOCK_STREAM, IPPROTO_TCP, &new_sock);
	if (err < 0) {
		printk(LOG_LEVEL "Failed to create new socket: %d\n", err);
		return;
	}
	new_sock->ops = sock->ops;

	/* Accept a connection with retries */
	while (retries > 0) {
		err = sock->ops->accept(sock, new_sock, O_NONBLOCK, true);
		if (err == 0) {
			/* Accept thành công */
			break;
		} else if (err == -EAGAIN) {
			/* Không có kết nối ngay, thử lại sau 500ms */
			printk(LOG_LEVEL "Accept retrying (%d retries left)\n", retries);
			msleep(500);
			retries--;
		} else {
			/* Lỗi khác */
			printk(LOG_LEVEL "Accept failed: %d\n", err);
			return;
		}
	}

	if (err < 0) {
		printk(LOG_LEVEL "Accept failed after retries: %d\n", err);
		return;
	}

	/* Get the address of the peer and print it */
	err = new_sock->ops->getname(new_sock, (struct sockaddr *)&raddr, &raddrlen);
	if (err < 0) {
		printk(LOG_LEVEL "Get peer name failed: %d\n", err);
		return;
	}
	print_sock_address(raddr);
}

int __init my_tcp_sock_init(void)
{
	int err;
	/* address to bind on */
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= htons(MY_TCP_PORT),
		.sin_addr	= { htonl(INADDR_LOOPBACK) }
	};
	int addrlen = sizeof(addr);

	/* TODO 1: create listening socket */
	err = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (err < 0) {
		printk(LOG_LEVEL "Failed to create socket: %d\n", err);
		goto out;
	}

	/* TODO 1: bind socket to loopback on port MY_TCP_PORT */
	err = sock->ops->bind(sock, (struct sockaddr *)&addr, addrlen);
	if (err < 0) {
		printk(LOG_LEVEL "Bind failed: %d\n", err);
		goto out_release;
	}

	/* TODO 1: start listening */
	err = sock->ops->listen(sock, LISTEN_BACKLOG);
	if (err < 0) {
		printk(LOG_LEVEL "Listen failed: %d\n", err);
		goto out_release;
	}

	/* Schedule delayed work for accept (for exercise 4) */
	INIT_DELAYED_WORK(&accept_work, accept_connection);
	schedule_delayed_work(&accept_work, msecs_to_jiffies(1000)); /* Chạy sau 1 giây */

	return 0;

out_release:
	if (sock) {
		sock_release(sock);
		sock = NULL;
	}
out:
	return err;
}

void __exit my_tcp_sock_exit(void)
{
	/* Cancel any pending work */
	cancel_delayed_work_sync(&accept_work);

	if (new_sock) {
		sock_release(new_sock);
		new_sock = NULL;
	}
	if (sock) {
		sock_release(sock);
		sock = NULL;
	}
}

module_init(my_tcp_sock_init);
module_exit(my_tcp_sock_exit);