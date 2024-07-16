#include <stdio.h>
#include <string.h>
#include "dseq.h"
#include "treestor.h"
#include "dynarr.h"
#include "demo.h"
#include "util.h"
#include "cgmath/cgmath.h"

struct key {
	long tm;
	int val;
};

struct transition {
	int id;
	int val[2];
	long start, end, dur;
};

struct track {
	char *name;
	int parent;
	int num;
	int cur_val;
	struct key *keys;	/* dynarr */
	enum dseq_interp interp;
	int trans_time[2];	/* transition times (in/out), 0 (default) means whole interval */
	int (*interp_func)(struct transition *trs, int t);

	dseq_callback_func trigcb;
	void *trigcls;
};

struct event {
	long reltime;
	int id;
	struct key *key;
	struct event *next;

	dseq_callback_func trigcb;
	void *trigcls;
};

static struct track *tracks;	/* dynarr */
static int num_tracks;
static long last_key_time;
static struct event *events;	/* linked list */
static struct event *next_event;
static long prev_upd;
static int started;

#define TRIGMAP_SIZE	4
static uint32_t trigmap[TRIGMAP_SIZE];

#define TRIGGER(x)	trigmap[(x) >> 5] |= (1 << ((x) & 31))
#define CLRTRIG(x)	trigmap[(x) >> 5] &= ~(1 << ((x) & 31))
#define ISTRIG(x)	(trigmap[(x) >> 5] & (1 << ((x) & 31)))

#define MAX_ACTIVE_TRANS	8
struct transition trans[MAX_ACTIVE_TRANS];

static int read_event(int parid, struct ts_node *tsn);
static void destroy_track(struct track *trk);
static int keycmp(const void *a, const void *b);
static int interp_step(struct transition *trs, int t);
static int interp_linear(struct transition *trs, int t);
static int interp_smoothstep(struct transition *trs, int t);

static void dump_track(FILE *fp, struct track *trk);


int dseq_open(const char *fname)
{
	int i;
	struct ts_node *ts, *tsnode;
	struct key **trk_key;
	struct event *ev, *evtail;

	if(tracks) {
		fprintf(stderr, "dseq_open: a demo sequence file is already open\n");
		return -1;
	}

	if(!(ts = ts_load(fname))) {
		return -1;
	}
	if(strcmp(ts->name, "demoseq") != 0) {
		fprintf(stderr, "invalid demo sequence file: %s\n", fname);
		ts_free_tree(ts);
		return -1;
	}

	if(!(tracks = dynarr_alloc(0, sizeof *tracks))) {
		fprintf(stderr, "dseq_open: failed to allocate tracks\n");
		goto err;
	}

	tsnode = ts->child_list;
	while(tsnode) {
		if(read_event(-1, tsnode) == -1) {
			goto err;
		}

		tsnode = tsnode->next;
	}

	num_tracks = dynarr_size(tracks);
	printf("dseq_open(%s): loaded %d event tracks\n", fname, num_tracks);
	ts_free_tree(ts); ts = 0;

	if(!(trk_key = malloc(num_tracks * sizeof *trk_key))) {
		fprintf(stderr, "dseq_open: failed to construct arrray of %d track pointers\n", num_tracks);
		goto err;
	}

	for(i=0; i<num_tracks; i++) {
		tracks[i].num = dynarr_size(tracks[i].keys);
		/* sort all track keys */
		qsort(tracks[i].keys, tracks[i].num, sizeof *tracks[i].keys, keycmp);
		/* initialize track pointer for the next event list construction step */
		trk_key[i] = tracks[i].keys;
	}

	/* generate global list of ordered relative events */
	evtail = 0;
	for(;;) {
		struct key *next_key = 0;
		int next_trk;

		for(i=0; i<num_tracks; i++) {
			if(trk_key[i] >= tracks[i].keys + tracks[i].num) {
				continue;
			}
			if(!next_key || trk_key[i]->tm < next_key->tm) {
				next_key = trk_key[i];
				next_trk = i;
			}
		}

		if(!next_key) break;

		/* create event and advance the track pointer */
		if(!(ev = calloc(1, sizeof *ev))) {
			fprintf(stderr, "dseq_open: failed to allocate event\n");
			goto err;
		}
		ev->id = next_trk;
		ev->next = 0;
		ev->key = next_key;
		if(events) {
			ev->reltime = next_key->tm - evtail->key->tm;
			evtail->next = ev;
			evtail = ev;
		} else {
			ev->reltime = next_key->tm;
			events = evtail = ev;
		}
		trk_key[next_trk]++;
	}

	started = 0;

	/*{
		FILE *fp = fopen("tracks", "wb");
		if(fp) {
			for(i=0; i<num_tracks; i++) {
				dump_track(fp, tracks + i);
			}
			fclose(fp);
		}
	}*/

	free(trk_key);
	return 0;

err:
	while(events) {
		ev = events;
		events = events->next;
		free(ev);
	}
	for(i=0; i<dynarr_size(tracks); i++) {
		destroy_track(tracks + i);
	}
	dynarr_free(tracks);
	tracks = 0;
	ts_free_tree(ts);
	return -1;
}

