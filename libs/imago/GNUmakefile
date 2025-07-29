include ../libs.mk

src = $(wildcard src/*.c) \
	  $(wildcard zlib/*.c) \
	  $(wildcard libpng/*.c) \
	  $(wildcard jpeglib/*.c)
obj = $(src:.c=.o)
alib = libimago.a

ifeq ($(sys), android-arm64)
	obj = $(src:.c=.arm64.o)
	alib = ../android64/libimago.a
	pic = -fPIC
endif
ifeq ($(sys), android-arm32)
	obj = $(src:.c=.arm32.o)
	alib = ../android32/libimago.a
	pic = -fPIC
endif

CFLAGS = $(arch) -Wno-main -g -O3 -Izlib -Ilibpng -Ijpeglib $(pic)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.arm64.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.arm32.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
