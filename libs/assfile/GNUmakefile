include ../libs.mk

src = $(wildcard *.c)
obj = $(src:.c=.o)
alib = libassfile.a

CFLAGS = $(arch)

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