static int add_key(struct track *trk, long tm, int val)
{
	void *tmp;
	struct key key;
	key.tm = tm;
	key.val = val;

	if(!(tmp = dynarr_push(trk->keys, &key))) {
		fprintf(stderr, "dseq_open: failed to add key\n");
		return -1;
	}
	trk->keys = tmp;
	last_key_time = tm;
	return 0;
}

static int prefix_length(int id)
{
	int len = 0;
	while(id >= 0) {
		len += strlen(tracks[id].name) + 1;
		id = tracks[id].parent;
	}
	return len;
}

static int fullpath(char *buf, int id)
{
	int len;
	if(id < 0) return 0;

	len = fullpath(buf, tracks[id].parent);
	return len + sprintf(buf + len, "%s.", tracks[id].name);
}

#define SKIP_NOTNUM(attrname) \
	do { \
		if(attr->val.type != TS_NUMBER) { \
			fprintf(stderr, "dseq_open: %s:%s expected number, got: \"%s\"\n", name, attrname, attr->val.str); \
			goto next; \
		} \
	} while(0)

#define ADD_START_END(s, e) \
	do { \
		add_key(trk, (s), 1024); \
		add_key(trk, (e), 0); \
		start = end = -1; \
	} while(0)

#define ADD_START_DUR(s, d) \
	do { \
		add_key(trk, (s), 1024); \
		add_key(trk, (s) + (d), 0); \
		start = dur = -1; \
	} while(0)


