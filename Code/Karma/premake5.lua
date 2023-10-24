project "Karma"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++20"
   dependson "Prebuild"

   targetdir ("%{wks.location}/Build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
   objdir ("%{wks.location}/Build/Objects/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")

   vpaths {
        ["Kr/Font"] = { "Kr/Font/**.h", "Kr/Font/**.cpp" },
        ["Kr/Shaders"] = { "Kr/**.hlsl", "Kr/**.fx" },
        ["Kr/Basic"] = {
            "Kr/kArray.h", "Kr/kLinkedList.h", "Kr/kStrings.h", "Kr/kPoolAllocator.h",
            "Kr/kCmdLine.h", "Kr/kCmdLine.cpp",
            "Kr/kCommon.h", "Kr/kCommon.cpp",
            "Kr/kContext.h", "Kr/kContext.cpp",
         },
         ["Kr/Math"] = { "Kr/kMath.h", "Kr/kMath.cpp" },
         ["Kr/Platform"] = { "Kr/kPlatform.h", "Kr/kPlatform.cpp", "kMain.cpp" },
         ["Kr/Media"] = { "Kr/kMedia.h", "Kr/kMediaBackend.h", "Kr/kMedia.cpp" },
         ["Kr/Render"] = { "Kr/kRender.h", "Kr/kRenderShared.h", "Kr/kRenderBackend.h", "Kr/kRender.cpp", "Kr/kRenderBackend.cpp" },
         ["Kr/Windows"] = { "Kr/kWindowsCommon.h", "Kr/kD3DShaders.h" },
         ["Kr/Windows/Media"] = { "Kr/kMediaBackendWin32.cpp" },
         ["Kr/Windows/D3D11"] = { "Kr/kRenderBackendD3D11.cpp" },
         ["Kr/Windows/D3D12"] = { "Kr/kRenderBackendD3D12.cpp" },
         ["Kr/Resource"] = { "Kr/kResource.h", "Kr/**.ico", "Kr/**.rc", "Kr/**.manifest" },
         ["Kr/Misc"] = { "Kr/**.natvis", "Kr/**.natstepfilter", ".clang-format" },
   }

   files { ".clang-format",
        "Kr/**.hlsl", "Kr/**.fx",
        "Kr/**.h", "Kr/**.cpp", "Kr/**.ico" }
   files { "**.h", "**.cpp", "premake5.lua" }
   files { "Kr/**.natvis", "Kr/**.rc", "Kr/**.manifest", "Kr/**.natstepfilter" }
   removefiles { "Kr/Shaders/Generated/**.h" }
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
      defines { "_CRT_SECURE_NO_WARNINGS" }
      flags { "NoManifest" }

      filter "files:**.hlsl"
        buildmessage 'Compiling shader %{file.relpath}'
        buildcommands {
            '%{wks.location}\\Build\\Prebuild\\Prebuild.exe "%{file.relpath}"'
        }
        buildoutputs { 'Kr/Shaders/Generated/%{file.basename}.hlsl.h' }
