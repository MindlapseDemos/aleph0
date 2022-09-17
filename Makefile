!ifdef __UNIX__
dosobj = src/dos/audos.obj src/dos/djdpmi.obj src/dos/gfx.obj src/dos/keyb.obj &
	src/dos/logger.obj src/dos/main.obj src/dos/sball.obj src/dos/timer.obj &
	src/dos/vbe.obj src/dos/vga.obj src/dos/s3.obj src/dos/watdpmi.obj &
	src/dos/mouse.obj src/dos/pci.obj
3dobj = src/3dgfx/3dgfx.obj src/3dgfx/mesh.obj src/3dgfx/meshload.obj &
	src/3dgfx/polyclip.obj src/3dgfx/polyfill.obj
rtobj = src/rt/rt.obj src/rt/rtgeom.obj
srcobj = src/bsptree.obj src/cfgopt.obj src/console.obj src/demo.obj &
	src/dynarr.obj src/gfxutil.obj src/metasurf.obj src/noise.obj &
	src/rbtree.obj src/screen.obj src/tinyfps.obj src/treestor.obj &
	src/image.obj src/ts_text.obj src/util.obj src/util_s.obj src/cpuid.obj &
	src/cpuid_s.obj src/darray.obj src/data.obj src/rlebmap.obj
scrobj = src/scr/bump.obj src/scr/fract.obj src/scr/greets.obj &
	src/scr/grise.obj src/scr/hairball.obj src/scr/infcubes.obj &
	src/scr/metaball.obj src/scr/plasma.obj src/scr/polytest.obj &
	src/scr/smoketxt.obj src/scr/thunder.obj src/scr/tilemaze.obj &
	src/scr/tunnel.obj src/scr/cybersun.obj src/scr/raytrace.obj &
	src/scr/minifx.obj
csprobj = cspr/dbgfont.obj cspr/confont.obj

incpath = -Isrc -Isrc/dos -Isrc/3dgfx -Isrc/rt -Ilibs -Ilibs/imago/src -Ilibs/anim/src &
	-Ilibs/midas
libpath = libpath libs/imago libpath libs/anim libpath libs/midas
!else

dosobj = src\dos\audos.obj src\dos\djdpmi.obj src\dos\gfx.obj src\dos\keyb.obj &
	src\dos\logger.obj src\dos\main.obj src\dos\sball.obj src\dos\timer.obj &
	src\dos\vbe.obj src\dos\vga.obj src\dos\s3.obj src\dos\watdpmi.obj &
	src\dos\mouse.obj src\dos\pci.obj
3dobj = src\3dgfx\3dgfx.obj src\3dgfx\mesh.obj src\3dgfx\meshload.obj &
	src\3dgfx\polyclip.obj src\3dgfx\polyfill.obj
rtobj = src\rt\rt.obj src\rt\rtgeom.obj
srcobj = src\bsptree.obj src\cfgopt.obj src\console.obj src\demo.obj &
	src\dynarr.obj src\gfxutil.obj src\metasurf.obj src\noise.obj &
	src\rbtree.obj src\screen.obj src\tinyfps.obj src\treestor.obj &
	src\image.obj src\ts_text.obj src\util.obj src\util_s.obj src\cpuid.obj &
	src\cpuid_s.obj src\darray.obj src\data.obj src\rlebmap.obj
scrobj = src\scr\bump.obj src\scr\fract.obj src\scr\greets.obj &
	src\scr\grise.obj src\scr\hairball.obj src\scr\infcubes.obj &
	src\scr\metaball.obj src\scr\plasma.obj src\scr\polytest.obj &
	src\scr\smoketxt.obj src\scr\thunder.obj src\scr\tilemaze.obj &
	src\scr\tunnel.obj src\scr\cybersun.obj src\scr\raytrace.obj &
	src\scr\minifx.obj
csprobj = cspr\dbgfont.obj cspr\confont.obj

incpath = -Isrc -Isrc\dos -Isrc\3dgfx -Isrc\rt -Ilibs -Ilibs\imago\src &
	-Ilibs\anim\src -Ilibs\midas
libpath = libpath libs\imago libpath libs\anim libpath libs\midas
!endif

obj = $(dosobj) $(3dobj) $(rtobj) $(scrobj) $(srcobj) $(csprobj)
bin = demo.exe

opt = -otexan
#opt = -od
def = -dM_PI=3.141592653589793 -dUSE_HLT -dNO_SOUND
libs = imago.lib anim.lib
# midas.lib

AS = nasm
CC = wcc386
LD = wlink
ASFLAGS = -fobj
CFLAGS = -d3 -5 -fp5 $(opt) $(def) -s -zq -bt=dos $(incpath)
LDFLAGS = option map $(libpath) library { $(libs) }

$(bin): cflags.occ $(obj) libs/imago/imago.lib libs/anim/anim.lib
	%write objects.lnk $(obj)
	%write ldflags.lnk $(LDFLAGS)
	$(LD) debug all name $@ system dos4g file { @objects } @ldflags

.c: src;src/dos;src/3dgfx;src/rt;src/scr;cspr
.asm: src;src/dos;src/3dgfx;src/rt;src/scr;cspr

cflags.occ: Makefile
	%write $@ $(CFLAGS)

!ifdef __UNIX__
src/dos/audos.obj: src/dos/audos.c
!else
src\dos\audos.obj: src\dos\audos.c
!endif
	$(CC) -fo=$@ @cflags.occ -zu $[*

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

.asm.obj:
	nasm -f obj -o $@ $[*.asm

!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(bin)
	rm -f cflags.occ *.lnk
!else
clean: .symbolic
	del src\*.obj
	del src\dos\*.obj
	del src\3dgfx\*.obj
	del src\rt\*.obj
	del src\scr\*.obj
	del *.lnk
	del cflags.occ
	del $(bin)
!endif
