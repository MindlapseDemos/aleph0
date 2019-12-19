src = $(wildcard src/*.c) $(wildcard src/scr/*.c) $(wildcard src/sdl/*.c)
obj = $(src:.c=.o) $(asmsrc:.asm=.o)
dep = $(obj:.o=.d)
bin = demo

asmsrc += font.asm

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

$(bin): $(obj) imago mikmod
	$(CC) -o $@ $(obj) $(LDFLAGS)

%.o: %.asm
	nasm -f elf -o $@ $<

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

tools/csprite/csprite:
	$(MAKE) -C tools/csprite

font.asm: data/legible.fnt tools/csprite/csprite
	tools/csprite/csprite -n cs_font -s 8x16 -conv565 -nasm $< >$@
