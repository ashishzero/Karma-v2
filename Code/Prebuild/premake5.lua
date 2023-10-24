project "Prebuild"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"

   targetdir ("%{wks.location}/Build/%{prj.name}")
   objdir ("%{wks.location}/Build/Objects/%{prj.name}")

   vpaths {
        [""] = { "../Karma/Kr/kPrebuild.cpp" },
        ["Misc"] = { "../Karma/.clang-format", "../Karma/Kr/**.natvis", "../Karma/Kr/**.natstepfilter" }
   }

   files { "../Karma/.clang-format" }
   files { "../Karma/Kr/kPrebuild.cpp", "premake5.lua" }
   files { "../Karma/Kr/**.natvis", "../Karma/Kr/**.natstepfilter" }

   defines { "K_CONSOLE_APPLICATION" }
   defines { "DEBUG", "K_BUILD_DEBUG" }
   symbols "On"
   runtime "Debug"

   filter "system:windows"
      systemversion "latest"
      defines { "_CRT_SECURE_NO_WARNINGS" }
      flags { "NoManifest" }
