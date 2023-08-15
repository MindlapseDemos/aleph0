# $(patsubst pattern,replacement,text)

demosrcdir = ../../src
libs_dir = ../../libs

demosrc = $(wildcard $(demosrcdir)/3dgfx/*.c) $(demosrcdir)/glut/miniglut.c \
		  $(demosrcdir)/gfxutil.c $(demosrcdir)/util.c $(demosrcdir)/dynarr.c \
		  $(demosrcdir)/darray.c $(demosrcdir)/rbtree.c $(demosrcdir)/treestor.c \
		  $(demosrcdir)/ts_text.o
src = src/main.c $(demosrc)
obj = $(src:.c=.o)
bin = 3dview

incdir = -I$(demosrcdir) -I$(demosrcdir)/3dgfx -I$(demosrcdir)/glut \
		 -I$(libs_dir)/goat3d/include
libdir = -L$(libs_dir)/goat3d -L$(libs_dir)/imago

CFLAGS = $(arch) -pedantic -Wall -g $(incdir)
LDFLAGS = $(arch) -no-pie $(libdir) $(libgl) -lgoat3d -limago -lm

cpu ?= $(shell uname -m | sed 's/i.86/i386/')
ifeq ($(cpu), x86_64)
	arch = -m32
endif

sys ?= $(shell uname -s | sed 's/MINGW.*/mingw/')
ifeq ($(sys), mingw)
	libgl = -lopengl32 -lglu32 -lwinmm
	bin = 3dview.exe

	objdir = outw32
else
	libgl = -lGL -lGLU -lX11

	objdir = outunix
endif

$(bin): $(obj) imago goat3d
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.w32.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

.PHONY: libs
libs: imago anim mikmod goat3d

.PHONY: imago
imago:
	$(MAKE) -C $(libs_dir)/imago

.PHONY: goat3d
goat3d:
	$(MAKE) -C $(libs_dir)/goat3d

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C $(libs_dir)/imago clean
	$(MAKE) -C $(libs_dir)/goat3d clean


.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)


.PHONY: crosswin
crosswin:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw
