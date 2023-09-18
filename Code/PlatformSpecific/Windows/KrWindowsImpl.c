#include "KrWindowsCommon.h"

#include <avrt.h>
#include <windowsx.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#include <VersionHelpers.h>
#include <objbase.h>
#include <shellapi.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")

//
// Files
//

//
// Window / Events Helpers
//


static const kKey VirtualKeyMap[] = {
	['A'] = kKey_A, ['B'] = kKey_B,
	['C'] = kKey_C, ['D'] = kKey_D,
	['E'] = kKey_E, ['F'] = kKey_F,
	['G'] = kKey_G, ['H'] = kKey_H,
	['I'] = kKey_I, ['J'] = kKey_J,
	['K'] = kKey_K, ['L'] = kKey_L,
	['M'] = kKey_M, ['N'] = kKey_N,
	['O'] = kKey_O, ['P'] = kKey_P,
	['Q'] = kKey_Q, ['R'] = kKey_R,
	['S'] = kKey_S, ['T'] = kKey_T,
	['U'] = kKey_U, ['V'] = kKey_V,
	['W'] = kKey_W, ['X'] = kKey_X,
	['Y'] = kKey_Y, ['Z'] = kKey_Z,

	['0'] = kKey_0, ['1'] = kKey_1,
	['2'] = kKey_2, ['3'] = kKey_3,
	['4'] = kKey_4, ['5'] = kKey_5,
	['6'] = kKey_6, ['7'] = kKey_7,
	['8'] = kKey_8, ['9'] = kKey_9,

	[VK_NUMPAD0] = kKey_0, [VK_NUMPAD1] = kKey_1,
	[VK_NUMPAD2] = kKey_2, [VK_NUMPAD3] = kKey_3,
	[VK_NUMPAD4] = kKey_4, [VK_NUMPAD5] = kKey_5,
	[VK_NUMPAD6] = kKey_6, [VK_NUMPAD7] = kKey_7,
	[VK_NUMPAD8] = kKey_8, [VK_NUMPAD9] = kKey_9,

	[VK_F1]  = kKey_F1,   [VK_F2]  = kKey_F2,
	[VK_F3]  = kKey_F3,   [VK_F4]  = kKey_F4,
	[VK_F5]  = kKey_F5,   [VK_F6]  = kKey_F6,
	[VK_F7]  = kKey_F7,   [VK_F8]  = kKey_F8,
	[VK_F9]  = kKey_F9,   [VK_F10] = kKey_F10,
	[VK_F11] = kKey_F11,  [VK_F12] = kKey_F12,

	[VK_SNAPSHOT] = kKey_PrintScreen,  [VK_INSERT]  = kKey_Insert,
	[VK_HOME]     = kKey_Home,         [VK_PRIOR]   = kKey_PageUp,
	[VK_NEXT]     = kKey_PageDown,     [VK_DELETE]  = kKey_Delete,
	[VK_END]      = kKey_End,          [VK_RIGHT]   = kKey_Right,
	[VK_LEFT]     = kKey_Left,         [VK_DOWN]    = kKey_Down,
	[VK_UP]       = kKey_Up,           [VK_DIVIDE]  = kKey_Divide,
	[VK_MULTIPLY] = kKey_Multiply,     [VK_ADD]     = kKey_Plus,
	[VK_SUBTRACT] = kKey_Minus,        [VK_DECIMAL] = kKey_Period,
	[VK_OEM_3]    = kKey_BackTick,     [VK_CONTROL] = kKey_Ctrl,
	[VK_RETURN]   = kKey_Return,       [VK_ESCAPE]  = kKey_Escape,
	[VK_BACK]     = kKey_Backspace,    [VK_TAB]     = kKey_Tab,
	[VK_SPACE]    = kKey_Space,        [VK_SHIFT]   = kKey_Shift,
};

static kWinWindow *kWinGetWindow(kWindow id) {
	kWinWindow *window = id.ptr;
	return window;
}

