#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include "vbe.h"
#include "cdpmi.h"


#define FIXPTR(ptr) \
	do { \
		uint32_t paddr = (uint32_t)(ptr); \
		uint16_t pseg = paddr >> 16; \
		uint16_t poffs = paddr & 0xffff; \
		if(pseg == seg && poffs < 512) { \
			paddr = ((uint32_t)seg << 4) + poffs; \
		} else { \
			paddr = ((uint32_t)pseg << 4) + poffs; \
		} \
		(ptr) = (void*)paddr; \
	} while(0)

/* hijack the "VESA" sig field, to pre-cache number of modes */
#define NMODES(inf) *(uint16_t*)((inf)->sig)
#define NACCMODES(inf) *(uint16_t*)((inf)->sig + 2)

int vbe_info(struct vbe_info *info)
{
	int i, num;
	void *lowbuf;
	uint16_t seg, sel;
	uint16_t *modeptr;
	struct dpmi_regs regs = {0};

	assert(sizeof *info == 512);

	if(!(seg = dpmi_alloc(sizeof *info / 16, &sel))) {
		return -1;
	}
	lowbuf = (void*)((uint32_t)seg << 4);

	memcpy(lowbuf, "VBE2", 4);

	regs.eax = 0x4f00;
	regs.es = seg;
	regs.edi = 0;
	dpmi_int(0x10, &regs);

	if((regs.eax & 0xffff) != 0x4f) {
		fprintf(stderr, "vbe_get_info (4f00) failed\n");
		dpmi_free(sel);
		return -1;
	}

	memcpy(info, lowbuf, sizeof *info);
	dpmi_free(sel);

	FIXPTR(info->oem_name);
	FIXPTR(info->vendor);
	FIXPTR(info->product);
	FIXPTR(info->revstr);
	FIXPTR(info->modes);
	FIXPTR(info->accel_modes);

	modeptr = info->modes;
	while(*modeptr != 0xffff) {
		if(modeptr - info->modes >= 256) {
			modeptr = info->modes;
			break;
		}
		modeptr++;
	}
	NMODES(info) = modeptr - info->modes;

	if(info->caps & VBE_ACCEL) {
		modeptr = info->accel_modes;
		while(*modeptr != 0xffff) {
			if(modeptr - info->accel_modes >= 256) {
				modeptr = info->accel_modes;
				break;
			}
			modeptr++;
		}
		NACCMODES(info) = modeptr - info->accel_modes;
	}
	return 0;
}

int vbe_num_modes(struct vbe_info *info)
{
	return NMODES(info);
}

int vbe_mode_info(int mode, struct vbe_mode_info *minf)
{
	int i, num;
	void *lowbuf;
	uint16_t seg, sel;
	struct dpmi_regs regs = {0};

	assert(sizeof *minf == 256);
	assert(offsetof(struct vbe_mode_info, max_pixel_clock) == 0x3e);

	if(!(seg = dpmi_alloc(sizeof *minf / 16, &sel))) {
		return -1;
	}
	lowbuf = (void*)((uint32_t)seg << 4);

	regs.eax = 0x4f01;
	regs.ecx = mode;
	regs.es = seg;
	dpmi_int(0x10, &regs);

	if((regs.eax & 0xffff) != 0x4f) {
		fprintf(stderr, "vbe_mode_info (4f01) failed\n");
		dpmi_free(sel);
		return -1;
	}

	memcpy(minf, lowbuf, sizeof *minf);
	dpmi_free(sel);
	return 0;
}

void vbe_print_info(FILE *fp, struct vbe_info *vinf)
{
	fprintf(fp, "vbe version: %u.%u\n", VBE_VER_MAJOR(vinf->ver), VBE_VER_MINOR(vinf->ver));
	if(VBE_VER_MAJOR(vinf->ver) >= 2) {
		fprintf(fp, "%s - %s (%s)\n", vinf->vendor, vinf->product, vinf->revstr);
		if(vinf->caps & VBE_ACCEL) {
			fprintf(fp, "vbe/af %d.%d\n", VBE_VER_MAJOR(vinf->accel_ver), VBE_VER_MINOR(vinf->accel_ver));
		}
	} else {
		fprintf(fp, "oem: %s\n", vinf->oem_name);
	}
	fprintf(fp, "video memory: %dkb\n", vinf->vmem_blk * 64);

	if(vinf->caps) {
		fprintf(fp, "caps:");
		if(vinf->caps & VBE_8BIT_DAC) fprintf(fp, " dac8");
		if(vinf->caps & VBE_NON_VGA) fprintf(fp, " non-vga");
		if(vinf->caps & VBE_DAC_BLANK) fprintf(fp, " dac-blank");
		if(vinf->caps & VBE_ACCEL) fprintf(fp, " af");
		if(vinf->caps & VBE_MUSTLOCK) fprintf(fp, " af-lock");
		if(vinf->caps & VBE_HWCURSOR) fprintf(fp, " af-curs");
		if(vinf->caps & VBE_HWCLIP) fprintf(fp, " af-clip");
		if(vinf->caps & VBE_TRANSP_BLT) fprintf(fp, " af-tblt");
		fprintf(fp, "\n");
	}

	fprintf(fp, "%d video modes available\n", NMODES(vinf));
	if(vinf->caps & VBE_ACCEL) {
		fprintf(fp, "%d accelerated (VBE/AF) modes available\n", NACCMODES(vinf));
	}
	fflush(fp);
}

