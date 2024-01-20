project "Kr"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   characterset "Unicode"

   targetdir ("%{wks.location}/Build/" .. OutputDir)
   objdir ("%{wks.location}/Build/Objects/" .. OutputDir .. "/%{prj.name}")

   vpaths
   {
        ["Resources"] = { "Resources/**.h" },
        ["Image"] = { "Image/*.h" },
        ["ImGui"] = { "ImGui/*.h", "ImGui/*.cpp", "kImGui.h" },
        ["Shaders"] = { "*.hlsl", "*.fx" },
        ["Basic"] = {
            "kArray.h", "kLinkedList.h", "kStrings.h", "kPool.h",
            "kImage.h", "kImage.cpp",
            "kCmdLine.h", "kCmdLine.cpp",
            "kCommon.h", "kCommon.cpp",
            "kContext.h", "kContext.cpp",
         },
         ["Math"] = { "kMath.h", "kMath.cpp" },
         ["Platform"] = { "kPlatform.h", "kPlatform.cpp", "kMain.cpp" },
         ["Media"] = { "kResource.h", "kMedia.h", "kMediaBackend.h", "kMedia.cpp" },
         ["Render"] = { "kRender.h", "kRender.cpp", "kRenderShared.h", "kRenderShared.cpp", "kRenderCommand.h" },
         ["Render/Backend"] = { "kRenderBackend.h", "kRenderBackend.cpp", "kRenderResource.h", "kRenderResource.cpp" },
         ["Render/D3D11"] = { "kRenderBackendD3D11.cpp", "kD3DShaders.h" },
         ["Windows"] = { "kWindowsCommon.h", "kMediaBackendWin32.cpp" },
         ["Misc"] = { "**.natvis", "**.natstepfilter", ".clang-format" },
   }

   files { ".clang-format",
        "**.hlsl", "**.fx",
        "**.h", "**.cpp", "**.ico" }
   files { "**.h", "**.cpp" }
   files { "**.natvis", "**.natstepfilter" }
   removefiles { "Shaders/Generated/**.h" }
   removefiles { "Tools/**.h", "Tools/**.cpp" }

   includedirs { "." }

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
            '%{prj.location}\\Tools\\kShader.exe "%{file.relpath}"'
        }
        buildoutputs { 'Shaders/Generated/%{file.basename}.hlsl.h' }
        buildinputs { "%{file.relpath}", "Shaders/kFilters.fx" }
