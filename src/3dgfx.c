#include <string.h>
#include "3dgfx.h"

#define STACK_SIZE	8
typedef float g3d_matrix[16];

struct g3d_state {
	unsigned int opt;

	g3d_matrix mat[G3D_NUM_MATRICES][STACK_SIZE];
	int mtop[G3D_NUM_MATRICES];

	int width, height;
	void *pixels;
};

static struct g3d_state st;

void g3d_init(void)
{
	int i;

	memset(&st, 0, sizeof st);

	for(i=0; i<G3D_NUM_MATRICES; i++) {
		g3d_set_matrix(i, 0);
	}
}

void g3d_framebuffer(int width, int height, void *pixels)
{
	st.width = width;
	st.height = height;
	st.pixels = pixels;
}

void g3d_enable(unsigned int opt)
{
	st.opt |= opt;
}

void g3d_disable(unsigned int opt)
{
	st.opt &= ~opt;
}

void g3d_setopt(unsigned int opt, unsigned int mask)
{
	st.opt = (st.opt & ~mask) | (opt & mask);
}

unsigned int g3d_getopt(unsigned int mask)
{
	return st.opt & mask;
}

void g3d_set_matrix(int which, const float *m)
{
	int top = st.mtop[which];
	memcpy(st.mat[which][top], m, 16 * sizeof(float));
}

#define M(i,j)	(((i) << 2) + (j))
void g3d_mult_matrix(int which, const float *m2)
{
	int i, j, top = st.mtop[which];
	float m1[16];
	float *dest = st.mat[which][top];

	memcpy(m1, dest, sizeof m1);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			*dest++ = m1[M(0,j)] * m2[M(i,0)] +
				m1[M(1,j)] * m2[M(i,1)] +
				m1[M(2,j)] * m2[M(i,2)] +
				m1[M(3,j)] * m2[M(i,3)];
		}
	}
}
/* TODO continue ... */
