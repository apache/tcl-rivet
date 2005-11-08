;; Rivet.nsi - Copyright (C) 2004 The Apache Software Foundation
;;
;; -------------------------------------------------------------------------
;;
;; This is an installer script to create a Windows installer for
;; Rivet.
;;
;; This script needs to be compiled by the NullSoft installer compiler.
;;
;; -------------------------------------------------------------------------
;;
;;   Copyright 2000-2004 The Apache Software Foundation
;;
;;   Licensed under the Apache License, Version 2.0 (the "License");
;;   you may not use this file except in compliance with the License.
;;   You may obtain a copy of the License at
;;
;;   	http://www.apache.org/licenses/LICENSE-2.0
;;
;;   Unless required by applicable law or agreed to in writing, software
;;   distributed under the License is distributed on an "AS IS" BASIS,
;;   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;;   See the License for the specific language governing permissions and
;;   limitations under the License.
;;
;; -------------------------------------------------------------------------
;; $Id$

!include "MUI.nsh"

Name "Rivet"
OutFile "Rivet070.exe"
SetCompressor lzma
CRCCheck on
ShowInstDetails hide
XPStyle on

Var APACHE_DIR
InstallDir "$PROGRAMFILES\Tcl"

;; -------------------------------------------------------------------------
;; Version resource
;;
ViProductVersion "0.7.0.0"
VIAddVersionKey ProductName "Rivet"
VIAddVersionKey CompanyName "Apache Software Foundation"
VIAddVersionKey FileVersion "0.7.0.0"
VIAddVersionKey ProductVersion "0.7.0.0"
VIAddVersionKey LegalCopyright "Copyright (c) 2004 The Apache Software Foundation"
VIAddVersionKey FileDescription "Apache Rivet Installer"

;; -------------------------------------------------------------------------
;; Language strings
;;
LangString DESC_SecRivet ${LANG_ENGLISH} \
    "Install the Apache Rivet loadable module and Tcl libraries."
LangString DESC_SecDocs ${LANG_ENGLISH} \
    "Install the Apache Rivet documentation -- a CHM help file."
LangString DESC_SecConfig ${LANG_ENGLISH} \
    "Append a sample config section to your httpd.config file to \
    configure Apache 1.3 to use Rivet."
LangString DESC_SecTests ${LANG_ENGLISH} \
    "Install the Apache Rivet test files. These can be used to check your \
     installation."

LangString DESC_ApachePageTextFrame ${LANG_ENGLISH} \
    "Select the Apache installation 1.3 folder."
LangString DESC_ApachePageText ${LANG_ENGLISH} \
    "The Rivet loadable module needs to be installed in your Apache 1.3 \
     modules folder so that Apache can find it.$\r$\n\
     You must select the Apache folder here - it is the folder that \
     contains Apache.exe"
LangString DESC_TclPageTextFrame ${LANG_ENGLISH} \
    "Select the Tcl installation folder."
LangString DESC_TclPageText ${LANG_ENGLISH} \
    "The Rivet Tcl packages need to be installed is a Tcl library folder. \
    $\r$\nThis is usually $\"\Program Files\Tcl\lib$\""
LangString DESC_ApacheDirNotFound ${LANG_ENGLISH} \
    "You must select the folder containing the Apache 1.3 executable program \
    (Apache.exe)!"
LangString DESC_FinishPageText ${LANG_ENGLISH} \
    "You should now edit your Apache httpd.conf file make use of Rivet."

;; -------------------------------------------------------------------------
;; Interface Settings

!define MUI_ABORTWARNING

;; Ensure the APACHE_DIR variable really points to an Apache installation.
Function ApacheDirValidate
    IfFileExists "$APACHE_DIR\Apache.exe" ApacheTrue ApacheFalse
  ApacheFalse:
    MessageBox MB_OK $(DESC_ApacheDirNotFound) /SD IDOK
    Abort
  ApacheTrue:
FunctionEnd

;; Ensure the INSTDIR points to the library directory of a Tcl installation
;; unless the use has gone for a site library dir
Function TclDirValidate
    IfFileExists "$INSTDIR\bin\tclsh.exe" TclTrue TclFalse
  TclTrue:
    StrCpy $INSTDIR "$INSTDIR\lib"
    DetailPrint "Adjusting INSTDIR to point at the library folder."
  TclFalse:
FunctionEnd

;; -------------------------------------------------------------------------
;; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_DIRECTORYPAGE_TEXT_TOP $(DESC_TclPageText)
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION $(DESC_TclPageTextFrame)
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE TclDirValidate
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_DIRECTORYPAGE_VARIABLE $APACHE_DIR
!define MUI_DIRECTORYPAGE_TEXT_TOP $(DESC_ApachePageText)
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION $(DESC_ApachePageTextFrame)
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE ApacheDirValidate
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_FINISHPAGE_NOAUTOCLOSE "True"
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT $(DESC_FinishPageText)
!insertmacro MUI_PAGE_FINISH
  
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!define MUI_UNFINISHPAGE_NOAUTOCLOSE "True"
!insertmacro MUI_UNPAGE_FINISH

