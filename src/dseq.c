#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dseq.h"
#include "demo.h"
#include "track.h"
#include "treestor.h"
#include "cgmath/cgmath.h"
#include "dynarr.h"
#include "anim.h"

struct dseq_event {
	char *name;
	struct dseq_event *parent;
	struct dseq_event *next;	/* next event to become active */
	long relstart;				/* start time relative to previous event in list */

	enum dseq_interp interp;
	enum dseq_extrap extrap;

	float cur_t, cur_val;		/* updated by dseq_update */
	int cur_key;				/* current key interval */
	long start, dur;
	struct anm_track track;

	dseq_callback_func cbfunc;
	void *cbcls;
	unsigned int trigmask:8;
	unsigned int active:1;
	unsigned int trigd:1;

	long trans_in, trans_out;	/* only used by demo screens */
};

static dseq_event *events;
static int num_events;

static dseq_event *nextev;			/* pointer to the next event to become active */
static dseq_event *active_events;	/* linked list of currently active events */

static int started;
static long prev_upd;

static int count_events(struct ts_node *ts);
static dseq_event *load_event(struct ts_node *tsev, dseq_event *evprev, dseq_event *par);
static int evcmp(const void *ap, const void *bp);

static void dump_event(FILE *fp, dseq_event *ev);


int dseq_open(const char *fname)
{
	struct ts_node *ts, *tsn;
	dseq_event *ev = 0;

	if(!(ts = ts_load(fname))) {
		fprintf(stderr, "dseq_open: failed to load: %s\n", fname);
		return -1;
	}
	if(strcmp(ts->name, "demoseq") != 0) {
		fprintf(stderr, "dseq_open: invalid demo script: %s\n", fname);
		goto err;
	}

	if((num_events = count_events(ts->child_list)) <= 0) {
		fprintf(stderr, "dseq_open: found no events in %s\n", fname);
		goto err;
	}
	if(!(events = calloc(num_events, sizeof *events))) {
		fprintf(stderr, "dseq_open: failed to allocate space for the %d events of %s\n", num_events, fname);
		goto err;
	}
	num_events = 0;

	tsn = ts->child_list;
	while(tsn) {
		if(!(ev = load_event(tsn, ev, 0))) {
			goto err;
		}
		tsn = tsn->next;
	}
	ts_free_tree(ts);

	qsort(events, num_events, sizeof *events, evcmp);

	printf("dseq_load: loaded %d events from %s\n", num_events, fname);

	{
		int i;
		FILE *fp = fopen("events", "wb");
		if(fp) {
			for(i=0; i<num_events; i++) {
				dump_event(fp, events + i);
			}
			fclose(fp);
		}
	}

	started = 0;
	nextev = active_events = 0;
	return 0;

err:
	free(events);
	events = 0;
	num_events = 0;
	nextev = active_events = 0;
	ts_free_tree(ts);
	return -1;
}

static int count_events(struct ts_node *tsev)
{
	int count = 0;
	if(!tsev) return 0;
	while(tsev) {
		count += 1 + count_events(tsev->child_list);
		tsev = tsev->next;
	}
	return count;
}

#define SKIP_NOTNUM(attrname) \
	do { \
		if(attr->val.type != TS_NUMBER) { \
			fprintf(stderr, "dseq_open: %s:%s expected number, got: \"%s\"\n", tsev->name, attrname, attr->val.str); \
			goto next; \
		} \
	} while(0)

