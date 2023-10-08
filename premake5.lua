workspace "Karma"
   architecture "x86_64"
   startproject "Karma"
   configurations { "Debug", "Developer", "Release" }

project "Karma"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++20"
   dependson "Prebuild"

   targetdir ("%{wks.location}/Build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("%{wks.location}/Build/Objects/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")

   files { ".clang-format" }
   files { "Code/Kr/**.h", "Code/Kr/**.cpp", "Code/Kr/**.ico" }
   files { "Code/Karma/**.h", "Code/Karma/**.cpp" }

   removefiles { "Code/Kr/kPrebuild.h", "Code/Kr/kPrebuild.cpp" }

   includedirs { "Code/Kr" }

   filter "configurations:Debug"
      defines { "DEBUG", "K_BUILD_DEBUG" }
      symbols "On"
      runtime "Debug"

   filter "configurations:Developer"
      defines { "NDEBUG", "K_BUILD_DEVELOPER" }
      optimize "On"
      runtime "Release"

   filter "configurations:Release"
      defines { "NDEBUG", "K_BUILD_RELEASE" }
      optimize "On"
      runtime "Release"

   filter "system:windows"
      systemversion "latest"
      files { "Code/**.natvis", "Code/**.hlsl", "Code/**.rc", "Code/Kr/kWindows.manifest" }
      defines { "_CRT_SECURE_NO_WARNINGS" }
      prebuildcommands { "\"%{wks.location}Build\\%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}\\Prebuild.exe\"" }
      filter { "files:**.hlsl" }
        flags { "ExcludeFromBuild" }

project "Prebuild"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"

   targetdir ("%{wks.location}/Build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("%{wks.location}/Build/Objects/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")

   files { ".clang-format" }
   files { "Code/Kr/kPrebuild.h", "Code/Kr/kPrebuild.cpp" }
   files { "Code/Kr/kPlatform.h", "Code/Kr/kPlatform.cpp" }
   files { "Code/Kr/kCmdLine.h", "Code/Kr/kCmdLine.cpp" }
   files { "Code/Kr/kCommon.h", "Code/Kr/kCommon.cpp" }
   files { "Code/Kr/kContext.h", "Code/Kr/kContext.cpp" }
   files { "Code/Kr/kMain.h", "Code/Kr/kMain.cpp" }
   files { "Code/Prebuild/**.h", "Code/Prebuild/**.cpp" }

   includedirs { "Code/Kr" }
   defines { "K_CONSOLE_APPLICATION" }

   filter "configurations:Debug"
      defines { "DEBUG", "K_BUILD_DEBUG" }
      symbols "On"
      runtime "Debug"

   filter "configurations:Developer"
      defines { "NDEBUG", "K_BUILD_DEVELOPER" }
      optimize "On"
      runtime "Release"

   filter "configurations:Release"
      defines { "NDEBUG", "K_BUILD_RELEASE" }
      optimize "On"
      runtime "Release"

   filter "system:windows"
      systemversion "latest"
      files { "Code/**.natvis" }
      defines { "_CRT_SECURE_NO_WARNINGS" }
