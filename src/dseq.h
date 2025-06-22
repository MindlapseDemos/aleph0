#ifndef DEMO_SEQUENCER_H_
#define DEMO_SEQUENCER_H_

enum dseq_interp {
	DSEQ_STEP,
	DSEQ_LINEAR,
	DSEQ_CUBIC,
	DSEQ_SMOOTHSTEP
};

enum dseq_extrap {
	DSEQ_EXTEND,
	DSEQ_CLAMP,
	DSEQ_REPEAT,
	DSEQ_PINGPONG
};

enum dseq_trig_mask {
	DSEQ_TRIG_START		= 0x01,		/* start of active range */
	DSEQ_TRIG_END		= 0x02,		/* end of active range */
	DSEQ_TRIG_KEY		= 0x04,		/* keyframe */
	DSEQ_TRIG_ALL		= 0xff
};

typedef struct dseq_event dseq_event;
typedef void (*dseq_callback_func)(dseq_event*, enum dseq_trig_mask, void*);

int dseq_open(const char *fname);
void dseq_close(void);
int dseq_isopen(void);

void dseq_start(void);
void dseq_stop(void);
void dseq_pause(void);
void dseq_resume(void);
int dseq_started(void);
void dseq_ffwd(long tm);

void dseq_update(void);

/* looks for an event by name, and returns its id, or -1 if it doesn't exist */
dseq_event *dseq_lookup(const char *evname);

const char *dseq_name(dseq_event *ev);
int dseq_transtime(dseq_event *ev, int *in, int *out);
long dseq_start_time(dseq_event *ev);
long dseq_end_time(dseq_event *ev);
long dseq_duration(dseq_event *ev);

int dseq_isactive(dseq_event *ev);
long dseq_time(dseq_event *ev);		/* the time from the start of the event */
float dseq_param(dseq_event *ev);	/* parametric t during active event */
float dseq_value(dseq_event *ev);	/* the current value of the event */
int dseq_triggered(dseq_event *ev);


void dseq_set_interp(dseq_event *ev, enum dseq_interp in);
void dseq_set_extrap(dseq_event *ev, enum dseq_extrap ex);

void dseq_set_trigger(dseq_event *ev, enum dseq_trig_mask mask,
		dseq_callback_func func, void *cls);

void dseq_dbg_draw(void);

#endif	/* DEMO_SEQUENCER_H_ */
