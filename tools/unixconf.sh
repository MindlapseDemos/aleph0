#!/bin/sh

cfgfile=ucfg.mk
sys=`uname -s | sed 's/IRIX.*/IRIX/; s/MINGW.*/mingw/'`

if [ "$sys" = IRIX ]; then
	echo "Generating IRIX makefile config: $cfgfile"
	echo 'CFLAGS_sys = -n32 -mips3 -DBUILD_BIGENDIAN -DNO_GLTEX' >$cfgfile
	echo 'LDFLAGS_sys = -n32 -mips3' >>$cfgfile
	echo 'LIBS_sys = -lX11 -lGL -laudio -lpthread' >>$cfgfile
	echo 'OPT_sys = -O3' >>$cfgfile

elif [ "$sys" = SunOS ]; then
	echo "Generating SunOS makefile config: $cfgfile"
	echo 'CFLAGS_sys = -DBUILD_BIGENDIAN -DNO_GLTEX' >$cfgfile
	echo 'LDFLAGS_sys = ' >>$cfgfile
	echo 'LIBS_sys = -lX11 -lGL -lpthread' >>$cfgfile
	echo 'OPT_sys = -xO5' >>$cfgfile

elif [ "$sys" = Linux ]; then
	echo "Generating GNU/Linux makefile config: $cfgfile"
	echo 'LIBS_sys = -lX11 -lGL -lasound -lpthread' >$cfgfile
	echo 'OPT_sys = -O3' >>$cfgfile

elif [ "$sys" = FreeBSD ]; then
	echo "Generating FreeBSD makefile config: $cfgfile"
	echo 'CFLAGS_sys = -I/usr/local/include' >$cfgfile
	echo 'LDFLAGS_sys = -L/usr/local/lib' >>$cfgfile
	echo 'LIBS_sys = -lX11 -lGL -lpthread' >>$cfgfile
	echo 'OPT_sys = -O3' >>$cfgfile
fi
