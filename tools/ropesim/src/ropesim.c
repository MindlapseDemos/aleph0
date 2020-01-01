#include <stdio.h>
#include <stdlib.h>
#include "ropesim.h"

static void step(struct rsim_world *rsim, struct rsim_rope *rope, float dt);

int rsim_init(struct rsim_world *rsim)
{
	rsim->ropes = 0;
	cgm_vcons(&rsim->grav, 0, -9.807, 0);
	rsim->damping = 0.995;
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

/* actual step function, called by rsim_step in a loop to microstep or once if
 * microstepping is disabled
 */
static void step(struct rsim_world *rsim, struct rsim_rope *rope, float dt)
{
	int i;
	cgm_vec3 npos, f, force;
	float inv_damp = rsim->damping == 0.0f ? 1.0f : 1.0f / rsim->damping;
	struct rsim_mass *mass = rope->masses;

	force.x = rsim->grav.x + rsim->extforce.x;
	force.y = rsim->grav.y + rsim->extforce.y;
	force.z = rsim->grav.z + rsim->extforce.z;

	for(i=0; i<rope->num_masses; i++) {
		if(mass->fixed) {
			mass++;
			continue;
		}

		npos = mass->p;

		npos.x = mass->p.x + mass->v.x * dt;
		npos.y = mass->p.y + mass->v.y * dt;
		npos.z = mass->p.z + mass->v.z * dt;

		f.x = force.x;/* + spring forces */
		f.y = force.y;
		f.z = force.z;

		mass->v.x = ((mass->v.x - mass->v.x * inv_damp) + f.x) * dt;
		mass->v.y = ((mass->v.y - mass->v.y * inv_damp) + f.y) * dt;
		mass->v.z = ((mass->v.z - mass->v.z * inv_damp) + f.z) * dt;

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

struct rsim_rope *rsim_alloc_rope(int nmasses, int nsprings)
{
	struct rsim_rope *rope;

	if(!(rope = malloc(sizeof *rope))) {
		return 0;
	}
	if(rsim_init_rope(rope, nmasses, nsprings) == -1) {
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

int rsim_init_rope(struct rsim_rope *rope, int nmasses, int nsprings)
{
	memset(rope, 0, sizeof *rope);

	if(!(rope->masses = calloc(nmasses, sizeof *rope->masses))) {
		return -1;
	}
	if(!(rope->springs = calloc(nsprings, sizeof *rope->springs))) {
		free(rope->masses);
		return -1;
	}
	rope->num_masses = nmasses;
	rope->num_springs = nsprings;
	return 0;
}

void rsim_destroy_rope(struct rsim_rope *rope)
{
	free(rope->masses);
	free(rope->springs);
}

int rsim_freeze_rope_mass(struct rsim_rope *rope, struct rsim_mass *m)
{
	if(m->fixed) return -1;

	m->fixed = 1;
	m->next = rope->fixedlist;
	rope->fixedlist = m;
	return 0;
}

int rsim_unfreeze_rope_mass(struct rsim_rope *rope, struct rsim_mass *m)
{
	struct rsim_mass *it, dummy;

	if(!m->fixed) return -1;

	dummy.next = rope->fixedlist;
	it = &dummy;
	while(it->next) {
		if(it->next == m) {
			m->fixed = 0;
			it->next = m->next;
			m->next = 0;
			break;
		}
		it = it->next;
	}
	rope->fixedlist = dummy.next;

	return m->fixed ? -1 : 0;
}