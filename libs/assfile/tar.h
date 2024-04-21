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
#ifndef TAR_H_
#define TAR_H_

#include <stdio.h>

struct tar_entry {
	char *path;
	unsigned long offset;
	unsigned long size;
};

struct tar {
	FILE *fp;
	struct tar_entry *files;
	int num_files;
};

int load_tar(struct tar *tar, const char *fname);
void close_tar(struct tar *tar);

#endif	/* TAR_H_ */
