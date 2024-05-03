premake5 gmake2
mingw32-make.exe CC=gcc -C bin/ config=%1_win64
xcopy /y /d lib\Win64\%1\*.dll .