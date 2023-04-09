#ifndef __RLE_BITMAP_H__
#define __RLE_BITMAP_H__

/* Limit streak count per scanline so we can directly jump to specific scanline
 */
#define RLE_STREAKS_PER_SCANLINE 4

/* If two streaks from consecutive scanlines overlap, they must be interpolated.
 * We have this many such pairs in a scan. */
#define RLE_MAX_STREAK_PAIRS (RLE_STREAKS_PER_SCANLINE * RLE_STREAKS_PER_SCANLINE)

/* Using the following type for storing start and for storing length in (start,
 * length) pairs. */
#define RLE_TYPE unsigned char

/* For now, keep a static fill color. We can change this. Not that much about
 * speed, but let's keep function definitions more compact. */
#define RLE_FILL_COLOR 0xFFFF

/* Two entries of RLE_FILL_COLOR (16 bits) packed one after the other. */
#define RLE_FILL_COLOR_32 ((RLE_FILL_COLOR << 16) | RLE_FILL_COLOR)

/* For fixed-point arithmetic. Used for scaling. */
#define RLE_FIXED_BITS 8

typedef struct {
    RLE_TYPE start;
    RLE_TYPE length;
} RleStreak;

#define RLE_PAIR_DISABLED 0xFF

typedef struct {
    RleStreak streaks[RLE_STREAKS_PER_SCANLINE];

    /* Holds the pairs with subsequent scan. Indices to this scan's streak ... */
    RLE_TYPE pairsFrom[RLE_MAX_STREAK_PAIRS];

    /* ...corresponding to indices of the next scan's streak: */
    RLE_TYPE pairsTo[RLE_MAX_STREAK_PAIRS];
} RleScan;

/* This is a bitmap (image in 1bpp), encoded as streaks of consecutive pixels.
 */
typedef struct {
    int w;
    int h;
    RleScan *scans;
} RleBitmap;

/* Constructor */
RleBitmap *rleCreate(int w, int h);

/* Destructor */
void rleDestroy(RleBitmap *rle);

/* Clears 'rle' to "all transparent" */
void rleClear(RleBitmap *rle);

/* Resets the data in rle. */
void rleReset(RleBitmap *rle, int w, int h);

/* Encode 'pixels' into 'rle' and also return it. Pixels are either 0 or 1. This
 * will create an RleBitmap of 'h' scanlines. */
RleBitmap *rleEncode(RleBitmap *rle, unsigned char *pixels, int pixelsW, int pixelsH);

/* Scaled blit. */
void rleBlitScale(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride,
                  int blitCenterX, int blitCenterY, float scaleX, float scaleY);

/* Interpolates between bitmaps a and b. Stores in 'result'. Only bitmaps of the same height accepted. Otherwise returns cleared. */
void rleInterpolate(RleBitmap *a, RleBitmap *b, float t, RleBitmap *result);

/* Loads bitmap from file and encodes rle from it. */
RleBitmap *rleFromFile(char *filename);

#endif // __RLE_BITMAP_H__
