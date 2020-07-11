src = $(wildcard src/*.c) $(wildcard src/3dgfx/*.c) $(wildcard src/scr/*.c) \
	  $(wildcard src/glut/*.c)
asmsrc = $(wildcard src/*.asm)
obj = $(src:.c=.o) $(asmsrc:.asm=.o)
dep = $(obj:.o=.d)
bin = demo

asmsrc += cspr/dbgfont.asm cspr/confont.asm
bindata = data/loading.img

inc = -I/usr/local/include -Isrc -Isrc/3dgfx -Isrc/scr -Isrc/glut -Ilibs \
	  -Ilibs/imago/src -Ilibs/mikmod/include
def = -DMINIGLUT_USE_LIBC -DMIKMOD_STATIC
warn = -pedantic -Wall -Wno-unused-variable -Wno-unused-function
#opt = -O3 -ffast-math
dbg = -g

CFLAGS = $(arch) $(warn) -MMD $(opt) -fno-strict-aliasing $(dbg) $(inc)
LDFLAGS = $(arch) -Llibs/imago -Llibs/mikmod -limago -lmikmod \
		  $(sndlib_$(sys)) -lm

cpu ?= $(shell uname -m | sed 's/i.86/i386/')

ifeq ($(cpu), x86_64)
	arch = -m32
endif

sys ?= $(shell uname -s | sed 's/MINGW.*/mingw/; s/IRIX.*/IRIX/')
ifeq ($(sys), mingw)
	obj = $(src:.c=.w32.o)

	bin = demo_win32.exe

	LDFLAGS += -static-libgcc -lmingw32 -mconsole -lgdi32 -lwinmm \
			   -lopengl32
else
	LDFLAGS += -lGL -lX11 -lpthread
endif

sndlib_Linux = -lasound
sndlib_IRIX = -laudio
sndlib_mingw = -ldsound

.PHONY: all
all: data $(bin)

$(bin): $(obj) imago mikmod
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.o: %.asm
	nasm -f elf -o $@ $<

%.w32.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

src/data.o: src/data.asm $(bindata)

.PHONY: libs
libs: imago anim mikmod

.PHONY: imago
imago:
	$(MAKE) -C libs/imago

.PHONY: anim
anim:
	$(MAKE) -C libs/anim

.PHONY: mikmod
mikmod:
	$(MAKE) -C libs/mikmod

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/imago clean
	$(MAKE) -C libs/anim clean
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


.PHONY: crosswin
crosswin:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw
