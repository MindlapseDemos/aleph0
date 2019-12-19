src = $(wildcard src/*.c)
obj = $(src:.c=.o)
alib = libanim.a

CFLAGS = -m32 -Wno-main -g -I.. -I../../src

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
