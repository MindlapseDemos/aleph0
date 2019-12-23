obj = img2bin.o
bin = img2bin

root = ../..

CFLAGS = -pedantic -Wall -g -I$(root)/libs/imago/src
LDFLAGS = -L$(root)/libs/imago -limago -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
