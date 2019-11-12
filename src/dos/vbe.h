#ifndef VBE_H_
#define VBE_H_

#include "inttypes.h"
#include "util.h"

#pragma pack (push, 1)
struct vbe_info {
	char sig[4];
	uint16_t ver;
	char *oem_name;
	uint32_t caps;
	uint16_t *modes;
	uint16_t oem_ver;
	char *vendor;
	char *product;
	char *revstr;
	uint16_t accel_ver;
	uint16_t accel_modes;
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

int vbe_getinfo(struct vbe_info *info);

#endif	/* VBE_H_ */