static int read_event(int parid, struct ts_node *tsn)
{
	int id, namelen;
	char *name;
	struct ts_attr *attr;
	struct ts_node *child;
	struct track *trk, tmptrk;
	long start = -1, dur = -1, end = -1;
	long par_start = 0;
	long key_time;


	namelen = strlen(tsn->name);
	if(parid >= 0) {
		namelen += strlen(tracks[parid].name) + 1;
	}
	if(!(name = malloc(namelen + 1))) {
		fprintf(stderr, "dseq_open: failed to allocate track name\n");
		return -1;
	}
	if(parid >= 0) {
		sprintf(name, "%s.%s", tracks[parid].name, tsn->name);
	} else {
		strcpy(name, tsn->name);
	}

	id = dynarr_size(tracks);
	if(!(trk = dynarr_push(tracks, &tmptrk))) {
		fprintf(stderr, "dseq_open: failed to resize tracks\n");
		return -1;
	}
	tracks = trk;
	trk = tracks + id;
	memset(trk, 0, sizeof *trk);
	trk->name = name;
	trk->parent = parid;

	if(!(trk->keys = dynarr_alloc(0, sizeof *trk->keys))) {
		fprintf(stderr, "dseq_open: failed to allocate event track\n");
		tracks = dynarr_pop(tracks);
		free(name);
		return -1;
	}

	if(parid >= 0) {
		par_start = dynarr_empty(tracks[parid].keys) ? 0 : tracks[parid].keys[0].tm;
	}

	attr = tsn->attr_list;
	while(attr) {
		if(strcmp(attr->name, "start") == 0) {
			SKIP_NOTNUM("start");
			start = attr->val.inum + par_start;
			if(end >= 0) {
				ADD_START_END(start, end);
			} else if(dur > 0) {
				ADD_START_DUR(start, dur);
			}

		} else if(strcmp(attr->name, "wait") == 0) {
			SKIP_NOTNUM("wait");
			start = last_key_time + attr->val.inum;
			if(end >= 0) {
				ADD_START_END(start, end);
			} else if(dur > 0) {
				ADD_START_DUR(start, dur);
			}

		} else if(strcmp(attr->name, "startabs") == 0) {
			SKIP_NOTNUM("startabs");
			start = attr->val.inum;
			if(end >= 0) {
				ADD_START_END(start, end);
			} else if(dur > 0) {
				ADD_START_DUR(start, dur);
			}

		} else if(strcmp(attr->name, "end") == 0) {
			SKIP_NOTNUM("end");
			end = attr->val.inum + par_start;
			if(start >= 0) {
				ADD_START_END(start, end);
			}

		} else if(strcmp(attr->name, "endabs") == 0) {
			SKIP_NOTNUM("endabs");
			end = attr->val.inum;
			if(start >= 0) {
				ADD_START_END(start, end);
			}

		} else if(strcmp(attr->name, "dur") == 0) {
			SKIP_NOTNUM("dur");
			dur = attr->val.inum;
			if(start >= 0) {
				ADD_START_DUR(start, dur);
			}

		} else if(sscanf(attr->name, "key:%ld", &key_time) == 1) {
			SKIP_NOTNUM("key");
			add_key(trk, key_time + par_start, attr->val.inum);

		} else if(sscanf(attr->name, "keyabs:%ld", &key_time) == 1) {
			SKIP_NOTNUM("keyabs");
			add_key(trk, key_time, attr->val.inum);

		} else if(strcmp(attr->name, "interp") == 0) {
			if(attr->val.type != TS_STRING) {
				fprintf(stderr, "dseq_open: %s:%s expected string, got: \"%s\"\n",
						name, attr->name, attr->val.str);
				goto next;
			}
			if(strcmp(attr->val.str, "step") == 0) {
				trk->interp = DSEQ_STEP;
				trk->interp_func = interp_step;
			} else if(strcmp(attr->val.str, "linear") == 0) {
				trk->interp = DSEQ_LINEAR;
				trk->interp_func = interp_linear;
			} else if(strcmp(attr->val.str, "smoothstep") == 0) {
				trk->interp = DSEQ_SMOOTHSTEP;
				trk->interp_func = interp_smoothstep;
			} else {
				fprintf(stderr, "dseq_open: %s: unrecognized interpolation type: %s\n",
						name, attr->val.str);
				goto next;
			}

		} else if(strcmp(attr->name, "trans") == 0) {
			SKIP_NOTNUM("trans");
			trk->trans_time[0] = trk->trans_time[1] = attr->val.inum;

		} else if(strcmp(attr->name, "trans_in") == 0) {
			SKIP_NOTNUM("trans_in");
			trk->trans_time[0] = attr->val.inum;

		} else if(strcmp(attr->name, "trans_out") == 0) {
			SKIP_NOTNUM("trans_out");
			trk->trans_time[1] = attr->val.inum;

		} else {
			fprintf(stderr, "dseq_open: ignoring unknown attribute: \"%s\"\n", attr->name);
			goto next;
		}
next:
		attr = attr->next;
	}

	child = tsn->child_list;
	while(child) {
		if(read_event(id, child) == -1) {
			return -1;
		}
		child = child->next;
	}
	return 0;
}

