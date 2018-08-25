#ifndef NOISE_H_
#define NOISE_H_

float noise1(float x);
float noise2(float x, float y);
float noise3(float x, float y, float z);
/*float noise4(float x, float y, float z, float w);*/

float pnoise1(float x, int period);
float pnoise2(float x, float y, int per_x, int per_y);
float pnoise3(float x, float y, float z, int per_x, int per_y, int per_z);
/*float pnoise4(float x, float y, float z, float w, int per_x, int per_y, int per_z, int per_w);*/

float fbm1(float x, int octaves);
float fbm2(float x, float y, int octaves);
float fbm3(float x, float y, float z, int octaves);
/*float fbm4(float x, float y, float z, float w, int octaves);*/

float pfbm1(float x, int per, int octaves);
float pfbm2(float x, float y, int per_x, int per_y, int octaves);
float pfbm3(float x, float y, float z, int per_x, int per_y, int per_z, int octaves);
/*float pfbm4(float x, float y, float z, float w, int per_x, int per_y, int per_z, int per_w, int octaves);*/

float turbulence1(float x, int octaves);
float turbulence2(float x, float y, int octaves);
float turbulence3(float x, float y, float z, int octaves);
/*float turbulence4(float x, float y, float z, float w, int octaves);*/

float pturbulence1(float x, int per, int octaves);
float pturbulence2(float x, float y, int per_x, int per_y, int octaves);
float pturbulence3(float x, float y, float z, int per_x, int per_y, int per_z, int octaves);
/*float pturbulence4(float x, float y, float z, float w, int per_x, int per_y, int per_z, int per_w, int octaves);*/


#endif	/* NOISE_H_ */
