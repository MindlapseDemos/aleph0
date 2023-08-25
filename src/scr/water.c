/* Water effect */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "opt_rend.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"water",
	init,
	destroy,
	start,
	0,
	draw
};


static unsigned long startingTime;

static unsigned short *waterPal;
static unsigned int *waterPal32;

static unsigned char *waterBuffer1;
static unsigned char *waterBuffer2;

unsigned char *wb1, *wb2;


static void swapWaterBuffers()
{
	unsigned char *temp = wb2;
	wb2 = wb1;
	wb1 = temp;
}

struct screen *water_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i,j,k;

	const int size = FB_WIDTH * FB_HEIGHT;

	waterBuffer1 = (unsigned char*)malloc(size);
	waterBuffer2 = (unsigned char*)malloc(size);
	memset(waterBuffer1, 0, size);
	memset(waterBuffer2, 0, size);

	wb1 = waterBuffer1;
	wb2 = waterBuffer2;

	waterPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
	waterPal32 = (unsigned int*)malloc(sizeof(unsigned int) * 256 * 256);

	setPalGradient(0,127, 0,0,0, 31,63,31, waterPal);
	setPalGradient(128,255, 0,0,0, 0,0,0, waterPal);

	k = 0;
	for (j=0; j<256; ++j) {
		const int c0 = waterPal[j];
		for (i=0; i<256; ++i) {
			const int c1 = waterPal[i];
			waterPal32[k++] = (c0 << 16) | c1;
		}
	}

	return 0;
}

static void destroy(void)
{
	free(waterBuffer1);
	free(waterBuffer2);
	free(waterPal);
	free(waterPal32);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void waterBufferToVram32()
{
	int i;
	unsigned short *src = (unsigned short*)wb1;
	unsigned int *dst = (unsigned int*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT / 2; ++i) {
		*dst++ = waterPal32[*src++];
	}
}

#ifdef __WATCOMC__
	static void updateWaterAsm5(void *buffer1, void *buffer2, void *vramStart)
	{
		_asm
		{
			push ebp

			mov esi,buffer1
			mov edi,buffer2

			mov ecx,(FB_WIDTH / 4) * (FB_HEIGHT - 2) - 2;
			waterI:
				mov eax,[esi-1]
				mov ebx,[esi+1]
				add eax,[esi-FB_WIDTH]
				add ebx,[esi+FB_WIDTH]
				add eax,ebx
				shr eax,1
				and eax,0x7f7f7f7f
				mov edx,[edi]
				sub eax,edx

				mov ebx,eax
				and ebx,0x80808080

				mov edx,ebx
				shr edx,7

				mov ebp,ebx
				sub ebp,edx
				or ebx,ebp
				xor eax,ebx
				add eax,edx

				add esi,4
				mov [edi],eax
				add edi,4

			dec ecx
			jnz waterI

			pop ebp
		}
	}
#else
	static void updateWater32(unsigned char *buffer1, unsigned char *buffer2)
	{
		int count = (FB_WIDTH / 4) * (FB_HEIGHT - 2) - 2;

		unsigned int *src1 = (unsigned int*)buffer1;
		unsigned int *src2 = (unsigned int*)buffer2;
		unsigned int *vram = (unsigned int*)((unsigned char*)wb1 + FB_WIDTH + 4);

		do {
			const unsigned int c0 = *(unsigned int*)((unsigned char*)src1-1);
			const unsigned int c1 = *(unsigned int*)((unsigned char*)src1+1);
			const unsigned int c2 = *(src1-(FB_WIDTH / 4));
			const unsigned int c3 = *(src1+(FB_WIDTH / 4));

			// Subtract and then absolute value of 4 bytes packed in 8bits (From Hacker's Delight)
			const unsigned int c = (((c0 + c1 + c2 + c3) >> 1) & 0x7f7f7f7f) - *src2;
			const unsigned int a = c & 0x80808080;
			const unsigned int b = a >> 7;
			const unsigned int cc = (c ^ ((a - b) | a)) + b;

			*src2++ = cc;
			src1++;
		} while (--count > 0);
	}
#endif

static void renderBlob(int xp, int yp, unsigned char *buffer)
{
	int i,j;
	unsigned char *dst = buffer + yp * FB_WIDTH + xp;

	for (j=0; j<3; ++j) {
		for (i=0; i<3; ++i) {
			*(dst + i) = 0x3f;
		}
		dst += FB_WIDTH;
	}
}

static void makeRipples(unsigned char *buff)
{
	int i;

	renderBlob(32 + (rand() & 255), 32 + (rand() & 127), buff);

	for (i=0; i<3; ++i) {
		const int xp = FB_WIDTH / 2 + (int)(sin((time_msec + 64*i)/24.0) * 128);
		const int yp = FB_HEIGHT / 2 + (int)(sin((time_msec + 64*i)/16.0) * 64);

		renderBlob(xp,yp,buff);
	}
}

static void draw(void)
{
	unsigned char *buff1 = wb1 + FB_WIDTH + 4;
	unsigned char *buff2 = wb2 + FB_WIDTH + 4;
	unsigned char *vramOffset = (unsigned char*)buff1 + FB_WIDTH + 4;

	makeRipples(buff1);
	
	#ifdef __WATCOMC__
		updateWaterAsm5(buff1, buff2, vramOffset);
	#else
		updateWater32(buff1, buff2);
	#endif

	waterBufferToVram32();

	swapWaterBuffers();

	swap_buffers(0);
}
