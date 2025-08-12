require("ecc/ecc")

-- =======================================
-- WORKSPACE(LearnDX11)
-- =======================================
workspace "LearnDX11"
architecture "x64"
configurations {"Debug", "Release"}

defines {"UNICODE", "_UNICODE"}

filter {"configurations:Debug"}
defines {"DEBUG", "_DEBUG"}
symbols "On"

filter {"configurations:Release"}
defines {"NDEBUG"}
optimize "On"

filter {"action:gmake", "configurations:Release"}
buildoptions {"-static-libgcc", "-static-libstdc++"}
linkoptions {"-static-libgcc", "-static-libstdc++"}

filter {"action:vs*", "configurations:Release"}
staticruntime "On"

filter {}

outdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- =======================================
-- PROJECT(DLL)
-- =======================================
project "DLL"
location "DLL"
kind "SharedLib"
language "C"
cdialect "C11"

targetdir("bin/" .. outdir)
objdir("bin-int/" .. outdir)

pchheader "pch.h"
pchsource "%{prj.location}/src/pch.c"

files {"%{prj.location}/include/**.h", "%{prj.location}/src/**.c"}
includedirs {"%{prj.location}", "%{prj.location}/include"}

defines {"DLL_EXPORTS"}
links {"user32", "d3d11", "dxgi", "dxguid", "d3dcompiler"}

-- =======================================
-- PROJECT(Test)
-- =======================================
project "Test"
location "Test"
kind "ConsoleApp"
language "C"
cdialect "C11"

targetdir("bin/" .. outdir)
objdir("bin-int/" .. outdir)

files {"%{prj.location}/*.c"}
includedirs {"DLL"}

links {"DLL"}
