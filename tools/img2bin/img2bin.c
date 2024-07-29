#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "imago2.h"

int proc_image(const char *fname);

int main(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(proc_image(argv[i]) == -1) {
			return 1;
		}
	}
	return 0;
}

int proc_image(const char *fname)
{
	int i, xsz, ysz, npixels, len;
	unsigned char *pixels24, *sptr;
	unsigned short *pixels16, *dptr;
	char *outfname, *suffix;
	FILE *out;

	len = strlen(fname);
	outfname = alloca(len + 4);
	memcpy(outfname, fname, len + 1);
	if((suffix = strrchr(outfname, '.')) && suffix > outfname) {
		strcpy(suffix, ".img");
	} else {
		strcpy(outfname + len, ".img");
	}

	if(!(pixels24 = img_load_pixels(fname, &xsz, &ysz, IMG_FMT_RGB24))) {
		fprintf(stderr, "failed to load image: %s\n", fname);
		return -1;
	}
	npixels = xsz * ysz;
	if(!(pixels16 = malloc(npixels * 2))) {
		perror("failed to allocate output image buffer");
		img_free_pixels(pixels24);
		return -1;
	}

	if(!(out = fopen(outfname, "wb"))) {
		fprintf(stderr, "failed to open %s for writing: %s\n", outfname, strerror(errno));
		img_free_pixels(pixels24);
		free(pixels16);
		return -1;
	}

	sptr = pixels24;
	dptr = pixels16;
	for(i=0; i<npixels; i++) {
		int r = *sptr++ >> 3;
		int g = *sptr++ >> 2;
		int b = *sptr++ >> 3;
		*dptr++ = (r << 11) | (g << 5) | b;
	}
	img_free_pixels(pixels24);

	fwrite(pixels16, 2, npixels, out);
	fclose(out);
	free(pixels16);
	return 0;
}
