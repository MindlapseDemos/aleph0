BUILD ?= debug

src = $(wildcard src/*.c) $(wildcard src/3dgfx/*.c) $(wildcard src/rt/*.c) \
	  $(wildcard src/scr/*.c) $(wildcard src/glut/*.c)
asmsrc = $(wildcard src/*.asm) $(wildcard src/scr/*.asm)
obj = $(src:.c=.o) $(asmsrc:.asm=.o)
dep = $(obj:.o=.d)
bin = demo

asmsrc += cspr/dbgfont.asm cspr/confont.asm
bindata = data/loading.img

inc = -Isrc -Isrc/3dgfx -Isrc/rt -Isrc/scr -Isrc/utils \
	  -Isrc/glut -Ilibs -Ilibs/imago/src -Ilibs/anim/src -Ilibs/mikmod/include \
	  -Ilibs/goat3d/include -I/usr/local/include
def = -DMINIGLUT_USE_LIBC -DMIKMOD_STATIC
warn = -pedantic -Wall -Wno-unused-variable -Wno-unused-function -Wno-address

ifeq ($(BUILD), release)
	opt = -O3 -ffast-math
	def += -DNDEBUG
else
	dbg = -g
endif

CFLAGS = $(arch) $(warn) -MMD $(opt) $(def) -fno-pie -fno-strict-aliasing $(dbg) $(inc)
LDFLAGS = $(arch) -static-libgcc -no-pie -Llibs/imago -Llibs/anim -Llibs/mikmod \
		  -Llibs/goat3d -limago -lanim -lmikmod -lgoat3d $(sndlib_$(sys)) -lm

cpu ?= $(shell uname -m | sed 's/i.86/i386/;s/arm.*/arm/')

ifeq ($(cpu), x86_64)
	arch = -m32
endif

ifeq ($(cpu), aarch64)
	cpu := arm
	CC = arm-linux-gnueabihf-gcc
	export CC
endif

ifeq ($(cpu), arm)
	obj = $(src:.c=.arm.o)
	def += -DNO_ASM
	bin = demopi
endif

sys ?= $(shell uname -s | sed 's/MINGW.*/mingw/; s/IRIX.*/IRIX/')
ifeq ($(sys), mingw)
	obj = $(src:.c=.w32.o) $(asmsrc:.asm=.w32.o)

	bin = demo_win32.exe

	LDFLAGS += -lmingw32 -mconsole -lgdi32 -lwinmm -lopengl32
else
	ifeq ($(sys), Darwin)
		arch =
		asmsrc =
		def += -DNO_ASM
		LDFLAGS = -Llibs/imago -Llibs/anim -Llibs/mikmod -Llibs/goat3d -limago \
				  -lanim -lmikmod -lgoat3d $(sndlib_$(sys)) -lm \
				  -framework OpenGL -framework GLUT
	else
		LDFLAGS += -lGL -lX11 -lpthread
	endif
endif

sndlib_Linux = -lasound
sndlib_IRIX = -laudio
sndlib_mingw = -ldsound
sndlib_Darwin = -framework CoreAudio

.PHONY: all
all: data $(bin)

.PHONY: nodata
nodata: $(bin)

$(bin): $(obj) imago anim mikmod goat3d
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.o: %.asm
	nasm -f elf -o $@ $<

%.w32.o: %.asm
	nasm -f coff -o $@ $<

%.w32.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.arm.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

src/data.o: src/data.asm $(bindata)

.PHONY: libs
libs: imago anim mikmod goat3d

.PHONY: imago
imago:
	$(MAKE) -C libs/imago

.PHONY: anim
anim:
	$(MAKE) -C libs/anim

.PHONY: mikmod
mikmod:
	$(MAKE) -C libs/mikmod

.PHONY: goat3d
goat3d:
	$(MAKE) -C libs/goat3d

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/imago clean
	$(MAKE) -C libs/anim clean
	$(MAKE) -C libs/mikmod clean
	$(MAKE) -C libs/goat3d clean

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

.PHONY: crosspi
crosspi:
	$(MAKE) CC=arm-linux-gnueabihf-gcc cpu=arm
