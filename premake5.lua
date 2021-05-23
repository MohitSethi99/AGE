include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Arc"
	architecture "x86_64"
	startproject "Arc-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}";

group "Dependencies"
	include "vendor/premake"
	include "Arc/vendor/GLFW"
	include "Arc/vendor/Glad"
	include "Arc/vendor/imgui"
	include "Arc/vendor/yaml-cpp"
	include "Arc/vendor/box2d"
	include "Arc/vendor/assimp"

group ""

include "Arc"
include "Sandbox"
include "Arc-Editor"
