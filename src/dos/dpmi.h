#ifndef DPMI_H_
#define DPMI_H_

#include "inttypes.h"

struct dpmi_real_regs {
	uint32_t edi, esi, ebp;
	uint32_t reserved;
	uint32_t ebx, edx, ecx, eax;
	uint16_t flags;
	uint16_t es, ds, fs, gs;
	uint16_t ip, cs, sp, ss;
};

unsigned short dpmi_alloc(unsigned int par);
#pragma aux dpmi_alloc = \
		"mov eax, 0x100" \
		"int 0x31" \
		value[ax] parm[ebx];

void dpmi_real_int(int inum, struct dpmi_real_regs *regs);

void *dpmi_mmap(uint32_t phys_addr, unsigned int size);
void dpmi_munmap(void *addr);

#endif	/* DPMI_H_ */
