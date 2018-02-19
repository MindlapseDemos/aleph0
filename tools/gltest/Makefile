root = ../..

src = $(wildcard src/*.c) \
	  $(root)/src/bsptree.c \
	  $(root)/src/polyclip.c \
	  $(root)/src/mesh.c \
	  $(root)/src/meshload.c \
	  $(root)/src/dynarr.c \
	  $(root)/src/rbtree.c
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = test

incpath = -I$(root)/src
libpath = -L$(root)/libs/imago -L$(root)/libs/mikmod

CFLAGS = -pedantic -Wall -g $(incpath)
LDFLAGS = $(libpath) -lGL -lGLU -lglut -lm -limago -lmikmod

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f src/*.o
