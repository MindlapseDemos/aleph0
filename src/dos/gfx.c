#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "gfx.h"
#include "vbe.h"
#include "cdpmi.h"

#ifdef __DJGPP__
#include <sys/nearptr.h>

#define REALPTR(s, o)	(void*)(((uint32_t)(s) << 4) - __djgpp_base_address + (uint32_t)(o))
#else
#define REALPTR(s, o)	(void*)(((uint32_t)(s) << 4) + (uint32_t)(o))
#endif

#define VBEPTR(x)		REALPTR(((x) & 0xffff0000) >> 16, (x) & 0xffff)
#define VBEPTR_SEG(x)	(((x) & 0xffff0000) >> 16)
#define VBEPTR_OFF(x)	((x) & 0xffff)

#define SAME_BPP(a, b)	\
	((a) == (b) || ((a) == 16 && (b) == 15) || ((a) == 15 && (b) == 16) || \
	 ((a) == 32 && (b) == 24) || ((a) == 24 && (b) == 32))

static unsigned int make_mask(int sz, int pos);

static struct vbe_info *vbe_info;
static struct vbe_mode_info *mode_info;
static int pal_bits = 6;

static void *vpgaddr[2];


void *set_video_mode(int xsz, int ysz, int bpp)
{
	int i;
	static uint16_t *modes;
	uint16_t best = 0;
	unsigned int fbsize, pgsize;

#ifdef __DJGPP__
	__djgpp_nearptr_enable();
#endif

	/* check for VBE2 support and output some info */
	if(!vbe_info) {
		if(!(vbe_info = vbe_get_info())) {
			fprintf(stderr, "VESA BIOS Extensions not available\n");
			return 0;
		}

		printf("VBE Version: %x.%x\n", vbe_info->version >> 8, vbe_info->version & 0xff);
		if(vbe_info->version < 0x200) {
			fprintf(stderr, "This program requires VBE 2.0 or greater. Try running UniVBE\n");
			return 0;
		}

		printf("Graphics adapter: %s, %s (%s)\n", (char*)VBEPTR(vbe_info->oem_vendor_name_ptr),
				(char*)VBEPTR(vbe_info->oem_product_name_ptr), (char*)VBEPTR(vbe_info->oem_product_rev_ptr));
		printf("Video memory: %dkb\n", vbe_info->total_mem << 6);

		modes = VBEPTR(vbe_info->vid_mode_ptr);
	}

	for(i=0; i<1024; i++) {	/* impose an upper limit to avoid inf-loops */
		if(modes[i] == 0xffff) {
			break;	/* reached the end */
		}

		mode_info = vbe_get_mode_info(modes[i] | VBE_MODE_LFB);
		if(!mode_info || mode_info->xres != xsz || mode_info->yres != ysz) {
			continue;
		}
		if(SAME_BPP(mode_info->bpp, bpp)) {
			best = modes[i];
		}
	}

	if(best) {
		mode_info = vbe_get_mode_info(best);
	} else {
		fprintf(stderr, "Requested video mode (%dx%d %dbpp) is unavailable\n", xsz, ysz, bpp);
		return 0;
	}

	if(vbe_set_mode(best | VBE_MODE_LFB) == -1) {
		fprintf(stderr, "Failed to set video mode %dx%d %dbpp\n", mode_info->xres, mode_info->yres, mode_info->bpp);
		return 0;
	}

	/* attempt to set 8 bits of color per component in palettized modes */
	/*if(bpp <= 8) {
		pal_bits = vbe_set_palette_bits(8);
		printf("palette bits per color primary: %d\n", pal_bits);
	}
	*/

	printf("avail video pages: %d\n", mode_info->num_img_pages);
	printf("bytes per scanline: %d (%d pixels)\n", vbe_get_scanlen(VBE_SCANLEN_BYTES),
			vbe_get_scanlen(VBE_SCANLEN_PIXELS));

	pgsize = xsz * ysz * (bpp / CHAR_BIT);
	fbsize = mode_info->num_img_pages * pgsize;
	vpgaddr[0] = (void*)dpmi_mmap(mode_info->fb_addr, fbsize);

	if(mode_info->num_img_pages > 1) {
		vpgaddr[1] = (char*)vpgaddr[0] + pgsize;
	} else {
		vpgaddr[1] = 0;
	}
	return vpgaddr[0];
}

int set_text_mode(void)
{
	vbe_set_mode(0x3);
	return 0;
}

int get_color_depth(void)
{
	if(!mode_info) {
		return -1;
	}
	return mode_info->bpp;
}

int get_color_bits(int *rbits, int *gbits, int *bbits)
{
	if(!mode_info) {
		return -1;
	}
	*rbits = mode_info->rmask_size;
	*gbits = mode_info->gmask_size;
	*bbits = mode_info->bmask_size;
	return 0;
}

int get_color_mask(unsigned int *rmask, unsigned int *gmask, unsigned int *bmask)
{
	if(!mode_info) {
		return -1;
	}
	*rmask = make_mask(mode_info->rmask_size, mode_info->rpos);
	*gmask = make_mask(mode_info->gmask_size, mode_info->gpos);
	*bmask = make_mask(mode_info->bmask_size, mode_info->bpos);
	return 0;
}

int get_color_shift(int *rshift, int *gshift, int *bshift)
{
	if(!mode_info) {
		return -1;
	}
	*rshift = mode_info->rpos;
	*gshift = mode_info->gpos;
	*bshift = mode_info->bpos;
	return 0;
}

void set_palette(int idx, int r, int g, int b)
{
	int col[3];
	col[0] = r;
	col[1] = g;
	col[2] = b;
	vbe_set_palette(idx, col, 1, pal_bits);
}

void *page_flip(int vsync)
{
	static int frame;
	void *nextaddr;
	int y, when, bufidx;

	if(!vpgaddr[1]) {
		/* page flipping not supported */
		return 0;
	}

	bufidx = ++frame & 1;

	y = bufidx ? mode_info->yres : 0;
	nextaddr = vpgaddr[bufidx];

	when = vsync ? VBE_SET_DISP_START_VBLANK : VBE_SET_DISP_START_NOW;
	if(vbe_set_disp_start(0, y, when) == -1) {
		return 0;
	}

	if(vsync == FLIP_VBLANK_WAIT) {
		wait_vsync();
	}
	return nextaddr;
}

static unsigned int make_mask(int sz, int pos)
{
	unsigned int i, mask = 0;

	for(i=0; i<sz; i++) {
		mask |= 1 << i;
	}
	return mask << pos;
}
