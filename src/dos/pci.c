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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "dosutil.h"
#include "inttypes.h"
#include "pci.h"
#include "cdpmi.h"
#include "demo.h"

#define CONFIG_ADDR_PORT	0xcf8
#define CONFIG_DATA_PORT	0xcfc

#define ADDR_ENABLE		0x80000000
#define ADDR_BUSID(x)	(((uint32_t)(x) & 0xff) << 16)
#define ADDR_DEVID(x)	(((uint32_t)(x) & 0x1f) << 11)
#define ADDR_FUNC(x)	(((uint32_t)(x) & 3) << 8)

/* signature returned in edx by the PCI BIOS present function: FOURCC "PCI " */
#define PCI_SIG		0x20494350

#define TYPE_MULTIFUNC	0x80


static struct pci_device *pcidev;
static int num_pcidevs, max_pcidevs;


static int enum_bus(int busid);
static int enum_dev(int busid, int dev);
static int read_dev_info(struct pci_config_data *res, int bus, int dev, int func);
static void print_dev_info(struct pci_config_data *info, int bus, int dev, int func);

static uint32_t cfg_read32_m1(int bus, int dev, int func, int reg);
static uint32_t cfg_read32_m2(int bus, int dev, int func, int reg);
static const char *class_str(int cc);
static const char *subclass_str(int cc, int sub);

static uint32_t (*cfg_read32)(int, int, int, int);

static void clear_devices(void);
static void add_device(struct pci_device *dev);

int init_pci(void)
{
	int i, count = 0;
	struct dpmi_regs regs = {0};

	clear_devices();

	regs.eax = 0xb101;
	dpmi_int(0x1a, &regs);

	/* PCI BIOS present if CF=0, AH=0, and EDX has the "PCI " sig FOURCC */
	if((regs.flags & FLAGS_CF)  || (regs.eax & 0xff00) || regs.edx != PCI_SIG) {
		fprintf(stderr, "No PCI BIOS present\n");
		return -1;
	}

	printf("PCI BIOS v%x.%x found\n", (unsigned int)((regs.ebx & 0xff00) >> 8),
			(unsigned int)(regs.ebx & 0xff));
	if(regs.eax & 1) {
		cfg_read32 = cfg_read32_m1;
	} else {
		if(!(regs.eax & 2)) {
			fprintf(stderr, "Failed to find supported PCI mess mechanism\n");
			return -1;
		}
		printf("PCI mess mechanism #1 unsupported, falling back to mechanism #2\n");
		cfg_read32 = cfg_read32_m2;
	}

	for(i=0; i<256; i++) {
		count += enum_bus(i);
	}
	printf("found %d PCI devices\n\n", count);
	return 0;
}

static int enum_bus(int busid)
{
	int i, count = 0;

	for(i=0; i<32; i++) {
		count += enum_dev(busid, i);
	}

	return count;
}

static int enum_dev(int busid, int devid)
{
	int i, count;
	struct pci_device dev;

	dev.bus = busid;
	dev.dev = devid;
	dev.func = 0;

	/* vendor id ffff is invalid */
	if((cfg_read32(busid, devid, 0, 0) & 0xffff) == 0xffff) {
		return 0;
	}
	if(read_dev_info(&dev.cfg, busid, devid, 0) == -1) {
		return 0;
	}
	print_dev_info(&dev.cfg, busid, devid, 0);
	add_device(&dev);

	count = 1;

	if(dev.cfg.hdr_type & TYPE_MULTIFUNC) {
		for(i=1; i<8; i++) {
			if(read_dev_info(&dev.cfg, busid, devid, i) == -1) {
				continue;
			}
			print_dev_info(&dev.cfg, busid, devid, i);
			dev.func = i;
			add_device(&dev);
			count++;
		}
	}
	return count;
}

static int read_dev_info(struct pci_config_data *res, int bus, int dev, int func)
{
	int i;
	uint32_t *ptr = (uint32_t*)res;

	*ptr++ = cfg_read32(bus, dev, func, 0);
	if(res->vendor == 0xffff) {
		return -1;
	}

	for(i=1; i<16; i++) {
		*ptr++ = cfg_read32(bus, dev, func, i * 4);
	}
	return 0;
}

static void print_dev_info(struct pci_config_data *info, int bus, int dev, int func)
{
	printf("- (%d:%d,%d) Device %04x:%04x: ", bus, dev, func, info->vendor, info->device);
	printf("\"%s\" (%d) - %s-func\n", class_str(info->class), info->class,
			info->hdr_type & TYPE_MULTIFUNC ? "multi" : "single");
	printf("    subclass: \"%s\" (%d), iface: %d\n", subclass_str(info->class, info->subclass),
			info->subclass, info->iface);
}

static uint32_t cfg_read32_m1(int bus, int dev, int func, int reg)
{
	uint32_t addr = ADDR_ENABLE | ADDR_BUSID(bus) | ADDR_DEVID(dev) |
		ADDR_FUNC(func) | reg;

	outpd(CONFIG_ADDR_PORT, addr);
	return inpd(CONFIG_DATA_PORT);
}

static uint32_t cfg_read32_m2(int bus, int dev, int func, int reg)
{
	fprintf(stderr, "BUG: PCI mess mechanism #2 not implemented yet!");
	demo_abort();
	return 0;
}

