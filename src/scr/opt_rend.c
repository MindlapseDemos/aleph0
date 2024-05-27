#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opt_rend.h"
#include "demo.h"
#include "screen.h"

/* #define USE_HALFRES_INTERPOLATION_RASTERIZER */

#define BLOB_SIZES_NUM_MAX 16
#define BLOB_SIZEX_PAD 4
#define MAX_BLOB_COLOR 15

#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)

#define FP_RAST 16

#define R_OVER_SHIFT 2
#define G_OVER_SHIFT 0
#define B_OVER_SHIFT 1
#define R_UNDER_SHIFT 2
#define G_UNDER_SHIFT 3
#define B_UNDER_SHIFT 0


static BlobData blobData[BLOB_SIZES_NUM_MAX][BLOB_SIZEX_PAD];

static int isBlobGfxInit = 0;

static int clipValY = 0;


typedef struct Edge
{
	int xs;
	int x, y, z;
	int c, u, v;
}Edge;

typedef struct Gradients
{
	int dx, dy, dz;
	int dc, du, dv;
}Gradients;


static Edge* leftEdge = 0;
static Edge* rightEdge = 0;

static int edgeYmin = 0;
static int edgeYmax = 0;

static Gradients grads;


static void(*drawEdge)(int, int);
static void drawEdgeFlat(int y, int dx);
static void drawEdgeGouraudClipY(int y, int dx);
static void drawEdgeTexturedGouraudClipY(int y, int dx);

static void (*prepareEdgeList)(Vertex3D*, Vertex3D*);
static void prepareEdgeListFlat(Vertex3D* v0, Vertex3D* v1);
static void prepareEdgeListGouraudClipY(Vertex3D* v0, Vertex3D* v1);
static void prepareEdgeListTexturedGouraudClipY(Vertex3D* v0, Vertex3D* v1);

static int renderingMode;

static uint16_t polyColor = 0xffff;

static int texWidth = 256;
static int texHeight = 256;
static unsigned char* texBmp;

static unsigned short* zBuffer;


void clearZbuffer()
{
	memset(zBuffer, 255, FB_WIDTH * FB_HEIGHT * sizeof(unsigned short));
}

void setMainTexture(int width, int height, unsigned char *texData)
{
	texWidth = width;
	texHeight = height;
	texBmp = texData;
}

void setRenderingMode(int mode)
{
	switch (mode) {
		case OPT_RAST_FLAT:
			prepareEdgeList = prepareEdgeListFlat;
			drawEdge = drawEdgeFlat;
		break;

		case OPT_RAST_GOURAUD_CLIP_Y:
			prepareEdgeList = prepareEdgeListGouraudClipY;
			drawEdge = drawEdgeGouraudClipY;
		break;

		case OPT_RAST_TEXTURED_GOURAUD_CLIP_Y:
			prepareEdgeList = prepareEdgeListTexturedGouraudClipY;
			drawEdge = drawEdgeTexturedGouraudClipY;
		break;
	}

	renderingMode = mode;
}

int getRenderingMode()
{
	return renderingMode;
}

void initOptRasterizer()
{
	leftEdge = (Edge*)malloc(FB_HEIGHT * sizeof(Edge));
	rightEdge = (Edge*)malloc(FB_HEIGHT * sizeof(Edge));

	setRenderingMode(OPT_RAST_FLAT);

	zBuffer = (unsigned short*)malloc(FB_WIDTH * FB_HEIGHT * sizeof(unsigned short));
}

void freeOptRasterizer()
{
	free(leftEdge);
	free(rightEdge);
	free(zBuffer);
}

void setClipValY(int y)
{
	clipValY = y;
}

static void drawEdgeFlat(int ys, int dx)
{
	const Edge* l = &leftEdge[ys];
	const Edge* r = &rightEdge[ys];

	const int xs0 = l->xs;
	const int xs1 = r->xs;

	uint16_t* vram = fb_pixels + ys * FB_WIDTH + xs0;

	int xs;
	for (xs = xs0; xs < xs1; xs++) {
		*vram++ = polyColor;
	}
}

