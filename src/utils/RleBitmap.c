#include "RleBitmap.h"
#include <malloc.h>
#include <string.h>

/* Number of numbers per scanline. Each streak has 2 numbers (start, length) */
#define RLE_ELEMENTS_PER_SCANLINE RLE_STREAKS_PER_SCANLINE * 2

/* Two RLE_TYPE elements per streak (= start,length) */
#define RLE_BYTES_PER_SCANLINE RLE_ELEMENTS_PER_SCANLINE * sizeof(RLE_TYPE)

/* RLE_TYPE count required for storing an RLE of w,h */
static int rleWorstCaseElementCount(int w, int h) {
	/* Allocate an extra worst case for one scanline, which is w/2 streaks = w
	 * (start,length) elements */
	return h * RLE_ELEMENTS_PER_SCANLINE + w;
}

/* Byte count of the 'scans' buffer */
static int rleScansByteCount(RleBitmap *rle) {
	return rleWorstCaseElementCount(rle->w, rle->h) * sizeof(RLE_TYPE);
}

RleBitmap *rleCreate(unsigned int w, unsigned int h) {
	RleBitmap *ret = (RleBitmap *)malloc(sizeof(RleBitmap));
	ret->w = w;
	ret->h = h;

	/* Allocate scans */
	ret->scans = (RLE_TYPE *)calloc(rleWorstCaseElementCount(w, h), sizeof(RLE_TYPE));

	return ret;
}

void rleDestroy(RleBitmap *b) {
	if (!b)
		return;
	free(b->scans);
	free(b);
}

void rleClear(RleBitmap *rle) { memset(rle->scans, 0, rleScansByteCount(rle)); }

RleBitmap *rleEncode(RleBitmap *rle, unsigned char *pixels, unsigned int pixelsW,
		     unsigned int pixelsH) {
	int x = 0;
	int y = 0;
	int streakActive = 0;
	int currentStreakLength = 0;
	RLE_TYPE *output = 0;
	unsigned char *currentInputPixel = pixels;

	/* https://www.youtube.com/watch?v=RKMR02o1I88&feature=youtu.be&t=55 */
	if (!rle)
		rle = rleCreate(pixelsW, pixelsH);
	else
		rleClear(rle); /* The following code assumes cleared array */

	for (y = 0; y < pixelsH; y++) {
		/* Go to the beginning of the RLE scan */
		output = rle->scans + y * RLE_ELEMENTS_PER_SCANLINE;

		for (x = 0; x < pixelsW; x++) {
			if (*currentInputPixel++) {
				if (streakActive) {
					if (currentStreakLength >= RLE_MAX_STREAK_LENGTH) {
						/* Do not allow streaks of more than max length -
						 * close current streak */
						*output++ = (RLE_TYPE)currentStreakLength;

						/* Begin new streak at current x */
						*output++ = (RLE_TYPE)x;
						currentStreakLength = 0;
					}
				} else {
					/* Begin new streak */
					*output++ = (RLE_TYPE)x;
					currentStreakLength = 0;
					streakActive = 1;
				}
				currentStreakLength++;
			} else {
				if (streakActive) {
					/* Close current streak */
					*output++ = (RLE_TYPE)currentStreakLength;
					currentStreakLength = 0;
					streakActive = 0;
				}
			} /* End if (current pixel on) */
		}	  /* End for (all x) */

		/* We reached the end of the scan - close any active streak */
		if (streakActive) {
			*output++ = (RLE_TYPE)currentStreakLength;
		}
		streakActive = 0;
		currentStreakLength = 0;
	} /* End for (all scans */

	return rle;
}

void rleDistributeStreaks(RleBitmap *rle) {
	int scanline = 0;
	int halfW = rle->w >> 1;
	RLE_TYPE *ptr = 0;
	RLE_TYPE tmp = 0;

#define LAST_STREAK RLE_STREAKS_PER_SCANLINE

	ptr = rle->scans;
	for (scanline = 0; scanline < rle->h; scanline++) {
		if (ptr[0] >= halfW) {
			/* Exchange first with last streak */
			tmp = ptr[0];
			ptr[0] = ptr[LAST_STREAK * 2 - 2];
			ptr[LAST_STREAK * 2 - 2] = tmp;
			tmp = ptr[1];
			ptr[1] = ptr[LAST_STREAK * 2 - 1];
			ptr[LAST_STREAK * 2 - 1] = tmp;
		}

		ptr += 8;
	}
}

void rleBlit(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride, int blitX,
	     int blitY) {
	int scanline = 0;
	int streakPos = 0;
	int streakLength = 0;
	int streak = 0;
	RLE_TYPE *input = rle->scans;
	unsigned short *output;
	unsigned int *output32;

	dst += blitX + blitY * dstStride;

	for (scanline = blitY; scanline < blitY + rle->h; scanline++) {
		if (scanline < 0 || scanline >= dstH)
			continue;
		for (streak = 0; streak < RLE_STREAKS_PER_SCANLINE; streak++) {
			streakPos = (int)*input++;
			streakLength = (int)*input++;

			if ((streakPos + blitX) <= 0)
				continue;

			output = dst + streakPos;

			/* Check if we need to write the first pixel as 16bit */
			if (streakLength % 2) {
				*output++ = RLE_FILL_COLOR;
			}

			/* Then, write 2 pixels at a time */
			streakLength >>= 1;
			output32 = (unsigned int *)output;
			while (streakLength--) {
				*output32++ = RLE_FILL_COLOR_32;
			}
		}

		dst += dstStride;
	}
}