static dseq_event *load_event(struct ts_node *tsev, dseq_event *evprev, dseq_event *par)
{
	int namelen;
	dseq_event *ev, *subev = 0;
	struct ts_attr *attr;
	struct ts_node *tssub;
	long par_start, key_msec, subend, max_subend = -1;
	float key_sec;
	long end;
	long prev_time, defer_end = -1, delay_end = 0;

	ev = events + num_events;
	memset(ev, 0, sizeof *ev);

	ev->start = -1;

	if(anm_init_track(&ev->track) == -1) {
		fprintf(stderr, "dseq_load: failed to initialize keyframe track\n");
		return 0;
	}
	dseq_set_interp(ev, DSEQ_STEP);
	dseq_set_extrap(ev, DSEQ_CLAMP);

	namelen = strlen(tsev->name);
	if(par) {
		namelen += strlen(par->name) + 1;
	}
	if(!(ev->name = malloc(namelen + 1))) {
		anm_destroy_track(&ev->track);
		fprintf(stderr, "dseq_load: failed to allocate event name\n");
		return 0;
	}
	if(par) {
		sprintf(ev->name, "%s.%s", par->name, tsev->name);
	} else {
		strcpy(ev->name, tsev->name);
	}
	num_events++;

	par_start = par ? par->start : 0;
	prev_time = evprev ? evprev->start + evprev->dur : par_start;

	attr = tsev->attr_list;
	while(attr) {
		if(strcmp(attr->name, "start") == 0) {
			SKIP_NOTNUM("start");
			ev->start = attr->val.inum + par_start;
		} else if(strcmp(attr->name, "startabs") == 0) {
			SKIP_NOTNUM("startabs");
			ev->start = attr->val.inum;
		} else if(strcmp(attr->name, "wait") == 0) {
			SKIP_NOTNUM("wait");
			ev->start = attr->val.inum + (evprev ? evprev->start + evprev->dur : 0);

		} else if(strcmp(attr->name, "dur") == 0) {
			SKIP_NOTNUM("dur");
			ev->dur = attr->val.inum;
		} else if(strcmp(attr->name, "end") == 0) {
			SKIP_NOTNUM("end");
			defer_end = attr->val.inum + par_start;
		} else if(strcmp(attr->name, "endabs") == 0) {
			SKIP_NOTNUM("endabs");
			defer_end = attr->val.inum;
		} else if(strcmp(attr->name, "delayend") == 0) {
			SKIP_NOTNUM("delayend");
			delay_end = attr->val.inum;

		} else if(strcmp(attr->name, "trans_in") == 0){
			SKIP_NOTNUM("trans_in");
			ev->trans_in = attr->val.inum;
		} else if(strcmp(attr->name, "trans_out") == 0) {
			SKIP_NOTNUM("trans_out");
			ev->trans_out = attr->val.inum;
		} else if(strcmp(attr->name, "trans") == 0) {
			SKIP_NOTNUM("trans");
			ev->trans_in = ev->trans_out = attr->val.inum;

		} else if(sscanf(attr->name, "key(%ld)", &key_msec) == 1) {
			SKIP_NOTNUM("key");
			key_msec += par_start;
			anm_set_value(&ev->track, key_msec, attr->val.fnum);
			prev_time = key_msec;
		} else if(sscanf(attr->name, "keyabs(%ld)", &key_msec) == 1) {
			SKIP_NOTNUM("keyabs");
			anm_set_value(&ev->track, key_msec, attr->val.fnum);
			prev_time = key_msec;
		} else if(sscanf(attr->name, "waitkey(%ld)", &key_msec) == 1) {
			SKIP_NOTNUM("waitkey");
			key_msec += prev_time;
			anm_set_value(&ev->track, key_msec, attr->val.fnum);
			prev_time = key_msec;
		} else if(sscanf(attr->name, "ksec(%f)", &key_sec) == 1) {
			SKIP_NOTNUM("ksec");
			key_msec = (long)(key_sec * 1000.0f) + par_start;
			anm_set_value(&ev->track, key_msec, attr->val.fnum);
			prev_time = key_msec;
		} else if(sscanf(attr->name, "ksecabs(%f)", &key_sec) == 1) {
			SKIP_NOTNUM("ksecabs");
			key_msec = (long)(key_sec * 1000.0f);
			anm_set_value(&ev->track, key_msec, attr->val.fnum);
			prev_time = key_msec;
		} else if(sscanf(attr->name, "waitksec(%f)", &key_sec) == 1) {
			SKIP_NOTNUM("waitksec");
			key_msec = (long)(key_sec * 1000.0f) + prev_time;
			anm_set_value(&ev->track, key_msec, attr->val.fnum);
			prev_time = key_msec;

		} else if(strcmp(attr->name, "interp") == 0) {
			if(attr->val.type != TS_STRING) {
				fprintf(stderr, "dseq_open: %s:%s expected string, got: \"%s\"\n",
						tsev->name, attr->name, attr->val.str);
				goto next;
			}
			if(strcmp(attr->val.str, "step") == 0) {
				anm_set_track_interpolator(&ev->track, ANM_INTERP_STEP);
				ev->interp = DSEQ_STEP;
			} else if(strcmp(attr->val.str, "linear") == 0) {
				anm_set_track_interpolator(&ev->track, ANM_INTERP_LINEAR);
				ev->interp = DSEQ_LINEAR;
			} else if(strcmp(attr->val.str, "cubic") == 0) {
				anm_set_track_interpolator(&ev->track, ANM_INTERP_CUBIC);
				ev->interp = DSEQ_CUBIC;
			} else if(strcmp(attr->val.str, "smoothstep") == 0) {
				anm_set_track_interpolator(&ev->track, ANM_INTERP_SIGMOID);
				ev->interp = DSEQ_SMOOTHSTEP;
			} else {
				fprintf(stderr, "dseq_open: %s: unrecognized interpolation type: %s\n",
						tsev->name, attr->val.str);
				goto next;
			}

		} else if(strcmp(attr->name, "extrap") == 0) {
			if(attr->val.type != TS_STRING) {
				fprintf(stderr, "dseq_open: %s: %s expected string, got: \"%s\"\n",
						tsev->name, attr->name, attr->val.str);
				goto next;
			}
			if(strcmp(attr->val.str, "extend") == 0) {
				anm_set_track_extrapolator(&ev->track, ANM_EXTRAP_EXTEND);
				ev->extrap = DSEQ_EXTEND;
			} else if(strcmp(attr->val.str, "clamp") == 0) {
				anm_set_track_extrapolator(&ev->track, ANM_EXTRAP_CLAMP);
				ev->extrap = DSEQ_CLAMP;
			} else if(strcmp(attr->val.str, "repeat") == 0) {
				anm_set_track_extrapolator(&ev->track, ANM_EXTRAP_REPEAT);
				ev->extrap = DSEQ_REPEAT;
			} else if(strcmp(attr->val.str, "pingpong") == 0) {
				anm_set_track_extrapolator(&ev->track, ANM_EXTRAP_PINGPONG);
				ev->extrap = DSEQ_PINGPONG;
			} else {
				fprintf(stderr, "dseq_open: %s: unrecognized extrapolation type: %s\n",
						tsev->name, attr->val.str);
				goto next;
			}
		}

next:	attr = attr->next;
	}

	if(ev->start < 0) {
		/* start not explicitly specified */
		if(ev->track.count > 0) {
			/* ... compute based on keyframe track start */
			ev->start = ev->track.keys[0].time;
		} else {
			ev->start = evprev ? evprev->start + evprev->dur : 0;
		}
	}


	/* load sub-events now, so that we can take their keyframe tracks into
	 * account for computing event duration
	 */

	tssub = tsev->child_list;
	while(tssub) {
		subev = load_event(tssub, subev, ev);
		tssub = tssub->next;

		subend = subev->start + subev->dur;
		if(subend > max_subend) {
			max_subend = subend;
		}
	}


	if(defer_end >= 0) {
		if(defer_end < ev->start) {
			ev->dur = ev->start - defer_end;
			ev->start = defer_end;
		} else {
			ev->dur = defer_end - ev->start;
		}
	}

	if(ev->dur <= 0) {
		/* duration not explicitly specified */
		if(ev->track.count > 0) {
			/* ... compute based on keyframe track length */
			ev->dur = ev->track.keys[ev->track.count - 1].time - ev->start;
		} else if(ev->trans_in | ev->trans_out) {
			/* ... if there's no keyframes, make it the sum of transition times */
			ev->dur = ev->trans_in + ev->trans_out;
		} else {
			ev->dur = 1000;
			fprintf(stderr, "warning: can't determine duration for event %s\n", ev->name);
		}
		ev->dur += delay_end;

		if(ev->start + ev->dur < max_subend) {
			ev->dur = max_subend - ev->start;
		}
	}
	end = ev->start + ev->dur;

	/* if we have no keyframes, add some */
	if(ev->track.count <= 0) {
		if(ev->interp == DSEQ_STEP) {
			anm_set_value(&ev->track, ev->start, 1);
		} else {
			long tt = ev->trans_in > 0 ? ev->trans_in : 1;
			anm_set_value(&ev->track, ev->start, 0);
			anm_set_value(&ev->track, ev->start + tt, 1);
		}

		if(ev->interp == DSEQ_STEP) {
			anm_set_value(&ev->track, end, 0);
		} else {
			long tt = ev->trans_out > 0 ? ev->trans_out : 1;
			anm_set_value(&ev->track, end - tt, 1);
			anm_set_value(&ev->track, end, 0);
		}
	}
	return ev;
}

