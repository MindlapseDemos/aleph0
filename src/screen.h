#ifndef SCREEN_H_
#define SCREEN_H_

struct screen {
	char *name;

	int (*init)(void);
	void (*shutdown)(void);

	void (*start)(long trans_time);
	void (*stop)(long trans_time);

	void (*draw)(void);

	void (*keypress)(int key);
};

int scr_init(void);
void scr_shutdown(void);

void scr_update(void);
void scr_draw(void);
void scr_keypress(int key);

struct screen *scr_lookup(const char *name);
struct screen *scr_screen(int idx);
int scr_num_screens(void);
int scr_change(struct screen *s, long trans_time);

#endif	/* SCREEN_H_ */
