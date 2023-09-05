obj = anim.obj track.obj

!include ../../watcfg.mk
def = -dM_PI=3.141592653589793

!ifdef __UNIX__
inc =  -I.. -I../../src
RM = rm -f
alib = ../../anim.lib
!else
inc =  -I.. -I..\..\src
RM = del
alib = ..\..\anim.lib
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
