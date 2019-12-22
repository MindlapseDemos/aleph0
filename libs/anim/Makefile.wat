obj = anim.obj track.obj
alib = anim.lib

def = -dM_PI=3.141592653589793
opt = -5 -fp5 -otexan -I.. -I../../src $(def)
dbg = -d1

!ifdef __UNIX__
RM = rm -f
!else
RM = del
!endif

CC = wcc386
CFLAGS = $(dbg) $(opt) $(def) -zq -bt=dos

$(alib): $(obj)
	wlib -b -n $@ $(obj)

.c: src

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*

clean: .symbolic
	$(RM) *.obj
	$(RM) $(alib)
