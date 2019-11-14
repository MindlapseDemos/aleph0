#ifndef VBE_H_
#define VBE_H_

#include <stdio.h>
#include "inttypes.h"
#include "util.h"

#pragma pack (push, 1)
struct vbe_info {
	char sig[4];
	uint16_t ver;
	char *oem_name;
	uint32_t caps;
	uint16_t *modes;
	uint16_t vmem_blk;	/* video memory size in 64k blocks */
	uint16_t oem_ver;
	char *vendor;
	char *product;
	char *revstr;
	uint16_t accel_ver;
	uint16_t *accel_modes;
	char reserved[216];
	char oem_data[256];
} PACKED;

struct vbe_mode_info {
	uint16_t attr;
	uint8_t wina_attr, winb_attr;
	uint16_t win_gran, win_size;
	uint16_t wina_seg, winb_seg;
	uint32_t win_func;
	uint16_t scanline_bytes;

	/* VBE 1.2 and above */
	uint16_t xres, yres;
	uint8_t xcharsz, ycharsz;
	uint8_t num_planes;
	uint8_t bpp;
	uint8_t num_banks;
	uint8_t mem_model;
	uint8_t bank_size;		/* bank size in KB */
	uint8_t num_img_pages;
	uint8_t reserved1;

	/* direct color fields */
	uint8_t rsize, rpos;
	uint8_t gsize, gpos;
	uint8_t bsize, bpos;
	uint8_t xsize, xpos;
	uint8_t cmode_info;		/* direct color mode attributes */

	/* VBE 2.0 and above */
	uint32_t fb_addr;		/* physical address of the linear framebuffer */
	uint32_t os_addr;		/* phys. address of off-screen memory */
	uint16_t os_size;		/* size in KB of off-screen memory */

	/* VBE 3.0 and above */
	uint16_t lfb_scanline_bytes;
	uint8_t banked_num_img_pages;
	uint8_t lfb_num_img_pages;
	uint8_t lfb_rsize, lfb_rpos;
	uint8_t lfb_gsize, lfb_gpos;
	uint8_t lfb_bsize, lfb_bpos;
	uint8_t lfb_xsize, lfb_xpos;
	uint32_t max_pixel_clock;

	uint8_t reserved2[190];
} PACKED;
#pragma pack (pop)

enum {
	VBE_8BIT_DAC	= 0x01,
	VBE_NON_VGA		= 0x02,
	VBE_DAC_BLANK	= 0x04,
	VBE_STEREO		= 0x08,	/* ? */
	VBE_ACCEL		= 0x08,
	VBE_STEREO_VESA	= 0x10,	/* ? */
	VBE_MUSTLOCK	= 0x10,
	VBE_HWCURSOR	= 0x20,
	VBE_HWCLIP		= 0x40,
	VBE_TRANSP_BLT	= 0x80
};

#define VBE_VER_MAJOR(v)	(((v) >> 8) & 0xff)
#define VBE_VER_MINOR(v)	((v) & 0xff)

/* VBE mode attribute flags (vbe_mode_info.attr) */
enum {
	VBE_ATTR_AVAIL		= 0x0001,
	VBE_ATTR_OPTINFO	= 0x0002,
	VBE_ATTR_TTY		= 0x0004,
	VBE_ATTR_COLOR		= 0x0008,
	VBE_ATTR_GFX		= 0x0010,
	/* VBE 2.0 */
	VBE_ATTR_NOTVGA		= 0x0020,
	VBE_ATTR_BANKED		= 0x0040,
	VBE_ATTR_LFB		= 0x0080,
	VBE_ATTR_2XSCAN		= 0x0100,
	/* VBE 3.0 */
	VBE_ATTR_ILACE		= 0x0200,	/* ! */
	VBE_ATTR_TRIPLEBUF	= 0x0400,
	VBE_ATTR_STEREO		= 0x0800,
	VBE_ATTR_STEREO_2FB	= 0x1000,
	/* VBE/AF */
	VBE_ATTR_MUSTLOCK	= 0x0200,	/* ! */
};

/* VBE memory model type (vbe_mode_info.mem_model) */
enum {
	VBE_TYPE_TEXT,
	VBE_TYPE_CGA,
	VBE_TYPE_HERCULES,
	VBE_TYPE_PLANAR,
	VBE_TYPE_PACKED,
	VBE_TYPE_UNCHAIN,
	VBE_TYPE_DIRECT,
	VBE_TYPE_YUV
};

/* VBE window attribute (vbe_mode_info.win(a|b)_attr) */
enum {
	VBE_WIN_AVAIL	= 0x01,
	VBE_WIN_RD		= 0x02,
	VBE_WIN_WR		= 0x04
};

/* mode number flags */
enum {
	VBE_MODE_RATE		= 0x0800,	/* VBE 3.0+ user-specified refresh rate */
	VBE_MODE_ACCEL		= 0x2000,	/* VBE/AF */
	VBE_MODE_LFB		= 0x4000,	/* VBE 2.0+ */
	VBE_MODE_PRESERVE	= 0x8000
};

int vbe_info(struct vbe_info *info);
int vbe_num_modes(struct vbe_info *info);
int vbe_mode_info(int mode, struct vbe_mode_info *minf);

void vbe_print_info(FILE *fp, struct vbe_info *info);
void vbe_print_mode_info(FILE *fp, struct vbe_mode_info *minf);

#endif	/* VBE_H_ */
