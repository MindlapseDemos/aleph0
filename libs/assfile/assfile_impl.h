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
#ifndef ASSFILE_IMPL_H_
#define ASSFILE_IMPL_H_

typedef struct ass_file ass_file;

#include "assfile.h"

struct ass_file {
	void *file;
	struct ass_fileops *fop;
};

struct mount {
	char *prefix;
	struct ass_fileops *fop;
	int type;

	struct mount *next;
};

enum {
	MOD_PATH,
	MOD_ARCHIVE,
	MOD_URL,
	MOD_USER
};

/* implemented in mod_*.c files */
struct ass_fileops *ass_alloc_path(const char *path);
void ass_free_path(struct ass_fileops *fop);
struct ass_fileops *ass_alloc_archive(const char *fname);
void ass_free_archive(struct ass_fileops *fop);
struct ass_fileops *ass_alloc_url(const char *url);
void ass_free_url(struct ass_fileops *fop);

extern int ass_mod_url_max_threads;
extern char ass_mod_url_cachedir[512];

extern int ass_verbose;

#endif	/* ASSFILE_IMPL_H_ */
