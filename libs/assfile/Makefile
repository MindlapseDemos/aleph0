obj = assfile.obj mod_path.obj mod_tar.obj mod_url.obj tar.obj

!include ../../watcfg.mk

!ifdef __UNIX__
RM = rm -f
alib = ../../assfile.lib
!else
RM = del
alib = ..\..\assfile.lib
!endif

CC = wcc386
CFLAGS = $(cfg_dbg) $(cfg_opt) $(inc) $(def) -zq -bt=dos

$(alib): $(obj)
	wlib -b -n $@ $(obj)

.c: src

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*

clean: .symbolic
	$(RM) *.obj
	$(RM) $(alib)
