#include <stdlib.h>
#include <math.h>
#include "noise.h"

/* ---- Ken Perlin's implementation of noise ---- */
#define B	0x100
#define BM	0xff
#define N	0x1000
#define NP	12   /* 2^N */
#define NM	0xfff

#define s_curve(t) (t * t * (3.0f - 2.0f * t))

#define setup(elem, b0, b1, r0, r1) \
	do {							\
		float t = elem + N;		\
		b0 = ((int)t) & BM;			\
		b1 = (b0 + 1) & BM;			\
		r0 = t - (int)t;			\
		r1 = r0 - 1.0f;				\
	} while(0)

#define setup_p(elem, b0, b1, r0, r1, p) \
	do {							\
		float t = elem + N;		\
		b0 = (((int)t) & BM) % p;	\
		b1 = ((b0 + 1) & BM) % p;	\
		r0 = t - (int)t;			\
		r1 = r0 - 1.0f;				\
	} while(0)


static int perm[B + B + 2];			/* permuted index from g_n onto themselves */
static float grad3[B + B + 2][3];		/* 3D random gradients */
static float grad2[B + B + 2][2];		/* 2D random gradients */
static float grad1[B + B + 2];		/* 1D random ... slopes */
static int tables_valid;

#define init_once()	if(!tables_valid) init_noise()

static void init_noise()
{
	int i;
	float len;

	/* calculate random gradients */
	for(i=0; i<B; i++) {
		perm[i] = i;	/* .. and initialize permutation mapping to identity */

		grad1[i] = (float)((rand() % (B + B)) - B) / B;

		grad2[i][0] = (float)((rand() % (B + B)) - B) / B;
		grad2[i][1] = (float)((rand() % (B + B)) - B) / B;
		if((len = sqrt(grad2[i][0] * grad2[i][0] + grad2[i][1] * grad2[i][1])) != 0.0f) {
			grad2[i][0] /= len;
			grad2[i][1] /= len;
		}

		grad3[i][0] = (float)((rand() % (B + B)) - B) / B;
		grad3[i][1] = (float)((rand() % (B + B)) - B) / B;
		grad3[i][2] = (float)((rand() % (B + B)) - B) / B;
		if((len = sqrt(grad3[i][0] * grad3[i][0] + grad3[i][1] * grad3[i][1]) + grad3[i][2] * grad3[i][2]) != 0.0f) {
			grad3[i][0] /= len;
			grad3[i][1] /= len;
			grad3[i][2] /= len;
		}
	}

	/* permute indices by swapping them randomly */
	for(i=0; i<B; i++) {
		int rand_idx = rand() % B;

		int tmp = perm[i];
		perm[i] = perm[rand_idx];
		perm[rand_idx] = tmp;
	}

	/* fill up the rest of the arrays by duplicating the existing gradients */
	/* and permutations */
	for(i=0; i<B+2; i++) {
		perm[B + i] = perm[i];
		grad1[B + i] = grad1[i];
		grad2[B + i][0] = grad2[i][0];
		grad2[B + i][1] = grad2[i][1];
		grad3[B + i][0] = grad3[i][0];
		grad3[B + i][1] = grad3[i][1];
		grad3[B + i][2] = grad3[i][2];
	}

	tables_valid = 1;
}

#define lerp(a, b, t) ((a) + ((b) - (a)) * (t))
#define dotgrad2(g, x, y) ((g)[0] * (x) + (g)[1] * (y))
#define dotgrad3(g, x, y, z) ((g)[0] * (x) + (g)[1] * (y) + (g)[2] * (z))

float noise1(float x)
{
	int bx0, bx1;
	float rx0, rx1, sx, u, v;

	init_once();

	setup(x, bx0, bx1, rx0, rx1);
	sx = s_curve(rx0);
	u = rx0 * grad1[perm[bx0]];
	v = rx1 * grad1[perm[bx1]];
	return lerp(u, v, sx);
}

float noise2(float x, float y)
{
	int i, j, b00, b10, b01, b11;
	int bx0, bx1, by0, by1;
	float rx0, rx1, ry0, ry1;
	float sx, sy, u, v, a, b;

	init_once();

	setup(x, bx0, bx1, rx0, rx1);
	setup(y, by0, by1, ry0, ry1);

	i = perm[bx0];
	j = perm[bx1];

	b00 = perm[i + by0];
	b10 = perm[j + by0];
	b01 = perm[i + by1];
	b11 = perm[j + by1];

	/* calculate hermite inteprolating factors */
	sx = s_curve(rx0);
	sy = s_curve(ry0);

	/* interpolate along the left edge */
	u = dotgrad2(grad2[b00], rx0, ry0);
	v = dotgrad2(grad2[b10], rx1, ry0);
	a = lerp(u, v, sx);

	/* interpolate along the right edge */
	u = dotgrad2(grad2[b01], rx0, ry1);
	v = dotgrad2(grad2[b11], rx1, ry1);
	b = lerp(u, v, sx);

	/* interpolate between them */
	return lerp(a, b, sy);
}