static void drawEdgeGouraudClipY(int ys, int dx)
{
	const Edge* l = &leftEdge[ys];
	const Edge* r = &rightEdge[ys];

	const int xs0 = l->xs;
	const int xs1 = r->xs;
	const int c0 = l->c;
	const int c1 = r->c;
	const int y0 = l->y;
	const int y1 = r->y;

	const int offset = ys * FB_WIDTH + xs0;
	const int z0 = l->z;
	const int z1 = r->z;
	unsigned short* zBuff = zBuffer + offset;
	uint16_t* vram = fb_pixels + offset;

	const int dc = grads.dc;
	const int dy = grads.dy;
	const int dz = grads.dz;

	int c = INT_TO_FIXED(c0, FP_RAST);
	int y = INT_TO_FIXED(y0, FP_RAST);
	int z = INT_TO_FIXED(z0, FP_RAST);

	int x;
	for (x = xs0; x < xs1; x++) {
		const unsigned short zz = (unsigned short)(FIXED_TO_INT(z, FP_RAST));
		if (zz < *zBuff) {
			const int cc = FIXED_TO_INT(c, FP_RAST);
			const int b = cc >> (3 + B_OVER_SHIFT);

			const int yy = FIXED_TO_INT(y, FP_RAST);
			if (yy >= clipValY) {
				const int r = cc >> (3 + R_OVER_SHIFT);
				const int g = cc >> (2 + G_OVER_SHIFT);
				*vram = (r << 11) | (g << 5) | b;
			} else {
				*vram += (1 + (b >> 3));
			}
			*zBuff = zz;
		}
		z += dz;
		zBuff++;
		vram++;
		c += dc;
		y += dy;
	}
}

