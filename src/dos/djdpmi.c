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
#ifdef __DJGPP__
#include <dpmi.h>
#include <sys/nearptr.h>
#include "cdpmi.h"
#include "szint.h"

uint16_t dpmi_alloc(unsigned int par, uint16_t *sel)
{
	int tmp;
	uint16_t seg = __dpmi_allocate_dos_memory(par, &tmp);
	*sel = tmp;
	return seg;
}

void dpmi_free(uint16_t sel)
{
	__dpmi_free_dos_memory(sel);
}

void *dpmi_mmap(uint32_t phys_addr, unsigned int size)
{
	__dpmi_meminfo mem;
	mem.address = phys_addr;
	mem.size = size;
	__dpmi_physical_address_mapping(&mem);
	return (void*)(mem.address - __djgpp_base_address);
}

void dpmi_munmap(void *addr)
{
	__dpmi_meminfo mem;
	mem.address = (uint32_t)addr + __djgpp_base_address;
	__dpmi_free_physical_address_mapping(&mem);
}
#endif	/* __DJGPP__ */
