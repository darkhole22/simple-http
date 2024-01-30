project "SimpleHTTP"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "SimpleHTTP/**.h",
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        ".",
        "src"
    }

    filter "system:windows"
        systemversion "latest"
        
        files
        {
            "platform/windows/**.h",
            "platform/windows/**.cpp"
        }

        includedirs
        {
            "platform/windows"
        }
        
        links
        {
        }
        
        defines {
            "NOMINMAX"
        }

    filter "system:linux"
        systemversion "latest"

        files
        {
            "platform/linux/**.h",
            "platform/linux/**.cpp"
        }

        includedirs
        {
            "platform/linux"
        }
        
        links {
        }
    
    filter "configurations:Debug"
        defines
        {
            "SIMPLE_HTTP_DEBUG_BUILD"
        }
        symbols "On"

    filter "configurations:Release"
        defines 
        {
            "SIMPLE_HTTP_NDEBUG_BUILD",
            "SIMPLE_HTTP_RELEASE_BUILD"
        }
        optimize "On"
