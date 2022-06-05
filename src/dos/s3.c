#include "vga.h"
#include "s3.h"
#include "s3regs.h"

void s3_extreg_unlock(void)
{
	unsigned char cur;

	vga_sc_write(REG_SCX_UNLOCK, 0x06);			/* unlock ext. SC regs SR9-SRff */
	vga_crtc_write(REG_CRTCX_UNLOCK1, 0x48);	/* unlock ext. CRTC regs CR2d-CR3f */
	vga_crtc_write(REG_CRTCX_UNLOCK2, 0xa5);	/* unlock ext. CRTC regs CR40-CRff */
	/* enable access to enhanced interface */
	cur = vga_crtc_read(REG_CRTCX_SYSCONF);
	outp(VGA_CRTC_DATA_PORT, cur | CRTCX_SYSCONF_ENH_EN);
}

