#!/usr/bin/sh

platform='linux64'
config='release'

display_help()
{
	echo 'comet.sh [-c|p|h]'
	exit
}

while getopts 'c:p:h' flag; do
	case "${flag}" in
		h) display_help ;;
		c) config="${OPTARG}" ;;
		p) platform="${OPTARG}" ;;
		\?) exit ;;
	esac
done

binpath='./bin/comet/bin/${platform}/${config}/comet'

[[ -f "$binpath" ]] && ./$binpath || ./build.sh -c $config -p $platform -r
