odrivers = drv_oss.obj drv_wss.obj drv_ultra.obj drv_sb.obj drv_sdl.obj drv_nos.obj &
dossb.obj dosirq.obj dosgus.obj dosdma.obj doswss.obj
oloaders = load_s3m.obj load_mod.obj load_it.obj load_xm.obj
ommio = mmalloc.obj mmerror.obj mmio.obj
odepackers = mmcmp.obj pp20.obj s404.obj xpk.obj
oposix = memcmp.obj strcasecmp.obj strstr.obj
oplayercode = mdreg.obj mlreg.obj mloader.obj virtch_common.obj munitrk.obj mplayer.obj &
mlutil.obj sloader.obj npertab.obj virtch.obj mdulaw.obj mwav.obj virtch2.obj mdriver.obj

obj = $(odrivers) $(oloaders) $(ommio) $(odepackers) $(oposix) $(oplayercode)

alib = mikmod.lib

opt = -5 -fp5 -otexan -zu
dbg = -d1
def = -DHAVE_CONFIG_H -DMIKMOD_BUILD

CC = wcc386
CFLAGS = $(dbg) $(opt) $(def) -zq -bt=dos -I. -Iinclude -Idrvdos

$(alib): cflags.occ $(obj)
	%write objects.lbc $(obj)
	wlib -b -n $@ @objects

.c: drivers;drvdos;loaders;mmio;depackers;posix;playercode

cflags.occ: Makefile
	%write $@ $(CFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

clean: .symbolic
	del *.obj
	del *.occ
	del *.lbc
	del $(alib)
