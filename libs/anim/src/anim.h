/*
libanim - hierarchical keyframe animation library
Copyright (C) 2012-2018 John Tsiombikas <nuclear@member.fsf.org>

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
#ifndef LIBANIM_H_
#define LIBANIM_H_

#include "config.h"

#if _MSC_VER >= 1900
#define _TIMESPEC_DEFINED
#endif

#ifdef ANIM_THREAD_SAFE
#include <pthread.h>
#endif

#include "track.h"

enum {
	ANM_TRACK_POS_X,
	ANM_TRACK_POS_Y,
	ANM_TRACK_POS_Z,

	ANM_TRACK_ROT_X,
	ANM_TRACK_ROT_Y,
	ANM_TRACK_ROT_Z,
	ANM_TRACK_ROT_W,

	ANM_TRACK_SCL_X,
	ANM_TRACK_SCL_Y,
	ANM_TRACK_SCL_Z,

	ANM_NUM_TRACKS
};

struct anm_animation {
	char *name;
	struct anm_track tracks[ANM_NUM_TRACKS];
};

struct anm_node {
	char *name;

	int cur_anim[2];
	anm_time_t cur_anim_offset[2];
	float cur_mix;

	/* high-level animation blending transition duration */
	anm_time_t blend_dur;

	struct anm_animation *animations;
	float pivot[3];

	/* matrix cache */
	struct mat_cache {
		float matrix[16], inv_matrix[16];
		anm_time_t time, inv_time;
		struct mat_cache *next;
#ifdef ANIM_THREAD_SAFE
	} *cache_list;
	pthread_key_t cache_key;
	pthread_mutex_t cache_list_lock;
#else
	} cache;
#endif

	/* matrix calculated by anm_eval functions (no locking, meant as a pre-pass) */
	float matrix[16];

	struct anm_node *parent;
	struct anm_node *child;
	struct anm_node *next;
};

