# Microsoft Developer Studio Project File - Name="dosdemo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=dosdemo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dosdemo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dosdemo.mak" CFG="dosdemo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dosdemo - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "dosdemo - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dosdemo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "src" /I "src/3dgfx" /I "src/sdl" /I "libs" /I "libs/imago/src" /I "libs/goat3d/include" /I "libs/anim/src" /I "libs/mikmod/include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "dosdemo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "src" /I "src/3dgfx" /I "src/sdl" /I "libs" /I "libs/imago/src" /I "libs/goat3d/include" /I "libs/anim/src" /I "libs/mikmod/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "dosdemo - Win32 Release"
# Name "dosdemo - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;h"
# Begin Group "scr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\scr\3dzoom.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\blobgrid.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\bump.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\credits.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\grise.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\hairball.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\hexfloor.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\infcubes.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\juliatun.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\metaball.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\minifx.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\molten.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\opt_3d.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\opt_3d.h
# End Source File
# Begin Source File

SOURCE=.\src\scr\opt_rend.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\opt_rend.h
# End Source File
# Begin Source File

SOURCE=.\src\scr\polka.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\smoketxt.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\smoketxt.h
# End Source File
# Begin Source File

SOURCE=.\src\scr\thunder.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\thunder.h
# End Source File
# Begin Source File

SOURCE=.\src\scr\tunnel.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\voxscape.c
# End Source File
# Begin Source File

SOURCE=.\src\scr\water.c
# End Source File
# End Group
# Begin Group "3dgfx"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\3dgfx\3dgfx.c
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\3dgfx.h
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\mesh.c
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\mesh.h
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\meshload.c
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\polyclip.c
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\polyclip.h
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\polyfill.c
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\polyfill.h
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\polytmpl.h
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\scene.c
# End Source File
# Begin Source File

SOURCE=.\src\3dgfx\scene.h
# End Source File
# End Group
# Begin Group "sdl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sdl\audio.c
# End Source File
# Begin Source File

SOURCE=.\src\sdl\gfx.h
# End Source File
# Begin Source File

SOURCE=.\src\sdl\main.c
# End Source File
# Begin Source File

SOURCE=.\src\sdl\sball.c
# End Source File
# Begin Source File

SOURCE=.\src\sdl\timer.c
# End Source File
# End Group
# Begin Group "cspr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cspr\confont.asm

!IF  "$(CFG)" == "dosdemo - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\cspr\confont.asm

"$(OutDir)\confont.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\confont.obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dosdemo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\cspr\confont.asm

"$(OutDir)\confont.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\confont.obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cspr\dbgfont.asm

!IF  "$(CFG)" == "dosdemo - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\cspr\dbgfont.asm

"$(OutDir)\dbgfont.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\dbgfont.obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dosdemo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\cspr\dbgfont.asm

"$(OutDir)\dbgfont.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\dbgfont.obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\audio.h
# End Source File
# Begin Source File

SOURCE=.\src\bsptree.c
# End Source File
# Begin Source File

SOURCE=.\src\bsptree.h
# End Source File
# Begin Source File

SOURCE=.\src\cfgopt.c
# End Source File
# Begin Source File

SOURCE=.\src\cfgopt.h
# End Source File
# Begin Source File

SOURCE=.\src\console.c
# End Source File
# Begin Source File

SOURCE=.\src\console.h
# End Source File
# Begin Source File

SOURCE=.\src\cpuid.c
# End Source File
# Begin Source File

SOURCE=.\src\cpuid.h
# End Source File
# Begin Source File

SOURCE=.\src\cpuid_s.asm

!IF  "$(CFG)" == "dosdemo - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\cpuid_s.asm

"$(OutDir)\cpuid_s.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\cpuid_s.obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dosdemo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\src\cpuid_s.asm

"$(OutDir)\cpuid_s.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\cpuid_s.obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\curve.c
# End Source File
# Begin Source File

SOURCE=.\src\curve.h
# End Source File
# Begin Source File

SOURCE=.\src\darray.c
# End Source File
# Begin Source File

SOURCE=.\src\darray.h
# End Source File
# Begin Source File

SOURCE=.\src\data.asm

!IF  "$(CFG)" == "dosdemo - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\data.asm

"$(OutDir)\data.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\data.obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dosdemo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\src\data.asm

"$(OutDir)\data.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\data.obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\demo.c
# End Source File
# Begin Source File

SOURCE=.\src\demo.h
# End Source File
# Begin Source File

SOURCE=.\src\dseq.c
# End Source File
# Begin Source File

SOURCE=.\src\dseq.h
# End Source File
# Begin Source File

SOURCE=.\src\dynarr.c
# End Source File
# Begin Source File

SOURCE=.\src\dynarr.h
# End Source File
# Begin Source File

SOURCE=.\src\font.c
# End Source File
# Begin Source File

SOURCE=.\src\font.h
# End Source File
# Begin Source File

SOURCE=.\src\gfxutil.c
# End Source File
# Begin Source File

SOURCE=.\src\gfxutil.h
# End Source File
# Begin Source File

SOURCE=.\src\image.c
# End Source File
# Begin Source File

SOURCE=.\src\image.h
# End Source File
# Begin Source File

SOURCE=.\src\mcubes.h
# End Source File
# Begin Source File

SOURCE=.\src\msurf2.c
# End Source File
# Begin Source File

SOURCE=.\src\msurf2.h
# End Source File
# Begin Source File

SOURCE=.\src\noise.c
# End Source File
# Begin Source File

SOURCE=.\src\noise.h
# End Source File
# Begin Source File

SOURCE=.\src\rbtree.c
# End Source File
# Begin Source File

SOURCE=.\src\rbtree.h
# End Source File
# Begin Source File

SOURCE=.\src\rlebmap.c
# End Source File
# Begin Source File

SOURCE=.\src\rlebmap.h
# End Source File
# Begin Source File

SOURCE=.\src\sball.h
# End Source File
# Begin Source File

SOURCE=.\src\screen.c
# End Source File
# Begin Source File

SOURCE=.\src\screen.h
# End Source File
# Begin Source File

SOURCE=.\src\timer.h
# End Source File
# Begin Source File

SOURCE=.\src\tinyfps.c
# End Source File
# Begin Source File

SOURCE=.\src\tinyfps.h
# End Source File
# Begin Source File

SOURCE=.\src\treestor.c
# End Source File
# Begin Source File

SOURCE=.\src\treestor.h
# End Source File
# Begin Source File

SOURCE=.\src\ts_text.c
# End Source File
# Begin Source File

SOURCE=.\src\util.c
# End Source File
# Begin Source File

SOURCE=.\src\util.h
# End Source File
# Begin Source File

SOURCE=.\src\util_s.asm

!IF  "$(CFG)" == "dosdemo - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\util_s.asm

"$(OutDir)\util_s.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\util_s.obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dosdemo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=.\src\util_s.asm

"$(OutDir)\util_s.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -o $(OutDir)\util_s.obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\vmath.h
# End Source File
# End Group
# End Target
# End Project