#ifndef USE_HALFRES_INTERPOLATION_RASTERIZER
static void drawEdgeTexturedGouraudClipY(int ys, int dx)
{
	const Edge* l = &leftEdge[ys];
	const Edge* r = &rightEdge[ys];

	const int xs0 = l->xs;
	const int xs1 = r->xs;
	const int c0 = l->c;
	const int c1 = r->c;
	const int u0 = l->u;
	const int u1 = r->u;
	const int v0 = l->v;
	const int v1 = r->v;
	const int y0 = l->y;
	const int y1 = r->y;
	const int z0 = l->z;
	const int z1 = r->z;

	uint16_t* vram = fb_pixels + ys * FB_WIDTH + xs0;
	unsigned short* zBuff = zBuffer + ys * FB_WIDTH + xs0;

	const int dc = grads.dc;
	const int du = grads.du;
	const int dv = grads.dv;
	const int dy = grads.dy;
	const int dz = grads.dz;

	int c = INT_TO_FIXED(c0, FP_RAST);
	int u = INT_TO_FIXED(u0, FP_RAST);
	int v = INT_TO_FIXED(v0, FP_RAST);
	int y = INT_TO_FIXED(y0, FP_RAST);
	int z = INT_TO_FIXED(z0, FP_RAST);

	int x;
	for (x = xs0; x < xs1; x++) {
		const unsigned short zz = (unsigned short)(FIXED_TO_INT(z, FP_RAST));
		if (zz < *zBuff) {
			const int cc = FIXED_TO_INT(c, FP_RAST);
			const int uu = FIXED_TO_INT(u, FP_RAST);
			const int vv = FIXED_TO_INT(v, FP_RAST);
			const int ct = texBmp[(vv & (texHeight - 1)) * texWidth + (uu & (texWidth - 1))];
			int b = (ct * cc) >> (8 + 3);

			const int yy = FIXED_TO_INT(y, FP_RAST);
			if (yy >= clipValY) {
				const int r = 7 + ((ct * cc) >> (8 + 2));
				const int g = (ct * cc) >> (8 + 1);
				*vram = (r << 11) | (g << 5) | b;
			} else {
				*vram += (1 + ((ct & 31) >> 3));
			}
			*zBuff = zz;
		}
		zBuff++;
		vram++;
		c += dc;
		u += du;
		v += dv;
		y += dy;
		z += dz;
	}
}
#else
static void drawEdgeTexturedGouraudClipY(int ys, int dx)
{
	const Edge* l = &leftEdge[ys];
	const Edge* r = &rightEdge[ys];

	const int c0 = l->c;
	const int c1 = r->c;
	const int u0 = l->u;
	const int u1 = r->u;
	const int v0 = l->v;
	const int v1 = r->v;
	const int y0 = l->y;
	const int y1 = r->y;
	const int z0 = l->z;
	const int z1 = r->z;

	const int offset = ys * FB_WIDTH + l->xs;
	uint16_t* vram = fb_pixels + offset;
	unsigned short* zBuff = zBuffer + offset;
	int xLength = r->xs - l->xs + 1;

	const int dc = grads.dc;
	const int du = grads.du;
	const int dv = grads.dv;
	const int dy = grads.dy;
	const int dz = grads.dz;

	int c = INT_TO_FIXED(c0, FP_RAST);
	int u = INT_TO_FIXED(u0, FP_RAST);
	int v = INT_TO_FIXED(v0, FP_RAST);
	int y = INT_TO_FIXED(y0, FP_RAST);
	int z = INT_TO_FIXED(z0, FP_RAST);

	uint32_t* vram32;

	if (l->xs & 1) {
		const unsigned short zz = (unsigned short)(FIXED_TO_INT(z, FP_RAST));
		if (zz < *zBuff) {
			const int cc = FIXED_TO_INT(c, FP_RAST); const int uu = FIXED_TO_INT(u, FP_RAST); const int vv = FIXED_TO_INT(v, FP_RAST);
			const int ct = texBmp[(vv & (texHeight - 1)) * texWidth + (uu & (texWidth - 1))];
			int b = (ct * cc) >> (8 + 3);

			const int yy = FIXED_TO_INT(y, FP_RAST);
			if (yy >= clipValY) {
				const int r = 7 + ((ct * cc) >> (8 + 2));
				const int g = (ct * cc) >> (8 + 1);
				*vram = (r << 11) | (g << 5) | b;
			} else {
				*vram += (1 + ((ct & 31) >> 3));
			}
			*zBuff = zz;
		}
		zBuff++;
		vram++;
		c += dc; u += du; v += dv; y += dy; z += dz;
		--xLength;
	}

	{
		vram32 = (uint32_t*)vram;
		{
			while (xLength > 1) {
				const unsigned short zz = (unsigned short)(FIXED_TO_INT(z, FP_RAST));
				int pix;
				if (zz < *(zBuff+1)) {
					const int cc = FIXED_TO_INT(c, FP_RAST); const int uu = FIXED_TO_INT(u, FP_RAST); const int vv = FIXED_TO_INT(v, FP_RAST);
					const int ct = texBmp[(vv & (texHeight - 1)) * texWidth + (uu & (texWidth - 1))];
					int b = (ct * cc) >> (8 + 3);

					const int yy = FIXED_TO_INT(y, FP_RAST);
					if (yy >= clipValY) {
						const int r = 7 + ((ct * cc) >> (8 + 2));
						const int g = (ct * cc) >> (8 + 1);
						pix = (r << 11) | (g << 5) | b;
					} else {
						pix = (*vram32 >> 16) + (1 + ((ct & 31) >> 3));
					}
					*vram32 = (pix << 16) | pix;
					*zBuff = zz;
					*(zBuff + 1) = zz;
				}
				zBuff+=2;
				vram32++;
				c += 2 * dc; u += 2 * du; v += 2 * dv; y += 2 * dy; z += 2 * dz;
				xLength -= 2;
			}
		}
	}

	vram = (uint16_t*)vram32;
	if (xLength == 1) {
		const unsigned short zz = (unsigned short)(FIXED_TO_INT(z, FP_RAST));
		if (zz < *zBuff) {
			const int cc = FIXED_TO_INT(c, FP_RAST); const int uu = FIXED_TO_INT(u, FP_RAST); const int vv = FIXED_TO_INT(v, FP_RAST);
			const int ct = texBmp[(vv & (texHeight - 1)) * texWidth + (uu & (texWidth - 1))];
			int b = (ct * cc) >> (8 + 3);

			const int yy = FIXED_TO_INT(y, FP_RAST);
			if (yy >= clipValY) {
				const int r = 7 + ((ct * cc) >> (8 + 2));
				const int g = (ct * cc) >> (8 + 1);
				*vram = (r << 11) | (g << 5) | b;
			} else {
				*vram += (1 + ((ct & 31) >> 3));
			}
			*zBuff = zz;
		}
	}
}
#endif

static void prepareEdgeListFlat(Vertex3D* v0, Vertex3D* v1)
{
	const int yp0 = v0->ys;
	int yp1 = v1->ys;

	Edge* edgeListToWrite;

	if (yp0 == yp1) return;

	if (yp0 < yp1) {
		edgeListToWrite = leftEdge;
	}
	else {
		edgeListToWrite = rightEdge;
		{
			Vertex3D* vTemp = v0;
			v0 = v1;
			v1 = vTemp;
		}
	}

	{
		const int xs0 = v0->xs; const int ys0 = v0->ys;
		const int xs1 = v1->xs; const int ys1 = v1->ys;

		const int dys = ys1 - ys0;
		const int dxs = INT_TO_FIXED(xs1 - xs0, FP_RAST) / dys;

		int xp = INT_TO_FIXED(xs0, FP_RAST);
		int yp = ys0;

		do {
			if (yp >= 0 && yp < FB_HEIGHT) {
				edgeListToWrite[yp].xs = FIXED_TO_INT(xp, FP_RAST);
			}
			xp += dxs;
		} while (yp++ < ys1);
	}
}

