src = $(wildcard src/*.c) \
	  $(wildcard zlib/*.c) \
	  $(wildcard libpng/*.c) \
	  $(wildcard jpeglib/*.c)
obj = $(src:.c=.emo)
alib = imago.ema

CC = emcc
AR = emar
CFLAGS = -fno-pie -Wno-main -g3 -O3 -Izlib -Ilibpng -Ijpeglib

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.emo: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
