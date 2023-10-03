@echo off

if not exist "build" mkdir "build"
if not exist "build/Prebuild" mkdir "build/Prebuild"

echo Executing Prebuild...
clang++ -Wall -Wno-unused-function -O2 -DUNICODE -std=c++20 -I Code -o Build.exe Prebuild.cpp
del Build.exp
del Build.lib
echo Done
