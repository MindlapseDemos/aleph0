/*
libanim - hierarchical keyframe animation library
Copyright (C) 2012-2023 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "track.h"
#include "dynarr.h"

#include "cgmath/cgmath.h"

static int find_prev_key(const struct anm_keyframe *arr, int start, int end, anm_time_t tm);

static float interp_step(float v0, float v1, float v2, float v3, float t);
static float interp_linear(float v0, float v1, float v2, float v3, float t);
static float interp_cubic(float v0, float v1, float v2, float v3, float t);

static anm_time_t remap_extend(anm_time_t tm, anm_time_t start, anm_time_t end);
static anm_time_t remap_clamp(anm_time_t tm, anm_time_t start, anm_time_t end);
static anm_time_t remap_repeat(anm_time_t tm, anm_time_t start, anm_time_t end);
static anm_time_t remap_pingpong(anm_time_t tm, anm_time_t start, anm_time_t end);

/* XXX keep this in sync with enum anm_interpolator at track.h */
static float (*interp[])(float, float, float, float, float) = {
	interp_step,
	interp_linear,
	interp_cubic,
	0
};

/* XXX keep this in sync with enum anm_extrapolator at track.h */
static anm_time_t (*remap_time[])(anm_time_t, anm_time_t, anm_time_t) = {
	remap_extend,
	remap_clamp,
	remap_repeat,
	remap_pingpong,
	0
};

int anm_init_track(struct anm_track *track)
{
	memset(track, 0, sizeof *track);

	if(!(track->keys = dynarr_alloc(0, sizeof *track->keys))) {
		return -1;
	}
	track->keys_sorted = 1;
	track->interp = ANM_INTERP_LINEAR;
	track->extrap = ANM_EXTRAP_CLAMP;
	return 0;
}

void anm_destroy_track(struct anm_track *track)
{
	dynarr_free(track->keys);
}

struct anm_track *anm_create_track(void)
{
	struct anm_track *track;

	if((track = malloc(sizeof *track))) {
		if(anm_init_track(track) == -1) {
			free(track);
			return 0;
		}
	}
	return track;
}

void anm_free_track(struct anm_track *track)
{
	anm_destroy_track(track);
	free(track);
}

void anm_copy_track(struct anm_track *dest, const struct anm_track *src)
{
	free(dest->name);
	if(dest->keys) {
		dynarr_free(dest->keys);
	}

	if(src->name) {
		dest->name = malloc(strlen(src->name) + 1);
		strcpy(dest->name, src->name);
	}

	dest->count = src->count;
	dest->keys = dynarr_alloc(src->count, sizeof *dest->keys);
	memcpy(dest->keys, src->keys, src->count * sizeof *dest->keys);

	dest->def_val = src->def_val;
	dest->interp = src->interp;
	dest->extrap = src->extrap;
	dest->keys_sorted = src->keys_sorted;
}

int anm_set_track_name(struct anm_track *track, const char *name)
{
	char *tmp;

	if(!(tmp = malloc(strlen(name) + 1))) {
		return -1;
	}
	free(track->name);
	track->name = tmp;
	return 0;
}

const char *anm_get_track_name(const struct anm_track *track)
{
	return track->name;
}

void anm_set_track_interpolator(struct anm_track *track, enum anm_interpolator in)
{
	track->interp = in;
}

void anm_set_track_extrapolator(struct anm_track *track, enum anm_extrapolator ex)
{
	track->extrap = ex;
}

anm_time_t anm_remap_time(const struct anm_track *track, anm_time_t tm, anm_time_t start, anm_time_t end)
{
	return remap_time[track->extrap](tm, start, end);
}

void anm_set_track_default(struct anm_track *track, float def)
{
	track->def_val = def;
}

static int keycmp(const void *a, const void *b)
{
	return ((struct anm_keyframe*)a)->time - ((struct anm_keyframe*)b)->time;
}

int anm_set_keyframe(struct anm_track *track, struct anm_keyframe *key)
{
	int idx = anm_get_key_interval(track, key->time);

	/* if we got a valid keyframe index, compare them... */
	if(idx >= 0 && idx < track->count && keycmp(key, track->keys + idx) == 0) {
		/* ... it's the same key, just update the value */
		track->keys[idx].val = key->val;
	} else {
		/* ... it's a new key, add it and re-sort them if necessary */
		void *tmp;
		if(!(tmp = dynarr_push(track->keys, key))) {
			return -1;
		}
		track->keys = tmp;
		idx = track->count++;
		if(idx > 0 && track->keys[idx - 1].time > key->time) {
			/* key shold not go to the end, mark for re-sorting */
			track->keys_sorted = 0;
		}
	}
	return 0;
}

#define lazysort_keys(track)	\
	if(track->count > 1 && !track->keys_sorted) { \
		qsort(track->keys, track->count, sizeof *track->keys, keycmp); \
		((struct anm_track*)track)->keys_sorted = 1; \
	}

struct anm_keyframe *anm_get_keyframe(const struct anm_track *track, int idx)
{
	if(idx < 0 || idx >= track->count) {
		return 0;
	}
	lazysort_keys(track);
	return track->keys + idx;
}

int anm_get_key_interval(const struct anm_track *track, anm_time_t tm)
{
	int last;

	lazysort_keys(track);

	if(!track->count || tm < track->keys[0].time) {
		return -1;
	}

	last = track->count - 1;
	if(tm > track->keys[last].time) {
		return last;
	}

	return find_prev_key(track->keys, 0, last, tm);
}