static const char *class_names[] = {
	"unknown",
	"mass storage controller",
	"network controller",
	"display controller",
	"multimedia device",
	"memory controller",
	"bridge device",
	"simple communication controller",
	"base system peripheral",
	"input device",
	"docking station",
	"processor",
	"serial bus controller",
	"wireless controller",
	"intelligent I/O controller",
	"satellite communication controller",
	"encryption/decryption controller",
	"data acquisition & signal processing controller"
};

static const char *class_mass_names[] = {
	"SCSI bus controller",
	"IDE controller",
	"floppy disk controller",
	"IPI bus controller",
	"RAID controller"
};

static const char *class_net_names[] = {
	"ethernet controller",
	"token ring controller",
	"FDDI controller",
	"ATM controller",
	"ISDN controller"
};

static const char *class_disp_names[] = {
	"VGA-compatible controller",
	"XGA controller",
	"3D controller"
};

static const char *class_mm_names[] = {
	"video device",
	"audio device",
	"telephony device"
};

static const char *class_bridge_names[] = {
	"host bridge",
	"ISA bridge",
	"EISA bridge",
	"MCA bridge",
	"PCI-to-PCI bridge",
	"Subtractive decode PCI-to-PCI bridge",
	"PCMCIA bridge",
	"NuBus bridge",
	"CardBus bridge",
	"RACEway bridge"
};

static const char *class_comm_names[] = {
	"serial controller",
	"parallel/IEEE1284",
	"multiport serial controller",
	"modem"
};

static const char *class_base_names[] = {
	"interrupt controller",
	"DMA controller",
	"timer",
	"RTC",
	"PCI hot-plug controller"
};

static const char *class_input_names[] = {
	"keyboard controller",
	"digitizer",
	"mouse controller",
	"scanner controller",
	"gameport controller"
};

static const char *class_ser_names[] = {
	"firewire",
	"ACCESS.bus",
	"SSA",
	"USB",
	"Fibre Channel",
	"SMBus"
};

static const char *class_sat_names[] = {
	"TV",
	"audio",
	"voice",
	"data"
};


static const char *class_str(int cc)
{
	if(cc == 0xff) {
		return "other";
	}
	if(cc >= 0x12) {
		return "unknown";
	}
	return class_names[cc];
}

static const char *subclass_str(int cc, int sub)
{
	if(sub == 0x80) return "other";

	switch(cc) {
	case 0:
		if(sub == 1) return "VGA-compatible device";
		return "unknown";

	case 1:
		if(sub > 4) return "unknown";
		return class_mass_names[sub];

	case 2:
		if(sub > 4) return "unknown";
		return class_net_names[sub];

	case 3:
		if(sub > 2) return "unknown";
		return class_disp_names[sub];

	case 4:
		if(sub > 2) return "unknown";
		return class_mm_names[sub];

	case 5:
		if(sub == 0) return "RAM";
		if(sub == 1) return "flash";
		return "unknown";

	case 6:
		if(sub > 8) return "unknown";
		return class_bridge_names[sub];

	case 7:
		if(sub > 3) return "unknown";
		return class_comm_names[sub];

	case 8:
		if(sub > 4) return "unknown";
		return class_base_names[sub];

	case 9:
		if(sub > 4) return "unknown";
		return class_input_names[sub];

	case 10:
		if(sub == 0) return "generic docking station";
		return "unknown";

	case 11:
		switch(sub) {
		case 0: return "386";
		case 1: return "486";
		case 2: return "pentium";
		case 0x10: return "alpha";
		case 0x20: return "powerpc";
		case 0x30: return "mips";
		case 0x40: return "co-processor";
		default:
			break;
		}
		return "unknown";

	case 12:
		if(sub > 5) return "unknown";
		return class_ser_names[sub];

	case 13:
		if(sub == 0) return "irda controller";
		if(sub == 1) return "IR controller";
		if(sub == 0x10) return "RF controller";
		return "unknonw";

	case 15:
		if(sub > 4) return "unknown";
		return class_sat_names[sub];

	case 16:
		if(sub == 0) return "network & computing crypto";
		if(sub == 1) return "entertainment crypto";
		return "unknown";

	case 17:
		if(sub == 0) return "DPIO module";
		return "unknown";

	default:
		break;
	}
	return "unknown";
}

static void clear_devices(void)
{
	free(pcidev);
	pcidev = 0;
	num_pcidevs = max_pcidevs = 0;
}

static void add_device(struct pci_device *dev)
{
	if(num_pcidevs >= max_pcidevs) {
		void *newarr;
		int newsz = max_pcidevs ? max_pcidevs << 1 : 8;

		if(!(newarr = realloc(pcidev, newsz * sizeof *pcidev))) {
			fprintf(stderr, "failed to resize PCI device array (%d)\n", newsz);
			return;
		}
		pcidev = newarr;
		max_pcidevs = newsz;
	}

	pcidev[num_pcidevs++] = *dev;
}

struct pci_device *find_pci_dev(uint16_t vendorid, uint16_t devid)
{
	int i;
	for(i=0; i<num_pcidevs; i++) {
		if(pcidev[i].cfg.vendor == vendorid && pcidev[i].cfg.device == devid) {
			return pcidev + i;
		}
	}
	return 0;
}
