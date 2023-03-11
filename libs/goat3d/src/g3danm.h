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
#ifndef G3DANM_H_
#define G3DANM_H_

#include "goat3d.h"
#include "track.h"

struct goat3d_track {
	char *name;
	enum goat3d_track_type type;
	struct anm_track trk[4];
	struct goat3d_node *node;	/* node associated with this track */
};

struct goat3d_anim {
	char *name;
	struct goat3d_track **tracks;	/* dynarr */
};

int g3dimpl_anim_init(struct goat3d_anim *anim);
void g3dimpl_anim_destroy(struct goat3d_anim *anim);

const char *g3dimpl_trktypestr(enum goat3d_track_type type);

#endif	/* G3DANM_H_ */
