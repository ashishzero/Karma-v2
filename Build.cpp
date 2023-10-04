#include "kBuild.h"
#include "kPlatform.h"
#include "kContext.h"
#include <stdio.h>

#include <string.h>

enum BuildConfig
{
	BuildConfig_Debug,
	BuildConfig_Developer,
	BuildConfig_Release,
};

enum BuildPlatform
{
	BuildPlatform_x64,
	BuildPlatform_x86,
};

static const char *BuildPlatformStrings[] = {"x64", "x86"};
static const char *BuildConfigStrings[]	  = {"debug", "developer", "release"};

struct BuildSettings
{
	BuildConfig	  config   = BuildConfig_Debug;
	BuildPlatform platform = BuildPlatform_x64;
};

static const char *NextCommandLineArg(int *argc, const char ***argv)
{
	if (*argc)
	{
		const char *arg = **argv;
		(*argc) -= 1;
		(*argv) += 1;
		return arg;
	}
	return nullptr;
}

// Build.exe -platform:x64 -config:debug
static BuildSettings ParseBuildSettings(int argc, const char **argv)
{
	BuildSettings settings = {};

	while (1)
	{
		const char *arg = NextCommandLineArg(&argc, &argv);

		if (!arg)
			break;

		if (strcmp(arg, "debug") == 0)
		{
			settings.config = BuildConfig_Debug;
		}
		else if (strcmp(arg, "developer") == 0)
		{
			settings.config = BuildConfig_Developer;
		}
		else if (strcmp(arg, "release") == 0)
		{
			settings.config = BuildConfig_Release;
		}
		else if (strcmp(arg, "x64") == 0)
		{
			settings.platform = BuildPlatform_x64;
		}
		else if (strcmp(arg, "x86") == 0)
		{
			settings.platform = BuildPlatform_x86;
		}
	}

	return settings;
}

K_BUILD_PROC void kBuild(int argc, const char **argv)
{
	BuildSettings settings = ParseBuildSettings(argc, argv);

	kLogTrace("== Build Configuration: %s | %s ==\n", BuildConfigStrings[settings.config],
			  BuildPlatformStrings[settings.platform]);

	uint	   flags = kBuild_DebugSymbols;
	kBuildArch arch	 = kBuildArch_x64;

	if (settings.platform == BuildPlatform_x64)
	{
		arch = kBuildArch_x64;
	}
	else if (settings.platform == BuildPlatform_x86)
	{
		arch = kBuildArch_x86;
	}

	if (settings.config == BuildConfig_Release || settings.config == BuildConfig_Developer)
	{
		flags |= kBuild_Optimization;
	}

	//
	//
	//

	kProject *karma = kCreateProject("Karma");

	kConfigureProject(karma, flags, kBuildKind_EXE, arch);

	kSetBuildDirectory(karma, "Build/Karma", "Build/Objects");
	kAddIncludeDirectory(karma, "Code");

	kString files[] = {"Main.cpp",		  "Code/kCommon.cpp",	 "Code/kContext.cpp",
					   "Code/kMath.cpp",  "Code/kRender.cpp",	 "Code/kPlatform.cpp",
					   "Code/kMedia.cpp", "Code/kRenderApi.cpp", "Code/kMain.cpp"};

	kAddUnityFiles(karma, files);
	kAddResourceFile(karma, "Code/kResource.rc");
	kAddManifestFile(karma, "Code/kWindows.manifest");

	kBuildProject(karma);
}
