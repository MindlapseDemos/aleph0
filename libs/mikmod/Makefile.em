src = $(wildcard drivers/*.c) \
	  $(wildcard loaders/*.c) \
	  $(wildcard mmio/*.c) \
	  $(wildcard depackers/*.c) \
	  $(wildcard posix/*.c) \
	  $(wildcard playercode/*.c)
obj = $(src:.c=.emo)
alib = mikmod.ema

def = -DHAVE_CONFIG_H -DMIKMOD_BUILD
inc = -I. -Iinclude
warn = -pedantic -Wall -Wno-unused-variable -Wno-unused-function

CC = emcc
AR = emar
CFLAGS = $(warn) -g $(def) $(inc)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.emo: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
