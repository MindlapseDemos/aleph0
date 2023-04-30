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
	/* Link parent */
	struct LinkStruct *parent;

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

void link_cons(Link *link, Link *parent, float elevation)
{
	link->child = 0;
	link->parent = parent;
	if (parent) {
		parent->child = link;
	}
	cgm_qrotation(&link->rotation, 0.0f, 0.0f, 0.0f, 1.0f);
	link->elevation = elevation;
}

/* Has to be called parent filst -> child next */
void link_update(Link *link)
{
	/* global transform */
	if (link->parent) {
		cgm_qcons(&link->global_rotation, 
			link->parent->global_rotation.x, link->parent->global_rotation.y, 
			link->parent->global_rotation.z, link->parent->global_rotation.w);
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
	if (link->parent) {
		cgm_vcons(&link->global_start, link->parent->global_end.x, 
			link->parent->global_end.y, link->parent->global_end.z);
	} else {
		cgm_vcons(&link->global_start, 0, 0, 0);
	}
	
	cgm_vcons(&link->global_end, link->global_up.x, link->global_up.y, link->global_up.z);
	cgm_vscale(&link->global_end, link->elevation);
	cgm_vadd(&link->global_end, &link->global_start);
}

/* Updates link and all children. Returns last child. */
Link *link_update_hierarchy(Link *link)
{
	Link *current = link;
	Link *last = link;

	while (current) {
		last = current;

		link_update(current);
		current = current->child;
	}

	return last;
}

/* Get angle to rotate point 'a' around pivot 'p' to reach point 'b' */
void extract_angle(cgm_quat *angle, cgm_vec3 *a, cgm_vec3 *b, cgm_vec3 *p) {
	cgm_vec3 axis;
	cgm_vec3 da, db;
	float l, dotValue;
	float rads;

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


	cgm_vcross(&axis, &da, &db);
	l = cgm_vlength_sq(&axis);
	if (l < 0.00001f) {
		/* Don't bother normalizing */
		cgm_qcons(angle, 0, 0, 0, 1);
		return;
	}
	l = 1.0f / sqrt(l);
	cgm_vscale(&axis, l);

	
	/* Get angle to go from a to b */
	dotValue = cgm_vdot(&da, &db);
	if (fabs(dotValue) > 0.999) {
		/* No deal. We can't construct a good axis of rotation. */
		cgm_qcons(angle, 0, 0, 0, 1);
		return;
	}
	rads = acos(dotValue);


	cgm_qrotation(angle, rads, axis.x, axis.y, axis.z);
}

/* Clamps so the quaternion does not exceed 90 degrees */
void clamp_quat90(cgm_quat *q)
{
	/* ChatGPT wrote this function for me - no kidding. 
	   It even used cgm_ types and xyz instead of ijk for quaternions because I gave it the below function to look at. */
	float angle, s;

	angle = acos(q->w) * 2.0f;
    if (angle > M_PI / 2.0f) {
        s = sin(M_PI / 4.0f) / sqrt(q->x * q->x + q->y * q->y + q->z * q->z);
        q->x *= s;
        q->y *= s;
        q->z *= s;
        q->w = cos(M_PI / 4.0f);
    }
}

/* Clamps so the quaternion does not exceed 45 degrees */
void clamp_quat45(cgm_quat *q)
{
    float angle, s;
    
    angle = acos(q->w) * 2.0f;
    if (angle > M_PI / 4.0f) {
        s = sin(M_PI / 8.0f) / sqrt(q->x * q->x + q->y * q->y + q->z * q->z);
        q->x *= s;
        q->y *= s;
        q->z *= s;
        q->w = cos(M_PI / 8.0f);
    }
}

/* Will both update the link hierarchy and rotate the links to cover 'ratio' oft he angle. */
void link_follow(Link *link, cgm_vec3 *target, float ratio)
{
	Link *lastLink = 0;
	cgm_quat fullRotation, rotation, tmp;


	lastLink = link_update_hierarchy(link);

	/* Get rotation to reach the target */
	extract_angle(&fullRotation, &lastLink->global_end, target, &link->global_start);

	/* Get part of that rotation */
	cgm_qcons(&tmp, 0, 0, 0, 1);
	cgm_qslerp(&rotation, &tmp, &fullRotation, ratio);
	cgm_qnormalize(&rotation);

	/* Rotate */
	cgm_qmul(&link->rotation, &rotation);
	cgm_qnormalize(&link->rotation);

	/* Maximum bend constraint */
	clamp_quat45(&link->rotation);
	
	/* Update local */
	link_update(link);

	/* Recurse */
	if (link->child) {
		/* Make the child a tiny bit faster so we don't lock ourselves in situations where the parent does the exact inverse rotation of the child. */
		link_follow(link->child, target, ratio * 1.01f);
	}
}

#define LINK_COUNT 3
static struct {
	Link links[LINK_COUNT];
	cgm_vec3 camera_pos;
	float camera_scale;
} state;

struct screen *dott_screen(void) {
	return &scr;
}

static int init(void) {
	/* Initialize links */
	link_cons(&state.links[0], 0, 1.5f);
	link_cons(&state.links[1], &state.links[0], 1.5f);
	link_cons(&state.links[2], &state.links[1], 1.5f);
	
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

static inline float bezier_segment(float y0, float y1, float y2, float y3, float t)
{
	float t2 = t * t;
   float a0 = y3 - y2 - y0 + y1;
   float a1 = y0 - y1 - a0;
   float a2 = y2 - y0;
   float a3 = y1;

   return a0 * t * t2 + a1 * t2 + a2 * t + a3;
}

static void bezier(cgm_vec3 *result, cgm_vec3 *points, int point_count, float t)
{
	float findex;
	int index;
	cgm_vec3 a, b, c, d;
	cgm_vec3 *tmp;

	if (point_count <= 0) {
		cgm_vcons(result, 0, 0, 0);
		return;
	}

	if (point_count == 1) {
		cgm_vcons(result, points->x, points->y, points->z);
		return;
	}

	if (point_count == 2) {
		cgm_vlerp(result, points, points + 1, t);
		return;
	}

	if (t < 0.001f) {
		cgm_vcons(result, points->x, points->y, points->z);
		return;
	}

	if (t > 0.999f) {
		points += point_count - 1;
		cgm_vcons(result, points->x, points->y, points->z);
		return;
	}

	findex = (point_count - 1) * t;
	index = (int) findex;
	t = findex - index;

	/* We have at least two points guaranteed by now */
	points += index;
	cgm_vcons(&a, points->x, points->y, points->z);
	if (index > 0) {
		tmp = points - 1;
		cgm_vcons(&a, tmp->x, tmp->y, tmp->z);
	}
	cgm_vcons(&b, points->x, points->y, points->z);
	points++;
	cgm_vcons(&c, points->x, points->y, points->z);
	cgm_vcons(&d, points->x, points->y, points->z);
	if (index < point_count - 2) {
		tmp = points + 1;
		cgm_vcons(&d, tmp->x, tmp->y, tmp->z);
	}

	result->x = bezier_segment(a.x, b.x, c.x, d.x, t);
	result->y = bezier_segment(a.y, b.y, c.y, d.y, t);
	result->z = bezier_segment(a.z, b.z, c.z, d.z, t);
}

#define RED 0xF800
#define GREEN 0x7D00
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
	cgm_vec3 v, v2;
	cgm_vec3 right;
	cgm_vec3 controls[4];

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
	link_follow(&state.links[0], &v, ratio);

	

	/* Render debugs */
#if 1
	for (i=0; i<LINK_COUNT; i++) {
		project(&x0, &y0, &state.links[i].global_start);
		project(&x1, &y1, &state.links[i].global_end);
		draw_line(x0, y0, x1, y1, BLUE);

		cgm_vcons(&right, state.links[i].global_right.x, state.links[i].global_right.y, state.links[i].global_right.z);
		cgm_vadd(&right, &state.links[i].global_start);
		project(&x1, &y1, &right);
		draw_line(x0, y0, x1, y1, RED);
	}
#endif

	/* Do a bezier curve */
	cgm_vcons(controls + 0, state.links[0].global_start.x, state.links[0].global_start.y, state.links[0].global_start.z);
	for (i=0; i<LINK_COUNT; i++) {
		cgm_vcons(controls + i + 1, state.links[i].global_end.x, state.links[i].global_end.y, state.links[i].global_end.z);
	}
	cgm_vcons(&v, state.links[0].global_start.x, state.links[0].global_start.y, state.links[0].global_start.z);
#define SEGS 10
	for (i=0; i<=SEGS; i++) {
		t = i / (float) SEGS;
		bezier(&v2, controls, 4, t);
		project(&x0, &y0, &v);
		project(&x1, &y1, &v2);
		draw_line(x0, y0, x1, y1, GREEN);

		cgm_vcons(&v, v2.x, v2.y, v2.z);
	}
		
	swap_buffers(0);
}

