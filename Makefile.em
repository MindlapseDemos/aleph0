src = $(wildcard src/*.c) $(wildcard src/3dgfx/*.c) $(wildcard src/rt/*.c) \
	  $(wildcard src/scr/*.c) $(wildcard src/sdl/*.c) src/glut/audio.c
obj = $(src:.c=.emo)
bin = demo.html

inc = -Isrc -Isrc/3dgfx -Isrc/rt -Isrc/scr -Isrc/utils -Isrc/sdl -Ilibs \
	  -Ilibs/imago/src -Ilibs/mikmod/include -Ilibs/goat3d/include
def = -DMIKMOD_STATIC
warn = -pedantic -Wall -Wno-unused-variable -Wno-unused-function -Wno-address
opt = -O3 -ffast-math
dbg = -g3

CC = emcc
CFLAGS = $(warn) $(opt) -fno-pie -fno-strict-aliasing $(dbg) $(inc) -sUSE_SDL
LDFLAGS = libs/imago/imago.ema libs/anim/anim.ema libs/goat3d/goat3d.ema \
		  libs/mikmod/mikmod.ema -lSDL --preload-file data --exclude-file data/.svn \
		  -sINITIAL_MEMORY=67108864 -sUSE_SDL --shell-file demopage_shell.html
#		  -gsource-maps --profile-funcs -sSAFE_HEAP=1

$(bin): $(obj) imago anim goat3d mikmod demopage_shell.html
	$(CC) -o $@ $(obj) $(LDFLAGS)

demopage_shell.html: tools/demotmpl.htm
	tools/fixhtml >$@

%.emo: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: libs
libs: imago anim mikmod goat3d

.PHONY: imago
imago:
	$(MAKE) -C libs/imago -f Makefile.em

.PHONY: anim
anim:
	$(MAKE) -C libs/anim -f Makefile.em

.PHONY: mikmod
mikmod:
	$(MAKE) -C libs/mikmod -f Makefile.em

.PHONY: goat3d
goat3d:
	$(MAKE) -C libs/goat3d -f Makefile.em

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/imago clean -f Makefile.em
	$(MAKE) -C libs/anim clean -f Makefile.em
	$(MAKE) -C libs/mikmod clean -f Makefile.em
	$(MAKE) -C libs/goat3d clean -f Makefile.em

.PHONY: clean
clean:
	rm -f $(obj) $(bin) demo.data demo.js demo.wasm

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: install
install: $(bin)
	rsync -vz -e ssh $(bin) goat.mutantstargoat.com:public_html/dosdemo/index.html
	rsync -vz -e ssh demo.data demo.js demo.wasm goat.mutantstargoat.com:public_html/dosdemo
