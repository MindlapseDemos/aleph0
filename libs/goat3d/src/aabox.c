/*
goat3d - 3D scene, and animation file format library.
Copyright (C) 2013-2018  John Tsiombikas <nuclear@member.fsf.org>

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
#include <float.h>
#include "aabox.h"

void g3dimpl_aabox_init(struct aabox *box)
{
	cgm_vcons(&box->bmin, FLT_MAX, FLT_MAX, FLT_MAX);
	cgm_vcons(&box->bmax, -FLT_MAX, -FLT_MAX, -FLT_MAX);
}

void g3dimpl_aabox_cons(struct aabox *box, float x0, float y0, float z0,
		float x1, float y1, float z1)
{
	cgm_vcons(&box->bmin, x0, y0, z0);
	cgm_vcons(&box->bmax, x1, y1, z1);
}


int g3dimpl_aabox_equal(const struct aabox *a, const struct aabox *b)
{
	if(a->bmin.x != b->bmin.x || a->bmin.y != b->bmin.y || a->bmin.z != b->bmin.z) {
		return 0;
	}
	if(a->bmax.x != b->bmax.x || a->bmax.y != b->bmax.y || a->bmax.z != b->bmax.z) {
		return 0;
	}
	return 1;
}

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
void g3dimpl_aabox_union(struct aabox *res, const struct aabox *a, const struct aabox *b)
{
	res->bmin.x = MIN(a->bmin.x, b->bmin.x);
	res->bmin.y = MIN(a->bmin.y, b->bmin.y);
	res->bmin.z = MIN(a->bmin.z, b->bmin.z);
	res->bmax.x = MAX(a->bmax.x, b->bmax.x);
	res->bmax.y = MAX(a->bmax.y, b->bmax.y);
	res->bmax.z = MAX(a->bmax.z, b->bmax.z);
}
