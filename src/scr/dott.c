#include "demo.h"
#include "gfxutil.h"
#include "screen.h"

#include "cgmath/cgmath.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static long lastFrameTime = 0;

static struct screen scr = {"dott", init, destroy, start, 0, draw};

struct LinkStruct {
	/* Link child */
	struct LinkStruct *child;

	/* Rotation of the base of the link. */
	cgm_quat rotation;

	/* Y translation in terms of the rotated coordinate system. */
	float elevation;

	/* rotation including parent rotations */
	cgm_quat global_rotation;

	/* Cache */
	cgm_vec3 global_start;
	cgm_vec3 global_end;
	cgm_vec3 global_up;
	cgm_vec3 global_right;

};

typedef struct LinkStruct Link;

void link_cons(Link *link, Link *child, float elevation)
{
	link->child = child;
	cgm_qrotation(&link->rotation, 0.0f, 0.0f, 0.0f, 1.0f);
	link->elevation = elevation;
}

/* Has to be called parent filst -> child next */
void link_update(Link *link, Link *parent)
{
	/* global transform */
	if (parent) {
		cgm_qcons(&link->global_rotation, 
			parent->global_rotation.x, parent->global_rotation.y, 
			parent->global_rotation.z, parent->global_rotation.w);
	} else {
		cgm_qcons(&link->global_rotation, 0, 0, 0, 1);
	}

	/* Combine global with local rotation */
	cgm_qmul(&link->global_rotation, &link->rotation);

	/* Compute caches */

	/* Up vector */
	cgm_vcons(&link->global_up, 0, 1, 0);
	cgm_vrotate_quat(&link->global_up, &link->global_rotation);

	/* Right vector */
	cgm_vcons(&link->global_right, 1, 0, 0);
	cgm_vrotate_quat(&link->global_right, &link->global_rotation);
	
	/* global position */
	if (parent) {
		cgm_vcons(&link->global_start, parent->global_end.x, parent->global_end.y, parent->global_end.z);
	} else {
		cgm_vcons(&link->global_start, 0, 0, 0);
	}
	
	cgm_vcons(&link->global_end, link->global_up.x, link->global_up.y, link->global_up.z);
	cgm_vscale(&link->global_end, link->elevation);
	cgm_vadd(&link->global_end, &link->global_start);
}

/* Updates link and all children. Returns last child. */
Link *link_update_hierarchy(Link *link, Link *parent)
{
	Link *currentParent = parent;
	Link *current = link;
	Link *last = link;

	while (current) {
		last = current;

		link_update(current, currentParent);

		currentParent = current;
		current = current->child;
	}

	return last;
}

/* Get angle to rotate point 'a' around pivot 'p' to reach point 'b' */
void extract_angle(cgm_quat *angle, cgm_vec3 *a, cgm_vec3 *b, cgm_vec3 *p) {
	cgm_vec3 axis;
	cgm_vec3 da, db;
	float l;
	float rads;

	cgm_vcross(&axis, a, b);
	l = cgm_vlength_sq(&axis);
	if (l < 0.00001f) {
		/* Don't bother normalizing */
		cgm_qcons(angle, 0, 0, 0, 1);
		return;
	}
	l = 1.0f / sqrt(l);
	cgm_vscale(&axis, l);

	/* Gt offsets from pivot */
	cgm_vcons(&da, a->x, a->y, a->z);
	cgm_vsub(&da, p);
	l = cgm_vlength_sq(&da);
	if (l < 0.00001f) {
		cgm_qcons(angle, 0, 0, 0, 1);
		return;
	}
	l = 1.0f / sqrt(l);
	cgm_vscale(&da, l);

	cgm_vcons(&db, b->x, b->y, b->z);
	cgm_vsub(&db, p);
	l = cgm_vlength_sq(&db);
	if (l < 0.00001f) {
		cgm_qcons(angle, 0, 0, 0, 1);
		return;
	}
	l = 1.0f / sqrt(l);
	cgm_vscale(&db, l);

	/* Get angle to go from a to b */
	rads = acos(cgm_vdot(&da, &db));

	cgm_qrotation(angle, rads, axis.x, axis.y, axis.z);
}

