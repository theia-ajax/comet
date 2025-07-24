workspace "comet"
	configurations { "debug", "release" }
	platforms { "win64", "linux64" }
	location "bin"
	files { "*.natvis" }
	includedirs { "include" }

filter "platforms:linux64"
	system "Linux"
	architecture "x86_64"
	toolset "gcc"
	buildoptions {"-Werror", "-ftrack-macro-expansion=0"}

filter "platforms:win64"
	system "Windows"
	architecture "x86_64"
	defines { "_CRT_SECURE_NO_WARNINGS" }
	disablewarnings { "4005" }

filter "configurations:debug"
	optimize "Off"
	symbols "On"
	defines { "_DEBUG" }
	
filter "configurations:release"
	optimize "On"

project "comet"
	kind "WindowedApp"
	language "C"
	cdialect "gnu17"
	toolset "gcc"
	location "bin/comet"
	files { 
		"src/common/**.c",
		"src/common/**.h",
		"src/common/**.inl",
		"src/main_%{cfg.platform:lower()}.c",
	}
	includedirs { "include" }
	debugdir "."

	filter "platforms:linux64"
		links { "SDL3", "m", "stdc++" }
		defines { "__LINUX__" }
		libdirs { "vcpkg_installed/x64-linux/lib" }

	filter "platforms:win64"
		links { "SDL3" }
		defines { "__WINDOWS__" }
		includedirs { "vcpkg_installed/x64-windows/include" }
		
	filter {"platforms:win64", "configurations:release"}
		libdirs { "vcpkg_installed/x64-windows/lib" }

	filter {"platforms:win64", "configurations:debug"}
		kind "ConsoleApp"
		libdirs { "vcpkg_installed/x64-windows/debug/lib" }



