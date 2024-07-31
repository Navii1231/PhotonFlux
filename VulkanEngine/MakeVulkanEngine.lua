outputDir = "%{cfg.buildcfg}/%{cfg.architecture}"

project "VulkanEngine"
	location ""
	kind "StaticLib"
	language "C++"

	targetdir ("../out/bin/" .. outputDir .. "/%{prj.name}")
    objdir ("../out/int/" .. outputDir .. "/%{prj.name}")
    flags {"MultiProcessorCompile"}

	files
	{
		"%{prj.location}/**.h",
		"%{prj.location}/**.hpp",
		"%{prj.location}/**.c",
		"%{prj.location}/**.cpp",
	}

	includedirs
	{
		"%{prj.location}/Dependencies/include/",
		"%{prj.location}/Include/",
	}

    libdirs
    {
    	"%{prj.location}/Dependencies/lib/",
    }

    links
    {
        "glfw3.lib",
        "vulkan-1.lib"
    }

		filter "system:windows"
        cppdialect "C++20"
        staticruntime "On"
        systemversion "10.0"

        defines
        {
            "_CONSOLE",
            "WIN32",
        }

        filter "configurations:Debug"
            defines 
            {
                "VULKAN_ENGINE_BUILD_STATIC",
                "_DEBUG",
            }

            inlining "Disabled"
            symbols "On"
            staticruntime "Off"
            runtime "Debug"

        filter "configurations:Release"

            defines "NDEBUG"
            optimize "Full"
            inlining "Auto"
            staticruntime "Off"
            runtime "Release"
