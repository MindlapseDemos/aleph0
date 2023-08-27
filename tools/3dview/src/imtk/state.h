#ifndef STATE_H_
#define STATE_H_

struct imtk_state {
	int scr_width, scr_height;
	int mousex, mousey, prevx, prevy, mouse_bnmask;
	int active, hot, input, prev_active;
};

void imtk_set_active(int id);
int imtk_is_active(int id);
int imtk_set_hot(int id);
int imtk_is_hot(int id);
void imtk_set_focus(int id);
int imtk_has_focus(int id);
int imtk_hit_test(int x, int y, int w, int h);

void imtk_get_mouse(int *xptr, int *yptr);
void imtk_set_prev_mouse(int x, int y);
void imtk_get_prev_mouse(int *xptr, int *yptr);
int imtk_button_state(int bn);

int imtk_get_key(void);


#endif	/* STATE_H_ */