#ifdef __cplusplus
extern "C" {
#endif

int anm_init_animation(struct anm_animation *anim);
void anm_destroy_animation(struct anm_animation *anim);

void anm_set_animation_name(struct anm_animation *anim, const char *name);


/* ---- node/hierarchy management ---- */

/* node constructor and destructor */
int anm_init_node(struct anm_node *node);
void anm_destroy_node(struct anm_node *node);

/* recursively destroy an animation node tree */
void anm_destroy_node_tree(struct anm_node *tree);

/* helper functions to allocate/construct and destroy/free with
 * a single call. They call anm_init_node and anm_destroy_node
 * internally.
 */
struct anm_node *anm_create_node(void);
void anm_free_node(struct anm_node *node);

/* recursively destroy and free the nodes of a node tree */
void anm_free_node_tree(struct anm_node *tree);

int anm_set_node_name(struct anm_node *node, const char *name);
const char *anm_get_node_name(struct anm_node *node);

/* link and unlink nodes with parent/child relations */
void anm_link_node(struct anm_node *parent, struct anm_node *child);
int anm_unlink_node(struct anm_node *parent, struct anm_node *child);

void anm_set_pivot(struct anm_node *node, float x, float y, float z);
void anm_get_pivot(struct anm_node *node, float *x, float *y, float *z);

/* ---- multiple animations and animation blending ---- */

/* set active animation(s) */
int anm_use_node_animation(struct anm_node *node, int aidx);
int anm_use_node_animations(struct anm_node *node, int aidx, int bidx, float t);
/* recursive variants */
int anm_use_animation(struct anm_node *node, int aidx);
int anm_use_animations(struct anm_node *node, int aidx, int bidx, float t);

/* set/get current animation offset(s) */
void anm_set_node_animation_offset(struct anm_node *node, anm_time_t offs, int which);
anm_time_t anm_get_animation_offset(const struct anm_node *node, int which);
/* recursive variant */
void anm_set_animation_offset(struct anm_node *node, anm_time_t offs, int which);

/* returns the requested current animation index, which can be 0 or 1 */
int anm_get_active_animation_index(const struct anm_node *node, int which);
/* returns the requested current animation, which can be 0 or 1 */
struct anm_animation *anm_get_active_animation(const struct anm_node *node, int which);
float anm_get_active_animation_mix(const struct anm_node *node);

int anm_get_animation_count(const struct anm_node *node);

/* add/remove an animation to the specified node */
int anm_add_node_animation(struct anm_node *node);
int anm_remove_node_animation(struct anm_node *node, int idx);

/* add/remove an animation to the specified node and all it's descendants */
int anm_add_animation(struct anm_node *node);
int anm_remove_animation(struct anm_node *node, int idx);

struct anm_animation *anm_get_animation(struct anm_node *node, int idx);
struct anm_animation *anm_get_animation_by_name(struct anm_node *node, const char *name);

int anm_find_animation(struct anm_node *node, const char *name);

/* set the interpolator for the (first) currently active animation */
void anm_set_interpolator(struct anm_node *node, enum anm_interpolator in);
/* set the extrapolator for the (first) currently active animation */
void anm_set_extrapolator(struct anm_node *node, enum anm_extrapolator ex);

/* set the name of the currently active animation of this node only */
void anm_set_node_active_animation_name(struct anm_node *node, const char *name);
/* recursively set the name of the currently active animation for this node
 * and all it's descendants */
void anm_set_active_animation_name(struct anm_node *node, const char *name);
/* get the name of the currently active animation of this node */
const char *anm_get_active_animation_name(struct anm_node *node);


/* ---- high level animation blending interface ---- */
/* XXX this convenience interface assumes monotonically increasing time values
 *     in all subsequent calls to anm_get_* and anm_eval_* functions.
 *
 * anmidx: index of the animation to transition to
 * start: when to start the transition
 * dur: transition duration
 *
 * sets up a transition from the current animation (cur_anim[0]) to another animation.
 * at time start + dur, the transition will be completed, cur_anim[0] will be the new
 * animation and cur_anim_offset[0] will be equal to start.
 */
void anm_transition(struct anm_node *node, int anmidx, anm_time_t start, anm_time_t dur);
/* non-recursive variant, acts on a single node (you probably DON'T want to use this) */
void anm_node_transition(struct anm_node *node, int anmidx, anm_time_t start, anm_time_t dur);


/* ---- keyframes / PRS interpolation ---- */

void anm_set_position(struct anm_node *node, const float *pos, anm_time_t tm);
void anm_set_position3f(struct anm_node *node, float x, float y, float z, anm_time_t tm);
void anm_get_node_position(struct anm_node *node, float *pos, anm_time_t tm);

void anm_set_rotation(struct anm_node *node, const float *qrot, anm_time_t tm);
void anm_set_rotation4f(struct anm_node *node, float x, float y, float z, float w, anm_time_t tm);
void anm_set_rotation_axis(struct anm_node *node, float angle, float x, float y, float z, anm_time_t tm);
void anm_get_node_rotation(struct anm_node *node, float *qrot, anm_time_t tm);

void anm_set_scaling(struct anm_node *node, const float *scale, anm_time_t tm);
void anm_set_scaling3f(struct anm_node *node, float x, float y, float z, anm_time_t tm);
void anm_get_node_scaling(struct anm_node *node, float *scale, anm_time_t tm);

/* these three return the full p/r/s taking hierarchy into account */
void anm_get_position(struct anm_node *node, float *pos, anm_time_t tm);
void anm_get_rotation(struct anm_node *node, float *qrot, anm_time_t tm);
void anm_get_scaling(struct anm_node *node, float *scale, anm_time_t tm);

/* those return the start and end times of the whole tree */
anm_time_t anm_get_start_time(struct anm_node *node);
anm_time_t anm_get_end_time(struct anm_node *node);


/* ---- transformation matrices ---- */

/* these calculate the matrix and inverse matrix of this node alone */
void anm_get_node_matrix(struct anm_node *node, float *mat, anm_time_t tm);
void anm_get_node_inv_matrix(struct anm_node *node, float *mat, anm_time_t tm);

/* ---- top-down matrix calculation interface ---- */

/* calculate and set the matrix of this node */
void anm_eval_node(struct anm_node *node, anm_time_t tm);
/* calculate and set the matrix of this node and all its children recursively */
void anm_eval(struct anm_node *node, anm_time_t tm);


/* ---- bottom-up lazy matrix calculation interface ---- */

/* These calculate the matrix and inverse matrix of this node taking hierarchy
 * into account. The results are cached in thread-specific storage and returned
 * if there's no change in time or tracks from the last query...
 *
 * A pointer to the internal cached matrix is returned, and also if mat is not
 * null, the matrix is copied there.
 */
float *anm_get_matrix(struct anm_node *node, float *mat, anm_time_t tm);
float *anm_get_inv_matrix(struct anm_node *node, float *mat, anm_time_t tm);

#ifdef __cplusplus
}
#endif

#endif	/* LIBANIM_H_ */
