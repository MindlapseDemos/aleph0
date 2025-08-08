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
#include "tar.h"

#define MAX_NAME_LEN	100
#define MAX_PREFIX_LEN	131

struct header {
	char name[MAX_NAME_LEN];
	char mode[8];
	char uid[8], gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32], gname[32];
	char devmajor[8], devminor[8];
	char prefix[MAX_PREFIX_LEN];
	char atime[12], ctime[12];
};

struct node {
	struct tar_entry file;
	struct node *next;
};


int load_tar(struct tar *tar, const char *fname)
{
	int i, res = -1;
	struct node *node, *head = 0, *tail = 0;
	char buf[512];
	struct header *hdr;
	unsigned long offset = 0, size, blksize;
	char *path, *endp;

	if(!(tar->fp = fopen(fname, "rb"))) {
		fprintf(stderr, "load_tar: failed to open %s: %s\n", fname, strerror(errno));
		return -1;
	}
	tar->num_files = 0;
	tar->files = 0;

	while(fread(buf, 1, sizeof buf, tar->fp) == sizeof buf) {
		hdr = (struct header*)buf;

		offset += 512;
		size = strtol(hdr->size, &endp, 8);
		if(endp == hdr->size || size <= 0) {
			continue;	/* skip invalid */
		}
		blksize = ((size - 1) | 0x1ff) + 1;	/* round to next 512-block */

		/* verify filesize reasonable */
		fseek(tar->fp, size - 1, SEEK_CUR);
		if(fgetc(tar->fp) == -1) {
			break;	/* invalid and reached EOF */
		}
		fseek(tar->fp, blksize - size, SEEK_CUR);

		if(memcmp(hdr->magic, "ustar", 5) == 0) {
			int nlen = strnlen(hdr->name, MAX_NAME_LEN);
			int plen = strnlen(hdr->prefix, MAX_PREFIX_LEN);
			if(!(path = malloc(nlen + plen + 1))) {
				perror("failed to allocate file path string");
				goto end;
			}
			memcpy(path, hdr->prefix, plen);
			memcpy(path + plen, hdr->name, nlen);
			path[nlen + plen] = 0;
		} else {
			int nlen = strnlen(hdr->name, MAX_NAME_LEN);
			if(!(path = malloc(nlen + 1))) {
				perror("failed to allocate file path string");
				goto end;
			}
			memcpy(path, hdr->name, nlen);
			path[nlen] = 0;
		}

		if(!(node = malloc(sizeof *node))) {
			perror("failed to allocate file list node");
			goto end;
		}
		node->file.path = path;
		node->file.offset = offset;
		node->file.size = size;
		node->next = 0;
		if(head) {
			tail->next = node;
			tail = node;
		} else {
			head = tail = node;
		}
		tar->num_files++;

		/*printf(" %s - size %ld @%ld\n", node->file.path, node->file.size, node->file.offset);*/

		offset += blksize;
	}

	if(!(tar->files = malloc(tar->num_files * sizeof *tar->files))) {
		perror("failed to allocate file list");
		goto end;
	}

	node = head;
	for(i=0; i<tar->num_files; i++) {
		tar->files[i] = node->file;
		node = node->next;
	}

	if(tar->num_files > 0) {
		res = 0;
	}

end:
	while(head) {
		node = head;
		head = head->next;
		if(node) {
			if(res == -1) {
				free(node->file.path);
			}
			free(node);
		}
	}
	return res;
}


void close_tar(struct tar *tar)
{
	if(!tar) return;

	if(tar->fp) {
		fclose(tar->fp);
		tar->fp = 0;
	}
	if(tar->files && tar->num_files > 0) {
		int i;
		for(i=0; i<tar->num_files; i++) {
			free(tar->files[i].path);
		}
		free(tar->files);
		tar->files = 0;
		tar->num_files = 0;
	}
}