static void prepareEdgeListGouraudClipY(Vertex3D* v0, Vertex3D* v1)
{
	const int yp0 = v0->ys;
	int yp1 = v1->ys;

	Edge* edgeListToWrite;

	if (yp0 == yp1) return;

	if (yp0 < yp1) {
		edgeListToWrite = leftEdge;
	}
	else {
		edgeListToWrite = rightEdge;
		{
			Vertex3D* vTemp = v0;
			v0 = v1;
			v1 = vTemp;
		}
	}

	{
		const int xs0 = v0->xs; const int ys0 = v0->ys;
		const int xs1 = v1->xs; const int ys1 = v1->ys;
		const int dys = ys1 - ys0;

		const int dxs = INT_TO_FIXED(xs1 - xs0, FP_RAST) / dys;
		int xp = INT_TO_FIXED(xs0, FP_RAST);
		int yp = ys0;

		if (edgeListToWrite == rightEdge) {
			do {
				if (yp >= 0 && yp < FB_HEIGHT) {
					edgeListToWrite[yp].xs = FIXED_TO_INT(xp, FP_RAST);
				}
				xp += dxs;
			} while (yp++ < ys1);
		} else {
			const int c0 = v0->c; const int c1 = v1->c;

			const int dc = INT_TO_FIXED(c1 - c0, FP_RAST) / dys;

			int c = INT_TO_FIXED(c0, FP_RAST);

			/* Extra interpolant to have 3D y interpolated only used for water floor object collision test */
			/* If I reuse those functions for other 3D I don't need to interpolate everything, will see how I'll refactor if things get slow (or even code duplicate) */
			const int y0 = v0->y; const int y1 = v1->y;
			const int dy = INT_TO_FIXED(y1 - y0, FP_RAST) / dys;
			int y = INT_TO_FIXED(y0, FP_RAST);

			/* for z-buffer */
			const int z0 = v0->z; const int z1 = v1->z;
			const int dz = INT_TO_FIXED(z1 - z0, FP_RAST) / dys;
			int z = INT_TO_FIXED(z0, FP_RAST);

			do {
				if (yp >= 0 && yp < FB_HEIGHT) {
					edgeListToWrite[yp].xs = FIXED_TO_INT(xp, FP_RAST);
					edgeListToWrite[yp].c = FIXED_TO_INT(c, FP_RAST);

					/* likewise */
					edgeListToWrite[yp].y = FIXED_TO_INT(y, FP_RAST);
					edgeListToWrite[yp].z = FIXED_TO_INT(z, FP_RAST);
				}
				xp += dxs;
				c += dc;

				/* likewise */
				y += dy;
				z += dz;

			} while (yp++ < ys1);
		}
	}
}

