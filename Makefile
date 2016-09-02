baseobj = main.obj
demoobj = demo.obj screen.obj tunnel.obj fract.obj gfxutil.obj mike.obj
sysobj = gfx.obj vbe.obj dpmi.obj timer.obj keyb.obj mouse.obj logger.obj
obj = $(baseobj) $(demoobj) $(sysobj)
bin = demo.exe

libs = imago.lib

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
CFLAGS = $(dbg) $(opt) -zq -bt=dos $(incpath)
CXXFLAGS = $(CFLAGS)
LDFLAGS = libpath $(libpath) library { $(libs) }
LD = wlink

$(bin): $(obj) libs/imago/imago.lib
	%write objects.lnk $(obj)
	$(LD) debug all name $@ system dos4g file { @objects } $(LDFLAGS)

.c: src;src/dos
.cc: src;src/dos
.asm: src;src/dos

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*

.cc.obj: .autodepend
	$(CXX) -fo=$@ $(CXXFLAGS) $[*

.asm.obj:
	$(AS) $(ASFLAGS) -o $@ $[*.asm

clean: .symbolic
	del *.obj
	del $(bin)
