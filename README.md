Unnamed Mindlapse DOS demo for Pentium 133
------------------------------------------
The demo requires VESA Bios Extensions (VBE) 2.0. If your graphics card doesn't
support VBE 2.0 or greater, then make sure to load the `univbe` TSR first.

Source structure
----------------
 - src/          cross-platform demo framework and miscellaneous utility code
 - src/scr/      demo screens (parts) and effects support code
 - src/dos/      DOS platform code
 - src/sdl/      SDL 1.x platform code (windows/UNIX version)
 - libs/cgmath/  math library, header-file only
 - libs/imago/   image loading library (includes libpng, zlib, libjpeg)
 - libs/anim/    keyframe animation library

Building on DOS with Watcom
---------------------------
NOTE: Don't. Watcom produces significantly worse code than GCC, and at the
moment watcom-compiled version of the demo crashes on 3D scenes for some reason
which I need to investigate at some point. Suspect either inline assembly with
missing "modify" part, or more likely some FPU optimization which fucks up the
clipper.

Make sure you have Watcom or OpenWatcom installed, and the appropriate env-vars
set (the watcom installer automatically adds them to autoexec.bat by default).

Run wmake to build. Needs dos4gw.exe in current dir.

OpenWatcom cross-compilation on UNIX
------------------------------------
source owdev script with contents (change WATCOM var as necessary):

  export WATCOM=$HOME/devel/ow
  export PATH=$WATCOM/binl:$PATH
  export INCLUDE=$WATCOM/h:$INCLUDE

Run wmake to build. Needs dos4gw.exe and wstub.exe in current dir or PATH

Simply running ./demo.exe might invoke magic of the ancients to start wine,
which will in turn start dosbox, which will execute the DOS binary! If the gods
are slumbering in valhalla, just typing `dosbox demo.exe` should do the trick.

Building with DJGPP
-------------------
The DJGPP port of GCC is the recommended toolchain to use for building the demo,
either natively or cross-compiled on UNIX.

For native DOS builds add the DJGPP bin directory to the path (usually
`c:\djgpp\bin`) and set the DJGPP environment variable to point to the
`djgpp.env` file.

For cross-compiling on UNIX simply source the `setenv` file which comes with
DJGPP, which will set the `PATH` and `DJDIR` variables as necessary.

In both cases, run `make -f Makefile.dj` to build. To run the resulting
`demo.exe` you'll need to copy `cwsdpmi.exe` to the same directory. You can find
it here: ftp://ftp.fu-berlin.de/pc/languages/djgpp/current/v2misc/csdpmi7b.zip

When building natively on an old computer, and encounter a huge amount of disk
swapping, and corresponding ridiculously long compile times, make sure to
disable any memory managers and try again. `EMM386` may interfere with
`CWSDPMI`'s ability to use more than 32mb of RAM, and modern versions of GCC
need way more than that. Disable the memory manager with `emm386 off`, and
verify the amount of usable RAM with `go32-v2`.

Another problem is the MS-DOS version of `HIMEM.SYS` which only reports up to
64mb. To use more than that, which is necessary for modern versions of GCC, you
can either disable `HIMEM.SYS` completely, or use the `HIMEM.SYS` driver that
comes with Windows 9x (DOS >= 7). Here's a copy in case you need it:
http://mutantstargoat.com/~nuclear/tmp/d7himem.sys


Building and running with the SDL backend
-----------------------------------------
Run make to build (assuming make on your system is GNU make), or use the visual
studio 2013 project on Windows.

The SDL backend will scale the framebuffer up, by the factor specified in the
`FBSCALE` environment variable. So run the demo as: `FBSCALE=3 ./demo` for
a 3x scale factor, or just export the `FBSCALE` env var in the shell you are
going to use for running the demo. The default scale factor is 2x.

Datafiles
---------
The demo datafiles are in their own subversion repo. To checkout the data files
run the following in the demo root directory:

  svn co svn://mutantstargoat.com/datadirs/dosdemo data

Random optimization details about the Pentium1 (p54c)
-----------------------------------------------------
Use cround64 (util.h) for float -> integer conversions, instead of casts.

Performance measurement with RDTSC:
    perf_start();
    /* code under test */
    perf_end(); /* result in perf_interval_count */

Cache organization (L1): 8kb data / 8kb instruction
128 sets of 2 cache lines, 32 bytes per cache line.

Addresses which are multiples of 4096 fall in the same set and can only have
two of them in cache at any time.

U/V pipe pairing rules:
 - both instructions must be simple
 - no read-after-write or write-after-write reg dependencies
 - no displacement AND immediate in either instruction
 - instr. with prefixes (except 0x0f) can only run on U pipe.
 - prefixes are treated as separate 1-byte instructions (except 0x0f).
 - branches can be paired if they are the second instr. of the pair only.

Simple instructions are:
 - mov reg, reg/mem/imm
 - mov mem, reg/imm
 - alu reg, reg/mem/imm (alu: add/sub/cmp/and/or/xor)
 - alu mem, reg/imm
 - inc reg/mem
 - dec reg/mem
 - push reg/mem
 - pop reg
 - lea reg,mem
 - jmp/call/jcc near
 - nop
 - test reg,reg/mem
 - test acc,imm

U-only pairable instructions:
 - adc, sbb
 - shr, sar, shl, sal with immediate
 - ror, rol, rcr, rcl with immediate=1

Notes about DJGPP & CWSDPMI
---------------------------
Can't use the `hlt` instruction for waiting for interrupts, because we're
running in ring3 by default. I surrounded all the `hlt` instructions with a
`USE_HLT` conditional, which is undefined when building with DJGPP.

It's possible to arrange for our code to run on ring0 by changing the DPMI
provider from `cwsdpmi.exe` to `cwsdpr0.exe` by running:
`stubedit demo.exe dpmi=cwsdpr0.exe`, but I haven't tested under win9x to see if
it still works if we do that.

Our fucking segments don't start at 0 ... to access arbitrary parts of physical
memory we need to call `__djgpp_nearptr_enable()` and use the following macros I
defined in `cdpmi.h`:

    #define virt_to_phys(v)	((v) + __djgpp_base_address)
    #define phys_to_virt(p)	((p) - __djgpp_base_address)
