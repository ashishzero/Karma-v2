#include "kBuild.h"
#include "kPlatform.h"

K_BUILD_PROC void kExecBuild(int argc, const char **argv)
{
	kProject *karma = kCreateProject("Karma");

	kConfigureProject(karma, kBuild_DebugSymbols, kBuildKind_EXE, kBuildArch_x64);

	kSetBuildDirectory(karma, "build", "build/objs");
	kAddIncludeDirectory(karma, "Code");

	kString files[] = {"Code/Main.cpp",	  "Code/kCommon.cpp",	 "Code/kContext.cpp",
					   "Code/kMath.cpp",  "Code/kRender.cpp",	 "Code/kPlatform.cpp",
					   "Code/kMedia.cpp", "Code/kRenderApi.cpp", "Code/kMain.cpp"};

	kAddUnityFiles(karma, files);
	kAddResourceFile(karma, "Code/kResource.rc");
	kAddManifestFile(karma, "Code/kWindows.manifest");


	kBuildProject(karma);
}

void Main(int argc, const char **argv)
{
	kBootstrapBuild(argc, argv, "build/bootstrap.dll");
}

#include "kBuild.cpp"
#include "kMain.cpp"