/* Will both update the link hierarchy and rotate the links to cover 'ratio' oft he angle. */
void link_follow(Link *link, Link *parent, cgm_vec3 *target, float ratio)
{
	Link *lastLink = 0;
	cgm_quat fullRotation, rotation, tmp;

	lastLink = link_update_hierarchy(link, parent);

	/* Get rotation to reach the target */
	extract_angle(&fullRotation, &lastLink->global_end, target, &link->global_start);

	/* Get part of that rotation */
	cgm_qcons(&tmp, 0, 0, 0, 1);
	cgm_qslerp(&rotation, &tmp, &fullRotation, ratio);

	/* Rotate */
	cgm_qmul(&link->rotation, &rotation);

	/* Update local */
	link_update(link, parent);

	/* Recurse */
	if (link->child) {
		link_follow(link->child, link, target, ratio);
	}
}

#define LINK_COUNT 3
struct {
	Link links[LINK_COUNT];
	cgm_vec3 camera_pos;
	float camera_scale;
} state;

struct screen *dott_screen(void) {
	return &scr;
}

static int init(void) {
	/* Initialize links */
	link_cons(&state.links[0], &state.links[1], 1.5f);
	link_cons(&state.links[1], &state.links[2], 1.5f);
	link_cons(&state.links[2], 0, 1.5f);
	
	/* Setup camera */
	cgm_vcons(&state.camera_pos, 0, 3, 0);
	state.camera_scale = 32.0f; /* 32 pixels per world unit */
	
	return 0;
}

static void destroy(void) {
	
}

static void start(long trans_time) { 
	lastFrameTime = time_msec; 
}

/* 2D projection */
static inline void project(int *x, int *y, cgm_vec3 *v) {
	*x = FB_WIDTH / 2 + (int)((v->x - state.camera_pos.x) * state.camera_scale + 0.5f);
	*y = FB_HEIGHT / 2 - (int)((v->y - state.camera_pos.y) * state.camera_scale + 0.5f);
}

static inline void unproject(cgm_vec3 *v, int x, int y) {
	v->x = (x - FB_WIDTH / 2) / state.camera_scale + state.camera_pos.x;
	v->y = (FB_HEIGHT / 2 - y) / state.camera_scale + state.camera_pos.y;
	v->z = 0.0f;
}

#define RED 0xF800
#define BLUE 0x1F
#define WHITE 0xFFFF



/* For approaching a value by a small portion of the distance. Assumes an imaginary framerate of 24 FPS */
static inline float frame_independent_feedback(float portion_per_frame, float dt)
{
	float frameCount;
	float omPortion;

	omPortion = 1.0f - portion_per_frame;

	/* How many imaginary frames have passed */
	frameCount = dt * 24;

	/* The remaining distance will become smaller exponentially by frame */
	omPortion = pow(omPortion, frameCount);

	return 1.0f - omPortion;

}

static void draw(void) {
	int i;
	int x0, y0, x1, y1;
	float t, dt;
	float ratio;
	cgm_vec3 v;
	cgm_vec3 right;

	t = time_msec / 1000.0f;
	dt = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;

	/* Clear */
	for (i=0; i<FB_WIDTH * FB_HEIGHT; i++) {
		fb_pixels[i] = 0;
	}

	/* Test unproject / project */
	cgm_vcons(&v, 0, 0, 0);
	project(&x0, &y0, &v);
	unproject(&v, mouse_x, mouse_y);
	project(&x1, &y1, &v);
	draw_line(x0, y0, x1, y1, WHITE);

	/* Frame-independent ratio */
	ratio = frame_independent_feedback(0.05f, dt);

	/* Update links */
	link_follow(&state.links[0], 0, &v, ratio);

	

	/* Render debugs */
	for (i=0; i<LINK_COUNT; i++) {
		project(&x0, &y0, &state.links[i].global_start);
		project(&x1, &y1, &state.links[i].global_end);
		draw_line(x0, y0, x1, y1, BLUE);

		cgm_vcons(&right, state.links[i].global_right.x, state.links[i].global_right.y, state.links[i].global_right.z);
		cgm_vadd(&right, &state.links[i].global_start);
		project(&x1, &y1, &right);
		draw_line(x0, y0, x1, y1, RED);
	}

	


	swap_buffers(0);
}

