workspace "Jobs"
	architecture "x86_64"
	platforms { "Win64" }
	configurations { "Debug", "Release" }
	
	EnableLogging = false
	EnableProfiling = true
	
	flags { "NoPCH", "MultiProcessorCompile" }
	clr "Off"
	rtti "Off"
	characterset "Unicode"
	staticruntime "Off"
	warnings "Extra"
	
	defines { "WIN32", "_WIN32" }
	system "Windows"
	systemversion "latest"
	
	filter { "configurations:Debug" }
		defines { "DEBUG", "_DEBUG" }
		symbols "On"
		optimize "Off"
		omitframepointer "Off"
		exceptionhandling "On"
	
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		flags { "LinkTimeOptimization", "NoRuntimeChecks" }
		symbols "Off"
		optimize "Speed"
		omitframepointer "On"
		exceptionhandling "Off"
		
	filter {}
		
project "Jobs"
	language "C++"
	cppdialect "C++17"
	kind "StaticLib"
	
	location "Build/Generated"
	buildlog "Build/Logs/Jobs.log"
	basedir "../../"
	objdir "Build/Intermediate/%{cfg.platform}_%{cfg.buildcfg}"
	targetdir "Build/Bin/%{cfg.platform}_%{cfg.buildcfg}"
	
	targetname "Jobs"
	
	includedirs { "Jobs/Include" }
	
	if EnableLogging then
		defines { "JOBS_ENABLE_LOGGING=1" }
	else
		defines { "JOBS_ENABLE_LOGGING=0" }
	end
	
	if EnableProfiling then
		defines { "JOBS_ENABLE_PROFILING=1", "TRACY_ENABLE" }
		includedirs { "tracy" }
	else
		defines { "JOBS_ENABLE_PROFILING=0" }
	end
	
	files { "Jobs/Include/Jobs/*.h", "Jobs/Source/*.cpp" }
	
	links { "Synchronization" }
	
if EnableProfiling then
	project "Profiling"
		language "C++"
		cppdialect "C++17"
		kind "ConsoleApp"
		
		location "Build/Generated"
		buildlog "Build/Logs/Profiling.log"
		basedir "../../"
		objdir "Build/Intermediate/%{cfg.platform}_%{cfg.buildcfg}"
		targetdir "Build/Bin/%{cfg.platform}_%{cfg.buildcfg}"
	
		targetname "Profiling"
	
		includedirs { "Jobs/Include", "tracy" }
		
		defines { "TRACY_ENABLE" }
		
		files { "Examples/*.cpp" }
		
		links { "Jobs" }
end