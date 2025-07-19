#!/bin/sh

cfgfile=ucfg.mk
sys=`uname -s | sed 's/IRIX.*/IRIX/; s/MINGW.*/mingw/'`

if [ "$sys" = IRIX ]; then
	echo "Generating IRIX makefile config: $cfgfile"
	echo 'CFLAGS_sys = -n32 -mips3 -DBUILD_BIGENDIAN -DNO_GLTEX' >$cfgfile
	echo 'LDFLAGS_sys = -n32 -mips3' >>$cfgfile
	echo 'LIBS_sys = -lX11 -lGL -laudio -lpthread' >>$cfgfile

elif [ "$sys" = Linux ]; then
	echo "Generating GNU/Linux makefile config: $cfgfile"
	echo 'LIBS_sys = -lX11 -lGL -lasound -lpthread' >$cfgfile
fi
