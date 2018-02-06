#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cfgopt.h"

struct options opt = {
	0,	/* start_scr */
	0,	/* music */
	0,	/* sball */
	1	/* vsync */
};

int parse_args(int argc, char **argv)
{
	int i;
	char *scrname = 0;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-music") == 0) {
				opt.music = 1;
			} else if(strcmp(argv[i], "-nomusic") == 0) {
				opt.music = 0;
			} else if(strcmp(argv[i], "-scr") == 0 || strcmp(argv[i], "-screen") == 0) {
				scrname = argv[++i];
			} else if(strcmp(argv[i], "-sball") == 0) {
				opt.sball = !opt.sball;
			} else if(strcmp(argv[i], "-vsync") == 0) {
				opt.vsync = 1;
			} else if(strcmp(argv[i], "-novsync") == 0) {
				opt.vsync = 0;
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

static char *strip_space(char *s)
{
	int len;
	char *end;

	while(*s && isspace(*s)) ++s;
	if(!*s) return 0;

	if((end = strrchr(s, '#'))) {
		--end;
	} else {
		len = strlen(s);
		end = s + len - 1;
	}

	while(end > s && isspace(*end)) *end-- = 0;
	return end > s ? s : 0;
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

		if(strcmp(line, "music") == 0) {
			opt.music = bool_value(value);
		} else if(strcmp(line, "screen") == 0) {
			opt.start_scr = strdup(value);
		} else if(strcmp(line, "sball") == 0) {
			opt.sball = bool_value(value);
		} else if(strcmp(line, "vsync") == 0) {
			opt.vsync = bool_value(value);
		} else {
			fprintf(stderr, "%s:%d invalid option: %s\n", fname, nline, line);
			return -1;
		}
	}
	return 0;
}
