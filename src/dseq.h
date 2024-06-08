#ifndef DEMO_SEQUENCER_H_
#define DEMO_SEQUENCER_H_

enum dseq_trig_type {
	DSEQ_TRIG_ONCE,
	DSEQ_TRIG_ALL
};

typedef void (*dseq_callback_func)(int evid, int val, void *cls);

int dseq_open(const char *fname);
void dseq_close(void);
int dseq_isopen(void);

void dseq_start(void);

void dseq_update(void);

/* looks for an event by name, and returns its id, or -1 if it doesn't exist */
int dseq_lookup(const char *evname);

const char *dseq_name(int evid);

/* returns the current value of event track by id */
int dseq_value(int evid);
/* returns 1 if specified event changed state since last update, 0 otherwise */
int dseq_triggered(int evid);

void dseq_trig_callback(int evid, enum dseq_trig_type type, dseq_callback_func func, void *cls);

#endif	/* DEMO_SEQUENCER_H_ */
