#ifndef ROPESIM_H_
#define ROPESIM_H_

#include "cgmath/cgmath.h"

struct rsim_mass {
	cgm_vec3 p;
	cgm_vec3 v;
	cgm_vec3 f;
	float m;
	int fixed;

	/* used for stringing fixed masses together */
	struct rsim_mass *next;
};

struct rsim_spring {
	float rest_len, k;
};

struct rsim_rope {
	struct rsim_mass *masses;
	int num_masses;
	struct rsim_spring *springs;	/* adjacency matrix */

	struct rsim_rope *next;
};

struct rsim_world {
	struct rsim_rope *ropes;	/* list of ropes to simulate */

	cgm_vec3 grav, extforce;
	float damping;

	float udt;	/* microstepping delta (valid if > 0) */
	float udelta_acc;	/* dt leftover accumulator for microstepping */
};

int rsim_init(struct rsim_world *rsim);
void rsim_destroy(struct rsim_world *rsim);

int rsim_add_rope(struct rsim_world *rsim, struct rsim_rope *rope);

void rsim_step(struct rsim_world *rsim, float dt);

struct rsim_rope *rsim_alloc_rope(int nmasses);
void rsim_free_rope(struct rsim_rope *rope);
int rsim_init_rope(struct rsim_rope *rope, int nmasses);
void rsim_destroy_rope(struct rsim_rope *rope);

#define RSIM_RLEN_DEFAULT	(-1.0f)
int rsim_set_rope_spring(struct rsim_rope *rope, int ma, int mb, float k, float rlen);
int rsim_have_spring(struct rsim_rope *rope, int ma, int mb);

int rsim_freeze_rope_mass(struct rsim_rope *rope, struct rsim_mass *m);
int rsim_unfreeze_rope_mass(struct rsim_rope *rope, struct rsim_mass *m);

#endif	/* ROPESIM_H_ */
