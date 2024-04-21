/*
assfile - library for accessing assets with an fopen/fread-like interface
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef ASSFILE_H_
#define ASSFILE_H_

#include <stdlib.h>

#ifndef ASSFILE_IMPL_H_
typedef void ass_file;
#endif

struct ass_fileops {
	void *udata;
	void *(*open)(const char *fname, void *udata);
	void (*close)(void *fp, void *udata);
	long (*seek)(void *fp, long offs, int whence, void *udata);
	long (*read)(void *fp, void *buf, long size, void *udata);
};

/* options (ass_set_option/ass_get_option) */
enum {
	ASS_OPEN_FALLTHROUGH	/* try all matching handlers if the first fails to open the file */
};

#ifdef __cplusplus
extern "C" {
#endif

extern int ass_errno;

void ass_set_option(int opt, int val);
int ass_get_option(int opt);

/* add a handler for a specific path prefixes. 0 matches every path */
int ass_add_path(const char *prefix, const char *path);
int ass_add_archive(const char *prefix, const char *arfile);
int ass_add_url(const char *prefix, const char *url);
int ass_add_user(const char *prefix, struct ass_fileops *cb);
void ass_clear(void);

ass_file *ass_fopen(const char *fname, const char *mode);
void ass_fclose(ass_file *fp);
long ass_fseek(ass_file *fp, long offs, int whence);
long ass_ftell(ass_file *fp);

size_t ass_fread(void *buf, size_t size, size_t count, ass_file *fp);

/* convenience functions, derived from the above */
int ass_fgetc(ass_file *fp);
char *ass_fgets(char *s, int size, ass_file *fp);

#ifdef __cplusplus
}
#endif


#endif	/* ASSFILE_H_ */
