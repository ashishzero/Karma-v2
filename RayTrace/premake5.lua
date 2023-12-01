project "RayTrace"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++20"
   characterset "Unicode"

   targetdir ("%{wks.location}/Build/" .. OutputDir)
   objdir ("%{wks.location}/Build/Objects/" .. OutputDir .. "/%{prj.name}")
   debugdir ("%{wks.location}")

   files
   {
      "**.cpp",
      "RayTrace.ico", "RayTrace.rc", "RayTrace.exe.manifest"
   }

   includedirs { "%{IncludeDir.Kr}" }
   links { "Kr" }

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
