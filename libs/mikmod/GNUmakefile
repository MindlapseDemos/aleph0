include ../libs.mk

src = $(wildcard drivers/*.c) \
	  $(wildcard loaders/*.c) \
	  $(wildcard mmio/*.c) \
	  $(wildcard depackers/*.c) \
	  $(wildcard posix/*.c) \
	  $(wildcard playercode/*.c)
obj = $(src:.c=.o)
alib = libmikmod.a

def = -DHAVE_CONFIG_H -DMIKMOD_BUILD
inc = -I. -Iinclude
CFLAGS = $(arch) -g $(def) $(inc)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