;; -------------------------------------------------------------------------
;; Installer Sections
;;
Section "Rivet" SecRivet
    SectionIn RO
    IfFileExists "$INSTDIR\bin\tclsh.exe" TclTrue TclFalse
  TclTrue:
    StrCpy $INSTDIR "$INSTDIR\lib"
    DetailPrint "Adjusting INSTDIR to point at the library folder."
  TclFalse:

    SetOutPath "$INSTDIR\rivet0.7.0"
    File /r "..\rivet\rivet-tcl"
    File /r "..\rivet\packages"
    File "..\rivet\init.tcl"
    File "..\rivet\pkgIndex.tcl"
    File "..\rivet\README"
    RMDir /r "$INSTDIR\rivet0.7.0\packages\CVS"
    RMDir /r "$INSTDIR\rivet0.7.0\rivet-tcl\CVS"
    SetOutPath "$INSTDIR\rivet0.7.0\packages\rivet"
    File "Release\rivet.dll"
    File "Release\rivetparser.dll"
    FileOpen $R6 "pkgIndex.tcl" "w"
    FileWrite $R6 "package ifneeded Rivet 1.1 [list load [file join $$dir rivet.dll]]$\r$\n"
    FileWrite $R6 "package ifneeded rivetparser 0.2 [list load [file join $$dir rivetparser.dll]]$\r$\n"
    FileClose $R6
    SetOutPath "$APACHE_DIR\modules"
    File "Release\mod_rivet.so"
    WriteUninstaller "$INSTDIR\rivet0.7.0\Uninstall.exe"
SectionEnd

Section "Documentation" SecDocs
    SetOutPath "$INSTDIR\rivet0.7.0"
    File "..\doc\Rivet.chm"
SectionEnd

Section /o "Modify httpd.conf" SecConfig
    IfFileExists "$APACHE_DIR\conf\httpd.conf" ConfigAppend ConfigSkip
  ConfigAppend:
    FileOpen $R6 "$APACHE_DIR\conf\httpd.conf" "a"
    FileSeek $R6 0 END
    FileWrite $R6 "$\r$\nLoadModule rivet_module modules/mod_rivet.so$\r$\n$\r$\n"
    FileWrite $R6 "<IfModule mod_rivet.c>$\r$\n"
    FileWrite $R6 "    RivetServerConf CacheSize 50$\r$\n"
    FileWrite $R6 "    AddType application/x-httpd-rivet .rvt$\r$\n"
    FileWrite $R6 "    AddType application/x-rivet-tcl .tcl$\r$\n$\r$\n"
    FileWrite $R6 "    Alias /rivet/ $\"$INSTDIR/rivet0.7.0/tests/$\"$\r$\n"
    FileWrite $R6 "    <Directory $\"$INSTDIR/rivet0.7.0/tests/$\">$\r$\n"
    FileWrite $R6 "        Options Indexes FollowSymlinks MultiViews$\r$\n"
    FileWrite $R6 "        AllowOverride None$\r$\n"
    FileWrite $R6 "        Order allow,deny$\r$\n"
    FileWrite $R6 "        Allow from all$\r$\n"
    FileWrite $R6 "    </Directory>$\r$\n"
    FileWrite $R6 "</IfModule>$\r$\n"
    FileClose $R6
    DetailPrint "Appended section to $\"$APACHE_DIR\conf\httpd.conf$\"."
    Goto ConfigEnd
  ConfigSkip:
    DetailPrint "$\r$\nFailed to find file $\"$APACHE_DIR\conf\httpd.conf$\".$\r$\n\
    Skipping httpd.conf edit."
  ConfigEnd:
SectionEnd

Section /o "Test files" SecTests
    SetOutPath "$INSTDIR\rivet0.7.0\tests"
    File "..\tests\*.*"
SectionEnd

;; -------------------------------------------------------------------------
;; Uninstaller Section
;;
Section "Uninstall"
    RMDir /r "$INSTDIR"
    Delete "$APACHE_DIR\modules\mod_rivet.so"
SectionEnd
 
;; -------------------------------------------------------------------------
;; Assign language strings to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecRivet}  $(DESC_SecRivet)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs}   $(DESC_SecDocs)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecConfig} $(DESC_SecConfig)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecTests}  $(DESC_SecTests)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!insertmacro MUI_LANGUAGE "English"

;; -------------------------------------------------------------------------
;; Initialize variables
;;
Function .onInit
    StrCpy $APACHE_DIR "$PROGRAMFILES\Apache"
FunctionEnd

