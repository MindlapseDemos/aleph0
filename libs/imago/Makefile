libpng = png.obj pngerror.obj pngget.obj pngmem.obj pngpread.obj pngread.obj &
pngrio.obj pngrtran.obj pngrutil.obj pngset.obj pngtrans.obj pngwio.obj &
pngwrite.obj pngwtran.obj pngwutil.obj
zlib = adler32.obj compress.obj crc32.obj deflate.obj gzio.obj infback.obj &
inffast.obj inflate.obj inftrees.obj trees.obj uncompr.obj zutil.obj
jpeglib = jcapimin.obj jcapistd.obj jccoefct.obj jccolor.obj jcdctmgr.obj &
jchuff.obj jcinit.obj jcmainct.obj jcmarker.obj jcmaster.obj jcomapi.obj &
jcparam.obj jcphuff.obj jcprepct.obj jcsample.obj jctrans.obj jdapimin.obj &
jdapistd.obj jdatadst.obj jdatasrc.obj jdcoefct.obj jdcolor.obj jddctmgr.obj &
jdhuff.obj jdinput.obj jdmainct.obj jdmarker.obj jdmaster.obj jdmerge.obj &
jdphuff.obj jdpostct.obj jdsample.obj jdtrans.obj jerror.obj jfdctflt.obj &
jfdctfst.obj jfdctint.obj jidctflt.obj jidctfst.obj jidctint.obj jidctred.obj &
jmemmgr.obj jmemnobs.obj jquant1.obj jquant2.obj jutils.obj

obj = conv.obj filejpeg.obj filepng.obj fileppm.obj filergbe.obj &
filetga.obj ftmodule.obj imago2.obj imago_gl.obj modules.obj &
$(libpng) $(zlib) $(jpeglib)

alib = imago.lib

opt = -5 -fp5 -otexan
dbg = -d1
def = -DPNG_NO_SNPRINTF

CC = wcc386
CFLAGS = $(dbg) $(opt) $(def) -zq -bt=dos -Ilibpng -Izlib -Ijpeglib

$(alib): $(obj)
	%write objects.lbc $(obj)
	wlib -b -n $@ @objects

.c: src;libpng;jpeglib;zlib

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*

clean: .symbolic
	del *.obj
	del $(alib)