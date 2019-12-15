#ifndef SMOKE_TEXT_H_
#define SMOKE_TEXT_H_

struct smktxt;

struct smktxt *create_smktxt(const char *imgname, const char *vfieldname);
void destroy_smktxt(struct smktxt *stx);

int gen_smktxt_vfield(struct smktxt *stx, int xres, int yres, float xfreq, float yfreq);
int dump_smktxt_vfield(struct smktxt *stx, const char *fname);

void set_smktxt_wind(struct smktxt *stx, float x, float y, float z);
void set_smktxt_plife(struct smktxt *stx, float life);
void set_smktxt_pcount(struct smktxt *stx, int count);
void set_smktxt_drag(struct smktxt *stx, float drag);

void update_smktxt(struct smktxt *stx, float dt);
void draw_smktxt(struct smktxt *stx);


#endif	/* SMOKE_TEXT_ */
