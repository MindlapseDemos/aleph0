#include <stdio.h>
#include <string.h>
#include "dseq.h"
#include "treestor.h"
#include "dynarr.h"
#include "demo.h"

struct key {
	long tm;
	int val;
};

struct track {
	char *name;
	int parent;
	int num;
	int cur_val;
	struct key *keys;	/* dynarr */

	dseq_callback_func trigcb;
	void *trigcls;
};

struct event {
	long reltime;
	int id;
	struct key key;
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

static int read_event(int parid, struct ts_node *tsn);
static void destroy_track(struct track *trk);
static int keycmp(const void *a, const void *b);


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
		fprintf(stderr, "failed to load demo sequence: %s\n", fname);
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
	printf("dseq_open: loaded %d event tracks\n", num_tracks);
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
		ev->key = *next_key;
		if(events) {
			ev->reltime = next_key->tm - evtail->key.tm;
			evtail->next = ev;
			evtail = ev;
		} else {
			ev->reltime = next_key->tm;
			events = evtail = ev;
		}
		trk_key[next_trk]++;
	}

	started = 0;

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

static long first_nonzero_key_time(struct track *trk)
{
	int i;
	for(i=0; i<trk->num; i++) {
		if(trk->keys[i].val) {
			return trk->keys[i].tm;
		}
	}
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
		namelen += strlen(tracks[parid].name);
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

	printf("READ EVENT: %s\n", name);

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
		par_start = first_nonzero_key_time(tracks + parid);
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

		} else if(strcmp(attr->name, "end") == 0) {
			SKIP_NOTNUM("end");
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
		ev->reltime = ev->key.tm - prev_time;
		prev_time = ev->key.tm;
		ev = ev->next;
	}
	next_event = events;
	prev_upd = 0;

	started = 1;
}

void dseq_stop(void)
{
	started = 0;
}

void dseq_update(void)
{
	int i, id;
	long dt;

	if(!started) return;

	dt = time_msec - prev_upd;
	prev_upd = time_msec;

	/* TODO active transitions */

	while(next_event) {
		id = next_event->id;
		next_event->reltime -= dt;
		if(next_event->reltime > 0) break;
		dt = -next_event->reltime;

		tracks[id].cur_val = next_event->key.val;
		TRIGGER(id);

		if(next_event->trigcb) {
			next_event->trigcb(id, next_event->key.val, next_event->trigcls);
		}
		if(tracks[id].trigcb) {
			tracks[id].trigcb(id, next_event->key.val, tracks[id].trigcls);
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
	return tracks[evid].name;
}

int dseq_value(int evid)
{
	return tracks[evid].cur_val;
}

int dseq_triggered(int evid)
{
	int trig = ISTRIG(evid);
	CLRTRIG(evid);
	return trig;
}

#define CHECK_REPLACE(x)	\
	if(x) fprintf(stderr, "warning replacing previous %s callback\n", type == DSEQ_TRIG_ONCE ? "event" : "track");

void dseq_trig_callback(int evid, enum dseq_trig_type type, dseq_callback_func func, void *cls)
{
	struct event *ev;

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
