
#include "rlebmap.h"
#include "util.h"

#include "imago2.h"

#include <stdlib.h>
#include <string.h>

void clearPairs(RleScan *scan)
{
    int i;

    for (i = 0; i < RLE_MAX_STREAK_PAIRS; i++) {
        scan->pairsFrom[i] = scan->pairsTo[i] = RLE_PAIR_DISABLED;
    }
}

void rleReset(RleBitmap *rle, int w, int h)
{
    int i;

    rle->w = w;
    rle->h = h;

    /* Allocate scans */
    rle->scans = calloc(h, sizeof(RleScan));

    /* Clear pairs */
    for (i = 0; i < h; i++) {
        clearPairs(rle->scans + i);
    }
}

RleBitmap *rleCreate(int w, int h)
{

    RleBitmap *ret = malloc(sizeof(RleBitmap));
    rleReset(ret, w, h);

    return ret;
}

void rleDestroy(RleBitmap *b)
{
    if (!b)
        return;
    free(b->scans);
    free(b);
}

void rleClear(RleBitmap *rle)
{
    int i;

    for (i = 0; i < rle->h; i++) {
        memset(rle->scans[i].streaks, 0, sizeof(RleScan));
        memset(rle->scans[i].pairsFrom, RLE_PAIR_DISABLED, RLE_MAX_STREAK_PAIRS * sizeof(RLE_TYPE));
        memset(rle->scans[i].pairsTo, RLE_PAIR_DISABLED, RLE_MAX_STREAK_PAIRS * sizeof(RLE_TYPE));
    }
}

static INLINE int overlap(RleStreak *a, RleStreak *b)
{
    int maxStart = 0;
    int minEnd = 0;
    int aEnd = a->start + a->length;
    int bEnd = b->start + b->length;

    // Maximum of starts
    maxStart = a->start > b->start ? a->start : b->start;

    // minimum of ends
    minEnd = aEnd < bEnd ? aEnd : bEnd;

    return maxStart <= minEnd;
}

/* Assumes cleared rle. */
void pairStreaks(RleBitmap *rle)
{
    int scan = 0;
    RleScan *scanA = rle->scans;
    RleScan *scanB = scanA + 1;
    int from, to;
    RLE_TYPE *pairFrom;
    RLE_TYPE *pairTo;

    if (rle->h <= 1) {
        return;
    }

    for (scan = 0; scan < rle->h - 1; scan++) {
        pairFrom = scanA->pairsFrom;
        pairTo = scanA->pairsTo;
        for (from = 0; from < RLE_STREAKS_PER_SCANLINE; from++) {
            for (to = 0; to < RLE_STREAKS_PER_SCANLINE; to++) {
                if (overlap(scanA->streaks + from, scanB->streaks + to)) {
                    *pairFrom++ = from;
                    *pairTo++ = to;
                }
            }
        }

        scanA++;
        scanB++;
    }
}

