newoption {
	trigger = "logging",
	description = "Enables internal logging for debugging"
}

newoption {
	trigger = "profiling",
	description = "Enables internal profiling utilities and Tracy zone emission. Also creates Tracy projects and a profiling project from the Examples/ directory."
}

EnableLogging = false
EnableProfiling = false

if _OPTIONS["logging"] then
	EnableLogging = true
end

if _OPTIONS["profiling"] then
	EnableProfiling = true
end

workspace "Jobs"
	platforms { "Static64" }
	configurations { "Debug", "Release" }
	
	filter { "platforms:*64" }
		architecture "x86_64"
		
	filter {}
	
	filter { "system:windows" }
		systemversion "latest"
		
	filter {}
	
	if EnableProfiling then
		startproject "Profiling"
	else
		startproject "Jobs"
	end
	
	flags { "NoPCH", "MultiProcessorCompile" }
	clr "Off"
	rtti "Off"
	characterset "Unicode"
	staticruntime "Off"
	warnings "Extra"
	
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
	buildlog "Build/Logs/Build.log"
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
	
	files { "Jobs/Include/Jobs/*.h", "Jobs/Include/Jobs/*/*.h", "Jobs/Source/*.cpp" }
	
	if EnableProfiling then
		filter { "system:windows", "configurations:Debug" }
			buildoptions "/Zi"  -- Disable edit and continue, this causes __LINE__ to not evaluate as a constant, which Tracy needs.
			
		filter {}
	end
	
	filter { "system:windows" }
		buildoptions "/GT"  -- Enable fiber-safe optimizations, prevents TLS array address from being cached since a fiber can be resumed on any thread.
		
	filter {}
	
	filter { "system:windows" }
		links { "Synchronization" }
		
	filter { "system:linux" }
		links { "pthread" }
	
	filter {}
	
	-- Assemble and link fiber routines.
	include "Jobs/ThirdParty/boost.context"
	
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
		
		if EnableLogging then
			defines { "JOBS_ENABLE_LOGGING=1" }
		else
			defines { "JOBS_ENABLE_LOGGING=0" }
		end
		
		files { "Examples/*.cpp" }
		
		libdirs "Build/Bin/*"
		libdirs "Build/TracyClient/Bin/*"
		
		links { "Jobs", "Tracy" }
		
		include "tracy"
end