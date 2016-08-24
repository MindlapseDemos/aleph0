#include <stdio.h>
#include <string.h>
#include "vbe.h"
#include "dpmi.h"

/* VGA DAC registers used for palette setting in 8bpp modes */
#define VGA_DAC_STATE		0x3c7
#define VGA_DAC_ADDR_RD		0x3c7
#define VGA_DAC_ADDR_WR		0x3c8
#define VGA_DAC_DATA		0x3c9

#define MODE_LFB	(1 << 14)


struct vbe_info *vbe_get_info(void)
{
	static unsigned short info_block_seg;
	static struct vbe_info *info;
	struct dpmi_real_regs regs;

	if(!info) {
		/* allocate 32 paragraphs (512 bytes) */
		info_block_seg = dpmi_alloc(32);
		info = (struct vbe_info*)(info_block_seg << 4);
	}

	memcpy(info->sig, "VBE2", 4);

	memset(&regs, 0, sizeof regs);
	regs.es = info_block_seg;
	regs.eax = 0x4f00;

	dpmi_real_int(0x10, &regs);

	return info;
}

struct vbe_mode_info *vbe_get_mode_info(int mode)
{
	static unsigned short mode_info_seg;
	static struct vbe_mode_info *mi;
	struct dpmi_real_regs regs;

	if(!mi) {
		/* allocate 16 paragraphs (256 bytes) */
		mode_info_seg = dpmi_alloc(16);
		mi = (struct vbe_mode_info*)(mode_info_seg << 4);
	}

	memset(&regs, 0, sizeof regs);
	regs.es = mode_info_seg;
	regs.eax = 0x4f01;
	regs.ecx = mode;
	regs.es = mode_info_seg;

	dpmi_real_int(0x10, &regs);
	if(regs.eax & 0xff00) {
		return 0;
	}

	return mi;
}

int vbe_set_mode(int mode)
{
	struct dpmi_real_regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = 0x4f02;
	regs.ebx = mode;
	dpmi_real_int(0x10, &regs);

	if(regs.eax == 0x100) {
		return -1;
	}
	return 0;
}

int vbe_set_palette_bits(int bits)
{
	struct dpmi_real_regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = 0x4f08;
	regs.ebx = bits << 8;	/* bits in bh */
	dpmi_real_int(0x10, &regs);

	if((regs.eax >> 8) & 0xff == 3) {
		return -1;
	}
	return regs.ebx >> 8 & 0xff;	/* new color bits in bh */
}

/* TODO: implement palette setting through the VBE2 interface for
 * non-VGA displays (actually don't).
 */
void vbe_set_palette(int idx, int *col, int count, int bits)
{
	int i, shift = 8 - bits;

	__asm {
		mov dx, VGA_DAC_ADDR_WR
		mov eax, idx
		out dx, al
	}

	for(i=0; i<count; i++) {
		unsigned char r = *col++;
		unsigned char g = *col++;
		unsigned char b = *col++;

		if(shift) {
			r >>= shift;
			g >>= shift;
			b >>= shift;
		}

		__asm {
			mov dx, VGA_DAC_DATA
			mov al, r
			out dx, al
			mov al, g
			out dx, al
			mov al, b
			out dx, al
		}
	}
}

static unsigned int get_mask(int sz, int pos)
{
	unsigned int i, mask = 0;

	for(i=0; i<sz; i++) {
		mask |= 1 << i;
	}
	return mask << pos;
}

void print_mode_info(FILE *fp, struct vbe_mode_info *mi)
{
	fprintf(fp, "resolution: %dx%d\n", mi->xres, mi->yres);
	fprintf(fp, "color depth: %d\n", mi->bpp);
	fprintf(fp, "mode attributes: %x\n", mi->mode_attr);
	fprintf(fp, "bytes per scanline: %d\n", mi->scanline_bytes);
	fprintf(fp, "number of planes: %d\n", (int)mi->num_planes);
	fprintf(fp, "number of banks: %d\n", (int)mi->num_banks);
	fprintf(fp, "mem model: %d\n", (int)mi->mem_model);
	fprintf(fp, "red bits: %d (mask: %x)\n", (int)mi->rmask_size, get_mask(mi->rmask_size, mi->rpos));
	fprintf(fp, "green bits: %d (mask: %x)\n", (int)mi->gmask_size, get_mask(mi->gmask_size, mi->gpos));
	fprintf(fp, "blue bits: %d (mask: %x)\n", (int)mi->bmask_size, get_mask(mi->bmask_size, mi->bpos));
	fprintf(fp, "framebuffer address: %x\n", mi->fb_addr);
}
