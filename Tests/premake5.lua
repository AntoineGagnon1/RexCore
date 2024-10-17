project "RexCoreTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    warnings "Extra"
    flags { "FatalCompileWarnings" }
    
    targetdir ("%{prj.location}/bin/")
    objdir ("%{prj.location}/obj/")
    
    includedirs "../"
    
    links "RexCore"
    
    files {
        "%{prj.location}/**.hpp", 
        "%{prj.location}/**.cpp"
    }