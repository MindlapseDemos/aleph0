/*
Tiny Mersenne Twister only 127 bit internal state

Copyright (C) 2011 Mutsuo Saito, Makoto Matsumoto,
Hiroshima University and The University of Tokyo.
All rights reserved.

The 3-clause BSD License
*/
#ifndef TINYMT32_H_
#define TINYMT32_H_

#include <stdint.h>
#include "util.h"

#define TINYMT32_MEXP 127
#define TINYMT32_SH0 1
#define TINYMT32_SH1 10
#define TINYMT32_SH8 8
#define TINYMT32_MASK 0x7fffffffu
#define TINYMT32_MUL (1.0f / 4294967296.0f)


typedef struct tinymt32 {
	uint32_t status[4];
	uint32_t mat1;
	uint32_t mat2;
	uint32_t tmat;
} tinymt32_t;


/**
 * This function changes internal state of tinymt32.
 * Users should not call this function directly.
 * @param random tinymt internal status
 */
INLINE static void tinymt32_next_state(tinymt32_t * random) {
	uint32_t x;
	uint32_t y;

	y = random->status[3];
	x = (random->status[0] & TINYMT32_MASK)
		^ random->status[1]
		^ random->status[2];
	x ^= (x << TINYMT32_SH0);
	y ^= (y >> TINYMT32_SH0) ^ x;
	random->status[0] = random->status[1];
	random->status[1] = random->status[2];
	random->status[2] = x ^ (y << TINYMT32_SH1);
	random->status[3] = y;
	random->status[1] ^= -((int32_t)(y & 1)) & random->mat1;
	random->status[2] ^= -((int32_t)(y & 1)) & random->mat2;
}

/**
 * This function outputs 32-bit unsigned integer from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return 32-bit unsigned pseudorandom number
 */
INLINE static uint32_t tinymt32_temper(tinymt32_t * random) {
	uint32_t t0, t1;
	t0 = random->status[3];
	t1 = random->status[0]
		+ (random->status[2] >> TINYMT32_SH8);
	t0 ^= t1;
	t0 ^= -((int32_t)(t1 & 1)) & random->tmat;
	return t0;
}

/**
 * This function outputs 32-bit unsigned integer from internal state.
 * @param random tinymt internal status
 * @return 32-bit unsigned integer r (0 <= r < 2^32)
 */
INLINE static uint32_t tinymt32_generate_uint32(tinymt32_t * random) {
	tinymt32_next_state(random);
	return tinymt32_temper(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using multiplying by 1 / 2^32.
 * floating point multiplication is faster than using union trick in
 * my Intel CPU.
 * @param random tinymt internal status
 * @return floating point number r (0.0 <= r < 1.0)
 */
INLINE static float tinymt32_generate_float(tinymt32_t * random) {
	tinymt32_next_state(random);
	return tinymt32_temper(random) * TINYMT32_MUL;
}

#define MIN_LOOP 8
#define PRE_LOOP 8

/**
 * This function certificate the period of 2^127-1.
 * @param random tinymt state vector.
 */
static void period_certification(tinymt32_t * random) {
	if ((random->status[0] & TINYMT32_MASK) == 0 && random->status[1] == 0 &&
			random->status[2] == 0 && random->status[3] == 0) {
		random->status[0] = 'T';
		random->status[1] = 'I';
		random->status[2] = 'N';
		random->status[3] = 'Y';
	}
}

/**
 * This function initializes the internal state array with a 32-bit
 * unsigned integer seed.
 * @param random tinymt state vector.
 * @param seed a 32-bit unsigned integer used as a seed.
 */
static void tinymt32_init(tinymt32_t * random, uint32_t seed) {
	int i;
	random->status[0] = seed;
	random->status[1] = random->mat1;
	random->status[2] = random->mat2;
	random->status[3] = random->tmat;
	for (i = 1; i < MIN_LOOP; i++) {
		random->status[i & 3] ^= i + 1812433253u * (random->status[(i - 1) & 3] ^
				(random->status[(i - 1) & 3] >> 30));
	}
	period_certification(random);
	for (i = 0; i < PRE_LOOP; i++) {
		tinymt32_next_state(random);
	}
}

#endif	/* TINYMT32_H_ */
