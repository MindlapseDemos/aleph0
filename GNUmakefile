src = $(wildcard src/*.c) $(wildcard src/sdl/*.c)
obj = $(src:.c=.o)
bin = demo

inc = -Isrc -Isrc/sdl

CFLAGS = -pedantic -Wall -g $(inc) `sdl-config --cflags`
LDFLAGS = `sdl-config --libs` -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
