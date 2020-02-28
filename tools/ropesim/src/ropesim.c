#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "ropesim.h"

static void step(struct rsim_world *rsim, struct rsim_rope *rope, float dt);

int rsim_init(struct rsim_world *rsim)
{
	rsim->ropes = 0;
	cgm_vcons(&rsim->grav, 0, -9.807, 0);
	rsim->damping = 0.5;
	return 0;
}

void rsim_destroy(struct rsim_world *rsim)
{
	while(rsim->ropes) {
		struct rsim_rope *rope = rsim->ropes;
		rsim->ropes = rsim->ropes->next;
		rsim_free_rope(rope);
	}
}

int rsim_add_rope(struct rsim_world *rsim, struct rsim_rope *rope)
{
	rope->next = rsim->ropes;
	rsim->ropes = rope;
	return 0;
}

static inline struct rsim_spring *getspring(struct rsim_rope *rope, int aidx, int bidx)
{
	struct rsim_spring *spr = rope->springs + aidx * rope->num_masses + bidx;
	return *(uint32_t*)&spr->rest_len == 0xffffffff ? 0 : spr;
}

/* actual step function, called by rsim_step in a loop to microstep or once if
 * microstepping is disabled
 */
static void step(struct rsim_world *rsim, struct rsim_rope *rope, float dt)
{
	int i, j;
	float len, fmag;
	cgm_vec3 npos, faccel, dir;
	float inv_damp = rsim->damping == 0.0f ? 1.0f : (1.0f - rsim->damping);
	struct rsim_mass *mass;
	struct rsim_spring *spr;

	/* for each mass, add spring forces to every other mass it's connected to */
	for(i=0; i<rope->num_masses; i++) {
		for(j=0; j<rope->num_masses; j++) {
			if(i == j || !(spr = getspring(rope, i, j))) {
				continue;
			}

			dir = rope->masses[i].p;
			cgm_vsub(&dir, &rope->masses[j].p);

			len = cgm_vlength(&dir);
			if(len > 100.0f) {
				abort();
			}
			if(len != 0.0f) {
				float s = 1.0f / len;
				cgm_vscale(&dir, s);
			}
			fmag = (len - spr->rest_len) * spr->k;
			if(i == 5) {
				printf("%d-%d fmag: %f\n", i, j, fmag);
				if(fmag > 20) asm volatile("int $3");
			}

			assert(rope->masses[j].m != 0.0f);
			cgm_vscale(&dir, fmag / rope->masses[j].m);
			cgm_vadd(&rope->masses[j].f, &dir);
		}
	}

	/* update masses */
	mass = rope->masses;
	for(i=0; i<rope->num_masses; i++) {
		if(mass->fixed) {
			mass++;
			continue;
		}

		/* add constant forces to accumulated mass force */
		cgm_vadd(&mass->f, &rsim->grav);

		faccel = rsim->extforce;
		cgm_vscale(&faccel, 1.0f / mass->m);
		cgm_vadd(&mass->f, &rsim->extforce);

		mass->v.x = (mass->v.x - mass->v.x * inv_damp * dt) + mass->f.x * dt;
		mass->v.y = (mass->v.y - mass->v.y * inv_damp * dt) + mass->f.y * dt;
		mass->v.z = (mass->v.z - mass->v.z * inv_damp * dt) + mass->f.z * dt;

		/* zero out the accumulated forces for next iter */
		mass->f.x = mass->f.y = mass->f.z = 0.0f;

		npos = mass->p;
		npos.x = mass->p.x + mass->v.x * dt;
		npos.y = mass->p.y + mass->v.y * dt;
		npos.z = mass->p.z + mass->v.z * dt;

		/* TODO collisions */

		mass->p = npos;
		mass++;
	}
}

void rsim_step(struct rsim_world *rsim, float dt)
{
	struct rsim_rope *rope = rsim->ropes;

	if(rsim->udt > 0.0f) {
		/* microstepping */
		float dt_acc = rsim->udelta_acc;

		while(dt_acc < dt) {
			while(rope) {
				step(rsim, rope, rsim->udt);
				rope = rope->next;
			}
			dt_acc += rsim->udt;
		}
		rsim->udelta_acc = dt_acc - dt;
	} else {
		while(rope) {
			step(rsim, rope, dt);
			rope = rope->next;
		}
	}
}

struct rsim_rope *rsim_alloc_rope(int nmasses)
{
	struct rsim_rope *rope;

	if(!(rope = malloc(sizeof *rope))) {
		return 0;
	}
	if(rsim_init_rope(rope, nmasses) == -1) {
		free(rope);
		return 0;
	}
	return rope;
}

void rsim_free_rope(struct rsim_rope *rope)
{
	rsim_destroy_rope(rope);
	free(rope);
}

int rsim_init_rope(struct rsim_rope *rope, int nmasses)
{
	memset(rope, 0, sizeof *rope);

	if(!(rope->masses = calloc(nmasses, sizeof *rope->masses))) {
		return -1;
	}
	rope->num_masses = nmasses;

	if(!(rope->springs = malloc(nmasses * nmasses * sizeof *rope->springs))) {
		free(rope->masses);
		return -1;
	}
	memset(rope->springs, 0xff, nmasses * nmasses * sizeof *rope->springs);
	return 0;
}

void rsim_destroy_rope(struct rsim_rope *rope)
{
	free(rope->masses);
	free(rope->springs);
}

int rsim_set_rope_spring(struct rsim_rope *rope, int ma, int mb, float k, float rlen)
{
	cgm_vec3 dir;
	struct rsim_spring *spr, *rps;

	if(ma == mb || ma < 0 || ma >= rope->num_masses || mb < 0 || mb >= rope->num_masses) {
		return -1;
	}

	if(rlen == RSIM_RLEN_DEFAULT) {
		dir = rope->masses[ma].p;
		cgm_vsub(&dir, &rope->masses[mb].p);
		rlen = cgm_vlength(&dir);
	}

	spr = rope->springs + ma * rope->num_masses + mb;
	rps = rope->springs + mb * rope->num_masses + ma;
	spr->k = rps->k = fabs(k);
	spr->rest_len = rps->rest_len = rlen;
	return 0;
}

int rsim_have_spring(struct rsim_rope *rope, int ma, int mb)
{
	return getspring(rope, ma, mb) ? 1 : 0;
}

int rsim_freeze_rope_mass(struct rsim_rope *rope, struct rsim_mass *m)
{
	if(m->fixed) return -1;

	m->fixed = 1;
	return 0;
}

int rsim_unfreeze_rope_mass(struct rsim_rope *rope, struct rsim_mass *m)
{
	if(!m->fixed) return -1;

	m->fixed = 0;
	return 0;
}
