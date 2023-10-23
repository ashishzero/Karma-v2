workspace "Karma"
   architecture "x86_64"
   startproject "Karma"
   configurations { "Debug", "Developer", "Release" }

group "Dependencies"
    include "Code/Prebuild"
group ""

include "Code/Karma"