void vbe_print_mode_info(FILE *fp, struct vbe_mode_info *minf)
{
	fprintf(fp, "%dx%d %dbpp", minf->xres, minf->yres, minf->bpp);

	switch(minf->mem_model) {
	case VBE_TYPE_DIRECT:
		fprintf(fp, " (rgb");
		if(0) {
	case VBE_TYPE_YUV:
			fprintf(fp, " (yuv");
		}
		fprintf(fp, " %d%d%d)", minf->rsize, minf->gsize, minf->bsize);
		break;
	case VBE_TYPE_PLANAR:
		fprintf(fp, " (%d planes)", minf->num_planes);
		break;
	case VBE_TYPE_PACKED:
		fprintf(fp, " (packed)");
		break;
	case VBE_TYPE_TEXT:
		fprintf(fp, " (%dx%d cells)", minf->xcharsz, minf->ycharsz);
		break;
	case VBE_TYPE_CGA:
		fprintf(fp, " (CGA)");
		break;
	case VBE_TYPE_UNCHAIN:
		fprintf(fp, " (unchained-%d)", minf->num_planes);
		break;
	}
	fprintf(fp, " %dpg", minf->num_img_pages);

	if(minf->attr & VBE_ATTR_LFB) {
		fprintf(fp, " lfb@%lx", (unsigned long)minf->fb_addr);
	}

	fprintf(fp, " [");
	if(minf->attr & VBE_ATTR_AVAIL) fprintf(fp, " avail");
	if(minf->attr & VBE_ATTR_OPTINFO) fprintf(fp, " opt");
	if(minf->attr & VBE_ATTR_TTY) fprintf(fp, " tty");
	if(minf->attr & VBE_ATTR_COLOR) fprintf(fp, " color");
	if(minf->attr & VBE_ATTR_GFX) fprintf(fp, " gfx");
	if(minf->attr & VBE_ATTR_NOTVGA) fprintf(fp, " non-vga");
	if(minf->attr & VBE_ATTR_BANKED) fprintf(fp, " banked");
	if(minf->attr & VBE_ATTR_LFB) fprintf(fp, " lfb");
	if(minf->attr & VBE_ATTR_DBLSCAN) fprintf(fp, " dblscan");
	if(minf->attr & VBE_ATTR_ILACE) fprintf(fp, " ilace");
	if(minf->attr & VBE_ATTR_TRIPLEBUF) fprintf(fp, " trplbuf");
	if(minf->attr & VBE_ATTR_STEREO) fprintf(fp, " stereo");
	if(minf->attr & VBE_ATTR_STEREO_2FB) fprintf(fp, " stdual");
	fprintf(fp, " ]\n");
	fflush(fp);
}

int vbe_setmode(uint16_t mode)
{
	struct dpmi_regs regs = {0};

	regs.eax = 0x4f02;
	regs.ebx = mode;
	dpmi_int(0x10, &regs);

	if((regs.eax & 0xffff) != 0x4f) {
		return -1;
	}
	return 0;
}

int vbe_setmode_crtc(uint16_t mode, struct vbe_crtc_info *crtc)
{
	void *lowbuf;
	uint16_t seg, sel;
	struct dpmi_regs regs = {0};

	assert(sizeof *crtc == 59);

	if(!(seg = dpmi_alloc((sizeof *crtc + 15) / 16, &sel))) {
		return -1;
	}
	lowbuf = (void*)((uint32_t)seg << 4);

	memcpy(lowbuf, crtc, sizeof *crtc);

	regs.eax = 0x4f02;
	regs.ebx = mode;
	regs.es = seg;
	dpmi_int(0x10, &regs);

	dpmi_free(sel);

	if((regs.eax & 0xffff) != 0x4f) {
		return -1;
	}
	return 0;
}