static void prepareEdgeListTexturedGouraudClipY(Vertex3D* v0, Vertex3D* v1)
{
	const int yp0 = v0->ys;
	int yp1 = v1->ys;

	Edge* edgeListToWrite;

	if (yp0 == yp1) return;

	if (yp0 < yp1) {
		edgeListToWrite = leftEdge;
	}
	else {
		edgeListToWrite = rightEdge;
		{
			Vertex3D* vTemp = v0;
			v0 = v1;
			v1 = vTemp;
		}
	}

	{
		const int xs0 = v0->xs; const int ys0 = v0->ys;
		const int xs1 = v1->xs; const int ys1 = v1->ys;
		const int dys = ys1 - ys0;

		const int dxs = INT_TO_FIXED(xs1 - xs0, FP_RAST) / dys;
		int xp = INT_TO_FIXED(xs0, FP_RAST);
		int yp = ys0;

		if (edgeListToWrite == rightEdge) {
			do {
				if (yp >= 0 && yp < FB_HEIGHT) {
					edgeListToWrite[yp].xs = FIXED_TO_INT(xp, FP_RAST);
				}
				xp += dxs;
			} while (yp++ < ys1);
		}
		else {
			const int c0 = v0->c; const int c1 = v1->c;
			const int tc_u0 = v0->u; const int tc_u1 = v1->u;
			const int tc_v0 = v0->v; const int tc_v1 = v1->v;

			const int dc = INT_TO_FIXED(c1 - c0, FP_RAST) / dys;
			const int du = INT_TO_FIXED(tc_u1 - tc_u0, FP_RAST) / dys;
			const int dv = INT_TO_FIXED(tc_v1 - tc_v0, FP_RAST) / dys;

			int c = INT_TO_FIXED(c0, FP_RAST);
			int u = INT_TO_FIXED(tc_u0, FP_RAST);
			int v = INT_TO_FIXED(tc_v0, FP_RAST);

			/* Extra interpolant to have 3D y interpolated only used for water floor object collision test */
			/* If I reuse those functions for other 3D I don't need to interpolate everything, will see how I'll refactor if things get slow (or even code duplicate) */
			const int y0 = v0->y; const int y1 = v1->y;
			const int dy = INT_TO_FIXED(y1 - y0, FP_RAST) / dys;
			int y = INT_TO_FIXED(y0, FP_RAST);

			/* for z-buffer */
			const int z0 = v0->z; const int z1 = v1->z;
			const int dz = INT_TO_FIXED(z1 - z0, FP_RAST) / dys;
			int z = INT_TO_FIXED(z0, FP_RAST);

			do {
				if (yp >= 0 && yp < FB_HEIGHT) {
					edgeListToWrite[yp].xs = FIXED_TO_INT(xp, FP_RAST);
					edgeListToWrite[yp].c = FIXED_TO_INT(c, FP_RAST);
					edgeListToWrite[yp].u = FIXED_TO_INT(u, FP_RAST);
					edgeListToWrite[yp].v = FIXED_TO_INT(v, FP_RAST);

					/* likewise */
					edgeListToWrite[yp].y = FIXED_TO_INT(y, FP_RAST);
					edgeListToWrite[yp].z = FIXED_TO_INT(z, FP_RAST);
				}
				xp += dxs;
				c += dc;
				u += du;
				v += dv;

				/* likewise */
				y += dy;
				z += dz;

			} while (yp++ < ys1);
		}
	}
}

static void drawEdges()
{
	int ys;

	for (ys = edgeYmin; ys <= edgeYmax; ys++)
	{
		Edge* l = &leftEdge[ys];
		Edge* r = &rightEdge[ys];

		const int xs0 = l->xs;
		const int xs1 = r->xs;

		int dsx = xs1 - xs0;
		if (xs0 < 0) {
			if (xs0 != xs1) {
				l->u += ((-xs0 * grads.du) >> FP_RAST);
				l->v += ((-xs0 * grads.dv) >> FP_RAST);
				l->c += ((-xs0 * grads.dc) >> FP_RAST);
				l->y += ((-xs0 * grads.dy) >> FP_RAST);
			}
			l->xs = 0;
			dsx = xs1;
		}
		if (xs1 > FB_WIDTH - 1) {
			r->xs = FB_WIDTH - 1;
		}

		if (l->xs < r->xs) {
			drawEdge(ys, dsx);
		}
	}
}

static void calculateTriangleGradients(Vertex3D *v0, Vertex3D *v1, Vertex3D *v2)
{
	const int xs0 = v0->xs; const int xs1 = v1->xs; const int xs2 = v2->xs;
	const int ys0 = v0->ys; const int ys1 = v1->ys; const int ys2 = v2->ys;

	const int dys0 = ys0 - ys2; const int dys1 = ys1 - ys2;
	const int dd = (xs1 - xs2) * dys0 - (xs0 - xs2) * dys1;

	if (dd == 0) return;

	if (renderingMode >= OPT_RAST_GOURAUD_CLIP_Y) {
		const int c0 = v0->c; const int c1 = v1->c; const int c2 = v2->c;
		const int y0 = v0->y; const int y1 = v1->y; const int y2 = v2->y;
		const int z0 = v0->z; const int z1 = v1->z; const int z2 = v2->z;
		grads.dc = (((c1 - c2) * dys0 - (c0 - c2) * dys1) << FP_RAST) / dd;
		grads.dy = (((y1 - y2) * dys0 - (y0 - y2) * dys1) << FP_RAST) / dd;
		grads.dz = (((z1 - z2) * dys0 - (z0 - z2) * dys1) << FP_RAST) / dd;
	}

	if (renderingMode >= OPT_RAST_TEXTURED_GOURAUD_CLIP_Y) {
		const int tu0 = v0->u; const int tu1 = v1->u; const int tu2 = v2->u;
		const int tv0 = v0->v; const int tv1 = v1->v; const int tv2 = v2->v;
		grads.du = (((tu1 - tu2) * dys0 - (tu0 - tu2) * dys1) << FP_RAST) / dd;
		grads.dv = (((tv1 - tv2) * dys0 - (tv0 - tv2) * dys1) << FP_RAST) / dd;
	}
}

