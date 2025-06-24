# Microsoft Developer Studio Project File - Name="goat3d" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=goat3d - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "goat3d.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "goat3d.mak" CFG="goat3d - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "goat3d - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "goat3d - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "goat3d - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /I ".." /I "..\..\src" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "goat3d - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "include" /I ".." /I "..\..\src" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "goat3d - Win32 Release"
# Name "goat3d - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;h"
# Begin Source File

SOURCE=.\src\aabox.c
# End Source File
# Begin Source File

SOURCE=.\src\aabox.h
# End Source File
# Begin Source File

SOURCE=.\src\chunk.c
# End Source File
# Begin Source File

SOURCE=.\src\chunk.h
# End Source File
# Begin Source File

SOURCE=.\src\dynarr.c
# End Source File
# Begin Source File

SOURCE=.\src\dynarr.h
# End Source File
# Begin Source File

SOURCE=.\src\extmesh.c
# End Source File
# Begin Source File

SOURCE=.\src\g3danm.c
# End Source File
# Begin Source File

SOURCE=.\src\g3danm.h
# End Source File
# Begin Source File

SOURCE=.\src\g3dscn.c
# End Source File
# Begin Source File

SOURCE=.\src\g3dscn.h
# End Source File
# Begin Source File

SOURCE=.\src\goat3d.c
# End Source File
# Begin Source File

SOURCE=.\include\goat3d.h
# End Source File
# Begin Source File

SOURCE=.\src\json.c
# End Source File
# Begin Source File

SOURCE=.\src\json.h
# End Source File
# Begin Source File

SOURCE=.\src\log.c
# End Source File
# Begin Source File

SOURCE=.\src\log.h
# End Source File
# Begin Source File

SOURCE=.\src\read.c
# End Source File
# Begin Source File

SOURCE=.\src\readgltf.c
# End Source File
# Begin Source File

SOURCE=.\src\track.c
# End Source File
# Begin Source File

SOURCE=.\src\track.h
# End Source File
# Begin Source File

SOURCE=.\src\util.c
# End Source File
# Begin Source File

SOURCE=.\src\util.h
# End Source File
# Begin Source File

SOURCE=.\src\write.c
# End Source File
# End Group
# End Target
# End Project
