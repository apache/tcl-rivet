# Microsoft Developer Studio Project File - Name="rivet" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=rivet - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rivet.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rivet.mak" CFG="rivet - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rivet - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "rivet - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "rivet - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f rivet.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "rivet.exe"
# PROP BASE Bsc_Name "rivet.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "Makefile.vc" TCLDIR=c:\opt\tcl INSTALLDIR=c:\opt\tcl OPTS=none"
# PROP Rebuild_Opt "/a"
# PROP Target_File "Release\mod_rivet.so"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "rivet - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rivet___Win32_Debug"
# PROP BASE Intermediate_Dir "rivet___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f rivet.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "rivet.exe"
# PROP BASE Bsc_Name "rivet.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "rivet___Win32_Debug"
# PROP Intermediate_Dir "rivet___Win32_Debug"
# PROP Cmd_Line "nmake /f "Makefile.vc" TCLDIR=c:\opt\tcl INSTALLDIR=c:\opt\tcl OPTS=symbols"
# PROP Rebuild_Opt "/a"
# PROP Target_File "Debug\mod_rivet.so"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "rivet - Win32 Release"
# Name "rivet - Win32 Debug"

!IF  "$(CFG)" == "rivet - Win32 Release"

!ELSEIF  "$(CFG)" == "rivet - Win32 Debug"

!ENDIF 

# Begin Group "mod_rivet"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\apache_multipart_buffer.c
# End Source File
# Begin Source File

SOURCE=..\src\apache_multipart_buffer.h
# End Source File
# Begin Source File

SOURCE=..\src\apache_request.c
# End Source File
# Begin Source File

SOURCE=..\src\apache_request.h
# End Source File
# Begin Source File

SOURCE=..\src\mod_rivet.c
# End Source File
# Begin Source File

SOURCE=..\src\mod_rivet.h
# End Source File
# Begin Source File

SOURCE=..\src\rivetChannel.c
# End Source File
# Begin Source File

SOURCE=..\src\rivetChannel.h
# End Source File
# Begin Source File

SOURCE=..\src\rivetCore.c
# End Source File
# Begin Source File

SOURCE=..\src\TclWebapache.c
# End Source File
# End Group
# Begin Group "rivet"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\rivetCrypt.c
# End Source File
# Begin Source File

SOURCE=..\src\rivetList.c
# End Source File
# Begin Source File

SOURCE=..\src\rivetPkgInit.c
# End Source File
# Begin Source File

SOURCE=..\src\rivetWWW.c
# End Source File
# End Group
# Begin Group "rivetparser"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\parserPkgInit.c
# End Source File
# Begin Source File

SOURCE=..\src\rivetParser.c
# End Source File
# Begin Source File

SOURCE=..\src\rivetParser.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\rivet.h
# End Source File
# Begin Source File

SOURCE=..\src\TclWeb.c
# End Source File
# Begin Source File

SOURCE=..\src\TclWeb.h
# End Source File
# Begin Source File

SOURCE=..\src\TclWebcgi.c
# End Source File
# End Target
# End Project
