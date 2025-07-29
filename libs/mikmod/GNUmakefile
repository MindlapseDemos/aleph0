include ../libs.mk

src = $(wildcard drivers/*.c) \
	  $(wildcard loaders/*.c) \
	  $(wildcard mmio/*.c) \
	  $(wildcard depackers/*.c) \
	  $(wildcard posix/*.c) \
	  $(wildcard playercode/*.c)
obj = $(src:.c=.o)
alib = libmikmod.a

ifeq ($(sys), android-arm64)
	obj = $(src:.c=.arm64.o)
	alib = ../android64/libmikmod.a
	pic = -fPIC
endif
ifeq ($(sys), android-arm32)
	obj = $(src:.c=.arm32.o)
	alib = ../android32/libmikmod.a
	pic = -fPIC
endif

def = -DHAVE_CONFIG_H -DMIKMOD_BUILD
inc = -I. -Iinclude
CFLAGS = $(arch) -g $(def) $(inc) $(pic)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.arm64.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.arm32.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<


.PHONY: clean
clean:
	rm -f $(obj) $(alib)
