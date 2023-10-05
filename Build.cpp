#include "kBuild.h"
#include "kPlatform.h"
#include "kContext.h"
#include "kStrings.h"
#include "kArray.h"
#include "kCmdLineParser.h"

enum BuildConfig
{
	BuildConfig_Debug,
	BuildConfig_Developer,
	BuildConfig_Release,
};

static kString ArchStrings[]   = {"x64", "x86"};
static kString ConfigStrings[] = {"debug", "developer", "release"};

struct BuildSettings
{
	BuildConfig config	= BuildConfig_Debug;
	kBuildArch	arch	= kBuildArch_x64;
	uint		flags	= 0;
	kString		outdir	= "";
	kString		objdir	= "";
	kString		tmpdir	= "";
	kString		process = "";
	bool		execute = false;
};

static BuildSettings ParseBuildSettings(int argc, const char **argv)
{
	BuildSettings settings;

	bool		  help	   = false;

	kCmdLineFlag("run", &settings.execute, "Runs the executable if build succeeds");
	kCmdLineFlag("help", &help, "Prints usage and exits");
	kCmdLineOptions("arch", 0, ArchStrings, (int *)&settings.arch, "Architecture for the build");
	kCmdLineOptions("config", 0, ConfigStrings, (int *)&settings.config, "Configuration for the build");

	kCmdLineParse(&argc, &argv);

	if (help)
	{
		kCmdLinePrintUsage();
		kTerminate(0);
	}

	settings.flags = kBuild_DebugSymbols;

	if (settings.config == BuildConfig_Release || settings.config == BuildConfig_Developer)
	{
		settings.flags |= kBuild_Optimization;
	}

	const char *base   = "Build";
	const char *arch   = (char *)ArchStrings[settings.arch].data;
	const char *config = (char *)ConfigStrings[settings.config].data;

	settings.outdir	   = kFormatString("%s/Karma/%s-%s", base, arch, config);
	settings.objdir	   = kFormatString("%s/Objects/Karma/%s-%s", base, arch, config);
	settings.tmpdir	   = kFormatString("%s/Generated/Karma/%s-%s", base, arch, config);
	settings.process   = kFormatString(kStrFmt "/Karma.exe", kStrArg(settings.outdir));

	return settings;
}

K_BUILD_PROC void kBuild(int argc, const char **argv)
{
	BuildSettings settings = ParseBuildSettings(argc, argv);

	kLogTrace("== Build Configuration: %s | %s ==\n", ArchStrings[settings.arch].data, ConfigStrings[settings.config].data);

	kProject *karma = kCreateProject("Karma");

	kConfigureProject(karma, settings.flags, kBuildKind_EXE, settings.arch);

	kSetBuildDirectory(karma, settings.outdir, settings.objdir);

	if (settings.config == BuildConfig_Debug)
		kAddDefine(karma, "K_BUILD_DEBUG");
	else if (settings.config == BuildConfig_Developer)
		kAddDefine(karma, "K_BUILD_DEVELOPER");
	else
		kAddDefine(karma, "K_BUILD_RELEASE");

	kAddIncludeDirectory(karma, "Code");
	kSetTemporaryDirectory(karma, settings.tmpdir);

	kBeginUnityBuild(karma);
	kAddSourceFile(karma, "Main.cpp");
	kAddSourceFile(karma, "Code/kCommon.cpp");
	kAddSourceFile(karma, "Code/kContext.cpp");
	kAddSourceFile(karma, "Code/kCmdLineParser.cpp");
	kAddSourceFile(karma, "Code/kMath.cpp");
	kAddSourceFile(karma, "Code/kRender.cpp");
	kAddSourceFile(karma, "Code/kPlatform.cpp");
	kAddSourceFile(karma, "Code/kMedia.cpp");
	kAddSourceFile(karma, "Code/kRenderApi.cpp");
	kAddSourceFile(karma, "Code/kMain.cpp");
	kEndUnityBuild(karma);

	kAddResourceFile(karma, "Code/kResource.rc");
	kAddManifestFile(karma, "Code/kWindows.manifest");

	kBuildProject(karma);

	if (settings.execute)
	{
		kLogTrace("Launching process " kStrFmt "\n", kStrArg(settings.process));
		int rc = kExecuteProcess(settings.process);
		kLogTrace("\n\nProcess exited with code: %d\n\n", rc);
	}
}
