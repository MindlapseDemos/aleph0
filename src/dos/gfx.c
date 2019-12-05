#include <stdio.h>
#include "gfx.h"
#include "vbe.h"
#include "vga.h"
#include "cdpmi.h"

#ifdef __DJGPP__
#include <sys/nearptr.h>
#define REALPTR(s, o)	(void*)(((uint32_t)(s) << 4) - __djgpp_base_address + ((uint32_t)(o)))
#else
#define REALPTR(s, o)	(void*)(((uint32_t)(s) << 4) + ((uint32_t)(o)))
#endif

#define SAME_BPP(a, b)  \
    ((a) == (b) || ((a) == 16 && (b) == 15) || ((a) == 15 && (b) == 16) || \
     ((a) == 32 && (b) == 24) || ((a) == 24 && (b) == 32))

static int vbe_init_ver;
static struct vbe_info vbe;
static int mode, pgsize, fbsize;
static struct vbe_mode_info mode_info;

static void *vpgaddr[2];
static int fbidx;

static int init_vbe(void)
{
	int i, num;

	if(vbe_info(&vbe) == -1) {
		fprintf(stderr, "failed to retrieve VBE information\n");
		return -1;
	}

	vbe_print_info(stdout, &vbe);

	num = vbe_num_modes(&vbe);
	for(i=0; i<num; i++) {
		struct vbe_mode_info minf;

		if(vbe_mode_info(vbe.modes[i], &minf) == -1) {
			continue;
		}
		printf("%04x: ", vbe.modes[i]);
		vbe_print_mode_info(stdout, &minf);
	}
	fflush(stdout);

	vbe_init_ver = VBE_VER_MAJOR(vbe.ver);
	return 0;
}

void *set_video_mode(int xsz, int ysz, int bpp)
{
	int i, nmodes;
	int best_match_mode = -1;
	struct vbe_mode_info minf;

	if(!vbe_init_ver) {
		if(init_vbe() == -1) {
			fprintf(stderr, "failed to initialize VBE\n");
			return 0;
		}
		if(vbe_init_ver < 2) {
			fprintf(stderr, "VBE >= 2.0 required\n");
			return 0;
		}
	}

	mode = -1;
	nmodes = vbe_num_modes(&vbe);
	for(i=0; i<nmodes; i++) {
		if(vbe_mode_info(vbe.modes[i], &minf) == -1) {
			continue;
		}
		printf("trying to match mode: %d (%dx%d %dbpp)\n", vbe.modes[i],
				minf.xres, minf.yres, minf.bpp);
		if(minf.xres != xsz || minf.yres != ysz) continue;
		if(minf.bpp == bpp) {
			mode = vbe.modes[i];
			break;
		}
		if(SAME_BPP(minf.bpp, bpp)) {
			best_match_mode = mode;
		}
	}

	if(mode == -1) {
		if(best_match_mode == -1) {
			fprintf(stderr, "failed to find requested video mode (%dx%d %d bpp)\n", xsz, ysz, bpp);
			return 0;
		}
		mode = best_match_mode;
	}

	vbe_mode_info(mode, &mode_info);
	printf("setting video mode %x: (%dx%d %d)\n", (unsigned int)mode, mode_info.xres,
			mode_info.yres, mode_info.bpp);

	if(vbe_setmode(mode) == -1) {
		fprintf(stderr, "failed to set video mode\n");
		return 0;
	}

	pgsize = mode_info.xres * mode_info.yres * (bpp / 8);
	fbsize = mode_info.num_img_pages * pgsize;

	vpgaddr[0] = (void*)dpmi_mmap(mode_info.fb_addr, fbsize);

	if(mode_info.num_img_pages > 1) {
		vpgaddr[1] = (char*)vpgaddr[0] + pgsize;
		fbidx = 1;
		page_flip(FLIP_NOW);	/* start with the second page visible */
	} else {
		fbidx = 0;
		vpgaddr[1] = 0;
	}

	return vpgaddr[0];
}

int set_text_mode(void)
{
	vga_setmode(3);
	return 0;
}

void *page_flip(int vsync)
{
	if(!vpgaddr[1]) {
		/* page flipping not supported */
		return vpgaddr[0];
	}

	vbe_swap(fbidx ? pgsize : 0, vsync ? VBE_SWAP_VBLANK : VBE_SWAP_NOW);
	fbidx = (fbidx + 1) & 1;

	return vpgaddr[fbidx];
}
