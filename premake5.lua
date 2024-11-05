workspace "RexCore"
    configurations { "Debug", "Release", "Final" }
    platforms { "Win64" }
    startproject "RexCoreTests"

    buildoptions "/Zc:preprocessor" -- enable the standard compliant vs2022 preprocessor	

    filter "configurations:Debug"
        symbols "On"
    
    filter "configurations:Release or Final"
        optimize "On"
        
	filter "configurations:not Final"
		-- Add this to PATH for vs22 to find the clang dlls : 
		-- C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.41.34120\bin\Hostx64\x64
		buildoptions "/fsanitize=address"
		editandcontinue "off"
		defines {"REX_CORE_TRACK_ALLOCS"}
		
    filter {}
    
    include "rexcore"
    include "tests"