#!/usr/bin/sh

premake5 gmake2
make -C bin/ config=$1_linux64
