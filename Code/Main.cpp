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

static const kString Code = R"Foo(
#include <stdio.h>
#include <stdint.h>

extern const uint8_t UntitledImage[];

int main() {
	printf("Hello world: %p\n", UntitledImage);
	return 0;
}
)Foo";

void Main(int argc, const char **argv)
{
	kProject *project = kCreateProject("example");

	kConfigureProject(project, kBuild_DebugSymbols, kBuildKind_EXE, kBuildArch_x64);

	kSetBuildDirectory(project, "build/example", "build/example/obj");
	kSetTemporaryDirectory(project, "build/example/gens");

	umem size = 0;
	u8 *bytes = kReadEntireFile("C:/Users/zeroa/Downloads/Untitled.png", &size);
	kEmbedBinaryData(project, kSlice<u8>(bytes, size), "UntitledImage");

	//kEmbedFile(project, "C:/Users/zeroa/Downloads/Untitled.png", "UntitledImage");
	kAddString(project, Code);

	kBuildProject(project);

	kExecuteProcess("build/example/example.exe");

	//kAddFilesFromDirectory(project, "Code");
	//kAddManifestFile(project, "Code/kWindows.manifest");


	kMediaUserEvents user = {.update = Update};

	kEventLoop(kDefaultSpec, user);
}
