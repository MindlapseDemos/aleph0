/*
S3 Virge driver hack
Copyright (C) 2021 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef S3REGS_H_
#define S3REGS_H_

#include <conio.h>		/* inp/outp */
#include "inttypes.h"

#define PCI_VENDOR_S3		0x5333
#define PCI_DEVICE_VIRGE	0x5631
#define PCI_DEVICE_TRIO		0x8811

#define DEVID_TRIO32		0x8810
#define DEVID_TRIO64		PCI_DEVICE_TRIO
#define DEVID_VIRGE			PCI_DEVICE_VIRGE

/* standard VGA ports */
#define SC_ADDR_PORT	0x3c4
#define SC_DATA_PORT	0x3c5
#define SC_PORT			SC_ADDR_PORT
#define CRTC_ADDR_PORT	0x3d4
#define CRTC_DATA_PORT	0x3d5
#define CRTC_PORT		CRTC_ADDR_PORT

#define sc_write(reg, data)		outpw(SC_PORT, (uint16_t)(reg) | ((uint16_t)(data) << 8))
#define sc_read(reg)			(outp(SC_ADDR_PORT, reg), inp(SC_DATA_PORT))
#define crtc_write(reg, data)	outpw(CRTC_PORT, (uint16_t)(reg) | ((uint16_t)(data) << 8))
#define crtc_read(reg)			(outp(CRTC_ADDR_PORT, reg), inp(CRTC_DATA_PORT))
#define crtc_wrmask(reg, data, mask) \
	outp(CRTC_DATA_PORT, (crtc_read(reg) & ~(mask)) | (data))

/* extended sequence controller registers */
#define REG_SCX_UNLOCK	0x08
#define REG_SCX_MMIO	0x09

#define SCX_MMIO_ONLY	0x80

/* CRTC registers */
#define REG_CRTC_STA_H		0x0c
#define REG_CRTC_STA_L		0x0d

/* extended CRTC registers */
#define REG_CRTCX_DEVID_H	0x2d
#define REG_CRTCX_DEVID_L 0x2e
#define REG_CRTCX_REV		0x2f
#define REG_CRTCX_CHIPID	0x30
#define REG_CRTCX_MEMCFG	0x31
#define REG_CRTCX_CFG1		0x36
#define REG_CRTCX_UNLOCK1	0x38
#define REG_CRTCX_UNLOCK2	0x39
#define REG_CRTCX_SYSCONF	0x40
#define REG_CRTCX_CURMODE	0x45
#define REG_CRTCX_CURX_H	0x46
#define REG_CRTCX_CURX_L	0x47
#define REG_CRTCX_CURY_H	0x48
#define REG_CRTCX_CURY_L	0x49
#define REG_CRTCX_CURFG		0x4a
#define REG_CRTCX_CURBG		0x4b
#define REG_CRTCX_CURADDR_H	0x4c
#define REG_CRTCX_CURADDR_L	0x4d
#define REG_CRTCX_CURXOFFS	0x4e
#define REG_CRTCX_CURYOFFS	0x4f
#define REG_CRTCX_MEMCTL1	0x53
#define REG_CRTCX_DACCTL	0x55
#define REG_CRTCX_WINCTL	0x58
#define REG_CRTCX_WINPOS_H	0x59
#define REG_CRTCX_WINPOS_L	0x5a
#define REG_CRTCX_MISC1		0x66

#define CRTCX_MEMCFG_BASEOFFS_EN	0x01
#define CRTCX_MEMCFG_ENHMAP			0x08

#define CRTCX_CFG1_VLB				0x01
#define CRTCX_CFG1_PCI				0x02
#define CRTCX_CFG1_EDO1				0
#define CRTCX_CFG1_EDO2				0x08
#define CRTCX_CFG1_VBE				0x10
#define CRTCX_CFG1_4MB				0
#define CRTCX_CFG1_2MB				0x80

#define CRTCX_SYSCONF_ENH_EN		0x01

#define CRTCX_CURMODE_EN			0x01
#define CRTCX_CURMODE_1280			0x10

#define CRTCX_MEMCTL1_SWAP_BYTES	0x02
#define CRTCX_MEMCTL1_SWAP_BIGEND	0x04
#define CRTCX_MEMCTL1_MMIO_NEW		0x08
#define CRTCX_MEMCTL1_MMIO_TRIO64	0x10
#define CRTCX_MEMCTL1_WIN_A8000		0
#define CRTCX_MEMCTL1_WIN_B8000		0x20
#define CRTCX_MEMCTL1_SWAP_NIBL		0x40

#define CRTCX_DACCTL_CURX11		0x10

#define CRTCX_WINCTL_SZMASK		0x03
#define CRTCX_WINCTL_64K		0x00
#define CRTCX_WINCTL_1M			0x01
#define CRTCX_WINCTL_2M			0x02
#define CRTCX_WINCTL_4M			0x03
#define CRTCX_WINCTL_LADDR_EN	0x10

#define CRTCX_MISC1_ENH_EN		0x01
#define CRTCX_MISC1_RST			0x02

