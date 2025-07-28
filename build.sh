#!/usr/bin/sh

target='comet'
run=false
platform='linux64'
config='debug'

display_help()
{
	echo 'build.sh [-c|p|r|h]'
	exit
}

while getopts 'c:p:hr' flag; do
	case "${flag}" in
		h) display_help ;;
		r) run=true ;;
		c) config="${OPTARG}" ;;
		p) platform="${OPTARG}" ;;
		\?) exit ;;
	esac
done

makeconfig="${config}_${platform}"
premake="premake5"

if [ "$platform" = "rpi" ]; then
	premake="./external/tools/rpi/premake5"
fi

$premake gmake
make -C bin/ config=$makeconfig

if $run; then
	./bin/$target/bin/$platform/$config/$target
fi