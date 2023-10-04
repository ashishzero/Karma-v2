@echo off

if not exist "Build" mkdir "Build"
if not exist "Build/Prebuild" mkdir "Build/Prebuild"

echo Executing Prebuild...
clang++ -DK_EXPORT_SYMBOLS -Wall -Wno-unused-function -O2 -DUNICODE -std=c++20 -I Code -o Build/Prebuild/kBuild.exe Code/kPrebuild.cpp
xcopy "Build\Prebuild\kBuild.exe" "." /Y
echo Done
