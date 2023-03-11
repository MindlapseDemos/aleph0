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
#ifndef AABOX_H_
#define AABOX_H_

#include "cgmath/cgmath.h"

struct aabox {
	cgm_vec3 bmin, bmax;
};

void g3dimpl_aabox_init(struct aabox *box);
void g3dimpl_aabox_cons(struct aabox *box, float x0, float y0, float z0,
		float x1, float y1, float z1);

int g3dimpl_aabox_equal(const struct aabox *a, const struct aabox *b);

void g3dimpl_aabox_union(struct aabox *res, const struct aabox *a,
		const struct aabox *b);

#endif	/* AABOX_H_ */