static void destroy_track(struct track *trk)
{
	free(trk->name);
	dynarr_free(trk->keys);
}

static int keycmp(const void *a, const void *b)
{
	return ((struct key*)a)->tm - ((struct key*)b)->tm;
}

void dseq_close(void)
{
	int i;
	struct event *ev;

	while(events) {
		ev = events;
		events = events->next;
		free(ev);
	}
	events = 0;

	for(i=0; i<dynarr_size(tracks); i++) {
		destroy_track(tracks + i);
	}
	dynarr_free(tracks);
	tracks = 0;

	started = 0;
}

int dseq_isopen(void)
{
	return tracks ? 1 : 0;
}

void dseq_start(void)
{
	int i;
	struct event *ev;
	long prev_time = 0;

	for(i=0; i<num_tracks; i++) {
		tracks[i].cur_val = 0;
	}

	ev = events;
	while(ev) {
		ev->reltime = ev->key->tm - prev_time;
		prev_time = ev->key->tm;
		ev = ev->next;
	}
	next_event = events;
	prev_upd = 0;

	for(i=0; i<MAX_ACTIVE_TRANS; i++) {
		trans[i].id = -1;
	}

	started = 1;
}

void dseq_stop(void)
{
	started = 0;
}

void dseq_ffwd(long tm)
{
	if(tm <= 0) return;

	while(next_event) {
		next_event->reltime -= tm;
		if(next_event->reltime > 0) break;
		tm = -next_event->reltime;
		next_event = next_event->next;
	}
}

int setup_transition(int id, struct event *trigev)
{
	int i;
	long trans_time;
	struct track *trk = tracks + id;
	struct transition *trs;

	for(i=0; i<MAX_ACTIVE_TRANS; i++) {
		if(trans[i].id == -1) break;
	}
	if(i >= MAX_ACTIVE_TRANS) {
		fprintf(stderr, "failed to allocate transition for %s\n", trk->name);
		return -1;
	}

	trs = trans + i;
	trs->id = id;
	trs->start = next_event->key->tm;


	/* if trans_time is non-zero, we ignore the interval and set the
	 * transition duration to whatever was requested.  Also if there's no
	 * other key after this, assume a transition time of 1sec
	 */
	if((trk->trans_time[0] | trk->trans_time[1]) || next_event->key >= trk->keys + trk->num - 1) {
		trs->val[0] = trk->cur_val;
		trs->val[1] = next_event->key->val;
		if(trs->val[0] < trs->val[1]) {	/* trans-in */
			trans_time = trk->trans_time[0] ? trk->trans_time[0] : 500;
		} else {	/* trans-out */
			trans_time = trk->trans_time[1] ? trk->trans_time[1] : 500;
		}
		trs->end = trs->start + trans_time;
	} else {
		trs->end = next_event->key[1].tm;
		trs->val[0] = next_event->key->val;
		trs->val[1] = next_event->key[1].val;
	}

	if(trs->val[0] == trs->val[1]) {
		trs->id = -1;
		return 0;
	}
	trs->dur = trs->end - trs->start;

	printf("DBG: setup transition for %s: %d->%d at %ld in %ld msec\n", tracks[id].name,
			trs->val[0], trs->val[1], trs->start, trs->dur);

	return 0;
}

