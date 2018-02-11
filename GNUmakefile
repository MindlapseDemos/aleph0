src = $(wildcard src/*.c) $(wildcard src/sdl/*.c)
obj = $(src:.c=.o) $(asmsrc:.asm=.o)
dep = $(obj:.o=.d)
bin = demo

inc = -I/usr/local/include -Isrc -Isrc/sdl -Ilibs/imago/src -Ilibs/mikmod/include

CFLAGS = -pedantic -Wall -g $(inc) `sdl-config --cflags`
LDFLAGS = -Llibs/imago -Llibs/mikmod -limago -lmikmod `sdl-config --libs` -lm

$(bin): $(obj) imago mikmod
	$(CC) -o $@ $(obj) $(LDFLAGS)

%.o: %.asm
	nasm -f elf -o $@ $<

-include $(dep)

%.d: %.c
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
