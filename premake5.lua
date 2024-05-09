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

	if os.istarget("windows") then
		filter "configurations:Debug"
			kind "ConsoleApp"
	end

	filter "platforms:Linux64"
		links { "SDL3", "m", "stdc++" }
		libdirs { "lib/Linux64" }

	filter "platforms:Win64"
		links { "SDL3" }
		libdirs { "lib/Win64/%{cfg.buildcfg}" }
		



