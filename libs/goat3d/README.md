goat3d
======

About
-----
Goat3D is a hierarchical 3D scene, character, and animation file format, and
acompanying read/write library, targeting mostly real-time applications.

The specification defines a hierarchical structure (see `doc/goatfmt`, and
`doc/goatanimfmt` for details), which can be stored in either text or binary
form. An application using the provided library to read/write goat3d files,
should be able to handle either variant, with no extra effort (NOTE: currently
the binary format is not implemented). The animations can be part of the scene
file, or in separate files.

This project provides the specification of the file format, a simple library
with a clean C API for reading and writing files in the goat3d scene and
animation files, as well as a number of tools dealing with such files.

Specifically, at the moment, the goat3d project provides the following:
 - *libgoat3d*, a library for reading and writing goat3d scene and animation files.
 - *ass2goat*, a universal 3D asset conversion utility based on the excellent
   assimp library, from a huge number of 3D file formats to the goat3d file
   format.
 - *goatinfo*, a command-line tool for inspecting the contents of goat3d files.
 - *goatview*, a 3D scene and animation preview tool, based on OpenGL and Qt.
 - *goatprim*, a procedural 3D model (primitive) generator for quick testing.

License
-------
Copyright (C) 2014-2023 John Tsiombikas <nuclear@member.fsf.org>

Goat3D is free software, you may use, modify and/or redistribute it under the
terms of the GNU Lesser General Public License v3, or at your option any later
version published by the Free Software Foundation. See COPYING and
COPYING.LESSER for details.

Build
-----
To build and install libgoat3d on UNIX, run the usual:

    ./configure
    make
    make install

See `./configure --help` for a complete list of build-time options.

To cross-compile for windows with mingw-w64, try the following incantation:

    ./configure --prefix=/usr/i686-w64-mingw32
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar sys=mingw
    make install sys=mingw

The rest of the tools can be built and installed in the exact same way from
their respective subdirectories.
