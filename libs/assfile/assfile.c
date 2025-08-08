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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "assfile_impl.h"

int ass_errno;

/* declared in assfile_impl.h */
int ass_mod_url_max_threads;
char ass_mod_url_cachedir[512];
int ass_verbose;

static int add_fop(const char *prefix, int type, struct ass_fileops *fop);
static const char *match_prefix(const char *str, const char *prefix);
static void upd_verbose_flag(void);

#define DEF_FLAGS	(1 << ASS_OPEN_FALLTHROUGH)

static unsigned int assflags = DEF_FLAGS;
static struct mount *mlist;

void ass_set_option(int opt, int val)
{
	if(val) {
		assflags |= 1 << opt;
	} else {
		assflags &= ~(1 << opt);
	}
}

int ass_get_option(int opt)
{
	return assflags & (1 << opt);
}

int ass_add_path(const char *prefix, const char *path)
{
	return add_fop(prefix, MOD_PATH, ass_alloc_path(path));
}

int ass_add_archive(const char *prefix, const char *arfile)
{
	return add_fop(prefix, MOD_ARCHIVE, ass_alloc_archive(arfile));
}

int ass_add_url(const char *prefix, const char *url)
{
	return add_fop(prefix, MOD_URL, ass_alloc_url(url));
}

int ass_add_user(const char *prefix, struct ass_fileops *fop)
{
	return add_fop(prefix, MOD_USER, fop);
}

static int add_fop(const char *prefix, int type, struct ass_fileops *fop)
{
	struct mount *m;

	upd_verbose_flag();

	if(!fop) {
		fprintf(stderr, "assfile: failed to allocate asset source\n");
		return -1;
	}

	if(!(m = malloc(sizeof *m))) {
		perror("assfile: failed to allocate mount node");
		return -1;
	}
	if(prefix) {
		if(!(m->prefix = malloc(strlen(prefix) + 1))) {
			free(m);
			return -1;
		}
		strcpy(m->prefix, prefix);
	} else {
		m->prefix = 0;
	}
	m->fop = fop;
	m->type = type;

	m->next = mlist;
	mlist = m;
	return 0;
}

void ass_clear(void)
{
	while(mlist) {
		struct mount *m = mlist;
		mlist = mlist->next;

		switch(m->type) {
		case MOD_PATH:
			ass_free_path(m->fop);
			break;
		case MOD_ARCHIVE:
			ass_free_archive(m->fop);
			break;
		case MOD_URL:
			ass_free_url(m->fop);
			break;
		default:
			break;
		}

		free(m->prefix);
		free(m);
	}
}

ass_file *ass_fopen(const char *fname, const char *mode)
{
	struct mount *m;
	void *mfile;
	ass_file *file;
	FILE *fp;
	const char *after_prefix;

	upd_verbose_flag();

	m = mlist;
	while(m) {
		if((after_prefix = match_prefix(fname, m->prefix))) {
			while(*after_prefix && (*after_prefix == '/' || *after_prefix == '\\')) {
				after_prefix++;
			}
			if((mfile = m->fop->open(after_prefix, m->fop->udata))) {
				if(!(file = malloc(sizeof *file))) {
					perror("assfile: ass_fopen failed to allocate file structure");
					m->fop->close(mfile, m->fop->udata);
					return 0;
				}
				file->file = mfile;
				file->fop = m->fop;
				return file;
			} else {
				if(!(assflags & (1 << ASS_OPEN_FALLTHROUGH))) {
					return 0;
				}
			}
		}
		m = m->next;
	}

	/* nothing matched, or failed to open, try the filesystem */
	if((fp = fopen(fname, mode))) {
		if(!(file = malloc(sizeof *file))) {
			ass_errno = errno;
			perror("assfile: ass_fopen failed to allocate file structure");
			fclose(fp);
			return 0;
		}
		file->file = fp;
		file->fop = 0;
		return file;
	}
	ass_errno = errno;
	return 0;
}

static const char *match_prefix(const char *str, const char *prefix)
{
	if(!prefix || !*prefix) return str;	/* match on null or empty prefix */

	while(*prefix) {
		if(*prefix++ != *str++) {
			return 0;
		}
	}
	return str;
}

void ass_fclose(ass_file *fp)
{
	if(fp->fop) {
		fp->fop->close(fp->file, fp->fop->udata);
	} else {
		fclose(fp->file);
	}
	free(fp);
}

long ass_fseek(ass_file *fp, long offs, int whence)
{
	if(fp->fop) {
		return fp->fop->seek(fp->file, offs, whence, fp->fop->udata);
	}

	if(fseek(fp->file, offs, whence) == -1) {
		return -1;
	}
	return ftell(fp->file);
}

long ass_ftell(ass_file *fp)
{
	return ass_fseek(fp, 0, SEEK_CUR);
}

size_t ass_fread(void *buf, size_t size, size_t count, ass_file *fp)
{
	if(fp->fop) {
		long res = fp->fop->read(fp->file, buf, size * count, fp->fop->udata);
		if(res <= 0) return 0;
		return res / size;
	}

	return fread(buf, size, count, fp->file);
}


/* --- convenience functions --- */

int ass_fgetc(ass_file *fp)
{
	unsigned char c;

	if(ass_fread(&c, 1, 1, fp) < 1) {
		return -1;
	}
	return (int)c;
}

char *ass_fgets(char *s, int size, ass_file *fp)
{
	int i, c;
	char *ptr = s;

	if(!size) return 0;

	for(i=0; i<size - 1; i++) {
		if((c = ass_fgetc(fp)) == -1) {
			break;
		}
		*ptr++ = c;

		if(c == '\n') break;
	}
	*ptr = 0;
	return ptr == s ? 0 : s;
}


static void upd_verbose_flag(void)
{
	const char *env;

	if((env = getenv("ASSFILE_VERBOSE"))) {
		ass_verbose = atoi(env);
	}
}
