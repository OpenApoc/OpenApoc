;NSIS OpenApoc Windows Installer

;--------------------------------
;Special

	;Enable support for Unicode
	Unicode true
	
	;Compress the hell out of it
	SetCompressor /SOLID lzma

;--------------------------------
;Includes

	!include "MUI2.nsh"
	!include "nsDialogs.nsh"
	!include "x64.nsh"
	!include "Sections.nsh"
	!include "LangFile.nsh"
	!include "StrFunc.nsh"

;--------------------------------
;Defines

	!define GAME_NAME "OpenApoc"
	!define GAME_AUTHOR "OpenApoc Contributors"

;--------------------------------
;General

	;Get major and minor version numbers from the git tag
	!searchparse /ignorecase /noerrors ${GAME_VERSION} `v` GAME_VERSION_MAJOR `.` GAME_VERSION_MINOR `-` GAME_VERSION_COMMIT `-`
	!ifndef GAME_VERSION_MAJOR
		!define GAME_VERSION_MAJOR 0
	!endif
	!ifndef GAME_VERSION_MINOR
		!define GAME_VERSION_MINOR 0
	!endif
	!ifndef GAME_VERSION_COMMIT
		!define GAME_VERSION_COMMIT 0
	!endif

	;Name and file
	Name "${GAME_NAME} ${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_COMMIT}"
	OutFile "install-openapoc-${GAME_VERSION}.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\${GAME_NAME}"

	;Get installation folder from registry if available
	InstallDirRegKey HKLM "Software\${GAME_NAME}" ""

	;Request application privileges for Windows Vista
	RequestExecutionLevel admin

;--------------------------------
;Variables

	Var XDialog
	Var XLabelHeader
	;Var XLabelDirUFO
	Var XDirApocalypse
	Var XBrowseApocalypse
	
	Var ZDialog
	Var ZLabelPortable
	Var ZCheckPortable
	
	Var StartMenuFolder
	Var APOCALYPSE_CD
	Var PortableMode

;--------------------------------
;Interface Settings

	!define MUI_HEADERIMAGE
	!define MUI_HEADERIMAGE_BITMAP logo.bmp
	!define MUI_WELCOMEFINISHPAGE_BITMAP side.bmp

;--------------------------------
;Language Selection Dialog Settings

	;Remember the installer language
	!define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
	!define MUI_LANGDLL_REGISTRY_KEY "Software\${GAME_NAME}"
	!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY

	Page custom XcomCD ValidateApocalypse

	Page custom ExtraOptions ExtraOptionsSave

	;Start Menu Folder Page Configuration
	!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
	!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${GAME_NAME}"
	!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
	
	!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
	
	!insertmacro MUI_PAGE_INSTFILES
	
	;Finish Page Configuration
	!define MUI_FINISHPAGE_RUN "$INSTDIR\OpenApoc_launcher.exe"
	!define MUI_FINISHPAGE_NOREBOOTSUPPORT
	
	!insertmacro MUI_PAGE_FINISH
	
	!insertmacro MUI_UNPAGE_COMPONENTS
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES


${StrStr}
Function ExtraOptions

	;Don't allow Portable Mode in Program Files
	${StrStr} $0 $INSTDIR $PROGRAMFILES32
	StrCmp $0 "" 0 portable_no
	${StrStr} $0 $INSTDIR $PROGRAMFILES64
	StrCmp $0 "" portable_yes portable_no
	
	portable_no:
	StrCpy $PortableMode ${BST_UNCHECKED}
	Abort
	
	portable_yes:
	!insertmacro MUI_HEADER_TEXT $(SETUP_EXTRA_OPTIONS_TITLE) $(SETUP_EXTRA_OPTIONS_SUBTITLE)
	
	nsDialogs::Create 1018
	Pop $ZDialog

	${If} $ZDialog == error
		Abort
	${EndIf}
	
	${NSD_CreateCheckBox} 0 0 100% 10u $(SETUP_PORTABLE)
	Pop $ZCheckPortable
	${NSD_SetState} $ZCheckPortable $PortableMode
	
	${NSD_CreateLabel} 0 15u 100% 20u $(SETUP_PORTABLE_DESC)
	Pop $ZLabelPortable
	
	nsDialogs::Show

FunctionEnd

Function ExtraOptionsSave

	${NSD_GetState} $ZCheckPortable $PortableMode

FunctionEnd
	
Function XcomCD

	!insertmacro MUI_HEADER_TEXT $(SETUP_APOCALYPSE_CD_TITLE) $(SETUP_APOCALYPSE_CD_SUBTITLE)
	nsDialogs::Create 1018
	Pop $XDialog

	${If} $XDialog == error
		Abort
	${EndIf}
	
	${NSD_CreateLabel} 0 0 100% 50% $(SETUP_APOCALYPSE_CD)
	Pop $XLabelHeader
	
	;${NSD_CreateLabel} 0 -67u 100% 10u $(SETUP_UFO_FOLDER)
	;Pop $XLabelDirUFO
	
	${NSD_CreateFileRequest} 0 -56u 95% 12u $APOCALYPSE_CD
	Pop $XDirApocalypse
	${NSD_OnChange} $XDirApocalypse XcomCDOnChange
	
	${NSD_CreateBrowseButton} -14u -56u 14u 12u "..."
	Pop $XBrowseApocalypse
	${NSD_OnClick} $XBrowseApocalypse XcomCDOnBrowse

	nsDialogs::Show

