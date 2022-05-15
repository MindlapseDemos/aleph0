#ifndef __RLE_BITMAP_H__
#define __RLE_BITMAP_H__

/* Limit streak count per scanline so we can directly jump to specific scanline
 */
#define RLE_STREAKS_PER_SCANLINE 4

/* Streaks will not exceed this many pixels. This allows us to optimize with a
 * padded framebuffer. If a streak start position happens to lie within
 * framebuffer, we will blit it without checking for out of bounds */
#define RLE_MAX_STREAK_LENGTH 32

/* Using the following type for storing start and for storing length in (start,
 * length) pairs. */
#define RLE_TYPE unsigned char

/* For now, keep a static fill color. We can change this. Not that much about
 * speed, but let's keep function definitions more compact. */
#define RLE_FILL_COLOR 0

/* Two entries of RLE_FILL_COLOR (16 bits) packed one after the other. */
#define RLE_FILL_COLOR_32 ((RLE_FILL_COLOR << 16) | RLE_FILL_COLOR)

/* For fixed-point arithmetic. Used for scaling. */
#define RLE_FIXED_BITS 16

/* This is a bitmap (image in 1bpp), encoded as streaks of consecutive pixels.
 */
typedef struct {
	unsigned int w, h;

	/* Each scan is RLE_BYTES_PER_SCANLINE long and contains pairs of
	 * (start, length). */
	RLE_TYPE *scans;
} RleBitmap;

/* Constructor */
RleBitmap *rleCreate(unsigned int w, unsigned int h);

/* Destructor */
void rleDestroy(RleBitmap *rle);

/* Clears 'rle' to "all transparent" */
void rleClear(RleBitmap *rle);

/* Encode 'pixels' into 'rle' and also return it. Pixels are either 0 or 1. This
 * will create an RleBitmap of 'h' scanlines. */
RleBitmap *rleEncode(RleBitmap *rle, unsigned char *pixels, unsigned int pixelsW,
		     unsigned int pixelsH);

/* Rearranges the streaks to make it less frequent that they produce garbege when interpolated */
void rleDistributeStreaks(RleBitmap *rle);

/* Blits without scaling */
void rleBlit(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride, int blitX,
	     int blitY);

/* Scaled blit */
void rleBlitScale(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride, int blitX,
		  int blitY, float scaleX, float scaleY);

/* Inverted blit (upside down) */
void rleBlitScaleInv(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride,
		     int blitX, int blitY, float scaleX, float scaleY);

#endif // __RLE_BITMAP_H__
