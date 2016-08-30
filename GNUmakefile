src = $(wildcard src/*.c) $(wildcard src/sdl/*.c)
obj = $(src:.c=.o)
bin = demo

inc = -Isrc -Isrc/sdl -Ilibs/imago/src

CFLAGS = -pedantic -Wall -g $(inc) `sdl-config --cflags`
LDFLAGS = -Llibs/imago -limago `sdl-config --libs` -lm

$(bin): $(obj) imago
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: imago
imago:
	$(MAKE) -C libs/imago

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
