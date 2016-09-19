src = $(wildcard src/*.c) $(wildcard src/sdl/*.c)
obj = $(src:.c=.o)
bin = demo

inc = -Isrc -Isrc/sdl -Ilibs/imago/src -Ilibs/mikmod/include

CFLAGS = -pedantic -Wall -g $(inc) `sdl-config --cflags`
LDFLAGS = -Llibs/imago -Llibs/mikmod -limago -lmikmod `sdl-config --libs` -lm

$(bin): $(obj) imago mikmod
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: imago
imago:
	$(MAKE) -C libs/imago

.PHONY: mikmod
mikmod:
	$(MAKE) -C libs/mikmod

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