static void drawTriangle(Vertex3D* v0, Vertex3D* v1, Vertex3D* v2)
{
	const int ys0 = v0->ys;
	const int ys1 = v1->ys;
	const int ys2 = v2->ys;

	int yMin = ys0;
	int yMax = yMin;
	if (ys1 < yMin) yMin = ys1;
	if (ys1 > yMax) yMax = ys1;
	if (ys2 < yMin) yMin = ys2;
	if (ys2 > yMax) yMax = ys2;
	edgeYmin = yMin;
	edgeYmax = yMax;

	if (edgeYmin < 0) edgeYmin = 0;
	if (edgeYmax > FB_HEIGHT - 1) edgeYmax = FB_HEIGHT - 1;

	if (edgeYmax > edgeYmin) {
		prepareEdgeList(v0, v1);
		prepareEdgeList(v1, v2);
		prepareEdgeList(v2, v0);

		drawEdges();
	}
}

void renderPolygons(Object3D* obj, Vertex3D* screenVertices)
{
	int i, n;
	const int count = obj->mesh->indicesNum / 3;
	int* p = obj->mesh->index;

	for (i = 0; i < count; i++) {
		Vertex3D* v0 = &screenVertices[*p++];
		Vertex3D* v1 = &screenVertices[*p++];
		Vertex3D* v2 = &screenVertices[*p++];

		n = (v0->xs - v1->xs) * (v2->ys - v1->ys) - (v2->xs - v1->xs) * (v0->ys - v1->ys);
		if (n >= 0) {
			calculateTriangleGradients(v0, v1, v2);
			drawTriangle(v0, v1, v2);
		}
	}
}

/* Needs 8bpp chunky buffer and then permutations of two pixels(256 * 256) to 32bit(two 16bit pixels) */
void buffer8bppToVram(unsigned char *buffer, unsigned int *colMap16to32)
{
	int i;
	unsigned short *src = (unsigned short*)buffer;
	unsigned int *dst = (unsigned int*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT / 2; ++i) {
		*dst++ = colMap16to32[*src++];
	}
}

unsigned int *createColMap16to32(unsigned short *srcPal)
{
	unsigned int *dstPal = (unsigned int*)malloc(sizeof(unsigned int) * 256 * 256);

	int i,j,k = 0;
	for (j=0; j<256; ++j) {
		const int c0 = srcPal[j];
		for (i=0; i<256; ++i) {
			const int c1 = srcPal[i];
			dstPal[k++] = (c0 << 16) | c1;
		}
	}

	return dstPal;
}

void initBlobGfx()
{
	if (!isBlobGfxInit) {
		int i,j,x,y;

		for (i=0; i<BLOB_SIZES_NUM_MAX; ++i) {
			const int blobSizeY = i+3;	/* 3 to 15 */
			const int blobSizeX = (((blobSizeY+3) >> 2) << 2) + BLOB_SIZEX_PAD;    /* We are padding this, as we generate four pixels offset(for dword rendering) */
			const float blobSizeYhalf = blobSizeY / 2.0f;
			const float blobSizeXhalf = blobSizeX / 2.0f;

			for (j=0; j<BLOB_SIZEX_PAD; ++j) {
				unsigned char *dst;

				blobData[i][j].sizeX = blobSizeX;
				blobData[i][j].sizeY = blobSizeY;

				blobData[i][j].data = (unsigned char*)malloc(blobSizeX * blobSizeY);
				dst = blobData[i][j].data;

				for (y=0; y<blobSizeY; ++y) {
					const float yc = (float)y - blobSizeYhalf + 0.5f;
					const float yci = yc / (blobSizeYhalf - 0.5f);

					for (x=0; x<blobSizeX; ++x) {
						const int xc = (float)(x - j) - blobSizeXhalf + 0.5f;
						const float xci = xc / (blobSizeYhalf - 0.5f);

						float r = 1.0f - (xci*xci + yci*yci);
						CLAMP01(r)
						*dst++ = (int)(pow(r, 2.0f) * MAX_BLOB_COLOR);
					}
				}
			}
		}
		isBlobGfxInit = 1;
	}
}

