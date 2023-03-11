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
#include "g3danm.h"
#include "dynarr.h"
#include "log.h"

int g3dimpl_anim_init(struct goat3d_anim *anim)
{
	anim->name = 0;
	if(!(anim->tracks = dynarr_alloc(0, sizeof *anim->tracks))) {
		goat3d_logmsg(LOG_ERROR, "g3dimpl_anim_init: failed to allocate tracks array\n");
		return -1;
	}
	return 0;
}

void g3dimpl_anim_destroy(struct goat3d_anim *anim)
{
	int i, num;

	if(!anim) return;

	free(anim->name);
	num = dynarr_size(anim->tracks);
	for(i=0; i<num; i++) {
		goat3d_destroy_track(anim->tracks[i]);
	}
	dynarr_free(anim->tracks);
}


const char *g3dimpl_trktypestr(enum goat3d_track_type type)
{
	switch(type) {
	case GOAT3D_TRACK_VAL:
		return "val";
	case GOAT3D_TRACK_VEC3:
		return "vec3";
	case GOAT3D_TRACK_VEC4:
		return "vec4";
	case GOAT3D_TRACK_QUAT:
		return "quat";
	case GOAT3D_TRACK_POS:
		return "pos";
	case GOAT3D_TRACK_ROT:
		return "rot";
	case GOAT3D_TRACK_SCALE:
		return "scale";
	}
	return 0;
}

