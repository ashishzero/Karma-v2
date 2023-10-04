#include "kBuild.h"
#include "kContext.h"
#include "kPlatform.h"

void Main(int argc, const char **argv)
{
	kString buildsrc = "Build.cpp";

	if (!kGetFileAttributes(buildsrc))
	{
		kFatalError(" \"Build.cpp\" file not present in current directory");
	}

	bool	rebuild	 = false;
	kString builddll = "Build/Prebuild/Bootstrap.dll";
	kString buildlib = "Build/Prebuild/kBuild.lib";

	kString files[]	 = {"Code/kCommon.cpp", "Code/kBuild.cpp", "Code/kContext.cpp", "Code/kPlatform.cpp", buildsrc};

	if (kGetFileAttributes(builddll))
	{
		u64 dll = kGetFileLastModifiedTime(builddll);
		for (kString src : files)
		{
			u64 time = kGetFileLastModifiedTime(src);
			if (time > dll)
			{
				rebuild = true;
				break;
			}
		}
	}
	else
	{
		rebuild = true;
	}

	if (rebuild)
	{
		kLogTrace("Bootstraping %.*s...\n", (int)builddll.count, builddll.data);

		kProject *bootstrap = kCreateProject("Bootstrap");
		kConfigureProject(bootstrap, kBuild_DebugSymbols, kBuildKind_DLL, kBuildArch_x64);

		kSetBuildDirectory(bootstrap, "Build/Prebuild", "Build/Objects");
		kAddDefine(bootstrap, "K_IMPORT_SYMBOLS");
		kAddIncludeDirectory(bootstrap, "Code");
		kAddLibrary(bootstrap, buildlib);

		kAddUnityFiles(bootstrap, files);

		if (kBuildProject(bootstrap) != 0)
		{
			kLogError("Failed to bootstrap.\n\n");
			kTerminate(1);
		}

		kLogTrace("Bootstrap done.\n\n");
	}

	kModule mod = kLoadModule(builddll);
	if (!mod)
	{
		kLogError("Failed to load Prebuild DLL: %.*s\n", (int)builddll.count, builddll.data);
		return;
	}

	kProjectBuildProc exec = (kProjectBuildProc)kGetProcAddress(mod, "kBuild");

	exec(argc, argv);

	kFreeModule(mod);
}

//
//
//

#include "kCommon.cpp"
#include "kContext.cpp"
#include "kBuild.cpp"
#include "kPlatform.cpp"
#include "kMain.cpp"
