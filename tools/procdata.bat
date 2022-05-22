@if exist procdata.bat cd ..

@if not exist tools\img2bin\img2bin.exe (
	echo img2bin not found. Please build img2bin before running procdata.
	pause
	exit 1
)
@if not exist tools\csprite\csprite.exe (
	echo csprite not found. Please build csprite before running procdata.
	pause
	exit 1
)

@mkdir data

tools\img2bin\img2bin data\loading.png
@if errorlevel 1 pause
	

tools\csprite\csprite -n cs_dbgfont -s 8x16 -conv565 -nasm -xor data/legible.fnt >cspr/dbgfont.asm
@if errorlevel 1 pause
tools\csprite\csprite -n cs_confont -s 6x7 -pad 1 -conv565 -nasm data/console.fnt >cspr/confont.asm
@if errorlevel 1 pause
