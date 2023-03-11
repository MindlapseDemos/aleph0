/*
goat3d - 3D scene, and animation file format library.
Copyright (C) 2013-2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "goat3d.h"
#include "chunk.h"

void g3dimpl_chunk_header(struct chunk_header *hdr, int id)
{
	hdr->id = id;
	hdr->size = sizeof *hdr;
}

int g3dimpl_write_chunk_header(const struct chunk_header *hdr, struct goat3d_io *io)
{
	io->seek(-(long)hdr->size, SEEK_CUR, io->cls);
	if(io->write(hdr, sizeof *hdr, io->cls) < (long)sizeof *hdr) {
		return -1;
	}
	return 0;
}

int g3dimpl_read_chunk_header(struct chunk_header *hdr, struct goat3d_io *io)
{
	if(io->read(hdr, sizeof *hdr, io->cls) < (long)sizeof *hdr) {
		return -1;
	}
	return 0;
}

void g3dimpl_skip_chunk(const struct chunk_header *hdr, struct goat3d_io *io)
{
	io->seek(hdr->size - sizeof *hdr, SEEK_CUR, io->cls);
}