void dseq_close(void)
{
	int i;

	for(i=0; i<num_events; i++) {
		free(events[i].name);
		anm_destroy_track(&events[i].track);
	}
	free(events);
	events = 0;
	num_events = 0;
}

int dseq_isopen(void)
{
	return events != 0;
}

void dseq_start(void)
{
	int i;
	long prev_start = 0;

	for(i=0; i<num_events; i++) {
		events[i].cur_t = 0;
		events[i].cur_val = 0;
		events[i].cur_key = -1;

		events[i].relstart = events[i].start - prev_start;
		prev_start = events[i].start;

		events[i].next = i < num_events - 1 ? events + i + 1 : 0;
	}

	started = 1;
	nextev = events;
	active_events = 0;
	prev_upd = time_msec;
}

void dseq_pause(void)
{
	started = 0;
}

void dseq_stop(void)
{
	dseq_pause();
	active_events = 0;
	nextev = 0;
}

void dseq_resume(void)
{
	started = 1;
	prev_upd = time_msec;
}

int dseq_started(void)
{
	return started;
}

void dseq_ffwd(long tm)
{
	if(tm <= 0) return;

	while(nextev) {
		nextev->relstart -= tm;
		if(nextev->relstart >= 0) break;
		tm = -nextev->relstart;
		nextev = nextev->next;
	}
}

