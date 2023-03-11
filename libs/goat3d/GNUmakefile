src = $(wildcard src/*.c)
obj = $(src:.c=.o)
alib = libgoat3d.a

CFLAGS = $(arch) -fno-pie -g -O3 -Iinclude -I.. -I../../src

ifeq ($(shell uname -m), x86_64)
	arch = -m32
endif

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
