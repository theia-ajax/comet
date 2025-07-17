@echo off

if "%1"=="debug" (
	set "BINPATH=lib\Win64\Debug\"
) else (
	set "BINPATH=lib\Win64\Release\"
)

del *.dll
xcopy /y /d %BINPATH%*.dll .
