project "RexCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    warnings "Extra"
    flags { "FatalCompileWarnings" }

    includedirs "../"
    files {
        "%{prj.location}/**.hpp", 
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.natvis" 
    }
	
	libdirs { "%{prj.location}/vendors/Superluminal/" }
	
	filter "configurations:Debug"
        links "PerformanceAPI_MDd.lib"
	filter "configurations:Release or Final"
        links "PerformanceAPI_MD.lib"
	filter {}
	
	externalincludedirs { "%{prj.location}/vendors/**" }
	externalwarnings "Off"
	
	filter "files:**.natvis"
		buildaction "Natvis"
	filter {}