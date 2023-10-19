#!/usr/bin/sh

premake5 gmake2
make -C bin/ config=debug_linux64