RleBitmap *rleEncode(RleBitmap *rle, unsigned char *pixels, int pixelsW, int pixelsH)
{
    int x = 0;
    int y = 0;
    int streakActive = 0;
    int currentStreakLength = 0;
    unsigned char *currentInputPixel = pixels;

    /* https://www.youtube.com/watch?v=RKMR02o1I88&feature=youtu.be&t=55 */
    if (!rle)
        rle = rleCreate(pixelsW, pixelsH);
    else
        rleClear(rle); /* The following code assumes cleared array */

    if (pixelsH > rle->h) {
        return rle; // break hard
    }

    for (y = 0; y < (int)pixelsH; y++) {
        RleScan *scan = rle->scans + y;
        int streakIndex = -1;
        currentInputPixel = pixels + y * pixelsW;

        for (x = 0; x < (int)pixelsW; x++) {
            if (*currentInputPixel++) {
                if (!streakActive) {
                    /* Begin new streak */
                    streakIndex++;
                    if (streakIndex >= RLE_STREAKS_PER_SCANLINE) {
                        /* Simplify your minifx if it takes more streaks per scan than
                         * supported */
                        streakActive = 0;
                        break;
                    }
                    scan->streaks[streakIndex].start = (RLE_TYPE)x;
                    currentStreakLength = 0;
                    streakActive = 1;
                }
                currentStreakLength++;
            } else {
                if (streakActive) {
                    /* Close current streak */
                    scan->streaks[streakIndex].length = (RLE_TYPE)currentStreakLength;
                    currentStreakLength = 0;
                    streakActive = 0;
                }
            } /* End if (current pixel on) */
        } /* End for (all x) */

        /* We reached the end of the scan - close any active streak */
        if (streakActive) {
            scan->streaks[streakIndex].length = (RLE_TYPE)currentStreakLength;
        }
        streakActive = 0;
        currentStreakLength = 0;
    } /* End for (all scans */

    pairStreaks(rle);

    return rle;
}

/* Big streak, to hold streaks more than 256 length, using shorts. */
typedef struct
{
    unsigned short start;
    unsigned short length;
} BigStreak;

/* Buffer for interpolating scans. */
BigStreak interpolatedStreaks[RLE_MAX_STREAK_PAIRS];

/* Produces interpolated streaks. Returns number of streaks placed in the buffer */
static INLINE int lerpScans(RleScan *a, RleScan *b, float t, float scale, BigStreak *streaks)
{
    int i;
    float omt = 1.0f - t;
    int streakCount = 0;
    RleStreak *streakA, *streakB;

    /* Clear all to zero */
    for (i=0; i<RLE_MAX_STREAK_PAIRS; i++) {
        streaks[i].start = streaks[i].length = 0;
    }

    /* Interpolate pairs */
    for (i=0; i<RLE_MAX_STREAK_PAIRS; i++) {
        if (a->pairsFrom[i] == RLE_PAIR_DISABLED) {
            break;
        }

        streakA = a->streaks + a->pairsFrom[i];
        streakB = b->streaks + a->pairsTo[i];

        streaks->start = (int) ((streakA->start * omt + streakB->start * t) * scale + 0.5f);
        streaks->length = (int) ((streakA->length * omt + streakB->length * t) * scale + 0.5f);
        streaks++;
        streakCount++;
    }

    return streakCount;
}

void rleBlitScale(RleBitmap *rle, unsigned short *dst, int dstW, int dstH, int dstStride,
                  int blitCenterX, int blitCenterY, float scaleX, float scaleY)
{
    /* Do it with floats now and optimize later */

    int screenX, screenY;
    int screenW, screenH;
    int j, k;
    int streakCount;

    int currentScanIndex;
    RleScan *currentScan, *nextScan;

    float v; /* v coordinate of the "texture" */
    float lerpFactor; /* What remains of v after we multiply by height and remove integer part. */

    /* For rasterization */
    BigStreak *currentStreak;
    int currentStreakStart;
    int currentStreakLength;
    int currentPenPosition;

    screenW = (int)(rle->w * scaleX + 0.5);
    screenH = (int)(rle->h * scaleY + 0.5);

    if (screenW < 0) {
        /* Don't support for now */
        return;
    }

    if (screenH < 0) {
        screenH *= -1;
    }

    screenX = blitCenterX - screenW / 2;
    screenY = blitCenterY - screenH / 2;

    

    for (j = screenY; j < (screenY + screenH); j++) {
        if (j < 0 || j >= dstH) {
            continue;
        }

        v = (j - screenY) / (float)screenH;
        if (scaleY < 0) {
            v = 1.0f - v;
        }
        
        if (v < 0 || v >= 1) {
            continue;
        }


        lerpFactor = v * (rle->h - 1);
        currentScanIndex = (int) lerpFactor;
        lerpFactor -= currentScanIndex;

        /* Interpolate scans */
        currentScan = rle->scans + currentScanIndex;
        nextScan = currentScan + 1;
        streakCount = lerpScans(currentScan, nextScan, lerpFactor, scaleX, interpolatedStreaks);

        /* Then render the streaks one after another */
        currentPenPosition = screenX;
        if (currentPenPosition < 0) {
            currentPenPosition = 0;
        }
        for (k=0; k<streakCount; k++) {
            currentStreak = interpolatedStreaks + k;
            currentStreakStart = screenX + currentStreak->start;
            currentStreakLength = currentStreak->length;
            
            if (currentStreakStart > currentPenPosition) {
                currentPenPosition = currentStreakStart;
            }

            while (currentPenPosition < currentStreakStart + currentStreakLength) {
                if (currentPenPosition >= dstW) {
                    break;
                }
                dst[currentPenPosition + j * dstStride] = RLE_FILL_COLOR;
                currentPenPosition++;
            }
        }

    }
}