FunctionEnd

Function XcomCDOnChange

	Pop $0
	
	${If} $0 == $XDirApocalypse
		${NSD_GetText} $0 $APOCALYPSE_CD
	${EndIf}

FunctionEnd

Function XcomCDOnBrowse

	Pop $0
	
	${If} $0 == $XBrowseApocalypse
		nsDialogs::SelectFileDialog open $APOCALYPSE_CD "ISO images (*.iso)|*.iso|CUE images (*.cue)|*.cue|All Files|*.*"
		Pop $1
		${If} $1 == error
			Return
		${EndIf}
		StrCpy $APOCALYPSE_CD $1
		${NSD_SetText} $XDirApocalypse $APOCALYPSE_CD
	${EndIf}

FunctionEnd

;--------------------------------
;Languages

	!insertmacro MUI_LANGUAGE "English" ;first language is the default language
	!insertmacro LANGFILE_INCLUDE "English.nsh"

;--------------------------------
;Reserve Files

	;If you are using solid compression, files that are required before
	;the actual installation should be stored first in the data block,
	;because this will make your installer start faster.
	
	!insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Installer Sections

Section "$(SETUP_GAME)" SecMain

	SectionIn RO
	
	SetOutPath "$INSTDIR"

	File /r "..\..\OpenApoc-${GAME_VERSION}\*"

	
	;Generate config
	${If} $PortableMode == ${BST_CHECKED}

		CreateDirectory "$INSTDIR\saves"
		File "..\..\portable.txt"

		StrCmp $APOCALYPSE_CD "" skip_portable_cd 0
		IfFileExists "$APOCALYPSE_CD" 0 skip_portable_cd

		WriteINIStr "$INSTDIR\OpenApoc_settings.conf" Framework CD "$APOCALYPSE_CD"
		skip_portable_cd:

	${Else}

		CreateDirectory "$APPDATA\OpenApoc\OpenApoc\saves"

		StrCmp $APOCALYPSE_CD "" skip_installed_cd 0
		IfFileExists "$APOCALYPSE_CD" 0 skip_installed_cd

		WriteINIStr "$APPDATA\OpenApoc\OpenApoc\settings.conf" Framework CD "$APOCALYPSE_CD"

		skip_installed_cd:
		WriteINIStr "$APPDATA\OpenApoc\OpenApoc\settings.conf" Game.Save Directory "$APPDATA\OpenApoc\OpenApoc\saves"

	${EndIf}
	
	;Store installation folder
	WriteRegStr HKLM "Software\${GAME_NAME}" "" $INSTDIR
	
	;Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "DisplayName" "${GAME_NAME} ${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_COMMIT}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "DisplayIcon" '"$INSTDIR\OpenApoc.exe",0'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "DisplayVersion" "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_COMMIT}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "Publisher" "${GAME_AUTHOR}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "URLInfoAbout" "http://openapoc.org"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}" "NoRepair" 1
	
	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	
	;Create shortcuts
	SetOutPath "$INSTDIR"
	
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${GAME_NAME}.lnk" "$INSTDIR\OpenApoc_launcher.exe"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(SETUP_SHORTCUT_README).lnk" "$INSTDIR\README.txt"
${If} $PortableMode == ${BST_CHECKED}
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(SETUP_SHORTCUT_USER).lnk" "$INSTDIR"
${Else}
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(SETUP_SHORTCUT_USER).lnk" "$APPDATA\OpenApoc\OpenApoc"
${EndIf}
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(SETUP_SHORTCUT_UNINSTALL).lnk" "$INSTDIR\Uninstall.exe"
	
	!insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section /o "$(SETUP_DESKTOP)" SecDesktop

	SetOutPath "$INSTDIR"
	
	CreateShortCut "$DESKTOP\${GAME_NAME}.lnk" "$INSTDIR\OpenApoc_launcher.exe"

SectionEnd

;--------------------------------
;Descriptions

	;Assign language strings to sections
	!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
		!insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(SETUP_GAME_DESC)
		!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(SETUP_DESKTOP_DESC)
	!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Installer functions

Function .onInit

!ifdef NSIS_WIN32_MAKENSIS
${If} ${RunningX64}
	StrCpy $INSTDIR "$PROGRAMFILES64\${GAME_NAME}"
${Else}
	StrCpy $INSTDIR "$PROGRAMFILES32\${GAME_NAME}"
