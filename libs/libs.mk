cpu ?= $(shell uname -m | sed 's/i.86/i386/')
sys ?= $(shell uname -s | sed 's/MINGW.*/mingw/; s/IRIX.*/IRIX/')

ifneq ($(sys), Darwin)
	ifeq ($(cpu), x86_64)
		arch = -m32
	endif
endif
