baseobj = main.obj
demoobj = demo.obj screen.obj gfxutil.obj 3dgfx.obj polyfill.obj
scrobj = tunnel.obj fract.obj grise.obj polytest.obj
sysobj = gfx.obj vbe.obj dpmi.obj timer.obj keyb.obj mouse.obj logger.obj
obj = $(baseobj) $(demoobj) $(sysobj) $(scrobj)
bin = demo.exe

libs = imago.lib

def = -dM_PI=3.141592653589793
opt = -5 -fp5 -otexan -oh -oi -ei
dbg = -d1

!ifdef __UNIX__
incpath = -Isrc -Isrc/dos -Ilibs/imago/src
libpath = libs/imago
!else
incpath = -Isrc -Isrc\dos -Ilibs\imago\src
libpath = libs\imago
!endif

AS = nasm
CC = wcc386
CXX = wpp386
ASFLAGS = -fobj
CFLAGS = $(dbg) $(opt) $(def) -zq -bt=dos $(incpath)
CXXFLAGS = $(CFLAGS)
LDFLAGS = libpath $(libpath) library { $(libs) }
LD = wlink

$(bin): cflags.occ $(obj) libs/imago/imago.lib
	%write objects.lnk $(obj)
	$(LD) debug all name $@ system dos4g file { @objects } $(LDFLAGS)

.c: src;src/dos
.cc: src;src/dos
.asm: src;src/dos

cflags.occ: Makefile
	%write $@ $(CFLAGS)

cxxflags.occ: Makefile
	%write $@ $(CXXFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

.cc.obj: .autodepend
	$(CXX) -fo=$@ @cxxflags.occ $[*

.asm.obj:
	$(AS) $(ASFLAGS) -o $@ $[*.asm

clean: .symbolic
	del *.obj
	del *.occ
	del *.lnk
	del $(bin)
