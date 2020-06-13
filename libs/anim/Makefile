obj = anim.obj track.obj
alib = anim.lib

def = -dM_PI=3.141592653589793
opt = -5 -fp5 -otexan
dbg = -d1

!ifdef __UNIX__
inc =  -I.. -I../../src
RM = rm -f
!else
inc =  -I.. -I..\..\src
RM = del
!endif

CC = wcc386
CFLAGS = $(dbg) $(opt) $(inc) $(def) -zq -bt=dos

$(alib): $(obj)
	wlib -b -n $@ $(obj)

.c: src

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*

clean: .symbolic
	$(RM) *.obj
	$(RM) $(alib)
