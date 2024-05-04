premake5 gmake2
mingw32-make.exe CC=gcc -C bin/ config=%1_win64
copy_dlls %1