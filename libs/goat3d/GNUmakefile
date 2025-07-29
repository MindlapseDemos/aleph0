include ../libs.mk

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
alib = libgoat3d.a

ifeq ($(sys), android-arm64)
	obj = $(src:.c=.arm64.o)
	alib = ../android64/libgoat3d.a
	pic = -fPIC
endif
ifeq ($(sys), android-arm32)
	obj = $(src:.c=.arm32.o)
	alib = ../android32/libgoat3d.a
	pic = -fPIC
endif

CFLAGS = $(arch) -g -O3 -Iinclude -I.. -I../../src $(pic)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.arm64.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.arm32.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<


.PHONY: clean
clean:
	rm -f $(obj) $(alib)
