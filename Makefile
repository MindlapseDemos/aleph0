src = $(wildcard src/*.c) $(wildcard src/scr/*.c) $(wildcard src/dos/*.c)
asmsrc = $(wildcard src/*.asm) $(wildcard src/scr/*.asm) $(wildcard src/dos/*.asm)
obj = $(src:.c=.odj) $(asmsrc:.asm=.odj)
dep = $(obj:.odj=.dep)
bin = demo.exe

asmsrc += cspr/dbgfont.asm cspr/confont.asm
bindata = data/loading.img

ifeq ($(findstring COMMAND.COM, $(SHELL)), COMMAND.COM)
	hostsys = dos
else
	hostsys = unix
	TOOLPREFIX = i586-pc-msdosdjgpp-
endif

inc = -Isrc -Isrc/scr -Isrc/dos -Ilibs -Ilibs/imago/src -Ilibs/anim/src
opt = -O3 -ffast-math -fno-strict-aliasing
dbg = -g
#prof = -pg
warn = -pedantic -Wall -Wno-unused-function -Wno-unused-variable
def = -DNO_MUSIC

CC = $(TOOLPREFIX)gcc
AR = $(TOOLPREFIX)ar
CFLAGS = $(warn) -march=pentium $(dbg) $(opt) $(prof) $(inc) $(def)
LDFLAGS = libs/imago/imago.dja libs/anim/anim.dja

ifneq ($(hostsys), dos)
.PHONY: all
all: data $(bin)
endif

$(bin): $(obj) imago anim
	$(CC) -o $@ -Wl,-Map=ld.map $(prof) $(obj) $(LDFLAGS)

%.odj: %.asm
	nasm -f coff -o $@ $<

src/data.odj: src/data.asm $(bindata)

ifneq ($(hostsys), dos)
-include $(dep)
endif

%.odj: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.dep: %.c
	@echo dep $@
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.dep=.odj) >$@

.PHONY: imago
imago:
	$(MAKE) -C libs/imago -f Makefile

.PHONY: anim
anim:
	$(MAKE) -C libs/anim -f Makefile

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/imago -f Makefile clean
	$(MAKE) -C libs/anim -f Makefile clean

.PHONY: clean
.PHONY: cleandep

ifeq ($(hostsys), dos)
clean:
	del src\*.odj
	del src\dos\*.odj
	del $(bin)

cleandep:
	del src\*.dep
	del src\dos\*.dep
else
clean:
	rm -f $(obj) $(bin)

cleandep:
	rm -f $(dep)

.PHONY: data
data:
	@tools/procdata
endif
