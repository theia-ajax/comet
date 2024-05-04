@echo off

@REM set TARGET=%1
@REM set TARGET_DIR=bin
@REM set TARGET_PATH=%TARGET_DIR%/%TARGET%

@REM set CFLAGS=--std=c99 -g
@REM set LFLAGS=-lSDL2 -lSDL2main -luser32 -lgdi32

@REM IF NOT EXIST %TARGET_DIR% mkdir %TARGET_DIR%

@REM clang %CFLAGS% src/corvid.c -Iexternal/include -Lexternal/lib -o %TARGET_PATH% %LFLAGS%

if "%1"=="" (
	set "CONFIG=debug"
) else (
	set "CONFIG=%1"
)
.\build_mingw64 %CONFIG%