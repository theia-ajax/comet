workspace "comet"
	configurations { "Debug", "Release" }
	platforms { "Win64", "Linux64" }
	location "bin"
	files { "*.natvis" }
	includedirs { "include" }

filter "platforms:Linux64"
	system "Linux"
	architecture "x86_64"
	toolset "gcc"
	buildoptions {"-Werror", "-ftrack-macro-expansion=0"}

filter "platforms:Win64"
	system "Windows"
	architecture "x86_64"
	defines { "_CRT_SECURE_NO_WARNINGS" }
	disablewarnings { "4005" }

filter "configurations:Debug"
	optimize "Off"
	symbols "On"
	defines { "_DEBUG" }
	
filter "configurations:Release"
	optimize "On"

project "comet"
	kind "ConsoleApp"
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

	filter "platforms:Linux64"
		links { "SDL3", "m", "stdc++" }
		libdirs { "lib/Linux64" }

	filter "platforms:Win64"
		links { "SDL3" }
		defines { "__WINDOWS__" }
		includedirs { "vcpkg_installed/x64-windows/include" }
		
	filter {"platforms:Win64", "configurations:Release"}
		libdirs { "vcpkg_installed/x64-windows/lib" }

	filter {"platforms:Win64", "configurations:Debug"}
		kind "ConsoleApp"
		libdirs { "vcpkg_installed/x64-windows/debug/lib" }



