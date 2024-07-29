#!/bin/sh

cfgfile=ucfg.mk
sys=`uname -s | sed 's/IRIX.*/IRIX/; s/MINGW.*/mingw/'`

if [ "$sys" = IRIX ]; then
	echo "Generating IRIX makefile config: $cfgfile"
	echo 'CFLAGS_sys = -n32 -mips3 -DBUILD_BIGENDIAN -I/usr/nekoware/include -I/usr/tgcware/include' >$cfgfile
	echo 'LDFLAGS_sys = -n32 -mips3 -L/usr/nekoware/lib -L/usr/tgcware/lib' >>$cfgfile
	echo 'LIBS_sys = -lpthread' >>$cfgfile

else
	echo >$cfgfile
fi
