premake5 gmake
mingw32-make.exe -C bin/ config=%1_win64
copy_dlls %1
