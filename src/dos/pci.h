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
#ifndef PCI_H_
#define PCI_H_

#include "inttypes.h"
#include "util.h"

#pragma pack (push, 1)
struct pci_config_data {
	uint16_t vendor, device;
	uint16_t cmd, status;
	uint8_t rev, iface, subclass, class;
	uint8_t cacheline_size;
	uint8_t latency_timer;
	uint8_t hdr_type;
	uint8_t bist;
	uint32_t base_addr[6];
	uint32_t cardbus_cis;
	uint16_t subsys_vendor;
	uint16_t subsys;
	uint32_t rom_addr;
	uint32_t reserved1, reserved2;
	uint8_t intr_line, intr_pin;
	uint8_t min_grant, max_latency;
} PACKED;
#pragma pop (push)

struct pci_device {
	int bus, dev, func;
	struct pci_config_data cfg;
};

int init_pci(void);

struct pci_device *find_pci_dev(uint16_t vendorid, uint16_t devid);

#endif	/* PCI_H_ */