${EndIf}
!endif
	StrCpy $StartMenuFolder "${GAME_NAME}"
	StrCpy $PortableMode ${BST_UNCHECKED}
	
	; Check for existing X-COM installs
	StrCpy $APOCALYPSE_CD ""
	
	Call ScanGOG
	Call ScanSteam
	
	!insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

;--------------------------------
;Scan Steam for X-COM installations
${StrLoc}
${StrTok}
${StrRep}

Function ScanSteam

	ReadRegStr $R1 HKLM "Software\Valve\Steam" "InstallPath"
	IfErrors steam_done
	Call ScanSteamLibrary
	
	ClearErrors
	FileOpen $0 "$R1\config\config.vdf" r
	IfErrors steam_read_done
	steam_read_loop:
	FileRead $0 $1
	IfErrors steam_read_done
	${StrLoc} $2 $1 "BaseInstallFolder_" ">"
	StrCmp $2 "" steam_read_next 0
	${StrTok} $R1 $1 '"' "3" "1"
	${StrRep} $R1 $R1 "\\" "\"
	Call ScanSteamLibrary
	steam_read_next:
	Goto steam_read_loop
	steam_read_done:
	FileClose $0
	
	steam_done:

FunctionEnd

;--------------------------------
;Scan a specific Steam Library

Function ScanSteamLibrary

	StrCpy $R0 "$R1\steamapps\common\XCom Apocalypse"
	IfFileExists "$R0\cd.iso" 0 steam_apocalypse_no
	StrCpy $APOCALYPSE_CD "$R0\cd.iso"
	steam_apocalypse_no:

FunctionEnd

;--------------------------------
;Scan GOG for X-COM installations

Function ScanGOG

	ReadRegStr $R0 HKLM "Software\GOG.com\Games\1445249430" "PATH"
	IfErrors gog_apocalypse_no
	IfFileExists "$R0\CD\XCOM.cue" 0 gog_apocalypse_no
	StrCpy $APOCALYPSE_CD "$R0\CD\XCOM.cue"
	gog_apocalypse_no:

FunctionEnd

;--------------------------------
;Validate X-COM folders

Function ValidateApocalypse

	StrCmp $APOCALYPSE_CD "" validate_apocalypse_yes
	IfFileExists "$APOCALYPSE_CD" validate_apocalypse_yes 0
	MessageBox MB_ICONEXCLAMATION|MB_YESNO $(SETUP_WARNING_APOCALYPSE) /SD IDYES IDYES validate_apocalypse_yes IDNO validate_apocalypse_no
	validate_apocalypse_no:
	Abort
	validate_apocalypse_yes:

FunctionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

	!insertmacro MUI_UNGETLANGUAGE

FunctionEnd

;--------------------------------
;Uninstaller Sections

Section /o "un.$(SETUP_UNUSER)" UnUser
	IfFileExists "$INSTDIR\portable.txt" 0 uninstall_user_nonportable
	RMDir /r "$INSTDIR\saves"
	Delete "$INSTDIR\OpenApoc_settings.conf"
	Goto uninstall_user_success
	uninstall_user_nonportable:
	RMDir /r "$APPDATA\OpenApoc\OpenApoc\saves"
	Delete "$APPDATA\OpenApoc\OpenApoc\settings.conf"
	RMDir "$APPDATA\OpenApoc"
	uninstall_user_success:
SectionEnd

Section "-un.Main"

	SetOutPath "$TEMP"
	
	Delete "$INSTDIR\*.exe"
	Delete "$INSTDIR\*.dll"
	Delete "$INSTDIR\*.txt"
	Delete "$INSTDIR\build-id"
	Delete "$INSTDIR\git-commit"
	RMDir /r "$INSTDIR\data"
	RMDir /r "$INSTDIR\iconengines"
	RMDir /r "$INSTDIR\imageformats"
	RMDir /r "$INSTDIR\platforms"
	RMDir /r "$INSTDIR\styles"
	RMDir /r "$INSTDIR\translations"

	Delete "$INSTDIR\Uninstall.exe"
	RMDir "$INSTDIR"
	
	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
	
	Delete "$SMPROGRAMS\$StartMenuFolder\*.*"
	RMDir "$SMPROGRAMS\$StartMenuFolder"
	
	Delete "$DESKTOP\${GAME_NAME}.lnk"
	
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${GAME_NAME}"
	DeleteRegKey /ifempty HKLM "Software\${GAME_NAME}"

SectionEnd

;--------------------------------
;Uninstaller Descriptions

	;Assign language strings to sections
	!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
		!insertmacro MUI_DESCRIPTION_TEXT ${UnUser} $(SETUP_UNUSER_DESC)
	!insertmacro MUI_UNFUNCTION_DESCRIPTION_END

;--------------------------------
;Version Information

	VIProductVersion "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_COMMIT}.0"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${GAME_NAME} Installer"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_COMMIT}.0"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${GAME_AUTHOR}"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright 2014-2019 ${GAME_AUTHOR}"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${GAME_NAME} Installer"
	VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_COMMIT}.0"