static void kWinMakeWindowBorderResizable(kMediaIo *io, kWindow id) {
	kWinWindow *window = kWinGetWindow(id);
	DWORD dw_style     = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	SetWindowLongPtrW(window->wnd, GWL_STYLE, dw_style | WS_OVERLAPPEDWINDOW);
	SetWindowPos(window->wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

static void kWinMakeWindowBorderFixed(kMediaIo *io, kWindow id) {
	kWinWindow *window = kWinGetWindow(id);
	DWORD dw_style     = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	SetWindowLongPtrW(window->wnd, GWL_STYLE, dw_style & ~(WS_MAXIMIZEBOX | WS_THICKFRAME));
	SetWindowPos(window->wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

static bool kIsWindowFullscreen(kWindow id) {
	kWinWindow *window = kWinGetWindow(id);
	DWORD dw_style     = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	if (dw_style & WS_OVERLAPPEDWINDOW) {
		return false;
	}
	return true;
}

static LRESULT CALLBACK kWinHandleWindowsEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	kWinWindow *window = (kWinWindow *)GetWindowLongPtrW(wnd, GWLP_USERDATA);

	LRESULT res = 0;

	switch (msg) {
		case WM_ACTIVATE:
		{
			int state  = LOWORD(wparam);
			bool focus = (state == WA_ACTIVE || state == WA_CLICKACTIVE);
			kPushWindowFocusEvent(window->io, focus);
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		} break;

		case WM_STYLECHANGED:
		{
			if (wparam == GWL_STYLE) {
				STYLESTRUCT *style = (STYLESTRUCT *)lparam;
				if (style->styleNew & (WS_MAXIMIZEBOX | WS_THICKFRAME)) {
					kPushWindowBorderEvent(window->io, true);
				} else {
					kPushWindowBorderEvent(window->io, false);
				}
			}
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		} break;

		case WM_CLOSE:
		{
			kPushWindowCloseEvent(window->io);
		} break;

		case WM_SIZE:
		{
			window->resized = 1;
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		} break;

		case WM_DISPLAYCHANGE:
		{
			window->monitor_w = (int)LOWORD(lparam);
			window->monitor_h = (int)HIWORD(lparam);
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		} break;

		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		{
			bool down = (msg == WM_LBUTTONDOWN);
			kPushButtonEvent(window->io, kButton_Left, down);
		} break;

		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		{
			bool down = (msg == WM_RBUTTONDOWN);
			kPushButtonEvent(window->io, kButton_Right, down);
		} break;

		case WM_MBUTTONUP:
		case WM_MBUTTONDOWN:
		{
			bool down = (msg == WM_MBUTTONDOWN);
			kPushButtonEvent(window->io, kButton_Middle, down);
		} break;

		case WM_LBUTTONDBLCLK:
		{
			kPushDoubleClickEvent(window->io, kButton_Left);
		} break;

		case WM_RBUTTONDBLCLK:
		{
			kPushDoubleClickEvent(window->io, kButton_Right);
		} break;

		case WM_MBUTTONDBLCLK:
		{
			kPushDoubleClickEvent(window->io, kButton_Middle);
		} break;

		case WM_MOUSEWHEEL:
		{
			float v = (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
			kPushWheelEvent(window->io, 0, v);
		} break;

		case WM_MOUSEHWHEEL:
		{
			float h = (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
			kPushWheelEvent(window->io, h, 0);
		} break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			if (wparam < kArrayCount(VirtualKeyMap)) {
				kKey key    = VirtualKeyMap[wparam];
				bool repeat = HIWORD(lparam) & KF_REPEAT;
				bool down   = (msg == WM_KEYDOWN);
				kPushKeyEvent(window->io, key, down, repeat);
			}
		} break;

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			if (wparam == VK_F10) {
				bool repeat = HIWORD(lparam) & KF_REPEAT;
				bool down   = (msg == WM_KEYDOWN);
				kPushKeyEvent(window->io, kKey_F10, down, repeat);
			}
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		} break;

		case WM_CHAR:
		{
			/*if (window->TextLength == ArrayCount(window->Text)) {
				memcpy(window->Text, window->Text + 1, sizeof(u16) * (window->TextLength - 1));
			}
			window->Text[window->TextLength] = (u16)wparam;
			window->TextLength += 1;*/
		} break;

		case WM_DPICHANGED:
		{
			RECT *scissor = (RECT *)lparam;
			LONG  left    = scissor->left;
			LONG  top     = scissor->top;
			LONG  width   = scissor->right - scissor->left;
			LONG  height  = scissor->bottom - scissor->top;
			SetWindowPos(wnd, 0, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
		} break;

		default:
		{
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		} break;
	}

	return res;
}

static void kWinDestroyWindow(kWindow id) {
	kWinWindow *window = kWinGetWindow(id);
	DestroyWindow(window->wnd);
	kFree(window, sizeof(*window));
}

static kWindow kWinCreateWindow(kMediaIo *io, const char *mb_title, uint w, uint h, uint flags) {
	HMODULE     instance           = GetModuleHandleW(0);
	WNDCLASSEXW window_class       = { 0 };

	{
		window_class.cbSize        = sizeof(window_class);
		window_class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		window_class.lpfnWndProc   = kWinHandleWindowsEvent;
		window_class.hInstance     = instance;
		window_class.hIcon         = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, 0, 0, LR_SHARED);
		window_class.hCursor       = (HCURSOR)LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
		window_class.lpszClassName = L"KrWindowClass";
		RegisterClassExW(&window_class);
	}

	wchar_t title[2048] = L"KrWindow | Windows";

	if (mb_title) {
		MultiByteToWideChar(CP_UTF8, 0, (char *)mb_title, -1, title, kArrayCount(title));
	}

	int width      = CW_USEDEFAULT;
	int height     = CW_USEDEFAULT;

	DWORD style    = WS_OVERLAPPEDWINDOW;

	if (flags & kWindow_FixedBorder) {
		style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
	}

	if (w > 0 && h > 0) {
		RECT rect = {
			.left   = 0,
			.right  = w,
			.top    = 0,
			.bottom = h
		};
		AdjustWindowRect(&rect, style, FALSE);
		width  = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, window_class.lpszClassName, title, style,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, instance, 0);

	if (!hwnd) {
		kWinLogError(GetLastError(), "Windows", "Failed to create window");
		return (kWindow){ 0 };
	}

	kWinWindow *window = kAlloc(sizeof(kWinWindow));
	if (!window) return (kWindow){ 0 };

	memset(window, 0, sizeof(*window));

	window->wnd = hwnd;
	window->io  = io;

	SetWindowLongPtrW(window->wnd, GWLP_USERDATA, (LONG_PTR)window);

	ShowWindow(window->wnd, SW_SHOWNORMAL);
	UpdateWindow(window->wnd);

	kWindow id = { .ptr = window };

	if (flags & kWindow_Fullscreen) {
		kWinToggleWindowFullscreen(io, id);
	}

	RECT rect;
	GetClientRect(hwnd, &rect);
	GetWindowPlacement(hwnd, &window->placement);
	window->style = (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE);

	width  = rect.right - rect.left;
	height = rect.bottom - rect.top;

	kPushWindowResizeEvent(io, width, height, flags & kWindow_Fullscreen);

	MONITORINFO mi = { .cbSize = sizeof(mi) };
	GetMonitorInfoW(MonitorFromWindow(window->wnd, MONITOR_DEFAULTTOPRIMARY), &mi);
	window->monitor_w = mi.rcMonitor.right - mi.rcMonitor.left;
	window->monitor_h = mi.rcMonitor.bottom - mi.rcMonitor.top;

	return id;
}

static bool kWinPollEvents(kMediaIo *io, int *status) {
	kWinWindow *window = kWinGetWindow(io->window.handle);

	window->resized    = 0;

	MSG msg;
	while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			*status = (int)msg.wParam;
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	RECT rect;
	GetClientRect(window->wnd, &rect);

	if (window->resized) {
		bool fullscreen = kIsWindowFullscreen(io->window.handle);
		GetWindowPlacement(window->wnd, &window->placement);

		u32 width  = rect.right - rect.left;
		u32 height = rect.bottom - rect.top;
		kPushWindowResizeEvent(io, width, height, fullscreen);
	}

	CURSORINFO ci = { .cbSize = sizeof(ci) };
	if (GetCursorInfo(&ci) && (ci.flags == 0) && (io->window.flags & kWindow_Focused)) {
		kWinClipCursor(io, window, &rect);
	}

	POINT point = { .x = INFINITE, .y = INFINITE };
	GetCursorPos(&point);
	ScreenToClient(window->wnd, &point);

	kVec2i cursor = { .x = point.x, .y = io->window.height - point.y };

	kVec2  delta  = {
		.x = (float)((double)(cursor.x - io->mouse.cursor.x) / (double)window->monitor_w),
		.y = (float)((double)(cursor.y - io->mouse.cursor.y) / (double)window->monitor_h)
	};

	kPushCursorEvent(io, cursor, delta);

	uint mods = 0;
	if (GetKeyState(VK_LSHIFT) & 0x8000)   mods |= kKeyMod_LeftShift;
	if (GetKeyState(VK_RSHIFT) & 0x8000)   mods |= kKeyMod_RightShift;
	if (GetKeyState(VK_LCONTROL) & 0x8000) mods |= kKeyMod_LeftCtrl;
	if (GetKeyState(VK_RCONTROL) & 0x8000) mods |= kKeyMod_RightCtrl;
	if (GetKeyState(VK_LMENU) & 0x8000)    mods |= kKeyMod_LeftAlt;
	if (GetKeyState(VK_RMENU) & 0x8000)    mods |= kKeyMod_RightAlt;

	io->keyboard.mods = mods;

	return true;
}

static int kWinEventLoop(kMediaIo *io) {
	io->user.proc.load(io);

	int   status       = 0;
	float dt           = 1.0f / 60.0f;
	u64   counter      = kGetPerformanceCounter();
	u64   frequency    = kGetPerformanceFrequency();

	while (1) {
		kNextFrame(io);

		if (!kWinPollEvents(io, &status))
			break;

		io->user.proc.update(io, dt);

		if (io->window.closed)
			break;

		u64 new_counter = kGetPerformanceCounter();
		u64 counts      = new_counter - counter;
		counter         = new_counter;
		dt              = (float)(((1000000.0 * (double)counts) / (double)frequency) / 1000000.0);
	}

	io->user.proc.release(io);

	return status;
}

static void kWinPostTerminateEvent(kMediaIo *io, int status) {
	PostQuitMessage(status);
}

//
//
//

int kEventLoop(const kMediaUser user, const kMediaSpec *spec) {
	kMediaIo *io = kAlloc(sizeof(*io));

	if (!io) {
		kFatalError("Failed to allocate kMediaIo");
		return INT_MAX;
	}

	memset(io, 0, sizeof(*io));

	kMediaSpec zero_spec = {0};

	if (!spec) {
		spec = &zero_spec;
	}

	io->user          = user;
	io->window.handle = kWinCreateWindow(io, spec->title, spec->width, spec->height, spec->flags);

	if (spec->flags & kWindow_CursorHidden) {
		kWinHideCursor(io);
	}

	if (!io->user.proc.load)
		io->user.proc.load = kFallbackUserLoad;

	if (!io->user.proc.release)
		io->user.proc.release = kFallbackUserRelease;

	if (!io->user.proc.update)
		io->user.proc.update = kFallbackUserUpdate;

	io->platform.break_event_loop             = kWinPostTerminateEvent;
	io->platform.resize_window                = kWinResizeWindow;
	io->platform.toggle_fullscreen            = kWinToggleWindowFullscreen;
	io->platform.make_window_border_fixed     = kWinMakeWindowBorderFixed;
	io->platform.make_window_border_resizable = kWinMakeWindowBorderResizable;
	io->platform.hide_cursor                  = kWinHideCursor;
	io->platform.show_cursor                  = kWinShowCursor;

	int status = kWinEventLoop(io);

	kWinDestroyWindow(io->window.handle);

	return status;
}

//
// Main
//

extern void Main(int argc, const char **argv);

static int kMain(void) {
	if (IsWindows10OrGreater()) {
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	} else {
		SetProcessDPIAware();
	}

#if defined(K_BUILD_DEBUG) || defined(M_BUILD_DEVELOPER)
	AllocConsole();
#endif

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	kAssert(hr != RPC_E_CHANGED_MODE);

	if (hr != S_OK && hr != S_FALSE) {
		FatalAppExitW(0, L"Windows: Failed to initialize COM Library");
	}

	int    argc        = 0;
	const  char **argv = 0;

	HANDLE heap  = GetProcessHeap();
	LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &argc);

	argv = (const char **)HeapAlloc(heap, 0, sizeof(char *) * argc);
	if (argv) {
		for (int index = 0; index < argc; ++index) {
			int len = WideCharToMultiByte(CP_UTF8, 0, args[index], -1, 0, 0, 0, 0);
			argv[index] = (char *)HeapAlloc(heap, 0, len + 1);
			if (argv[index]) {
				WideCharToMultiByte(CP_UTF8, 0, args[index], -1, (LPSTR)argv[index], len + 1, 0, 0);
			} else {
				argv[index] = "";
			}
		}
	} else {
		argc = 0;
	}

	Main(argc, argv);

	CoUninitialize();

	return 0;
}

#if defined(K_BUILD_DEBUG) || defined(BUILD_DEVELOP) || defined(CONSOLE_APPLICATION)
#pragma comment(linker, "/subsystem:console" )
#else
#pragma comment(linker, "/subsystem:windows" )
#endif

// SUBSYSTEM:CONSOLE
int wmain() { return kMain(); }

// SUBSYSTEM:WINDOWS
int __stdcall wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd, int n) { return kMain(); }