void freeBlobGfx()
{
	if (isBlobGfxInit) {
		int i,j;
		for (i=0; i<BLOB_SIZES_NUM_MAX; ++i) {
			for (j=0; j<BLOB_SIZEX_PAD; ++j) {
				free(blobData[i][j].data);
			}
		}
		isBlobGfxInit = 0;
	}
}

void drawBlobs(Vertex3D *v, int count, unsigned char *blobBuffer)
{
	if (count <=0) return;

	do {
		const int size = 1 + (v->z >> 6);
		if (size < BLOB_SIZES_NUM_MAX) {
			const int posX = v->xs;
			const int posY = v->ys;
			BlobData *bd = &blobData[size][posX & (BLOB_SIZEX_PAD-1)];
			const int sizeX = bd->sizeX;
			const int sizeY = bd->sizeY;

			if (!(posX <= sizeX / 2 || posX >= FB_WIDTH - sizeX / 2 || posY <= sizeY / 2 || posY >= FB_HEIGHT - sizeY / 2))
			{
				const int posX32 = posX & ~(BLOB_SIZEX_PAD-1);
				const int wordsX = sizeX / 4;

				unsigned int *dst = (unsigned int*)(blobBuffer + (posY - sizeY / 2) * FB_WIDTH + (posX32 - sizeX / 2));
				unsigned int *src = (unsigned int*)bd->data;

				int y;
				for (y=0; y<sizeY; ++y) {
					int x;
					for (x=0; x<wordsX; ++x) {
						*(dst+x) += *(src+x);
					}
					src += wordsX;
					dst += FB_WIDTH / 4;
				}
			}
		}
		++v;
	}while(--count != 0);
}

void drawBlob(int posX, int posY, int size, int shift, unsigned char *blobBuffer)
{
	if (size < BLOB_SIZES_NUM_MAX) {
		int x,y;
		const int posX32 = posX & ~(BLOB_SIZEX_PAD-1);
		
		BlobData *bd = &blobData[size][posX & (BLOB_SIZEX_PAD-1)];
		const int sizeX = bd->sizeX;
		const int sizeY = bd->sizeY;

		unsigned int *dst = (unsigned int*)(blobBuffer + (posY - sizeY / 2) * FB_WIDTH + (posX32 - sizeX / 2));

		const int wordsX = sizeX / 4;
		unsigned int *src = (unsigned int*)bd->data;

		if (posX <= sizeX / 2 || posX >= FB_WIDTH - sizeX / 2 || posY <= sizeY / 2 || posY >= FB_HEIGHT - sizeY / 2) return;

		for (y=0; y<sizeY; ++y) {
			for (x=0; x<wordsX; ++x) {
				const unsigned int c = *(src+x) << shift;
				*(dst+x) += c;
			}
			src += wordsX;
			dst += FB_WIDTH / 4;
		}
	}
}

void drawAntialiasedLine8bpp(Vertex3D *v1, Vertex3D *v2, int shadeShift, unsigned char *buffer)
{
	int x1 = v1->xs;
	int y1 = v1->ys;
	int x2 = v2->xs;
	int y2 = v2->ys;

	int vramofs;
	int frac, shade;

    int chdx, chdy;

	int dx = x2 - x1;
	int dy = y2 - y1;

	if (dx==0 && dy==0) return;

    chdx = dx;
	chdy = dy;
    if (dx<0) chdx = -dx;
    if (dy<0) chdy = -dy;

	if (chdy < chdx) {
		int x, yy, ddy;
		if (x1 > x2) {
			int temp = x1; x1 = x2; x2 = temp;
			y1 = y2;
		}

		if (dx==0) return;
        ddy = (dy << LN_BASE) / dx;
        yy = y1 << LN_BASE;
		for (x=x1; x<x2; x++) {
			const int yp = yy >> LN_BASE;

			if (x >= 0 && x < FB_WIDTH && yp >=0 && yp < FB_HEIGHT - 1) {
				vramofs = yp*FB_WIDTH + x;
				frac = yy & LN_AND;

				shade = (LN_AND - frac) >> shadeShift;
				*(buffer + vramofs) |= shade;

				shade = frac >> shadeShift;
				*(buffer + vramofs+FB_WIDTH) |= shade;
			}
            yy+=ddy;
		}
	}
	else {
		int y, xx, ddx;
		if (y1 > y2) {
			int temp = y1; y1 = y2; y2 = temp;
			x1 = x2;
		}

		if (dy==0) return;
        ddx = (dx << LN_BASE) / dy;
        xx = x1 << LN_BASE;

		for (y=y1; y<y2; y++) {
			const int xp = xx >> LN_BASE;

			if (y >= 0 && y < FB_HEIGHT && xp >=0 && xp < FB_WIDTH - 1) {
				vramofs = y*FB_WIDTH + xp;
				frac = xx & LN_AND;

				shade = (LN_AND - frac) >> shadeShift;
				*(buffer + vramofs) |= shade;

				shade = frac >> shadeShift;
				*(buffer + vramofs + 1) |= shade;
			}
            xx+=ddx;
		}
	}
}

