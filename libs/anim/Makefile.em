src = $(wildcard src/*.c)
obj = $(src:.c=.emo)
alib = anim.ema

CC = emcc
AR = emar
CFLAGS = -Wno-main -g3 -I.. -I../../src

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.emo: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
