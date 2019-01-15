;NSIS OpenXcom Windows Installer Language

!insertmacro LANGFILE_EXT "English" ; Must be the lang name define by NSIS

;--------------------------------
;Pages

${LangFileString} SETUP_APOCALYPSE_CD_TITLE "Choose X-COM Apocalypse Location"
${LangFileString} SETUP_APOCALYPSE_CD_SUBTITLE "Choose the CD image file from where you have X-COM Apocalypse installed."
${LangFileString} SETUP_APOCALYPSE_CD "${GAME_NAME} ${GAME_VERSION} requires a CD image of X-COM Apocalypse. You can skip this step if you're upgrading an existing installation.$\n$\nSetup will point the initial configuration to your CD file. To select from a different folder, click ... and select another folder. Click Next to continue."

${LangFileString} SETUP_EXTRA_OPTIONS_TITLE "Choose Options"
${LangFileString} SETUP_EXTRA_OPTIONS_SUBTITLE "Choose which additional options of $(^NameDA) you want to install."
${LangFileString} SETUP_PORTABLE "Portable Installation"
${LangFileString} SETUP_PORTABLE_DESC "${GAME_NAME} will store user data in the installation folder. The folder must be user-writable."

;--------------------------------
;Installer Sections

${LangFileString} SETUP_GAME "Game Files"
${LangFileString} SETUP_GAME_DESC "Files required to run ${GAME_NAME}."
${LangFileString} SETUP_DESKTOP "Desktop Shortcut"
${LangFileString} SETUP_DESKTOP_DESC "Creates a shortcut in the desktop to play ${GAME_NAME}."

;--------------------------------
;Uninstaller Descriptions

${LangFileString} SETUP_UNDATA "Delete X-COM Data"
${LangFileString} SETUP_UNDATA_DESC "Deletes all ${GAME_NAME} install data, including X-COM resources. Recommended for a clean reinstall."
${LangFileString} SETUP_UNUSER "Delete User Data"
${LangFileString} SETUP_UNUSER_DESC "Deletes all ${GAME_NAME} user data, including mods, savegames, screenshots and options. Only use this for a complete wipe."

;--------------------------------
;Shortcuts

${LangFileString} SETUP_SHORTCUT_CHANGELOG "Changelog"
${LangFileString} SETUP_SHORTCUT_README "Readme"
${LangFileString} SETUP_SHORTCUT_USER "User Folder"
${LangFileString} SETUP_SHORTCUT_UNINSTALL "Uninstall"

;--------------------------------
;Warnings

${LangFileString} SETUP_WARNING_APOCALYPSE "X-COM Apocalypse CD wasn't found. ${GAME_NAME} requires a valid CD image of X-COM Apocalypse to run. Continue?"