/* This is madness. We could at least check that we are not interpolating from 0 -> something
 * (length). This could remove the need for 'distributeScans' */
void interpolateScan(RLE_TYPE *output, RLE_TYPE *a, RLE_TYPE *b, float t) {
	static int div = 1 << 23;
	int ti, i;

	t += 1.0f;
	ti = (*((unsigned int *)&t)) & 0x7FFFFF;

	for (i = 0; i < RLE_ELEMENTS_PER_SCANLINE; i++) {
		if (*a == 0) {
			*output++ = *b++;
			a++;
		} else {
			if (*b == 0) {
				*output++ = *a++;
				b++;
			} else {
				*output++ = ((*b++ * ti) + (*a++ * (div - ti))) >> 23;
			}
		}
	}
}

void rleBlitScale(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride, int blitX,
		  int blitY, float scaleX, float scaleY) {
	int scanline = 0;
	int streakPos = 0;
	int streakLength = 0;
	int streak = 0;
	unsigned short *output;
	unsigned int *output32;
	unsigned char *input;
	int scanlineCounter = 0;
	int scaleXFixed;
	static unsigned char scan[512];

	/*int blitW = (int)(rle->w * scaleX + 0.5f);*/
	int blitH = (int)(rle->h * scaleY + 0.5f);

	/* From this point on, scaleY will be inverted */
	scaleY = 1.0f / scaleY;

	scaleXFixed = (int)(scaleX * (float)(1 << RLE_FIXED_BITS) + 0.5f);

	dst += blitX + blitY * dstStride;

	for (scanline = blitY; scanline < blitY + blitH; scanline++) {
		float normalScan = scanlineCounter * scaleY; /* ScaleY  is inverted */
		unsigned char *scan0 = rle->scans + RLE_BYTES_PER_SCANLINE * (int)normalScan;
		unsigned char *scan1 = scan0 + RLE_BYTES_PER_SCANLINE;
		normalScan -= (int)normalScan;
		interpolateScan(scan, scan0, scan1, normalScan);
		input = scan;
		scanlineCounter++;

		if (scanline < 0 || scanline >= dstH)
			continue;
		for (streak = 0; streak < RLE_STREAKS_PER_SCANLINE; streak++) {
			streakPos = (*input++ * scaleXFixed) >> RLE_FIXED_BITS;
			streakLength = (*input++ * scaleXFixed) >> RLE_FIXED_BITS;

			if ((streakPos + blitX) <= 0)
				continue;

			output = dst + streakPos;

			/* Check if we need to write the first pixel as 16bit */
			if (streakLength % 2) {
				*output++ = RLE_FILL_COLOR;
			}

			/* Then, write 2 pixels at a time */
			streakLength >>= 1;
			output32 = (unsigned int *)output;
			while (streakLength--) {
				*output32++ = RLE_FILL_COLOR_32;
			}
		}

		dst += dstStride;
	}
}

void rleBlitScaleInv(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride,
		     int blitX, int blitY, float scaleX, float scaleY) {
	int scanline = 0;
	int streakPos = 0;
	int streakLength = 0;
	int streak = 0;
	unsigned short *output;
	unsigned int *output32;
	unsigned char *input;
	int scanlineCounter = 0;
	int scaleXFixed;
	static unsigned char scan[512];

	/*int blitW = (int)(rle->w * scaleX + 0.5f);*/
	int blitH = (int)(rle->h * scaleY + 0.5f);

	/* From this point on, scaleY will be inverted */
	scaleY = 1.0f / scaleY;

	scaleXFixed = (int)(scaleX * (float)(1 << RLE_FIXED_BITS) + 0.5f);

	dst += blitX + blitY * dstStride;

	for (scanline = blitY; scanline > blitY - blitH; scanline--) {
		float normalScan = scanlineCounter * scaleY; /* ScaleY is inverted */
		unsigned char *scan0 = rle->scans + RLE_BYTES_PER_SCANLINE * (int)normalScan;
		unsigned char *scan1 = scan0 + RLE_BYTES_PER_SCANLINE;
		normalScan -= (int)normalScan;
		interpolateScan(scan, scan0, scan1, normalScan);
		input = scan;
		scanlineCounter++;

		if (scanline < 0 || scanline >= dstH)
			continue;
		for (streak = 0; streak < RLE_STREAKS_PER_SCANLINE; streak++) {
			streakPos = (*input++ * scaleXFixed) >> RLE_FIXED_BITS;
			streakLength = (*input++ * scaleXFixed) >> RLE_FIXED_BITS;

			if ((streakPos + blitX) <= 0)
				continue;

			output = dst + streakPos;

			/* Check if we need to write the first pixel as 16bit */
			if (streakLength % 2) {
				*output++ = RLE_FILL_COLOR;
			}

			/* Then, write 2 pixels at a time */
			streakLength >>= 1;
			output32 = (unsigned int *)output;
			while (streakLength--) {
				*output32++ = RLE_FILL_COLOR_32;
			}
		}

		dst -= dstStride;
	}
}