void dseq_update(void)
{
	int i, id;
	long dt;
	int32_t t;
	struct transition *trs;
	struct track *trk;
	struct event *ev;

	if(!started) return;

	dt = time_msec - prev_upd;
	prev_upd = time_msec;

	/* update active transitions */
	for(i=0; i<MAX_ACTIVE_TRANS; i++) {
		if(trans[i].id == -1) continue;

		trs = trans + i;
		trk = tracks + trs->id;
		if(time_msec >= trs->end) {
			trk->cur_val = trs->val[1];
			trs->id = -1;
		} else {
			t = ((time_msec - trs->start) << 10) / trs->dur;
			trk->cur_val = trk->interp_func(trs, t);
		}
	}

	/* trigger any pending events */
	while(next_event) {
		id = next_event->id;
		next_event->reltime -= dt;
		if(next_event->reltime > 0) break;
		dt = -next_event->reltime;

		trk = tracks + id;

		/* step can't have transitions */
		if(trk->interp == DSEQ_STEP || setup_transition(id, next_event) == -1) {
			trk->cur_val = next_event->key->val;
		}
		TRIGGER(id);

		/* call trigger callbacks */
		if(next_event->trigcb) {
			next_event->trigcb(id, next_event->key->val, next_event->trigcls);
		}
		if(trk->trigcb) {
			trk->trigcb(id, next_event->key->val, trk->trigcls);
		}

		next_event = next_event->next;
	}
}


int dseq_lookup(const char *evname)
{
	int i;
	for(i=0; i<num_tracks; i++) {
		if(strcmp(tracks[i].name, evname) == 0) {
			return i;
		}
	}
	return -1;
}

const char *dseq_name(int evid)
{
	if(evid < 0) return "unknown";
	return tracks[evid].name;
}

int dseq_transtime(int evid, int *in, int *out)
{
	struct track *trk = tracks + evid;
	if(evid < 0) return 0;
	if(out) *out = trk->trans_time[1];
	if(in) *in = trk->trans_time[0];
	return trk->trans_time[0];
}

long dseq_evstart(int evid)
{
	if(evid < 0) return -1;
	return tracks[evid].num > 0 ? tracks[evid].keys[0].tm : -1;
}

int dseq_value(int evid)
{
	if(evid < 0) return 0;
	return tracks[evid].cur_val;
}

int dseq_triggered(int evid)
{
	int trig;
	if(evid < 0) return 0;
	trig = ISTRIG(evid);
	CLRTRIG(evid);
	return trig;
}

#define CHECK_REPLACE(x)	\
	if(x) fprintf(stderr, "warning replacing previous %s callback\n", type == DSEQ_TRIG_ONCE ? "event" : "track");

void dseq_trig_callback(int evid, enum dseq_trig_type type, dseq_callback_func func, void *cls)
{
	struct event *ev;

	if(evid < 0) return;

	if(type == DSEQ_TRIG_ONCE) {
		ev = events;
		while(ev) {
			if(ev->id == evid) {
				CHECK_REPLACE(ev->trigcb);
				ev->trigcb = func;
				ev->trigcls = cls;
			}
			ev = ev->next;
		}
	} else {
		CHECK_REPLACE(tracks[evid].trigcb);
		tracks[evid].trigcb = func;
		tracks[evid].trigcls = cls;
	}
}

static int interp_step(struct transition *trs, int t)
{
	return trs->val[0];
}

static int interp_linear(struct transition *trs, int t)
{
	return trs->val[0] + (((trs->val[1] - trs->val[0]) * t) >> 10);
}

static int interp_smoothstep(struct transition *trs, int t)
{
	float tt = cgm_smoothstep(0, 1024, t);
	return cround64(trs->val[0] + (trs->val[1] - trs->val[0]) * tt);	/* TODO: fixed point */
}

static void dump_track(FILE *fp, struct track *trk)
{
	int i;
	static const char *interpstr[] = {"step", "linear", "smoothstep"};

	fprintf(fp, "%s (%s/%d/%d)\n", trk->name, interpstr[trk->interp], trk->trans_time[0], trk->trans_time[1]);

	for(i=0; i<trk->num; i++) {
		fprintf(fp, "\t%ld: %d\n", trk->keys[i].tm, trk->keys[i].val);
	}
}
