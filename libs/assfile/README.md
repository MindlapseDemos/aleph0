assfile
======

About
-----
Assman is a library for accessing assets (read only access to data files) in
multiple ways through a simple file I/O interface designed as a drop-in
replacement to C fopen/fread/etc I/O calls. In most cases you can just prefix
all your I/O calls with `ass_` and change `FILE*` to `ass_file*`, and it will
just work.

The access modules provided are:
 - `mod_path`: maps an arbitrary filesystem path to your chosen prefix. For
   instance, after calling `ass_add_path("data", "/usr/share/mygame")` you can
   access the data file `/usr/share/mygame/foo.png` by calling
   `ass_fopen("data/foo.png", "rb")`.

 - `mod_archive`: mounts the contents of an archive to your chosen prefix. For
   example, after calling `ass_add_archive("data", "data.tar")` you can access
   the contents of the tarball as if they where contents of a virtual `data`
   directory.

 - `mod_url`: maps a url prefix to your chosen prefix. For example, after
   calling `ass_add_url("data", "http://mydomain/myapp/data")` you can access
   `http://mydomain/myapp/data/foo.png` by calling
   `ass_fopen("data/foo.png", "rb")`.

License
-------
Copyright (C) 2018 John Tsiombikas <nuclear@member.fsf.org>

The assfile library is free software. Feel free to use, modify, and/or
redistribute it under the terms of the GNU Lesser General Public License
version 3, or at your option any later version published by the Free Software
Foundation. See COPYING and COPYING.LESSER for details.

Build
-----
To build and install assfile on UNIX, run the usual:

    ./configure
    make
    make install

The `mod_url` module depends on `libcurl` and uses POSIX threads. If you don't
want that dependency, you can disable `mod_url` by passing `--disable-url` to
`configure`.

See `./configure --help` for a complete list of build-time options.

To cross-compile for windows with mingw-w64, try the following incantation:

    ./configure --prefix=/usr/i686-w64-mingw32
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar sys=mingw
    make install sys=mingw
