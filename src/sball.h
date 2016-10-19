#ifndef SBALL_H_
#define SBALL_H_

enum {
	SBALL_EV_NONE,
	SBALL_EV_MOTION,
	SBALL_EV_BUTTON
};

struct sball_event_motion {
	int type;
	int motion[6];
};

struct sball_event_button {
	int type;
	int id;
	int pressed;
	unsigned int state;
};

typedef union sball_event {
	int type;
	struct sball_event_motion motion;
	struct sball_event_button button;
} sball_event;

int sball_init(void);
void sball_shutdown(void);

int sball_getdev(void);

int sball_pending(void);
int sball_getevent(sball_event *ev);

#endif	/* SBALL_H_ */
