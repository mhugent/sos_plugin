!include x64.nsh

Name "QGIS SOS plugin"

OutFile "sos_plugin_setup.exe"
DirText "Select path to QGIS LTR installation" "QGIS LTR directory"

InstallDir "$PROGRAMFILES64\QGIS 3.4"
Page directory
Page instfiles

Section "install"

SetOutPath "$INSTDIR\apps\qgis-ltr\plugins"

File sosprovider.dll sosplugin.dll

SectionEnd