src = $(wildcard *.c) $(wildcard imago/*.c)
obj = $(src:.c=.o)
bin = img2bin

CFLAGS = -Iimago
LDFLAGS = -lpng -lz -ljpeg

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
