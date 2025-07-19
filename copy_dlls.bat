@echo off

if "%1"=="debug" (
	set "BINPATH=vcpkg_installed\x64-windows\debug\bin\"
) else (
	set "BINPATH=vcpkg_installed\x64-windows\bin\"
)

del *.dll
xcopy /y /d %BINPATH%*.dll .
