/*
DOS demo/game framework
Copyright (C) 2016-2025  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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

