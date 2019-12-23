src = $(wildcard src/*.c) $(wildcard src/scr/*.c) $(wildcard src/sdl/*.c)
asmsrc = $(wildcard src/*.asm)
obj = $(src:.c=.o) $(asmsrc:.asm=.o)
dep = $(obj:.o=.d)
bin = demo

asmsrc += cspr/dbgfont.asm cspr/confont.asm
bindata = data/loading.img

inc = -I/usr/local/include -Isrc -Isrc/scr -Isrc/sdl -Ilibs -Ilibs/imago/src -Ilibs/mikmod/include
warn = -pedantic -Wall -Wno-unused-variable -Wno-unused-function

CFLAGS = $(arch) $(warn) -g $(inc) `sdl-config --cflags`
LDFLAGS = $(arch) -Llibs/imago -Llibs/mikmod -limago -lmikmod $(sdl_ldflags) -lm

ifneq ($(shell uname -m), i386)
	arch = -m32
	sdl_ldflags = -L/usr/lib/i386-linux-gnu -lSDL
else
	sdl_ldflags = `sdl-config --libs`
endif

.PHONY: all
all: data $(bin)

$(bin): $(obj) imago mikmod
	$(CC) -o $@ $(obj) $(LDFLAGS)

%.o: %.asm
	nasm -f elf -o $@ $<

src/data.o: src/data.asm $(bindata)

-include $(dep)

%.d: %.c
	@echo dep $@
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: imago
imago:
	$(MAKE) -C libs/imago

.PHONY: mikmod
mikmod:
	$(MAKE) -C libs/mikmod

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/imago clean
	$(MAKE) -C libs/mikmod clean

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: data
data:
	@tools/procdata