void rleInterpolate(RleBitmap *a, RleBitmap *b, float t, RleBitmap *result)
{
    int i, j;
    RleScan *scanA, *scanB, *scanResult;
    RleStreak *streakA, *streakB, *streakResult;
    float centerA, centerB, center, length;    
    float omt = 1.0f - t;

    rleClear(result);
    if (a->h != b->h) {
        return;
    }

    if (result->h != a->h) {
        rleReset(result, a->w, a->h);
    }

    scanA = a->scans;
    scanB = b->scans;
    scanResult = result->scans;
    for (i=0; i<a->h; i++) {
        streakA = scanA->streaks;
        streakB = scanB->streaks;
        streakResult = scanResult->streaks;

        for (j=0; j<RLE_STREAKS_PER_SCANLINE; j++) {
            if (streakA->length <= 0 && streakB->length <= 0) {
                break; // no more streaks
            }

            if (streakA->length > 0 && streakB->length > 0) {
                centerA = streakA->start + 0.5 * streakA->length;
                centerB = streakB->start + 0.5 * streakB->length;
                center = omt * centerA + t * centerB;
                length = omt * streakA->length + t * streakB->length;
            } else if (streakA->length > 0) {
                // StreakA only - fade out
                center = streakA->start + 0.5f * streakA->length;
                length = omt * streakA->length;
            } else {
                // StreakB only - fade in
                center = streakB->start + 0.5f * streakB->length;
                length = t * streakB->length;
            }

            // Write interpolated streak
            streakResult->start = (int) (center - 0.5f * length + 0.5f);
            streakResult->length = (int) (length + 0.5f);

            streakA++;
            streakB++;
            streakResult++;
        }

        scanA++;
        scanB++;
        scanResult++;
    }

    pairStreaks(result);
}

RleBitmap *rleFromFile(char *filename)
{
    unsigned int *data = 0;
    int w, h;
    unsigned char *alpha;
    int i;
    RleBitmap *result = 0;

    if (!(data =
		  img_load_pixels(filename, &w, &h, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image %s\n", filename);
		return 0;
	}

    /* Get alpha channel only */
    alpha = malloc(w * h);

    for (i=0; i<w*h; i++) {
        alpha[i] = data[i] >> 24;
    }

    free(data);

    /* Encode */
    result = rleEncode(0, alpha, w, h);
	/*
    for (i=0; i<result->h; i++) {
        printf("(%d, %d) ", result->scans[i].streaks[0].start, result->scans[i].streaks[0].length);
        printf("(%d, %d) ", result->scans[i].streaks[1].start, result->scans[i].streaks[1].length);
        printf("(%d, %d) ", result->scans[i].streaks[2].start, result->scans[i].streaks[2].length);
        printf("(%d, %d) ", result->scans[i].streaks[3].start, result->scans[i].streaks[3].length);
        printf("\n");
    }
	*/

    free(alpha);

    return result;
}
