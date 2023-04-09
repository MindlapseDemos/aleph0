/*
goat3d - 3D scene, and animation file format library.
Copyright (C) 2013-2023  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <ctype.h>
#include "goat3d.h"
#include "g3dscn.h"
#include "log.h"
#include "json.h"

static struct goat3d_material *read_material(struct goat3d *g, struct json_obj *jmtl);

int g3dimpl_loadgltf(struct goat3d *g, struct goat3d_io *io)
{
	long i, filesz;
	char *filebuf;
	struct json_obj root;
	struct json_value *jval;
	struct json_item *jitem;
	struct goat3d_material *mtl;

	if(!(filebuf = malloc(4096))) {
		goat3d_logmsg(LOG_ERROR, "goat3d_load: failed to allocate file buffer\n");
		return -1;
	}
	filesz = io->read(filebuf, 4096, io->cls);
	if(filesz < 2) {
		free(filebuf);
		return -1;
	}
	for(i=0; i<filesz; i++) {
		if(!isspace(filebuf[i])) {
			if(filebuf[i] != '{') {
				free(filebuf);
				return -1;		/* not json */
			}
			break;
		}
	}
	free(filebuf);

	/* alright, it looks like json, load into memory and parse it to continue */
	filesz = io->seek(0, SEEK_END, io->cls);
	io->seek(0, SEEK_SET, io->cls);
	if(!(filebuf = malloc(filesz + 1))) {
		goat3d_logmsg(LOG_ERROR, "goat3d_load: failed to load file into memory\n");
		return -1;
	}
	if(io->read(filebuf, filesz, io->cls) != filesz) {
		goat3d_logmsg(LOG_ERROR, "goat3d_load: EOF while reading file\n");
		free(filebuf);
		return -1;
	}
	filebuf[filesz] = 0;

	json_init_obj(&root);
	if(json_parse(&root, filebuf) == -1) {
		free(filebuf);
		return -1;
	}
	free(filebuf);

	/* a valid gltf file needs to have an "asset" node with a version number */
	if(!(jval = json_lookup(&root, "asset.version"))) {
		json_destroy_obj(&root);
		return -1;
	}

	/* read all materials */
	if((jitem = json_find_item(&root, "materials"))) {
		if(jitem->val.type != JSON_ARR) {
			goat3d_logmsg(LOG_ERROR, "goat3d_load: gltf materials value is not an array!\n");
			goto skipmtl;
		}

		for(i=0; i<jitem->val.arr.size; i++) {
			jval = jitem->val.arr.val + i;

			if(jval->type != JSON_OBJ) {
				goat3d_logmsg(LOG_ERROR, "goat3d_load: gltf material is not a json object!\n");
				continue;
			}

			if((mtl = read_material(g, &jval->obj))) {
				goat3d_add_mtl(g, mtl);
			}
		}
	}
skipmtl:

	/* ... */

	json_destroy_obj(&root);
	return 0;
}

static int jarr_to_vec(struct json_arr *jarr, float *vec)
{
	int i;

	if(jarr->size < 3 || jarr->size > 4) {
		return -1;
	}

	for(i=0; i<4; i++) {
		if(i >= jarr->size) {
			vec[i] = 0;
			continue;
		}
		if(jarr->val[i].type != JSON_NUM) {
			return -1;
		}
		vec[i] = jarr->val[i].num;
	}
	return jarr->size;
}

static int jval_to_vec(struct json_value *jval, float *vec)
{
	if(jval->type != JSON_ARR) return -1;
	return jarr_to_vec(&jval->arr, vec);
}

static struct goat3d_material *read_material(struct goat3d *g, struct json_obj *jmtl)
{
	struct goat3d_material *mtl;
	const char *str;
	struct json_value *jval;
	float color[4], specular[4], roughness, metal, ior;
	int nelem;

	if(!(mtl = malloc(sizeof *mtl)) || g3dimpl_mtl_init(mtl) == -1) {
		free(mtl);
		goat3d_logmsg(LOG_ERROR, "read_material: failed to allocate material\n");
		return 0;
	}

	if((str = json_lookup_str(jmtl, "name", 0))) {
		goat3d_set_mtl_name(mtl, str);
	}

	if((jval = json_lookup(jmtl, "pbrMetallicRoughness.baseColorFactor")) &&
			(nelem = jval_to_vec(jval, color)) != -1) {
		goat3d_set_mtl_attrib(mtl, "diffuse", color);
	}
	/* TODO textures */

	if((roughness = json_lookup_num(jmtl, "pbrMetallicRoughness.roughnessFactor", -1.0)) >= 0) {
		goat3d_set_mtl_attrib1f(mtl, "roughness", roughness);
		goat3d_set_mtl_attrib1f(mtl, GOAT3D_MAT_ATTR_SHININESS, (1.0f - roughness) * 100.0f + 1.0f);
	}
	if((metal = json_lookup_num(jmtl, "pbrMetallicRoughness.metallicFactor", -1.0)) >= 0) {
		goat3d_set_mtl_attrib1f(mtl, "metal", metal);
	}
	if((jval = json_lookup(jmtl, "extensions.KHR_materials_specular.specularColorFactor")) &&
			(nelem = jval_to_vec(jval, specular)) != -1) {
		goat3d_set_mtl_attrib(mtl, GOAT3D_MAT_ATTR_SPECULAR, specular);
	}
	if((ior = json_lookup_num(jmtl, "extensions.KHR_materials_ior.ior", -1.0)) >= 0) {
		goat3d_set_mtl_attrib1f(mtl, GOAT3D_MAT_ATTR_IOR, ior);
	}
	/* TODO more attributes */

	return mtl;
}
