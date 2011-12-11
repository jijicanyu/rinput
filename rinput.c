/* rinput.c
 * Heiher <admin@heiher.info>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#include "rinput.h"

#define SWITCH_KEYCODE	KEY_PAUSE

static int r = 1;

static void signal_handler(int signal)
{
	r = 0;
}

int main(int argc, char *argv[])
{
	int net_fd = 0, i = 0;
	int grab = 0;
	struct pollfd ev_fds[10];
	struct input_event event;
	struct rinput_event revent;
	struct sockaddr_in saddr;

	if((4>argc) && (13<=argc))
	{
		printf("Usage: %s address port event_device ... (<=10)\n",
					argv[0]);
	}

	/* Create socket */
	net_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(-1 == net_fd)
	{
		printf("Create socket failed!\n");
		goto err;
	}

	/* Init socket address */
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(argv[2]));
	saddr.sin_addr.s_addr = inet_addr(argv[1]);

	for(i=3; i<argc; i++)
	{
		/* Open event device */
		ev_fds[i-3].fd = open(argv[i], O_RDONLY|O_NONBLOCK);
		if(-1 == ev_fds[i-3].fd)
		{
			printf("Open event device '%s' failed!\n",
						argv[i]);
			goto err;
		}
		ev_fds[i-3].events = POLLIN;
	}

	/* Set signal handler */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	while(r)
	{
		i = poll(ev_fds, argc-3, 500);
		if(0 < i)
		{
			for(i=3; i<argc; i++)
			{
				if(POLLIN & ev_fds[i-3].revents)
				{
					ssize_t len = 0;

					len = read(ev_fds[i-3].fd, &event, sizeof(event));
					if(sizeof(event) == len)
					{
						if((EV_KEY==event.type) &&
								(SWITCH_KEYCODE==event.code) &&
								(1==event.value))
						{
							int j = 0;

							grab = grab ? 0 : 1;
							for(j=3; j<argc; j++)
							{
								ioctl(ev_fds[j-3].fd, EVIOCGRAB, grab);
							}

							continue;
						}

						if(grab)
						{
							revent.type = event.type;
							revent.code = event.code;
							revent.value = event.value;
							sendto(net_fd, &revent, sizeof(revent), 0,
										(const struct sockaddr*)&saddr, sizeof(saddr));
						}
					}
				}
			}
		}
	}

	for(i=3; i<argc; i++)
	{
		/* Close event device */
		close(ev_fds[i-3].fd);
	}

	/* Close socket */
	close(net_fd);

	return 0;
err:
	return -1;
}