static unsigned short unpackBlend(unsigned short c, unsigned char shade)
{
	int r = (c >> 11) & 31;
	int g = (c >> 6) & 63;
	int b = c & 31;

	r += shade;
	g += shade;
	b += shade;

	CLAMP(r, 0, 31)
		CLAMP(g, 0, 63)
		CLAMP(b, 0, 31)

		return (r << 11) | (g << 6) | b;
}

void drawAntialiasedLine16bpp(Vertex3D* v1, Vertex3D* v2, int shadeShift, unsigned short* vram)
{
	int x1 = v1->xs;
	int y1 = v1->ys;
	int x2 = v2->xs;
	int y2 = v2->ys;

	int vramofs;
	int frac, shade;

	int chdx, chdy;

	unsigned short c;

	int dx = x2 - x1;
	int dy = y2 - y1;

	if (dx == 0 && dy == 0) return;

	chdx = dx;
	chdy = dy;
	if (dx < 0) chdx = -dx;
	if (dy < 0) chdy = -dy;

	if (chdy < chdx) {
		int x, yy, ddy;
		if (x1 > x2) {
			int temp = x1; x1 = x2; x2 = temp;
			y1 = y2;
		}

		if (dx == 0) return;
		ddy = (dy << LN_BASE) / dx;
		yy = y1 << LN_BASE;
		for (x = x1; x < x2; x++) {
			const int yp = yy >> LN_BASE;

			if (x >= 0 && x < FB_WIDTH && yp >= 0 && yp < FB_HEIGHT - 1) {
				vramofs = yp * FB_WIDTH + x;
				frac = yy & LN_AND;

				shade = (LN_AND - frac) >> shadeShift;
				c = *(vram + vramofs);
				*(vram + vramofs) = unpackBlend(c, shade);

				shade = frac >> shadeShift;
				c = *(vram + vramofs + FB_WIDTH);
				*(vram + vramofs + FB_WIDTH) = unpackBlend(c, shade);
			}
			yy += ddy;
		}
	}
	else {
		int y, xx, ddx;
		if (y1 > y2) {
			int temp = y1; y1 = y2; y2 = temp;
			x1 = x2;
		}

		if (dy == 0) return;
		ddx = (dx << LN_BASE) / dy;
		xx = x1 << LN_BASE;

		for (y = y1; y < y2; y++) {
			const int xp = xx >> LN_BASE;

			if (y >= 0 && y < FB_HEIGHT && xp >= 0 && xp < FB_WIDTH - 1) {
				vramofs = y * FB_WIDTH + xp;
				frac = xx & LN_AND;

				shade = (LN_AND - frac) >> shadeShift;
				c = *(vram + vramofs);
				*(vram + vramofs) = unpackBlend(c, shade);

				shade = frac >> shadeShift;
				c = *(vram + vramofs + 1);
				*(vram + vramofs + 1) = unpackBlend(c, shade);
			}
			xx += ddx;
		}
	}
}

void setPalGradient(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, unsigned short* pal)
{
	int i;
	const int dc = (c1 - c0);
	const int dr = ((r1 - r0) << 16) / dc;
	const int dg = ((g1 - g0) << 16) / dc;
	const int db = ((b1 - b0) << 16) / dc;

	r0 <<= 16;
	g0 <<= 16;
	b0 <<= 16;

	for (i = c0; i <= c1; i++)
	{
		int r = r0 >> 16;
		int g = g0 >> 16;
		int b = b0 >> 16;

		if (r < 0) r = 0; if (r > 31) r = 31;
		if (g < 0) g = 0; if (g > 63) g = 63;
		if (b < 0) b = 0; if (b > 31) b = 31;

		pal[i] = (r<<11) | (g<<5) | b;

		r0 += dr;
		g0 += dg;
		b0 += db;
	}
}
