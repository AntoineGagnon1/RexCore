project "RexCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    warnings "Extra"
    flags { "FatalCompileWarnings" }

    includedirs "../"
    files {
        "%{prj.location}/**.hpp", 
        "%{prj.location}/**.cpp" 
    }