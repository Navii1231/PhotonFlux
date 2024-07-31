workspace "VulkanEngine"
architecture "x64"
configurations { "Debug", "Release" }

startproject "vkEngineTester"

include "VulkanEngine/MakeVulkanEngine.lua"
include "vkEngineTester/MakevkEngineTester.lua"
