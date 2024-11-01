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
	
	externalincludedirs { "%{prj.location}/vendors/**" }
	externalwarnings "Off"
	
	filter "files:**.natvis"
		buildaction "Natvis"
	filter {}