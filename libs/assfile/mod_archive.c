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
#include "tar.h"

struct file_info {
	struct tar_entry *tarent;
	long roffs;
	int eof;
};

static void *fop_open(const char *fname, void *udata);
static void fop_close(void *fp, void *udata);
static long fop_seek(void *fp, long offs, int whence, void *udata);
static long fop_read(void *fp, void *buf, long size, void *udata);


struct ass_fileops *ass_alloc_archive(const char *fname)
{
	struct ass_fileops *fop;
	struct tar *tar;

	if(!(tar = malloc(sizeof *tar))) {
		return 0;
	}
	if(load_tar(tar, fname) == -1) {
		free(tar);
		return 0;
	}

	if(!(fop = malloc(sizeof *fop))) {
		return 0;
	}
	fop->udata = tar;
	fop->open = fop_open;
	fop->close = fop_close;
	fop->seek = fop_seek;
	fop->read = fop_read;
	return fop;
}

void ass_free_archive(struct ass_fileops *fop)
{
	close_tar(fop->udata);
	free(fop->udata);
	fop->udata = 0;
}

static void *fop_open(const char *fname, void *udata)
{
	int i;
	struct file_info *file;
	struct tar *tar = udata;

	for(i=0; i<tar->num_files; i++) {
		if(strcmp(fname, tar->files[i].path) == 0) {
			if(!(file = malloc(sizeof *file))) {
				ass_errno = ENOMEM;
				return 0;
			}
			file->tarent = tar->files + i;
			file->roffs = 0;
			file->eof = 0;
			return file;
		}
	}

	ass_errno = ENOENT;
	return 0;
}

static void fop_close(void *fp, void *udata)
{
	free(fp);
}

static long fop_seek(void *fp, long offs, int whence, void *udata)
{
	long newoffs;
	struct file_info *file = fp;

	switch(whence) {
	case SEEK_SET:
		newoffs = offs;
		break;

	case SEEK_CUR:
		newoffs = file->roffs + offs;
		break;

	case SEEK_END:
		newoffs = file->tarent->size + offs;
		break;

	default:
		ass_errno = EINVAL;
		return -1;
	}

	if(newoffs < 0) {
		ass_errno = EINVAL;
		return -1;
	}

	file->eof = 0;
	file->roffs = newoffs;
	return newoffs;
}

static long fop_read(void *fp, void *buf, long size, void *udata)
{
	struct tar *tar = udata;
	struct file_info *file = fp;
	long newoffs = file->roffs + size;
	size_t rdbytes;

	if(file->roffs >= file->tarent->size) {
		file->eof = 1;
		return -1;
	}

	if(newoffs > file->tarent->size) {
		size = file->tarent->size - file->roffs;
		newoffs = file->tarent->size;
	}

	if(fseek(tar->fp, file->tarent->offset + file->roffs, SEEK_SET) == -1) {
		fprintf(stderr, "assfile mod_archive: fop_read failed to seek to %ld (%ld + %ld)\n",
				file->tarent->offset + file->roffs, file->tarent->offset, file->roffs);
		return -1;
	}
	if((rdbytes = fread(buf, 1, size, tar->fp)) < size) {
		fprintf(stderr, "assfile mod_archive: fop_read unexpected EOF while trying to read %ld bytes\n", size);
		size = rdbytes;
	}
	file->roffs = newoffs;
	return size;
}
