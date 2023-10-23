#include "kCommon.h"

extern void Main(int argc, const char **argv);

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <objbase.h>
#include <shellapi.h>

#pragma comment(lib, "Ole32.lib")   // COM
#pragma comment(lib, "Shell32.lib") // CommandLineToArgvW

static int kWinMain(void)
{
#ifdef K_WINDOW_DPI_AWARENESS_VIA_API
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
	AllocConsole();
#endif

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	kAssert(hr != RPC_E_CHANGED_MODE);

	if (hr != S_OK && hr != S_FALSE)
	{
		FatalAppExitW(0, L"Windows: Failed to initialize COM Library");
	}

	int          argc = 0;
	const char **argv = 0;

	HANDLE       heap = GetProcessHeap();
	LPWSTR      *args = CommandLineToArgvW(GetCommandLineW(), &argc);

	argv              = (const char **)HeapAlloc(heap, 0, sizeof(char *) * argc);
	if (argv)
	{
		for (int index = 0; index < argc; ++index)
		{
			int len     = WideCharToMultiByte(CP_UTF8, 0, args[index], -1, 0, 0, 0, 0);
			argv[index] = (char *)HeapAlloc(heap, 0, len + 1);
			if (argv[index])
			{
				WideCharToMultiByte(CP_UTF8, 0, args[index], -1, (LPSTR)argv[index], len + 1, 0, 0);
			}
			else
			{
				argv[index] = "";
			}
		}
	}
	else
	{
		argc = 0;
	}

	Main(argc, argv);

	CoUninitialize();

	return 0;
}

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER) || defined(K_CONSOLE_APPLICATION)
#pragma comment(linker, "/subsystem:console")
#else
#pragma comment(linker, "/subsystem:windows")
#endif

// SUBSYSTEM:CONSOLE
int wmain() { return kWinMain(); }

// SUBSYSTEM:WINDOWS
int __stdcall wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd, int n) { return kWinMain(); }

#endif
