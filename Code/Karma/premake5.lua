project "Karma"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++20"
   dependson "Prebuild"

   targetdir ("%{wks.location}/Build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("%{wks.location}/Build/Objects/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")

   files { ".clang-format",
    "Kr/**.hlsl", "Kr/**.fx",
    "Kr/**.h", "Kr/**.cpp", "Kr/**.ico" }
   files { "**.h", "**.cpp", "premake5.lua" }

   removefiles { "Kr/kShader.h", "Kr/kShader.cpp", "Kr/kPrebuild.cpp" }
   includedirs { "Kr" }

   dpiawareness "HighPerMonitor"

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
      files { "Kr/**.natvis", "Kr/**.rc", "Kr/**.manifest", "Kr/**.natstepfilter" }
      defines { "_CRT_SECURE_NO_WARNINGS" }
      flags { "NoManifest" }

      filter "files:**.hlsl"
        buildmessage 'Compiling shader %{file.relpath}'
        buildcommands {
            '%{wks.location}\\Build\\Prebuild\\Prebuild.exe "%{file.relpath}"'
        }
        buildoutputs { 'Kr/Shaders/Generated/%{file.basename}.hlsl.h' }
