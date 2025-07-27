#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "szint.h"

int con_init(void);
void con_start(void);
void con_stop(void);
void con_draw(uint16_t *fb);
int con_input(int key);

void con_printf(const char *fmt, ...);

#endif	/* CONSOLE_H_ */
