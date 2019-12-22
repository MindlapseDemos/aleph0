obj = 3dgfx.obj bsptree.obj bump.obj cfgopt.obj demo.obj djdpmi.obj dynarr.obj &
fract.obj gfx.obj gfxutil.obj greets.obj grise.obj hairball.obj infcubes.obj &
keyb.obj logger.obj main.obj mesh.obj meshload.obj metaball.obj metasurf.obj &
mouse.obj music.obj noise.obj plasma.obj polyclip.obj polyfill.obj polytest.obj &
rbtree.obj sball.obj screen.obj smoketxt.obj thunder.obj tilemaze.obj timer.obj &
tinyfps.obj treestor.obj ts_text.obj tunnel.obj util.obj vbe.obj vga.obj

bin = demo.exe

libs = imago.lib anim.lib

def = -dM_PI=3.141592653589793
opt = -5 -fp5 -otexan -oh -oi -ei
dbg = -d2

!ifdef __UNIX__
incpath = -Isrc -Isrc/dos -Ilibs -Ilibs/imago/src -Ilibs/anim/src
libpath = libpath libs/imago libpath libs/anim
RM = rm -f
!else
incpath = -Isrc -Isrc\dos -Ilibs -Ilibs\imago\src -Ilibs\anim\src
libpath = libpath libs\imago libpath libs\anim
RM = del
!endif

AS = nasm
CC = wcc386
CXX = wpp386
ASFLAGS = -fobj
CFLAGS = $(dbg) $(opt) $(def) -zq -bt=dos $(incpath)
CXXFLAGS = $(CFLAGS)
LDFLAGS = option stack=16k option map $(libpath) library { $(libs) }
LD = wlink

$(bin): cflags.occ $(obj) libs/imago/imago.lib
	%write objects.lnk $(obj)
	%write ldflags.lnk $(LDFLAGS)
	$(LD) debug all name $@ system dos4g file { @objects } @ldflags

.c: src;src/dos;src/scr
.cc: src;src/dos;src/scr
.asm: src;src/dos;src/scr

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
	$(RM) *.obj
	$(RM) *.occ
	$(RM) *.lnk
	$(RM) $(bin)
