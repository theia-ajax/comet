@echo off

set TARGET=%1
set TARGET_DIR=bin
set TARGET_PATH=%TARGET_DIR%/%TARGET%

set CFLAGS=--std=c99 -g
set LFLAGS=-lSDL2 -lSDL2main -luser32 -lgdi32

IF NOT EXIST %TARGET_DIR% mkdir %TARGET_DIR%

clang %CFLAGS% src/corvid.c -Iexternal/include -Lexternal/lib -o %TARGET_PATH% %LFLAGS%
