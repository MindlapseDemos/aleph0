#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

struct vec2 {
	int x, y;
};

int main(int argc, char **argv)
{
	int i, j, frm, imgrad, out_nlines, xsz = 240, ysz = 160, texsz = 128;
	int half_y = 0;
	int center = 0;
	struct vec2 *tunbuf, *tun;
	float prev_r;
	struct vec2 *buf, *ptr;
	char *endp;
	int num_frames = 1;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(argv[i][2] != 0) goto invalopt;
			switch(argv[i][1]) {
			case 's':
				if(sscanf(argv[++i], "%dx%d", &xsz, &ysz) != 2 || xsz <= 1 || ysz <= 1) {
					fprintf(stderr, "-s must be followed by WxH\n");
					return 1;
				}
				break;

			case 't':
				if(!(texsz = atoi(argv[++i])) || texsz > 256) {
					fprintf(stderr, "-t must be followed by the texture size (1-256)\n");
					return 1;
				}
				break;

			case 'y':
				half_y = 1;
				break;

			case 'c':
				if(!argv[++i]) {
					fprintf(stderr, "-c must be followed by a center pixel\n");
					return 1;
				}
				center = strtol(argv[i], &endp, 10);
				if(endp == argv[i]) {
					fprintf(stderr, "-c invalid center position: %s\n", argv[i]);
					return 1;
				}
				break;

			case 'n':
				if(!argv[++i]) {
					fprintf(stderr, "-n must be followed by the number of frames\n");
					return 1;
				}
				if(!(num_frames = atoi(argv[i]))) {
					fprintf(stderr, "-n invalid number of frames: %s\n", argv[i]);
					return 1;
				}
				break;

			default:
				goto invalopt;
			}
		} else {
invalopt:	fprintf(stderr, "invalid argument: %s\n", argv[i]);
			return 1;
		}
	}

	out_nlines = half_y ? ysz / 2 : ysz;

	if(!(buf = malloc(xsz * ysz * sizeof *buf))) {
		perror("failed to allocate buffer");
		return 1;
	}
	imgrad = sqrt(xsz * xsz + ysz * ysz);

	FILE *fp = fopen("tun_preview.ppm", "wb");
	if(fp) {
		fprintf(fp, "P6\n%d %d\n255\n", xsz, out_nlines * num_frames);
	}


	for(frm=0; frm<num_frames; frm++) {
		int coffs = num_frames > 1 ? frm * center / (num_frames - 1) : center;

		memset(buf, 0xff, xsz * ysz * sizeof *buf);

#define UDIV	2048
#define VDIV	32768
		prev_r = 0.0f;
#pragma omp parallel for private(i, j, prev_r, ptr) schedule(dynamic)
		for(i=0; i<VDIV; i++) {
			float v = (float)(VDIV - i) / (float)VDIV;
			float r = 4.0 / v + 16;
			float z = v * coffs;

			/* don't bother drawing rings < 1 pixel apart */
			if(r < 0 || fabs(r - prev_r) < 0.05) continue;

			for(j=0; j<UDIV; j++) {
				float u = (float)j / (float)(UDIV - 1);
				float theta = 2.0f * u * M_PI;

				int x = (int)(cos(theta) * r - z) + xsz / 2;
				int y = (int)(sin(theta) * r) + ysz / 2;

				if(x >= 0 && x < xsz && y >= 0 && y < ysz) {
					ptr = buf + y * xsz + x;
					ptr->x = (j << 8) / UDIV;
					ptr->y = ((VDIV - i - 1) << 10) / VDIV;
				}
			}
			prev_r = r;
		}

		ptr = buf;
		for(i=0; i<out_nlines; i++) {
			for(j=0; j<xsz; j++) {
				uint16_t out;
				int u = ptr->x;
				int v = ptr->y;
				int r = u & 0x7f;
				int g = (v >> 1) & 0xff;

				/*if(v > 2.0) r = g = b = 0;*/

				ptr++;

				out = ((uint16_t)u & 0x7f) | (((uint16_t)v & 0x1ff) << 7);
				fwrite(&out, sizeof out, 1, stdout);

				if(fp) {
					fputc(r, fp);
					fputc(g, fp);
					fputc(0, fp);
				}
			}
		}

	}
	fflush(stdout);

	if(fp) fclose(fp);
	return 0;
}