float noise3(float x, float y, float z)
{
	int i, j;
	int bx0, bx1, by0, by1, bz0, bz1;
	int b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, rz0, rz1;
	float sx, sy, sz;
	float u, v, a, b, c, d;

	init_once();

	setup(x, bx0, bx1, rx0, rx1);
	setup(y, by0, by1, ry0, ry1);
	setup(z, bz0, bz1, rz0, rz1);

	i = perm[bx0];
	j = perm[bx1];

	b00 = perm[i + by0];
	b10 = perm[j + by0];
	b01 = perm[i + by1];
	b11 = perm[j + by1];

	/* calculate hermite interpolating factors */
	sx = s_curve(rx0);
	sy = s_curve(ry0);
	sz = s_curve(rz0);

	/* interpolate along the top slice of the cell */
	u = dotgrad3(grad3[b00 + bz0], rx0, ry0, rz0);
	v = dotgrad3(grad3[b10 + bz0], rx1, ry0, rz0);
	a = lerp(u, v, sx);

	u = dotgrad3(grad3[b01 + bz0], rx0, ry1, rz0);
	v = dotgrad3(grad3[b11 + bz0], rx1, ry1, rz0);
	b = lerp(u, v, sx);

	c = lerp(a, b, sy);

	/* interpolate along the bottom slice of the cell */
	u = dotgrad3(grad3[b00 + bz0], rx0, ry0, rz1);
	v = dotgrad3(grad3[b10 + bz0], rx1, ry0, rz1);
	a = lerp(u, v, sx);

	u = dotgrad3(grad3[b01 + bz0], rx0, ry1, rz1);
	v = dotgrad3(grad3[b11 + bz0], rx1, ry1, rz1);
	b = lerp(u, v, sx);

	d = lerp(a, b, sy);

	/* interpolate between slices */
	return lerp(c, d, sz);
}

float noise4(float x, float y, float z, float w)
{
	return 0;	/* TODO */
}


float pnoise1(float x, int period)
{
	int bx0, bx1;
	float rx0, rx1, sx, u, v;

	init_once();

	setup_p(x, bx0, bx1, rx0, rx1, period);
	sx = s_curve(rx0);
	u = rx0 * grad1[perm[bx0]];
	v = rx1 * grad1[perm[bx1]];
	return lerp(u, v, sx);
}

float pnoise2(float x, float y, int per_x, int per_y)
{
	int i, j, b00, b10, b01, b11;
	int bx0, bx1, by0, by1;
	float rx0, rx1, ry0, ry1;
	float sx, sy, u, v, a, b;

	init_once();

	setup_p(x, bx0, bx1, rx0, rx1, per_x);
	setup_p(y, by0, by1, ry0, ry1, per_y);

	i = perm[bx0];
	j = perm[bx1];

	b00 = perm[i + by0];
	b10 = perm[j + by0];
	b01 = perm[i + by1];
	b11 = perm[j + by1];

	/* calculate hermite inteprolating factors */
	sx = s_curve(rx0);
	sy = s_curve(ry0);

	/* interpolate along the left edge */
	u = dotgrad2(grad2[b00], rx0, ry0);
	v = dotgrad2(grad2[b10], rx1, ry0);
	a = lerp(u, v, sx);

	/* interpolate along the right edge */
	u = dotgrad2(grad2[b01], rx0, ry1);
	v = dotgrad2(grad2[b11], rx1, ry1);
	b = lerp(u, v, sx);

	/* interpolate between them */
	return lerp(a, b, sy);
}

