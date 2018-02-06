#ifndef CFGOPT_H_
#define CFGOPT_H_

struct options {
	const char *start_scr;
	int music;
	int sball;
	int vsync;
};

extern struct options opt;

int parse_args(int argc, char **argv);
int load_config(const char *fname);

#endif	/* CFGOPT_H_ */
