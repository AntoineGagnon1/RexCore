workspace "RexCore"
    configurations { "Debug", "Release", "Final" }
    platforms { "Win64" }
    startproject "RexCoreTests"

    buildoptions "/Zc:preprocessor" -- enable the standard compliant vs2022 preprocessor	

    filter "configurations:Debug"
		-- Add this to PATH for vs22 to find the clang dlls : 
		-- C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.41.34120\bin\Hostx64\x64
		defines {"REX_CORE_TRACK_ALLOCS", "REX_CORE_TRACE_ENABLED"}
		buildoptions "/fsanitize=address"
		editandcontinue "off"
        symbols "On"
    
    filter "configurations:Release"
		defines {"REX_CORE_TRACK_ALLOCS", "REX_CORE_TRACE_ENABLED"}
        optimize "On"
        
	filter "configurations:Final"
        optimize "Speed"
		
    filter {}
    
    include "rexcore"
    include "tests"