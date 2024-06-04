#ifndef DEMO_SEQUENCER_H_
#define DEMO_SEQUENCER_H_

int dseq_open(const char *fname);
void dseq_close(void);

void dseq_start(void);

void dseq_update(void);

/* looks for an event by name, and returns its id, or -1 if it doesn't exist */
int dseq_lookup(const char *evname);

/* returns the current value of event track by id */
int dseq_value(int evid);
/* returns 1 if specified event changed state since last update, 0 otherwise */
int dseq_triggered(int evid);

#endif	/* DEMO_SEQUENCER_H_ */
