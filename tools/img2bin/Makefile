obj = img2bin.o
bin = img2bin

root = ../..

imagodir = $(root)/libs/imago
libimago = $(imagodir)/libimago.a

CFLAGS = -pedantic -Wall -g -I$(imagodir)/src
LDFLAGS = -L$(imagodir) -limago -lm

$(bin): $(obj) $(libimago)
	$(CC) -o $@ $(obj) $(LDFLAGS)

$(libimago):
	$(MAKE) -C $(imagodir)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
