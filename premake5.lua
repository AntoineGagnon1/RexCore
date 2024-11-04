workspace "RexCore"
    configurations { "Debug", "Release", "DebugAsan", "ReleaseAsan" }
    platforms { "Win64" }
    startproject "RexCoreTests"

    buildoptions "/Zc:preprocessor" -- enable the standard compliant vs2022 preprocessor	

	defines {"REX_CORE_TRACK_ALLOCS"}

    filter "configurations:Debug or DebugAsan"
        symbols "On"
    
    filter "configurations:Release or ReleaseAsan"
        optimize "On"
        
	filter "configurations:DebugAsan or ReleaseAsan"
		-- Add this to PATH for vs22 to find the clang dlls : 
		-- C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.41.34120\bin\Hostx64\x64
		buildoptions "/fsanitize=address"
		editandcontinue "off"
		
    filter {}
    
    include "rexcore"
    include "tests"