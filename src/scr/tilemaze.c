#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilemaze.h"
#include "mesh.h"
#include "treestor.h"
#include "dynarr.h"
#include "scene.h"

struct object {
	char *name;
	struct g3d_mesh mesh;

	void *texels;
	int tex_width, tex_height;

	struct object *next;
};

struct tile {
	char *name;
	struct object *objlist;
};

struct tilemaze {
	struct tile *tileset;
	int num_tiles;

	struct object *objects;
};

void free_object(struct object *obj);

struct tilemaze *load_tilemaze(const char *fname)
{
	struct tilemaze *tmz;

	if(!(tmz = calloc(sizeof *tmz, 1))) {
		return 0;
	}
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

struct tile *load_tileset(const char *fname, int *count)
{
	int i;
	struct tile *tiles = 0, *tile;
	struct ts_node *ts = 0, *node;
	struct ts_attr *attr;
	struct object *obj;

	if(!(ts = ts_load(fname))) {
		fprintf(stderr, "load_tileset: failed to load: %s\n", fname);
		goto err;
	}
	if(strcmp(ts->name, "tileset") != 0) {
		fprintf(stderr, "load_tileset: %s is not a tileset file\n", fname);
		goto err;
	}

	if(!(tiles = dynarr_alloc(sizeof *tiles, 0))) {
		fprintf(stderr, "load_tileset: failed to create tiles array\n");
		goto err;
	}

	node = ts->child_list;
	while(node) {
		if(strcmp(node->name, "tile") != 0) {
			fprintf(stderr, "load_tileset: skipping unexpected node %s in %s\n", node->name, fname);
			node = node->next;
			continue;
		}

		if(!(tile = malloc(sizeof *tile))) {
			fprintf(stderr, "load_tileset: failed to allocate tile\n");
			goto err;
		}
		if(!(tile->name = malloc(strlen(node->name) + 1))) {
			fprintf(stderr, "load_tileset: failed to allocate tile name\n");
			free(tile);
			goto err;
		}
		strcpy(tile->name, node->name);
		tile->objlist = 0;

		attr = node->attr_list;
		while(attr) {
			/*
			if(strcmp(attr->name, "scn") == 0) {
				struct object *last;
				if(!(obj = load_objlist(attr->value.str, &last))) {
					free(tile);
					goto err;
				}
				last->next = tile->objlist;
				tile->objlist = obj;
			}
			*/
			attr = attr->next;
		}

		node = node->next;
	}

err:
	if(tiles) {
		for(i=0; i<dynarr_size(tiles); i++) {
			free(tiles[i].name);
			obj = tiles[i].objlist;
			while(obj) {
				struct object *tmp = obj;
				obj = obj->next;
				free_object(obj);
			}
		}
		dynarr_free(tiles);
	}
	if(ts) ts_free_tree(ts);
	return 0;
}

void free_object(struct object *obj)
{
	if(!obj) return;

	free(obj->name);
	free(obj->texels);
	destroy_mesh(&obj->mesh);

	free(obj);
}
