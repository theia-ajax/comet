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
	optimize "Full"
	defines { "_NDEBUG" }

project "comet"
	kind "WindowedApp"
	language "C"
	cdialect "C11"
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
			links { "SDL2maind" }
			
		filter "configurations:Release"
			links { "SDL2main" }
	end

	filter "platforms:Linux64"
		cdialect "gnu11"
		links { "SDL2", "m", "stdc++" }