void dseq_update(void)
{
	int curkey;
	long dt;
	dseq_event *ev, *prev, dummy;
	float newval;
	unsigned int reason;

	if(!started) return;

	/* update active events */
	dummy.next = active_events;
	prev = &dummy;
	while((ev = prev->next)) {
		reason = 0;
		if(time_msec >= ev->start + ev->dur) {
			ev->cur_t = 1.0f;
			/* remove from active list */
			prev->next = ev->next;
			ev->active = 0;
			reason = DSEQ_TRIG_END;
		} else {
			ev->cur_t = (float)(time_msec - ev->start) / (float)ev->dur;
			prev = prev->next;
		}
		ev->cur_val = anm_get_value_ex(&ev->track, time_msec, &curkey);
		if(curkey != ev->cur_key) {
			reason |= DSEQ_TRIG_KEY;
			ev->cur_key = curkey;
		}

		if(reason) {
			ev->trigd = 1;
			if(ev->cbfunc) {
				ev->cbfunc(ev, reason, ev->cbcls);
			}
		}
	}
	active_events = dummy.next;


	dt = time_msec - prev_upd;
	prev_upd = time_msec;

	/* activate any pending events */
	while(nextev) {
		nextev->relstart -= dt;
		if(nextev->relstart > 0) break;	/* no pending events reached yet */
		dt = -nextev->relstart;

		ev = nextev;
		nextev = nextev->next;

		/* add to the active events */
		ev->active = 1;
		ev->next = active_events;
		active_events = ev;

		ev->cur_t = (float)(time_msec - ev->start) / (float)ev->dur;
		ev->cur_val = anm_get_value_ex(&ev->track, time_msec, &curkey);

		reason = DSEQ_TRIG_START;
		if(curkey != ev->cur_key) {
			reason |= DSEQ_TRIG_KEY;
			ev->cur_key = curkey;
		}

		/* trigger any start callbacks */
		if(ev->trigmask & reason) {
			ev->trigd = 1;
			if(ev->cbfunc) {
				ev->cbfunc(ev, reason, ev->cbcls);
			}
		}
	}
}


dseq_event *dseq_lookup(const char *evname)
{
	int i;
	for(i=0; i<num_events; i++) {
		if(strcmp(events[i].name, evname) == 0) {
			return events + i;
		}
	}
	return 0;
}

