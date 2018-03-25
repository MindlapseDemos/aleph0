#include <stdlib.h>
#include "tilemaze.h"
#include "mesh.h"

struct object {
	char *name;
	struct g3d_mesh mesh;

	void *texels;
	int tex_width, tex_height;

	struct object *next;
};

struct tile {
	struct object *objlist;
};

struct tilemaze {
	struct tile *tileset;
	int num_tiles;

	struct object *objects;
};


struct tilemaze *load_tilemaze(const char *fname)
{
	struct tilemaze *tmz;

	if(!(tmz = calloc(sizeof *tmz, 1))) {
		return 0;
	}
	/* TODO */
	return tmz;
}

void destroy_tilemaze(struct tilemaze *tmz)
{
	if(!tmz) return;

	free(tmz->tileset);

	while(tmz->objects) {
		struct object *o = tmz->objects;
		tmz->objects = tmz->objects->next;

		free(o->mesh.varr);
		free(o->mesh.iarr);
		free(o);
	}

	free(tmz);
}

void update_tilemaze(struct tilemaze *tmz)
{
}

void draw_tilemaze(struct tilemaze *tmz)
{
}
