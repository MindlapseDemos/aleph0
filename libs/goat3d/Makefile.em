src = $(wildcard src/*.c)
obj = $(src:.c=.emo)
alib = goat3d.ema

CC = emcc
AR = emar
CFLAGS = -fno-pie -g -O3 -Iinclude -I.. -I../../src

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.emo: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
