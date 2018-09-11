#include "sball.h"

#ifdef __unix__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define SPNAV_SOCK_PATH "/var/run/spnav.sock"
#define IS_OPEN		(sock != -1)

struct event_node {
	sball_event event;
	struct event_node *next;
};

/* only used for non-X mode, with spnav_remove_events */
static struct event_node *ev_queue, *ev_queue_tail;

/* AF_UNIX socket used for alternative communication with daemon */
static int sock = -1;

static int conn_unix(int s, const char *path)
{
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));

	return connect(s, (struct sockaddr*)&addr, sizeof addr);
}


int sball_init(void)
{
	int s;

	if(IS_OPEN) {
		return -1;
	}

	if(!(ev_queue = malloc(sizeof *ev_queue))) {
		return -1;
	}
	ev_queue->next = 0;
	ev_queue_tail = ev_queue;

	if((s = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
		return -1;
	}

	if(conn_unix(s, SPNAV_SOCK_PATH) == -1) {
		perror("failed to connect");
		close(s);
		return -1;
	}

	sock = s;
	return 0;
}

void sball_shutdown(void)
{
	if(!IS_OPEN) {
		return;
	}

	if(sock != -1) {
		while(ev_queue) {
			void *tmp = ev_queue;
			ev_queue = ev_queue->next;
			free(tmp);
		}

		close(sock);
		sock = -1;
	}
}

int sball_getdev(void)
{
	return sock;
}

/* Checks both the event queue and the daemon socket for pending events.
 * In either case, it returns immediately with true/false values (doesn't block).
 */
int sball_pending(void)
{
	fd_set rd_set;
	struct timeval tv;

	if(ev_queue->next) {
		return 1;
	}

	FD_ZERO(&rd_set);
	FD_SET(sock, &rd_set);

	/* don't block, just poll */
	tv.tv_sec = tv.tv_usec = 0;

	if(select(sock + 1, &rd_set, 0, 0, &tv) > 0) {
		return 1;
	}
	return 0;
}


/* If there are events waiting in the event queue, dequeue one and
 * return that, otherwise read one from the daemon socket.
 * This might block unless we called event_pending() first and it returned true.
 */
static int read_event(int s, sball_event *event)
{
	int i, rd;
	int data[8];

	/* if we have a queued event, deliver that one */
	if(ev_queue->next) {
		struct event_node *node = ev_queue->next;
		ev_queue->next = ev_queue->next->next;

		/* dequeued the last event, must update tail pointer */
		if(ev_queue_tail == node) {
			ev_queue_tail = ev_queue;
		}

		memcpy(event, &node->event, sizeof *event);
		free(node);
		return event->type;
	}

	/* otherwise read one from the connection */
	do {
		rd = read(s, data, sizeof data);
	} while(rd == -1 && errno == EINTR);

	if(rd <= 0) {
		return 0;
	}

	if(data[0] < 0 || data[0] > 2) {
		return 0;
	}
	event->type = data[0] ? SBALL_EV_BUTTON : SBALL_EV_MOTION;

	if(event->type == SBALL_EV_MOTION) {
		for(i=0; i<6; i++) {
			event->motion.motion[i] = data[i + 1];
		}
		event->motion.motion[2] = -event->motion.motion[2];
		event->motion.motion[5] = -event->motion.motion[5];
	} else {
		event->button.pressed = data[0] == 1 ? 1 : 0;
		event->button.id = data[1];
	}

	return event->type;
}


int sball_getevent(sball_event *ev)
{
	if(sock) {
		if(read_event(sock, ev) > 0) {
			return ev->type;
		}
	}
	return 0;
}

#else	/* not __unix__ */

int sball_init(void)
{
	return 0;
}

void sball_shutdown(void)
{
}

int sball_getdev(void)
{
	return -1;
}

int sball_pending(void)
{
	return 0;
}

int sball_getevent(sball_event *ev)
{
	return SBALL_EV_NONE;
}

#endif
