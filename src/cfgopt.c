#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cfgopt.h"
#include "screen.h"

#ifdef NDEBUG
/* release build default options */
struct options opt = {
	0,	/* start_scr */
	"demo.log",	/* logfile */
	1,	/* music */
	0,	/* mouse */
	0,	/* sball */
	0,	/* vsync */
	0	/* dbgmode */
};
#else
/* debug build default options */
struct options opt = {
	0,	/* start_scr */
	"demo.log",	/* logfile */
	0,	/* music */
	1,	/* mouse */
	0,	/* sball */
	0,	/* vsync */
	1	/* dbgmode */
};
#endif

static void print_usage(const char *argv0);
static void print_screen_list(void);

int parse_args(int argc, char **argv)
{
	int i, n;
	char *scrname = 0;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-music") == 0) {
				opt.music = 1;
			} else if(strcmp(argv[i], "-nomusic") == 0) {
				opt.music = 0;
			} else if(strcmp(argv[i], "-scr") == 0 || strcmp(argv[i], "-screen") == 0) {
				scrname = argv[++i];
			} else if(strcmp(argv[i], "-list") == 0) {
				print_screen_list();
				exit(0);
			} else if(strcmp(argv[i], "-logfile") == 0) {
				opt.logfile = argv[++i];
			} else if(strcmp(argv[i], "-mouse") == 0) {
				opt.mouse = 1;
			} else if(strcmp(argv[i], "-nomouse") == 0) {
				opt.mouse = 0;
			} else if(strcmp(argv[i], "-sball") == 0) {
				opt.sball = !opt.sball;
			} else if(strcmp(argv[i], "-vsync") == 0) {
				if(argv[i + 1] && (n = atoi(argv[i + 1])) >= 0 && n <= 9) {
					opt.vsync = n;
					i++;
				} else {
					opt.vsync = 1;
				}
			} else if(strcmp(argv[i], "-novsync") == 0) {
				opt.vsync = 0;
			} else if(strcmp(argv[i], "-dbg") == 0) {
				opt.dbgmode = 1;
			} else if(strcmp(argv[i], "-nodbg") == 0) {
				opt.dbgmode = 0;
#ifndef MSDOS
			} else if(strcmp(argv[i], "-fs") == 0) {
				opt.fullscreen = 1;
			} else if(strcmp(argv[i], "-win") == 0) {
				opt.fullscreen = 0;
			} else if(strcmp(argv[i], "-scaler-nearest") == 0) {
				opt.scaler = SCALER_NEAREST;
			} else if(strcmp(argv[i], "-scaler-linear") == 0) {
				opt.scaler = SCALER_LINEAR;
#else
			} else if(strcmp(argv[i], "-setup") == 0) {
				opt.sndsetup = 1;
#endif
			} else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
				print_usage(argv[0]);
				exit(0);
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(scrname) {
				fprintf(stderr, "unexpected option: %s\n", argv[i]);
				return -1;
			}
			scrname = argv[i];
		}
	}

	if(scrname) {
		opt.start_scr = scrname;
	}
	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [options] [screen name]\n", argv0);
	printf("Options:\n");
	printf(" -music/-nomusic: enable/disable music playback\n");
	printf(" -screen <name>: select starting screen (alias: -scr)\n");
	printf(" -list: list available screens\n");
	printf(" -logfile <path>: choose where to log messages\n");
	printf(" -mouse/-nomouse: enable/disable mouse input\n");
	printf(" -sball: enable 6dof (spaceball) input\n");
	printf(" -vsync/-novsync: enable/disable vertical sync\n");
	printf(" -dbg/-nodbg: enable/disable running in debug mode\n");
#ifndef MSDOS
	printf(" -fs/-win: run in fullscreen or windowed mode\n");
	printf(" -scaler-nearest: use nearest-neighbor upscaling\n");
	printf(" -scaler-linear: use linear interpolation for upscaling\n");
#else
	printf(" -setup: run sound setup utility before starting demo\n");
#endif
	printf(" -h/-help: print help and exit\n");
}

void populate_screens(void);	/* defined in screen.c */

static void print_screen_list(void)
{
	int i, count, len, col;
	struct screen *scr;

	populate_screens();

	col = 0;
	count = scr_num_screens();
	printf("%d screens:\n", count);
	for(i=0; i<count; i++) {
		scr = scr_screen(i);
		col += (len = strlen(scr->name) + 2);
		if(col >= 80) {
			putchar('\n');
			col = len;
		}
		printf("  %s", scr->name);
	}
	putchar('\n');
}

static char *strip_space(char *s)
{
	char *end;

	while(*s && isspace(*s)) ++s;

	if((end = strchr(s, '#'))) {
		*end-- = 0;
	} else {
		end = s + strlen(s) - 1;
	}

	while(end >= s && isspace(*end)) {
		*end-- = 0;
	}
	return *s ? s : 0;
}

static int bool_value(char *s)
{
	char *ptr = s;
	while(*ptr) {
		*ptr = tolower(*ptr);
		++ptr;
	}

	return strcmp(s, "true") == 0 || strcmp(s, "yes") == 0 || strcmp(s, "1") == 0;
}

int load_config(const char *fname)
{
	FILE *fp;
	char buf[256];
	int nline = 0;

	if(!(fp = fopen(fname, "rb"))) {
		return 0;	/* just ignore missing config files */
	}

	while(fgets(buf, sizeof buf, fp)) {
		char *line, *key, *value;

		++nline;
		if(!(line = strip_space(buf))) {
			continue;
		}

		if(!(value = strchr(line, '='))) {
			fprintf(stderr, "%s:%d invalid key/value pair\n", fname, nline);
			return -1;
		}
		*value++ = 0;

		if(!(key = strip_space(line)) || !(value = strip_space(value))) {
			fprintf(stderr, "%s:%d invalid key/value pair\n", fname, nline);
			return -1;
		}

		if(strcmp(key, "music") == 0) {
			opt.music = bool_value(value);
		} else if(strcmp(key, "screen") == 0) {
			opt.start_scr = strdup(value);
		} else if(strcmp(key, "logfile") == 0) {
			opt.logfile = strdup(value);
		} else if(strcmp(key, "mouse") == 0) {
			opt.mouse = bool_value(value);
		} else if(strcmp(key, "sball") == 0) {
			opt.sball = bool_value(value);
		} else if(strcmp(key, "vsync") == 0) {
			if(isdigit(value[0])) {
				if((opt.vsync = atoi(value)) < 0 || opt.vsync > 9) {
					fprintf(stderr, "%s:%d invalid vsync value: %s\n", fname, nline, line);
					return -1;
				}
			} else {
				opt.vsync = bool_value(value);
			}
		} else if(strcmp(key, "debug") == 0) {
			opt.dbgmode = bool_value(value);
#ifndef MSDOS
		} else if(strcmp(key, "fullscreen") == 0) {
			opt.fullscreen = bool_value(value);
		} else if(strcmp(key, "scaler") == 0) {
			if(strcmp(value, "linear") == 0) {
				opt.scaler = SCALER_LINEAR;
			} else {
				opt.scaler = SCALER_NEAREST;
			}
#endif
		} else {
			fprintf(stderr, "%s:%d invalid option: %s\n", fname, nline, line);
			return -1;
		}
	}
	return 0;
}
