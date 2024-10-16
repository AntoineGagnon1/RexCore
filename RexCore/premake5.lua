project "RexCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    warnings "Extra"
    flags { "FatalCompileWarnings" }

    includedirs "./"
    files {
        "%{prj.location}/RexCore/**.hpp", 
        "%{prj.location}/RexCore/**.cpp" 
    }