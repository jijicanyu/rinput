/* rinputd.c
 * Heiher <admin@heiher.info>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#include "rinput.h"

static int r = 1;

static void signal_handler(int signal)
{
	r = 0;
}

int main(int argc, char *argv[])
{
	int uin_fd = 0, i =0;
	struct pollfd net_fds[1];
	struct input_event event;
	struct rinput_event revent;
	struct uinput_user_dev uin_dev;
	struct sockaddr_in saddr_srv, saddr_cli;
	socklen_t slen = sizeof(saddr_cli);

	/* Check arguments */
	if(3 != argc)
	{
		printf("Usage: %s address port\n",
					argv[0]);
		return 0;
	}

	/* Create socket */
	net_fds[0].fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(-1 == net_fds[0].fd)
	{
		printf("Create socket failed !\n");
		goto err;
	}
	net_fds[0].events = POLLIN;

	/* Set nonblock for socket */
	fcntl(net_fds[0].fd, F_SETFL, O_NONBLOCK);

	/* Init socket address */
	memset(&saddr_srv, 0, sizeof(saddr_srv));
	saddr_srv.sin_family = AF_INET;
	saddr_srv.sin_port = htons(atoi(argv[2]));
	saddr_srv.sin_addr.s_addr = inet_addr(argv[1]);
	/* Bind address */
	if(-1 == bind(net_fds[0].fd, (const struct sockaddr*)&saddr_srv,
					sizeof(saddr_srv)))
	{
		printf("Bind address failed!\n");
		goto err;
	}

	/* Open uinput device */
	uin_fd = open("/dev/uinput", O_WRONLY);
	if(-1 == uin_fd)
	{
		printf("Open uinput device failed!\n");
		goto err;
	}

	/* Set uinput capabilities */
	ioctl(uin_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(uin_fd, UI_SET_EVBIT, EV_REP);
	for(i=KEY_RESERVED; i<=KEY_RFKILL; i++)
	  ioctl(uin_fd, UI_SET_KEYBIT, i);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_MIDDLE);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_FORWARD);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_BACK);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_SIDE);
	ioctl(uin_fd, UI_SET_KEYBIT, BTN_EXTRA);
	ioctl(uin_fd, UI_SET_EVBIT, EV_REL);
	ioctl(uin_fd, UI_SET_RELBIT, REL_X);
	ioctl(uin_fd, UI_SET_RELBIT, REL_Y);
	ioctl(uin_fd, UI_SET_RELBIT, REL_WHEEL);

	/* Set uinput props */
	memset(&uin_dev, 0, sizeof(uin_dev));
	strncpy(uin_dev.name, "Remote Input Device", UINPUT_MAX_NAME_SIZE);
	uin_dev.id.bustype = BUS_USB;
	uin_dev.id.vendor  = 0x1;
	uin_dev.id.product = 0x1;
	uin_dev.id.version = 1;
	write(uin_fd, &uin_dev, sizeof(uin_dev));

	/* Create uinput device */
	if(-1 == ioctl(uin_fd, UI_DEV_CREATE))
	{
		printf("Create uinput device failed!\n");
		goto err;
	}

	/* Set signal handler */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* Main Loop */
	while(r)
	{
		i = poll(net_fds, 1, 500);
		if(0 < i)
		{
			ssize_t len = 0;

			len = recvfrom(net_fds[0].fd, &revent, sizeof(revent), 0,
						(struct sockaddr*)&saddr_cli, &slen);
			if(sizeof(revent) == len)
			{
				/* Update time by local */
				if(EV_REL == revent.type)
				  gettimeofday(&event.time, NULL);

				event.type = revent.type;
				event.code = revent.code;
				event.value = revent.value;
				write(uin_fd, &event, sizeof(event));
			}
			else if((-1==len) || (0==len))
			{
				break;
			}
		}
	}

	/* Close socket */
	close(net_fds[0].fd);

	/* Destroy uinput device */
	ioctl(uin_fd, UI_DEV_DESTROY);

	/* Close uinput device */
	close(uin_fd);

	return 0;
err:
	return -1;
}

