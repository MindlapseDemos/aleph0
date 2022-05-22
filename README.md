Unnamed Mindlapse DOS demo for Pentium-era PCs
----------------------------------------------

![dos (watcom) build status](https://github.com/MindlapseDemos/wip-dosdemo/actions/workflows/build_dos_watcom.yml/badge.svg)
![dos (djgpp) build status](https://github.com/MindlapseDemos/wip-dosdemo/actions/workflows/build_dos_djgpp.yml/badge.svg)
![GNU/Linux build status](https://github.com/MindlapseDemos/wip-dosdemo/actions/workflows/build_gnulinux32.yml/badge.svg)
![win32 (msvc) build status](https://github.com/MindlapseDemos/wip-dosdemo/actions/workflows/build_win_msvc.yml/badge.svg)

The demo uses VBE 320x240 16bpp. Some VBE implementations do not expose
double-scan video modes (240 lines), but can be made to work with a third-party
VBE TSR like `univbe` or `s3vbe`. Linear framebuffer (VBE 2.0) support is
recommended, but not necessary. The demo will fallback to VBE 1.2 banked modes
if LFB modes are not available.

Source structure
----------------
 - src/          cross-platform demo framework and miscellaneous utility code
 - src/scr/      demo screens (parts) and effects support code
 - src/dos/      DOS platform code
 - src/glut/     GLUT platform code (windows/UNIX version)
 - src/sdl/      SDL 1.x platform code (windows/UNIX version)
 - libs/cgmath/  math library, header-file only
 - libs/imago/   image loading library (includes libpng, zlib, libjpeg)
 - libs/anim/    keyframe animation library

Coding style conventions
------------------------
Very few style issues are mandated:

  - All filenames should be lowercase unless convention says otherwise
    (`Makefile`, `README`, etc).
  - All filenames under `src/` and of any tools necessary to build from MS-DOS
    will have to use maximum 8.3 characters.
  - Source code should be C89-compliant. Any compiler-specific features beyond
    that will have to be ifdefed.
  - Use tabs for indentation, where each tab is equivalent to 4 spaces.

Everything else is left to the discretion of each individual, but also if
you're editing an existing file, try to match the style of the surrounding code.

Some general style suggestions, which will not be enforced:

  - Don't use overly long names, abbreviate wherever it makes sense.
  - Don't cast the return value of malloc. `void*` are converted implicitly, and
    the cast hides forgetting to include `stdlib.h`.
  - Preferably use lowercase with underscores for everything.
  - Preferably use the K&R brace style if possible.

This section will be expanded as necessary.

Building on DOS with Watcom
---------------------------
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


Building the modern PC cross-platform version
---------------------------------------------
The cross-platform version for modern PCs uses OpenGL textures to present the
demo framebuffer.

On UNIX, or windows with msys2, just run `make` to build.

Alternatively on windows you can also use the included visual studio 2022
project files. In this case you'll have to do some steps manually, at least the
first time:
  - Build the `img2bin` and `csprite` tool subprojects first (or just do a full
    build and let it fail the first time).
  - Run the `procdata.bat` batch file in the `tools` directory to run the tools
    on the data files.
  - Build the demo project as usual.

### Assembly source files in visual studio
If you're using the visual studio project, and you need to add a new assembly
file to the project, you'll have to set a "custom build tool" for it to let
visual studio know to use nasm to assemble it:
  - Right click on the new assembly file and go to "properties".
  - Change "Item Type" to "Custom Build Tool", and set "Exclude From Build" to
    "No".
  - Apply the changes, for the "Custom Build Tool" dropdown to appear under
    "Configuration Properties".
  - Under "Custom Build Tool" "General" settings:
    * Set "Command Line" to: `nasm -o .%(Filename).obj -f win32 %(FullPath)`.
    * Set "Outputs" to: `.%(Filename).obj`.

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

Notes about moving code to/from a DOS computer
----------------------------------------------
The easiest way to move code back and forth to a DOS computer, is to connect it
to the local network. For this you need a DOS packet driver for your NIC, which
thankfully most NIC vendors seem to provide, and a number of useful network
utilities which come with their own TCP/IP stack (mTCP and WATTCP). The
following are recommended:

 - mTCP: http://www.brutman.com/mTCP
 - WATTCP: http://www.watt-32.net
 - ssh2dos: http://sshdos.sourceforge.net
 - rsync: http://www.2net.co.uk/rsync.html

Here's an example batch file I'm using to set up the network:

    @echo off
    c:\net\rtspkt\rtspkt 0x61
    set MTCPCFG=c:\net\mtcp\mtcp.cfg
    set WATT_ROOT=c:\net\watt
    set WATTCP.CFG=c:\net\watt\bin
    set ETC=c:\net\watt\bin
    set PATH=%PATH%;c:\net\mtcp;c:\net\watt\bin

The rtspkt program is the packet driver for my realtek NIC, and I'm instructing
it to use interrupt 61h. The rest are environment variables needed by mTCP and
WATTCP. If you run out of environment space you might need to increase it with
`SHELL=C:\DOS\COMMAND.COM /e:1024 /p` in `config.sys`, or just put all binaries
in a single place instead of adding multiple directories to the `PATH`.

### mTCP configuration
The `mtcp.cfg` file configures the mTCP TCP/IP stack. Go through the file, which
comes with mTCP, and make any necessary changes. For instance I've set
`packetint 0x61` to match the packet driver interrupt, and I'm using static IP
assignments, so I've set it up like this:

    ipaddr 192.168.0.21
    netmask 255.255.0.0
    gateway 192.168.1.1
    nameserver 1.1.1.1

### WATTCP configuration
The `wattcp.cfg` file is in the wattcp bin directory, and includes similar
configuration options:

    my_ip = 192.168.0.21
    hostname = "retrop1"
    netmask = 255.255.0.0
    nameserver = 1.1.1.1
    gateway = 192.168.1.1
    domain.suffix = localdomain
    pkt.vector = 0x61
    hosts = $(ETC)\hosts

### Server-side configuration
The `pull.bat` file in the demo repo uses rsync to get the source code from the
git repo on my main GNU/Linux computer. To avoid having to type passwords all
the time, I've configures rsyncd to allow access to the demo directory in the
`/etc/rsyncd.conf` file:

    [dosdemo]
    path = /home/nuclear/code/demoscene/dosdemo
    comment = DOS demo project

Since the DOS rsync utility is unfortunately read-only, the `push.bat` relies on
ssh2dos instead, which does require a password. The sshd on the server might
need to be configured to allow older encryption algorithms, depending on your
current setup.
