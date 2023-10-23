project "Prebuild"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"

   targetdir ("%{wks.location}/Build/%{prj.name}")
   objdir ("%{wks.location}/Build/Objects/%{prj.name}")

   files { "../Karma/.clang-format" }
   files { "../Karma/Kr/KPrebuild.cpp", "premake5.lua" }

   defines { "K_CONSOLE_APPLICATION" }
   defines { "DEBUG", "K_BUILD_DEBUG" }
   symbols "On"
   runtime "Debug"

   filter "system:windows"
      systemversion "latest"
      files { "../Karma/Kr/**.natvis", "../Karma/Kr/**.natstepfilter" }
      defines { "_CRT_SECURE_NO_WARNINGS" }
      flags { "NoManifest" }
