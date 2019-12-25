src = $(wildcard src/*.c)
obj = $(src:.c=.o)
bin = ropesim

def = -DUSE_ASSIMP
inc = -I../../src
warn = -pedantic -Wall -Wno-int-to-pointer-cast

CFLAGS = $(warn) -g $(def)
LDFLAGS = $(libgl) -lm -lassimp

libgl = -lGL -lGLU -lglut

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
