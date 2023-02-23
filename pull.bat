mkdir src
rsync -v 192.168.0.4::dosdemo/Makefile Makefile
rsync -v 192.168.0.4::dosdemo/src/*.c src
rsync -v 192.168.0.4::dosdemo/src/*.h src
rsync -v 192.168.0.4::dosdemo/src/*.asm src
mkdir src\dos
rsync -v 192.168.0.4::dosdemo/src/dos/*.c src/dos
rsync -v 192.168.0.4::dosdemo/src/dos/*.h src/dos
rsync -v 192.168.0.4::dosdemo/src/dos/*.asm src/dos
mkdir src\glut
rsync -v 192.168.0.4::dosdemo/src/glut/*.c src/glut
rsync -v 192.168.0.4::dosdemo/src/glut/*.h src/glut
mkdir src\sdl
rsync -v 192.168.0.4::dosdemo/src/sdl/*.c src/sdl
mkdir src\scr
rsync -v 192.168.0.4::dosdemo/src/scr/*.c src/scr
rsync -v 192.168.0.4::dosdemo/src/scr/*.h src/scr
mkdir src\3dgfx
rsync -v 192.168.0.4::dosdemo/src/3dgfx/*.c src/3dgfx
rsync -v 192.168.0.4::dosdemo/src/3dgfx/*.h src/3dgfx
mkdir src\rt
rsync -v 192.168.0.4::dosdemo/src/rt/*.c src/rt
rsync -v 192.168.0.4::dosdemo/src/rt/*.h src/rt

mkdir libs\cgmath
rsync -v 192.168.0.4::dosdemo/libs/cgmath/*.h libs/cgmath
rsync -v 192.168.0.4::dosdemo/libs/cgmath/*.inl libs/cgmath

mkdir tools
mkdir tools\csprite
rsync -v 192.168.0.4::dosdemo/tools/csprite/Makefile tools/csprite/Makefile
rsync -v 192.168.0.4::dosdemo/tools/csprite/src/*.c tools/csprite/src
rsync -v 192.168.0.4::dosdemo/tools/csprite/src/*.h tools/csprite/src

mkdir tools\img2bin
rsync -v 192.168.0.4::dosdemo/tools/img2bin/Makefile tools/img2bin/Makefile
rsync -v 192.168.0.4::dosdemo/tools/img2bin/*.c tools/img2bin
