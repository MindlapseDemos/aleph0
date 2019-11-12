#include "vbe.h"
#include "cdpmi.h"

int vbe_getinfo(struct vbe_info *info)
{
	void *lowbuf;
	uint16_t seg, sel;
	struct dpmi_regs regs = {0};

	if(!(seg = dpmi_alloc(512 / 16, &sel))) {
		return -1;
	}

	regs.eax = 0x4f00;
	regs.es = seg;
	regs.edi = 0;
	dpmi_int(0x10, &regs);

	if(regs.eax & 0xff00) {
		dpmi_free(sel);
		return -1;
	}

	lowbuf = (void*)((uint32_t)seg << 4);
	memcpy(info, lowbuf, sizeof info);
	dpmi_free(sel);
	return 0;
}
