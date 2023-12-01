workspace "Karma"
   architecture "x86_64"
   startproject "Karma"
   configurations { "Debug", "Developer", "Release" }

IncludeDir = {}
IncludeDir["Kr"] = "%{wks.location}/Dependencies/Kr"

OutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "Dependencies/Kr"
    include "Dependencies/Premake"
group ""

include "Karma"
include "RayTrace"