const char *dseq_name(dseq_event *ev)
{
	return ev->name;
}

int dseq_transtime(dseq_event *ev, int *in, int *out)
{
	if(in) *in = ev->trans_in;
	if(out) *out = ev->trans_out;
	return ev->trans_in;
}

long dseq_start_time(dseq_event *ev)
{
	return ev->start;
}

long dseq_end_time(dseq_event *ev)
{
	return ev->start + ev->dur;
}

long dseq_duration(dseq_event *ev)
{
	return ev->dur;
}

int dseq_isactive(dseq_event *ev)
{
	return ev->active;
}

long dseq_time(dseq_event *ev)
{
	return time_msec - ev->start;
}

float dseq_param(dseq_event *ev)
{
	return ev->cur_t;
}

float dseq_value(dseq_event *ev)
{
	return ev->cur_val;
}

int dseq_triggered(dseq_event *ev)
{
	int trig = ev->trigd;
	ev->trigd = 0;
	return trig;
}

void dseq_set_interp(dseq_event *ev, enum dseq_interp in)
{
	enum anm_interpolator anm_in;

	ev->interp = in;

	switch(in) {
	case DSEQ_LINEAR:
		anm_in = ANM_INTERP_LINEAR;
		break;
	case DSEQ_CUBIC:
		anm_in = ANM_INTERP_CUBIC;
		break;
	case DSEQ_SMOOTHSTEP:
		anm_in = ANM_INTERP_SIGMOID;
		break;
	case DSEQ_STEP:
	default:
		anm_in = ANM_INTERP_STEP;
	}

	anm_set_track_interpolator(&ev->track, anm_in);
}

void dseq_set_extrap(dseq_event *ev, enum dseq_extrap ex)
{
	enum anm_extrapolator anm_ex;

	ev->extrap = ex;

	switch(ex) {
	case DSEQ_EXTEND:
		anm_ex = ANM_EXTRAP_EXTEND;
		break;
	case DSEQ_REPEAT:
		anm_ex = ANM_EXTRAP_REPEAT;
		break;
	case DSEQ_PINGPONG:
		anm_ex = ANM_EXTRAP_PINGPONG;
		break;
	case DSEQ_CLAMP:
	default:
		anm_ex = ANM_EXTRAP_CLAMP;
	}

	anm_set_track_extrapolator(&ev->track, anm_ex);
}

void dseq_set_trigger(dseq_event *ev, enum dseq_trig_mask mask,
		dseq_callback_func func, void *cls)
{
	ev->cbfunc = func;
	ev->cbcls = cls;
	ev->trigmask = mask;
}


void dseq_dbg_draw(void)
{
	int y = 20;
	char buf[128];
	dseq_event *ev;

	sprintf(buf, "time: %ld", time_msec);
	cs_cputs(fb_pixels, 5, y, buf); y += 8;
	cs_cputs(fb_pixels, 5, y, "active:"); y += 8;
	ev = active_events;
	while(ev) {
		sprintf(buf, "%s: %.2f -> %.2f", ev->name, ev->cur_t, ev->cur_val);
		cs_cputs(fb_pixels, 10, y, buf); y += 8;
		ev = ev->next;
	}

	if(nextev) {
		sprintf(buf, "next: %s in %ld", nextev->name, nextev->relstart);
		cs_cputs(fb_pixels, 5, y + 5, buf);
	}
}


static int evcmp(const void *ap, const void *bp)
{
	return ((dseq_event*)ap)->start - ((dseq_event*)bp)->start;
}


static void dump_event(FILE *fp, dseq_event *ev)
{
	int i;
	static const char *instr[] = {"step", "linear", "cubic", "smoothstep"};
	static const char *exstr[] = {"extend", "clamp", "repeat", "pingpong"};

	fprintf(fp, "%s (%s|%s) [%ld - %ld (%ld)]\n", ev->name, instr[ev->interp],
			exstr[ev->extrap], ev->start, ev->start + ev->dur, ev->dur);

	for(i=0; i<ev->track.count; i++) {
		fprintf(fp, "\t%ld: %.2f\n", ev->track.keys[i].time, ev->track.keys[i].val);
	}
}
