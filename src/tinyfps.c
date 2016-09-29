#include <stdio.h>
#include <stdlib.h>

#include "tinyfps.h"
#include "demo.h"

// TinyFPS, just a minimal fraps like font to show FPS during the demo and not just after.
// I'll be using it in my effects for my performance test purposes, just adding it here.
// Maybe it would be nice if initFpsFonts would be called in demo.c once but I avoided touching that code.

/*
1110 0010 1110 1110 1010 1110 1110 1110 1110 1110
1010 0010 0010 0010 1010 1000 1000 0010 1010 1010
1010 0010 1110 0110 1110 1110 1110 0010 1110 1110
1010 0010 1000 0010 0010 0010 1010 0010 1010 0010
1110 0010 1110 1110 0010 1110 1110 0010 1110 1110
*/

static const unsigned char miniDecimalData[] = { 0xE2, 0xEE, 0xAE, 0xEE, 0xEE,
0xA2, 0x22, 0xA8, 0x82, 0xAA,
0xA2, 0xE6, 0xEE, 0xE2, 0xEE,
0xA2, 0x82, 0x22, 0xA2, 0xA2,
0xE2, 0xEE, 0x2E, 0xE2, 0xEE };

static unsigned short miniDecimalFonts[FPS_FONT_NUM_PIXELS];

static unsigned long startingFpsTime = 0;
static int fpsFontsInit = 0;
static int nFrames = 0;

void initFpsFonts()
{
	if (fpsFontsInit == 0)
	{
		unsigned char miniDecimalPixels[FPS_FONT_NUM_PIXELS];
		int i, j, k = 0;
		int x, y, n;

		for (i = 0; i < FPS_FONT_NUM_PIXELS / 8; i++)
		{
			unsigned char d = miniDecimalData[i];
			for (j = 0; j < 8; j++)
			{
				unsigned char c = (d & 0x80) >> 7;
				miniDecimalPixels[k++] = c;
				d <<= 1;
			}
		}

		i = 0;
		for (n = 0; n < FPS_FONTS_NUM; n++)
		{
			for (y = 0; y < FPS_FONT_HEIGHT; y++)
			{
				for (x = 0; x < FPS_FONT_WIDTH; x++)
				{
					miniDecimalFonts[i++] = miniDecimalPixels[n * FPS_FONT_WIDTH + x + y * FPS_FONT_WIDTH * FPS_FONTS_NUM] * 0xFFFF;
				}
			}
		}

		fpsFontsInit = 1;
	}
}

static void drawFont(unsigned char decimal, int posX, int posY, unsigned char zoom, unsigned short *vram)
{
	int x, y, j, k;
	unsigned short c;

    unsigned short *fontData = (unsigned short*)&miniDecimalFonts[decimal * FPS_FONT_WIDTH * FPS_FONT_HEIGHT];

	vram += posY * fb_width + posX;

	if (zoom < 1) zoom = 1;
	if (zoom > 4) zoom = 4;

    for (y = 0; y<FPS_FONT_HEIGHT; y++)
    {
		for (x = 0; x<FPS_FONT_WIDTH; x++)
        {
			c = *fontData++;

            if (c!=0)
            {
                for (j=0; j<zoom; j++)
                {
                    for (k=0; k<zoom; k++)
                    {
                        *(vram + j * fb_width + k) ^= c;
                    }
                }
            }
            vram += zoom;
        }
        vram += (-FPS_FONT_WIDTH * zoom + fb_width * zoom);
    }
}

static void drawDecimal(unsigned int number, int posX, int posY, unsigned char zoom, unsigned short *vram)
{
	int i = 0;
    char buffer[8];

	sprintf(buffer, "%d", number);

	while(i < 8 && buffer[i] != 0)
    {
        drawFont(buffer[i] - 48, posX + i * zoom * FPS_FONT_WIDTH, posY, zoom, vram);
		i++;
    }
}

void drawFps(unsigned short *vram)
{
	unsigned long dt = time_msec - startingFpsTime;
	static int fps = 0;

	nFrames++;
	if (dt >= 1000)
	{
		fps = (nFrames * 1000) / dt;
		startingFpsTime = time_msec;
		nFrames = 0;
	}
	//drawDecimal(fps, 4, 4, 2, vram);
	// Moving this on the lower left side of screen for now, since the lack of double buffering generates flickering for this atm
	drawDecimal(fps, 4, fb_height - 12, 2, vram);
}
