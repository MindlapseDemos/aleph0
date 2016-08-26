baseobj = main.obj
demoobj = demo.obj screen.obj tunnel.obj
sysobj = gfx.obj vbe.obj dpmi.obj timer.obj keyb.obj logger.obj
obj = $(baseobj) $(demoobj) $(sysobj)
bin = demo.exe

opt = -5 -fp5 -otexan
dbg = -d1

AS = nasm
CC = wcc386
CXX = wpp386
ASFLAGS = -fobj
CFLAGS = $(dbg) $(opt) -zq -bt=dos -Isrc -Isrc\dos
CXXFLAGS = $(CFLAGS)
LD = wlink

$(bin): $(obj)
	%write objects.lnk system dos4g file { $(obj) }
	$(LD) debug all name $@ @objects $(LDFLAGS)

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
