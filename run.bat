@echo off
if "%1"=="" (
	set "CONFIG=debug"
) else (
	set "CONFIG=%1"
)
copy_dlls %CONFIG%
bin\comet\bin\Win64\Debug\comet.exe