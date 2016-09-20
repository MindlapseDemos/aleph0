oloader = mloader.obj load_s3m.obj load_mod.obj load_uni.obj load_mtm.obj &
load_m15.obj load_ult.obj load_stm.obj load_xm.obj 
odriver = mdriver.obj drv_nos.obj drv_gus.obj drv_sb.obj drv_ss.obj

obj = mmio.obj mirq.obj mdma.obj virtch.obj resample.obj munitrk.obj mplayer.obj &
$(oloader) $(odriver)

alib = mikmod.lib

opt = -5 -oxl+
#dbg = -d1

CC = wcc386
AS = wasm
CFLAGS = $(dbg) $(opt) -zq -zu -j -s -bt=dos -Isrc
ASFLAGS = -zq -5pr

$(alib): cflags.occ $(obj)
	%write objects.lbc $(obj)
	wlib -b -n $@ @objects

.c: src
.asm: src

cflags.occ: Makefile
	%write $@ $(CFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

.asm.obj:
	$(AS) -fo=$@ $(ASFLAGS) $[*

clean: .symbolic
	del *.obj
	del *.occ
	del *.lbc
	del $(alib)