static int find_prev_key(const struct anm_keyframe *arr, int start, int end, anm_time_t tm)
{
	int mid;

	if(end - start <= 1) {
		return start;
	}

	mid = (start + end) / 2;
	if(tm < arr[mid].time) {
		return find_prev_key(arr, start, mid, tm);
	}
	if(tm > arr[mid].time) {
		return find_prev_key(arr, mid, end, tm);
	}
	return mid;
}

int anm_set_value(struct anm_track *track, anm_time_t tm, float val)
{
	struct anm_keyframe key;
	key.time = tm;
	key.val = val;

	return anm_set_keyframe(track, &key);
}

float anm_get_value(const struct anm_track *track, anm_time_t tm)
{
	int idx0, idx1, last_idx;
	anm_time_t tstart, tend;
	float t, dt;
	float v0, v1, v2, v3;

	if(!track->count) {
		return track->def_val;
	}
	lazysort_keys(track);

	last_idx = track->count - 1;

	tstart = track->keys[0].time;
	tend = track->keys[last_idx].time;

	if(tstart == tend) {
		return track->keys[0].val;
	}

	tm = remap_time[track->extrap](tm, tstart, tend);

	idx0 = anm_get_key_interval(track, tm);
	assert(idx0 >= 0 && idx0 < track->count);
	idx1 = idx0 + 1;

	if(idx0 == last_idx) {
		return track->keys[idx0].val;
	}

	dt = (float)(track->keys[idx1].time - track->keys[idx0].time);
	t = (float)(tm - track->keys[idx0].time) / dt;

	v1 = track->keys[idx0].val;
	v2 = track->keys[idx1].val;

	/* get the neigboring values to allow for cubic interpolation */
	v0 = idx0 > 0 ? track->keys[idx0 - 1].val : v1;
	v3 = idx1 < last_idx ? track->keys[idx1 + 1].val : v2;

	return interp[track->interp](v0, v1, v2, v3, t);
}


void anm_get_quat(const struct anm_track *xtrk, const struct anm_track *ytrk,
		const struct anm_track *ztrk, const struct anm_track *wtrk, anm_time_t tm, float *qres)
{
	int idx0, idx1, last_idx;
	anm_time_t tstart, tend;
	float t, dt;
	cgm_quat q1, q2;

	if(!xtrk->count) {
		qres[0] = xtrk->def_val;
		qres[1] = ytrk->def_val;
		qres[2] = ztrk->def_val;
		qres[3] = wtrk->def_val;
		return;
	}

	lazysort_keys(xtrk);
	lazysort_keys(ytrk);
	lazysort_keys(ztrk);
	lazysort_keys(wtrk);

	last_idx = xtrk->count - 1;

	tstart = xtrk->keys[0].time;
	tend = xtrk->keys[last_idx].time;

	if(tstart == tend) {
		qres[0] = xtrk->keys[0].val;
		qres[1] = ytrk->keys[0].val;
		qres[2] = ztrk->keys[0].val;
		qres[3] = wtrk->keys[0].val;
		return;
	}

	tm = anm_remap_time(xtrk, tm, tstart, tend);

	idx0 = anm_get_key_interval(xtrk, tm);
	assert(idx0 >= 0 && idx0 < xtrk->count);
	idx1 = idx0 + 1;

	if(idx0 == last_idx) {
		qres[0] = xtrk->keys[idx0].val;
		qres[1] = ytrk->keys[idx0].val;
		qres[2] = ztrk->keys[idx0].val;
		qres[3] = wtrk->keys[idx0].val;
		return;
	}

	dt = (float)(xtrk->keys[idx1].time - xtrk->keys[idx0].time);
	t = (float)(tm - xtrk->keys[idx0].time) / dt;

	q1.x = xtrk->keys[idx0].val;
	q1.y = ytrk->keys[idx0].val;
	q1.z = ztrk->keys[idx0].val;
	q1.w = wtrk->keys[idx0].val;

	q2.x = xtrk->keys[idx1].val;
	q2.y = ytrk->keys[idx1].val;
	q2.z = ztrk->keys[idx1].val;
	q2.w = wtrk->keys[idx1].val;

	cgm_qslerp((cgm_quat*)qres, &q1, &q2, t);
}


static float interp_step(float v0, float v1, float v2, float v3, float t)
{
	return v1;
}

static float interp_linear(float v0, float v1, float v2, float v3, float t)
{
	return v1 + (v2 - v1) * t;
}

static float interp_cubic(float a, float b, float c, float d, float t)
{
	float x, y, z, w;
	float tsq = t * t;

	x = -a + 3.0 * b - 3.0 * c + d;
	y = 2.0 * a - 5.0 * b + 4.0 * c - d;
	z = c - a;
	w = 2.0 * b;

	return 0.5 * (x * tsq * t + y * tsq + z * t + w);
}

static anm_time_t remap_extend(anm_time_t tm, anm_time_t start, anm_time_t end)
{
	return remap_repeat(tm, start, end);
}

static anm_time_t remap_clamp(anm_time_t tm, anm_time_t start, anm_time_t end)
{
	if(start == end) {
		return start;
	}
	return tm < start ? start : (tm >= end ? end : tm);
}

static anm_time_t remap_repeat(anm_time_t tm, anm_time_t start, anm_time_t end)
{
	anm_time_t x, interv = end - start;

	if(interv == 0) {
		return start;
	}

	x = (tm - start) % interv;
	if(x < 0) {
		x += interv;
	}
	return x + start;

	/*if(tm < start) {
		while(tm < start) {
			tm += interv;
		}
		return tm;
	}
	return (tm - start) % interv + start;*/
}

static anm_time_t remap_pingpong(anm_time_t tm, anm_time_t start, anm_time_t end)
{
	anm_time_t interv = end - start;
	anm_time_t x = remap_repeat(tm, start, end + interv);

	return x > end ? end + interv - x : x;
}