/* MMIO registers */
uint32_t s3v_mmio_vaddr;
#define MMIO_REGBASE		0x1000000
#define MMIO_REG(x) \
	(*(volatile uint32_t*)(s3v_mmio_vaddr | MMIO_REGBASE | (x)))
#define MMIO_IMG_ADDR		(s3v_mmio_vaddr | MMIO_REGBASE)

#define MMREG_STAT			MMIO_REG(0x8504)
#define MMREG_ADVFN			MMIO_REG(0x850c)
#define MMREG_CDMA_BUF		MMIO_REG(0x8590)
#define MMREG_CDMA_WR		MMIO_REG(0x8594)
#define MMREG_CDMA_RD		MMIO_REG(0x8598)
#define MMREG_CDMA			MMIO_REG(0x859c)

#define STAT_VSYNC			0x0001
#define STAT_FIFO_OVF		0x0004
#define STAT_S3D_DONE		0x0002
#define STAT_FIFO_EMPTY		0x0008
#define STAT_HDMA_DONE		0x0010
#define STAT_CDMA_DONE		0x0020
#define STAT_S3DFIFO_EMPTY	0x0040
#define STAT_S3DFIFO_FREE(rval)	(((rval) >> 8) & 0x1f)
#define STAT_S3D_IDLE		0x2000

#define ADVFN_ENHFC_EN		0x0001
#define ADVFN_RST_DMA		0x0002
#define ADVFN_LADDR_EN		0x0010
#define ADVFN_FIFO_MASK		0x03c0
#define ADVFN_FIFO_FREE(rval)	(((rval) >> 6) & 0xf)

#define CDMA_BUF_64K		0x00000002

#define CDMA_WR_UPD			0x00010000

#define CDMA_EN				0x00000001

/* S3D 2D registers */
#define REGNO_S3D_SRCBASE	0xa4d4
#define REGNO_S3D_DSTBASE	0xa4d8
#define REGNO_S3D_XCLIP		0xa4dc
#define REGNO_S3D_YCLIP		0xa4e0
#define REGNO_S3D_STRIDE	0xa4e4
#define REGNO_S3D_FGCOL		0xa4f4
#define REGNO_S3D_CMD		0xa500
#define REGNO_S3D_RECTSZ	0xa504
#define REGNO_S3D_SRCPOS	0xa508
#define REGNO_S3D_DSTPOS	0xa50c

#define MMREG_S3D_IMG		MMIO_REG(0)
#define MMREG_S3D_SRCBASE	MMIO_REG(REGNO_S3D_SRCBASE)
#define MMREG_S3D_DSTBASE	MMIO_REG(REGNO_S3D_DSTBASE)
#define MMREG_S3D_XCLIP		MMIO_REG(REGNO_S3D_XCLIP)
#define MMREG_S3D_YCLIP		MMIO_REG(REGNO_S3D_YCLIP)
#define MMREG_S3D_STRIDE	MMIO_REG(REGNO_S3D_STRIDE)
#define MMREG_S3D_FGCOL		MMIO_REG(REGNO_S3D_FGCOL)
#define MMREG_S3D_CMD		MMIO_REG(REGNO_S3D_CMD)
#define MMREG_S3D_RECTSZ	MMIO_REG(REGNO_S3D_RECTSZ)
#define MMREG_S3D_SRCPOS	MMIO_REG(REGNO_S3D_SRCPOS)
#define MMREG_S3D_DSTPOS	MMIO_REG(REGNO_S3D_DSTPOS)

#define S3D_CMD_AUTO		0x00000001
#define S3D_CMD_CLIP		0x00000002
#define S3D_CMD_DFMT8		0
#define S3D_CMD_DFMT16		(1 << 2)
#define S3D_CMD_DFMT24		(2 << 2)
#define S3D_CMD_DRAW		0x00000020
#define S3D_CMD_SRCMONO		0x00000040
#define S3D_CMD_SRCCPU		0x00000080
#define S3D_CMD_PATMONO		0x00000100
#define S3D_CMD_TRANSP		0x00000200
#define S3D_CMD_IALIGN_BYTE		0
#define S3D_CMD_IALIGN_WORD		(1 << 10)
#define S3D_CMD_IALIGN_DWORD	(2 << 10)
#define S3D_CMD_IOFFS0		0
#define S3D_CMD_IOFFS1		(1 << 12)
#define S3D_CMD_IOFFS2		(2 << 12)
#define S3D_CMD_IOFFS3		(3 << 12)
#define S3D_CMD_ROP(rop)	((rop) << 17)
#define S3D_CMD_LR			0x02000000
#define S3D_CMD_TB			0x04000000
#define S3D_CMD_BLIT		0
#define S3D_CMD_RECT		0x10000000
#define S3D_CMD_LINE		0x18000000
#define S3D_CMD_POLY		0x50000000
#define S3D_CMD_NOP			0x78000000
#define S3D_CMD_3D			0x80000000

#define ROP_ZERO		0
#define ROP_DST			0xaa
#define ROP_SRC			0xcc
#define ROP_PAT			0xf0
#define ROP_ONE			0xff

#endif	/* S3REGS_H_ */