float pnoise3(float x, float y, float z, int per_x, int per_y, int per_z)
{
	int i, j;
	int bx0, bx1, by0, by1, bz0, bz1;
	int b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, rz0, rz1;
	float sx, sy, sz;
	float u, v, a, b, c, d;

	init_once();

	setup_p(x, bx0, bx1, rx0, rx1, per_x);
	setup_p(y, by0, by1, ry0, ry1, per_y);
	setup_p(z, bz0, bz1, rz0, rz1, per_z);

	i = perm[bx0];
	j = perm[bx1];

	b00 = perm[i + by0];
	b10 = perm[j + by0];
	b01 = perm[i + by1];
	b11 = perm[j + by1];

	/* calculate hermite interpolating factors */
	sx = s_curve(rx0);
	sy = s_curve(ry0);
	sz = s_curve(rz0);

	/* interpolate along the top slice of the cell */
	u = dotgrad3(grad3[b00 + bz0], rx0, ry0, rz0);
	v = dotgrad3(grad3[b10 + bz0], rx1, ry0, rz0);
	a = lerp(u, v, sx);

	u = dotgrad3(grad3[b01 + bz0], rx0, ry1, rz0);
	v = dotgrad3(grad3[b11 + bz0], rx1, ry1, rz0);
	b = lerp(u, v, sx);

	c = lerp(a, b, sy);

	/* interpolate along the bottom slice of the cell */
	u = dotgrad3(grad3[b00 + bz0], rx0, ry0, rz1);
	v = dotgrad3(grad3[b10 + bz0], rx1, ry0, rz1);
	a = lerp(u, v, sx);

	u = dotgrad3(grad3[b01 + bz0], rx0, ry1, rz1);
	v = dotgrad3(grad3[b11 + bz0], rx1, ry1, rz1);
	b = lerp(u, v, sx);

	d = lerp(a, b, sy);

	/* interpolate between slices */
	return lerp(c, d, sz);
}

float pnoise4(float x, float y, float z, float w, int per_x, int per_y, int per_z, int per_w)
{
	return 0;
}


float fbm1(float x, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += noise1(x * freq) / freq;
		freq *= 2.0f;
	}
	return res;
}

float fbm2(float x, float y, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += noise2(x * freq, y * freq) / freq;
		freq *= 2.0f;
	}
	return res;
}

float fbm3(float x, float y, float z, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += noise3(x * freq, y * freq, z * freq) / freq;
		freq *= 2.0f;
	}
	return res;

}

float fbm4(float x, float y, float z, float w, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += noise4(x * freq, y * freq, z * freq, w * freq) / freq;
		freq *= 2.0f;
	}
	return res;
}


float pfbm1(float x, int per, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += pnoise1(x * freq, per) / freq;
		freq *= 2.0f;
		per *= 2;
	}
	return res;
}

float pfbm2(float x, float y, int per_x, int per_y, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += pnoise2(x * freq, y * freq, per_x, per_y) / freq;
		freq *= 2.0f;
		per_x *= 2;
		per_y *= 2;
	}
	return res;
}

float pfbm3(float x, float y, float z, int per_x, int per_y, int per_z, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += pnoise3(x * freq, y * freq, z * freq, per_x, per_y, per_z) / freq;
		freq *= 2.0f;
		per_x *= 2;
		per_y *= 2;
		per_z *= 2;
	}
	return res;
}

float pfbm4(float x, float y, float z, float w, int per_x, int per_y, int per_z, int per_w, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += pnoise4(x * freq, y * freq, z * freq, w * freq,
				per_x, per_y, per_z, per_w) / freq;
		freq *= 2.0f;
		per_x *= 2;
		per_y *= 2;
		per_z *= 2;
		per_w *= 2;
	}
	return res;
}


float turbulence1(float x, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(noise1(x * freq) / freq);
		freq *= 2.0f;
	}
	return res;
}

float turbulence2(float x, float y, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(noise2(x * freq, y * freq) / freq);
		freq *= 2.0f;
	}
	return res;
}

float turbulence3(float x, float y, float z, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(noise3(x * freq, y * freq, z * freq) / freq);
		freq *= 2.0f;
	}
	return res;
}

float turbulence4(float x, float y, float z, float w, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(noise4(x * freq, y * freq, z * freq, w * freq) / freq);
		freq *= 2.0f;
	}
	return res;
}


float pturbulence1(float x, int per, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(pnoise1(x * freq, per) / freq);
		freq *= 2.0f;
		per *= 2;
	}
	return res;
}

float pturbulence2(float x, float y, int per_x, int per_y, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(pnoise2(x * freq, y * freq, per_x, per_y) / freq);
		freq *= 2.0f;
		per_x *= 2;
		per_y *= 2;
	}
	return res;
}

float pturbulence3(float x, float y, float z, int per_x, int per_y, int per_z, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(pnoise3(x * freq, y * freq, z * freq, per_x, per_y, per_z) / freq);
		freq *= 2.0f;
		per_x *= 2;
		per_y *= 2;
		per_z *= 2;
	}
	return res;
}

float pturbulence4(float x, float y, float z, float w, int per_x, int per_y, int per_z, int per_w, int octaves)
{
	int i;
	float res = 0.0f, freq = 1.0f;
	for(i=0; i<octaves; i++) {
		res += fabs(pnoise4(x * freq, y * freq, z * freq, w * freq,
				per_x, per_y, per_z, per_w) / freq);
		freq *= 2.0f;
		per_x *= 2;
		per_y *= 2;
		per_z *= 2;
		per_w *= 2;
	}
	return res;
}
