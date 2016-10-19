/*
metasurf - a library for implicit surface polygonization
Copyright (C) 2011-2015  John Tsiombikas <nuclear@member.fsf.org>

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

#ifndef METASURF_H_
#define METASURF_H_

#define MSURF_GREATER	1
#define MSURF_LESS		0

struct metasurface;

typedef float (*msurf_eval_func_t)(struct metasurface *ms, float, float, float);
typedef void (*msurf_vertex_func_t)(struct metasurface *ms, float, float, float);
typedef void (*msurf_normal_func_t)(struct metasurface *ms, float, float, float);

#ifdef __cplusplus
extern "C" {
#endif

struct metasurface *msurf_create(void);
void msurf_free(struct metasurface *ms);

void msurf_set_user_data(struct metasurface *ms, void *udata);
void *msurf_get_user_data(struct metasurface *ms);

/* which is inside above or below the threshold */
void msurf_set_inside(struct metasurface *ms, int inside);
int msurf_get_inside(struct metasurface *ms);

/* set a scalar field evaluator function */
void msurf_eval_func(struct metasurface *ms, msurf_eval_func_t func);

/* set a generated vertex callback function */
void msurf_vertex_func(struct metasurface *ms, msurf_vertex_func_t func);

/* set a generated surface normal callback function (unused yet) */
void msurf_normal_func(struct metasurface *ms, msurf_normal_func_t func);

/* set the bounding box (default: -1, -1, -1, 1, 1, 1)
 * keep this as tight as possible to avoid wasting grid resolution
 */
void msurf_set_bounds(struct metasurface *ms, float xmin, float ymin, float zmin, float xmax, float ymax, float zmax);
void msurf_get_bounds(struct metasurface *ms, float *xmin, float *ymin, float *zmin, float *xmax, float *ymax, float *zmax);

/* resolution of the 3D evaluation grid, the bigger, the better, the slower
 * (default: 40, 40, 40)
 */
void msurf_set_resolution(struct metasurface *ms, int xres, int yres, int zres);
void msurf_get_resolution(struct metasurface *ms, int *xres, int *yres, int *zres);

/* isosurface threshold value (default: 0) */
void msurf_set_threshold(struct metasurface *ms, float thres);
float msurf_get_threshold(struct metasurface *ms);


/* finally call this to perform the polygonization */
int msurf_polygonize(struct metasurface *ms);


#ifdef __cplusplus
}
#endif

#endif	/* METASURF_H_ */
