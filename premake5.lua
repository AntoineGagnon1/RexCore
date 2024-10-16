workspace "RexCore"
    configurations { "Debug", "Release" }
    platforms { "Win64" }
    startproject "RexCoreTests"

    buildoptions "/Zc:preprocessor" -- enable the standard compliant vs2022 preprocessor

    filter "configurations:Debug"
        symbols "On"
    
    filter "configurations:Release"
        optimize "On"
        
    filter {}
    
    include "RexCore"
    include "Tests"