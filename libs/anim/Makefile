src = $(wildcard src/*.c)
obj = $(src:.c=.odj)
alib = anim.dja

ifeq ($(findstring COMMAND.COM, $(SHELL)), COMMAND.COM)
	hostsys = dos
else
	hostsys = unix
	TOOLPREFIX = i586-pc-msdosdjgpp-
endif

CC = $(TOOLPREFIX)gcc
AR = $(TOOLPREFIX)ar
CFLAGS = -Wno-main -march=pentium -g -O3 -ffast-math -I.. -I../../src

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

%.odj: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
ifeq ($(hostsys), dos)
clean:
	del src\*.odj
	del $(alib)
else
clean:
	rm -f $(obj) $(alib)
endif
