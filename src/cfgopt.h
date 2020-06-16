#ifndef CFGOPT_H_
#define CFGOPT_H_

#ifndef MSDOS
enum {
	SCALER_NEAREST,
	SCALER_LINEAR,

	NUM_SCALERS
};
#endif

struct options {
	const char *start_scr;
	int music;
	int mouse, sball;
	int vsync;
	int dbginfo;
#ifndef MSDOS
	int fullscreen;
	int scaler;
#endif
};

extern struct options opt;

int parse_args(int argc, char **argv);
int load_config(const char *fname);

#endif	/* CFGOPT_H_ */
