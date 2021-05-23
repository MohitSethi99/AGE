project "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Arc/vendor/spdlog/include",
		"%{wks.location}/Arc/src",
		"%{wks.location}/Arc/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.Box2D}"
	}

	links
	{
		"Arc"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "ARC_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ARC_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ARC_DIST"
		runtime "Release"
		optimize "on"