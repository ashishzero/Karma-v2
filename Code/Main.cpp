#include "kMedia.h"
#include "kArray.h"
#include "kBuild.h"

void Update(float dt)
{
	kSlice<kEvent> events = kGetEvents();

	if (kKeyPressed(kKey_Escape))
	{
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey_F11))
	{
		kToggleWindowFullscreen();
	}

	if (kKeyPressed(kKey_H))
	{
		if (kIsCursorEnabled())
		{
			kDisableCursor();
		}
		else
		{
			kEnableCursor();
		}
	}

	if (kKeyPressed(kKey_M))
	{
		if (kIsWindowMaximized())
		{
			kRestoreWindow();
		}
		else
		{
			kMaximizeWindow();
		}
	}
}

void Main(int argc, const char **argv)
{
	kProject *project = kCreateProject("Karma");

	kConfigureProject(project, kBuild_DebugSymbols, kBuildKind_EXE, kBuildArch_x64);

	kAddFilesFromDirectory(project, "Code");
	kAddManifestFile(project, "Code/kWindows.manifest");

	kBuildProject(project);

	kMediaUserEvents user = {.update = Update};

	kEventLoop(kDefaultSpec, user);
}
