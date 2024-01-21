#include "kMedia.h"
#include "kMediaBackend.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <windowsx.h>
#include <malloc.h>
#include <ShellScalingApi.h>
#include <dwmapi.h>
#include <avrt.h>

#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>

#include "kResource.h"
#include "ImGui/imgui_impl_win32.h"

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

#ifdef CreateWindow
#undef CreateWindow
#endif

#ifndef IDI_KICON
#define IDI_KICON 101
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define DWMWCP_DEFAULT                 0
#define DWMWCP_DONOTROUND              1
#define DWMWCP_ROUND                   2
#define DWMWCP_ROUNDSMALL              3
#endif

#pragma comment(lib, "Avrt.lib")   // AvSetMmThreadCharacteristicsW
#pragma comment(lib, "Shcore.lib") // GetDpiForMonitor
#pragma comment(lib, "Dwmapi.lib") // DwmSetWindowAttribute

DEFINE_GUID(WIN32_CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(WIN32_IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(WIN32_IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(WIN32_IID_IMMNotificationClient, 0x7991eec9, 0x7e89, 0x4d85, 0x83, 0x90, 0x6c, 0x70, 0x3c, 0xec, 0x60,
            0xc0);
DEFINE_GUID(WIN32_IID_IMMEndpoint, 0x1BE09788, 0x6894, 0x4089, 0x85, 0x86, 0x9A, 0x2A, 0x6C, 0x26, 0x5A, 0xC5);
DEFINE_GUID(WIN32_IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(WIN32_IID_IAudioRenderClient, 0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);
DEFINE_GUID(WIN32_IID_IAudioCaptureClient, 0xc8adbd64, 0xe71e, 0x48a0, 0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17);
DEFINE_GUID(WIN32_KSDATAFORMAT_SUBTYPE_PCM, 0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(WIN32_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b,
            0x71);
DEFINE_GUID(WIN32_KSDATAFORMAT_SUBTYPE_WAVEFORMATEX, 0x00000000L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38,
            0x9b, 0x71);

//
// Window
//

typedef struct kWin32Window
{
	HWND            Wnd;
	u32             HighSurrogate;
	bool            RawInput;
	bool            Resized;
	bool            Captions;
	bool            DpiChanged;
	bool            Fullscreen;
	RECT            Border;
	DWORD           Style;
	WINDOWPLACEMENT Placement;
} kWin32Window;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//
//
//

static DWORD kGetWindowStyleFromFlags(u32 flags)
{
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if ((flags & kWindowStyle_FixedBorders) == 0)
	{
		style |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
	}
	return style;
}

//
//
//

static void kWin32_ResizeWindow(kWin32Window *window, u32 w, u32 h)
{
	kDebugTraceEx("Windows", "Resizing window to size %ux%u.", w, h);
	DWORD style = (DWORD)GetWindowLongPtrW(window->Wnd, GWL_STYLE);
	RECT  rect  = {.left = 0, .top = 0, .right = (LONG)w, .bottom = (LONG)h};
	UINT  dpi   = GetDpiForWindow(window->Wnd);
	AdjustWindowRectExForDpi(&rect, style, FALSE, 0, dpi);
	SetWindowPos(window->Wnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
	             SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

static void kWin32_ToggleWindowFullscreen(kWin32Window *window)
{
	if (!window->Fullscreen)
	{
		kLogInfoEx("Windows", "Switching to fullscreen mode.");

		DWORD       style = (DWORD)GetWindowLongPtrW(window->Wnd, GWL_STYLE);
		MONITORINFO mi    = {.cbSize = sizeof(mi)};
		if (GetWindowPlacement(window->Wnd, &window->Placement) &&
		    GetMonitorInfoW(MonitorFromWindow(window->Wnd, MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			SetWindowLongPtrW(window->Wnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(window->Wnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
			             mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
			             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
		window->Resized    = true;
		window->Fullscreen = true;
	}
	else
	{
		kLogInfoEx("Windows", "Restoring window from fullscreen mode.");

		SetWindowLongPtrW(window->Wnd, GWL_STYLE, window->Style);
		SetWindowPlacement(window->Wnd, &window->Placement);
		SetWindowPos(window->Wnd, NULL, 0, 0, 0, 0,
		             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		window->Resized    = true;
		window->Fullscreen = false;
	}
}

static void kWin32_ClipCursorToWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Clipping cursor to window.");

	RECT rect;
	GetClientRect(window->Wnd, &rect);

	POINT pt  = {rect.left, rect.top};
	POINT pt2 = {rect.right, rect.bottom};
	ClientToScreen(window->Wnd, &pt);
	ClientToScreen(window->Wnd, &pt2);

	SetRect(&rect, pt.x, pt.y, pt2.x, pt2.y);
	ClipCursor(&rect);

	POINT center_pt = {.x = (rect.right - rect.left) / 2, .y = (rect.bottom - rect.top) / 2};
	ClientToScreen(window->Wnd, &center_pt);
	SetCursorPos(center_pt.x, center_pt.y);
}

static void kWin32_ForceReleaseCursor(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Releasing cursor from window.");

	ShowCursor(TRUE);
	ClipCursor(NULL);

	RAWINPUTDEVICE rid = {.usUsagePage = 0x1, .usUsage = 0x2, .dwFlags = 0, .hwndTarget = window->Wnd};
	window->RawInput   = RegisterRawInputDevices(&rid, 1, sizeof(rid));

	kAddWindowCursorReleaseEvent();
}

static void kWin32_ForceCaptureCursor(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Capturing cursor in window.");

	if (GetActiveWindow() == window->Wnd)
	{
		ShowCursor(FALSE);
	}

	RAWINPUTDEVICE rid = {.usUsagePage = 0x1, .usUsage = 0x2, .dwFlags = RIDEV_REMOVE, .hwndTarget = window->Wnd};
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	kAddWindowCursorCaptureEvent();
	window->RawInput = false;
}

static void kWin32_ReleaseCursor(kWin32Window *window)
{
	if (!kIsCursorCaptured())
		return;
	kWin32_ForceReleaseCursor(window);
}

static void kWin32_CaptureCursor(kWin32Window *window)
{
	if (kIsCursorCaptured())
		return;
	kWin32_ForceCaptureCursor(window);
}

static int kWin32_GetWindowCaptionSize(kWin32Window *window)
{
	return window->Border.top;
}

static void kWin32_MaximizeWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Maximizing window.");
	ShowWindow(window->Wnd, SW_MAXIMIZE);
}

static void kWin32_RestoreWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Restoring window.");
	ShowWindow(window->Wnd, SW_RESTORE);
}

static void kWin32_MinimizeWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Minimizing window.");
	ShowWindow(window->Wnd, SW_MINIMIZE);
}

static void kWin32_CloseWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Closing window.");
	PostMessageW(window->Wnd, WM_CLOSE, 0, 0);
}

//
//
//

static kKey  VirtualKeyMap[255];
static DWORD InvVirtualKeyMap[255];

static void  kWin32_MapVirutalKeys(void)
{
	VirtualKeyMap[(int)'A']                  = kKey::A;
	VirtualKeyMap[(int)'B']                  = kKey::B;
	VirtualKeyMap[(int)'C']                  = kKey::C;
	VirtualKeyMap[(int)'D']                  = kKey::D;
	VirtualKeyMap[(int)'E']                  = kKey::E;
	VirtualKeyMap[(int)'F']                  = kKey::F;
	VirtualKeyMap[(int)'G']                  = kKey::G;
	VirtualKeyMap[(int)'H']                  = kKey::H;
	VirtualKeyMap[(int)'I']                  = kKey::I;
	VirtualKeyMap[(int)'J']                  = kKey::J;
	VirtualKeyMap[(int)'K']                  = kKey::K;
	VirtualKeyMap[(int)'L']                  = kKey::L;
	VirtualKeyMap[(int)'M']                  = kKey::M;
	VirtualKeyMap[(int)'N']                  = kKey::N;
	VirtualKeyMap[(int)'O']                  = kKey::O;
	VirtualKeyMap[(int)'P']                  = kKey::P;
	VirtualKeyMap[(int)'Q']                  = kKey::Q;
	VirtualKeyMap[(int)'R']                  = kKey::R;
	VirtualKeyMap[(int)'S']                  = kKey::S;
	VirtualKeyMap[(int)'T']                  = kKey::T;
	VirtualKeyMap[(int)'U']                  = kKey::U;
	VirtualKeyMap[(int)'V']                  = kKey::V;
	VirtualKeyMap[(int)'W']                  = kKey::W;
	VirtualKeyMap[(int)'X']                  = kKey::X;
	VirtualKeyMap[(int)'Y']                  = kKey::Y;
	VirtualKeyMap[(int)'Z']                  = kKey::Z;

	VirtualKeyMap[(int)'0']                  = kKey::N0;
	VirtualKeyMap[(int)'1']                  = kKey::N1;
	VirtualKeyMap[(int)'2']                  = kKey::N2;
	VirtualKeyMap[(int)'3']                  = kKey::N3;
	VirtualKeyMap[(int)'4']                  = kKey::N4;
	VirtualKeyMap[(int)'5']                  = kKey::N5;
	VirtualKeyMap[(int)'6']                  = kKey::N6;
	VirtualKeyMap[(int)'7']                  = kKey::N7;
	VirtualKeyMap[(int)'8']                  = kKey::N8;
	VirtualKeyMap[(int)'9']                  = kKey::N9;

	VirtualKeyMap[VK_NUMPAD0]                = kKey::N0;
	VirtualKeyMap[VK_NUMPAD1]                = kKey::N1;
	VirtualKeyMap[VK_NUMPAD2]                = kKey::N2;
	VirtualKeyMap[VK_NUMPAD3]                = kKey::N3;
	VirtualKeyMap[VK_NUMPAD4]                = kKey::N4;
	VirtualKeyMap[VK_NUMPAD5]                = kKey::N5;
	VirtualKeyMap[VK_NUMPAD6]                = kKey::N6;
	VirtualKeyMap[VK_NUMPAD7]                = kKey::N7;
	VirtualKeyMap[VK_NUMPAD8]                = kKey::N8;
	VirtualKeyMap[VK_NUMPAD9]                = kKey::N9;

	VirtualKeyMap[VK_F1]                     = kKey::F1;
	VirtualKeyMap[VK_F2]                     = kKey::F2;
	VirtualKeyMap[VK_F3]                     = kKey::F3;
	VirtualKeyMap[VK_F4]                     = kKey::F4;
	VirtualKeyMap[VK_F5]                     = kKey::F5;
	VirtualKeyMap[VK_F6]                     = kKey::F6;
	VirtualKeyMap[VK_F7]                     = kKey::F7;
	VirtualKeyMap[VK_F8]                     = kKey::F8;
	VirtualKeyMap[VK_F9]                     = kKey::F9;
	VirtualKeyMap[VK_F10]                    = kKey::F10;
	VirtualKeyMap[VK_F11]                    = kKey::F11;
	VirtualKeyMap[VK_F12]                    = kKey::F12;

	VirtualKeyMap[VK_SNAPSHOT]               = kKey::PrintScreen;
	VirtualKeyMap[VK_INSERT]                 = kKey::Insert;
	VirtualKeyMap[VK_HOME]                   = kKey::Home;
	VirtualKeyMap[VK_PRIOR]                  = kKey::PageUp;
	VirtualKeyMap[VK_NEXT]                   = kKey::PageDown;
	VirtualKeyMap[VK_DELETE]                 = kKey::Delete;
	VirtualKeyMap[VK_END]                    = kKey::End;
	VirtualKeyMap[VK_RIGHT]                  = kKey::Right;
	VirtualKeyMap[VK_LEFT]                   = kKey::Left;
	VirtualKeyMap[VK_DOWN]                   = kKey::Down;
	VirtualKeyMap[VK_UP]                     = kKey::Up;
	VirtualKeyMap[VK_DIVIDE]                 = kKey::Divide;
	VirtualKeyMap[VK_MULTIPLY]               = kKey::Multiply;
	VirtualKeyMap[VK_ADD]                    = kKey::Plus;
	VirtualKeyMap[VK_SUBTRACT]               = kKey::Minus;
	VirtualKeyMap[VK_DECIMAL]                = kKey::Period;
	VirtualKeyMap[VK_OEM_3]                  = kKey::BackTick;
	VirtualKeyMap[VK_CONTROL]                = kKey::Ctrl;
	VirtualKeyMap[VK_RETURN]                 = kKey::Return;
	VirtualKeyMap[VK_ESCAPE]                 = kKey::Escape;
	VirtualKeyMap[VK_BACK]                   = kKey::Backspace;
	VirtualKeyMap[VK_TAB]                    = kKey::Tab;
	VirtualKeyMap[VK_SPACE]                  = kKey::Space;
	VirtualKeyMap[VK_SHIFT]                  = kKey::Shift;

	InvVirtualKeyMap[(int)kKey::A]           = 'A';
	InvVirtualKeyMap[(int)kKey::B]           = 'B';
	InvVirtualKeyMap[(int)kKey::C]           = 'C';
	InvVirtualKeyMap[(int)kKey::D]           = 'D';
	InvVirtualKeyMap[(int)kKey::E]           = 'E';
	InvVirtualKeyMap[(int)kKey::F]           = 'F';
	InvVirtualKeyMap[(int)kKey::G]           = 'G';
	InvVirtualKeyMap[(int)kKey::H]           = 'H';
	InvVirtualKeyMap[(int)kKey::I]           = 'I';
	InvVirtualKeyMap[(int)kKey::J]           = 'J';
	InvVirtualKeyMap[(int)kKey::K]           = 'K';
	InvVirtualKeyMap[(int)kKey::L]           = 'L';
	InvVirtualKeyMap[(int)kKey::M]           = 'M';
	InvVirtualKeyMap[(int)kKey::N]           = 'N';
	InvVirtualKeyMap[(int)kKey::O]           = 'O';
	InvVirtualKeyMap[(int)kKey::P]           = 'P';
	InvVirtualKeyMap[(int)kKey::Q]           = 'Q';
	InvVirtualKeyMap[(int)kKey::R]           = 'R';
	InvVirtualKeyMap[(int)kKey::S]           = 'S';
	InvVirtualKeyMap[(int)kKey::T]           = 'T';
	InvVirtualKeyMap[(int)kKey::U]           = 'U';
	InvVirtualKeyMap[(int)kKey::V]           = 'V';
	InvVirtualKeyMap[(int)kKey::W]           = 'W';
	InvVirtualKeyMap[(int)kKey::X]           = 'X';
	InvVirtualKeyMap[(int)kKey::Y]           = 'Y';
	InvVirtualKeyMap[(int)kKey::Z]           = 'Z';

	InvVirtualKeyMap[(int)kKey::N0]          = '0';
	InvVirtualKeyMap[(int)kKey::N1]          = '1';
	InvVirtualKeyMap[(int)kKey::N2]          = '2';
	InvVirtualKeyMap[(int)kKey::N3]          = '3';
	InvVirtualKeyMap[(int)kKey::N4]          = '4';
	InvVirtualKeyMap[(int)kKey::N5]          = '5';
	InvVirtualKeyMap[(int)kKey::N6]          = '6';
	InvVirtualKeyMap[(int)kKey::N7]          = '7';
	InvVirtualKeyMap[(int)kKey::N8]          = '8';
	InvVirtualKeyMap[(int)kKey::N9]          = '9';

	InvVirtualKeyMap[(int)kKey::N0]          = VK_NUMPAD0;
	InvVirtualKeyMap[(int)kKey::N1]          = VK_NUMPAD1;
	InvVirtualKeyMap[(int)kKey::N2]          = VK_NUMPAD2;
	InvVirtualKeyMap[(int)kKey::N3]          = VK_NUMPAD3;
	InvVirtualKeyMap[(int)kKey::N4]          = VK_NUMPAD4;
	InvVirtualKeyMap[(int)kKey::N5]          = VK_NUMPAD5;
	InvVirtualKeyMap[(int)kKey::N6]          = VK_NUMPAD6;
	InvVirtualKeyMap[(int)kKey::N7]          = VK_NUMPAD7;
	InvVirtualKeyMap[(int)kKey::N8]          = VK_NUMPAD8;
	InvVirtualKeyMap[(int)kKey::N9]          = VK_NUMPAD9;

	InvVirtualKeyMap[(int)kKey::F1]          = VK_F1;
	InvVirtualKeyMap[(int)kKey::F2]          = VK_F2;
	InvVirtualKeyMap[(int)kKey::F3]          = VK_F3;
	InvVirtualKeyMap[(int)kKey::F4]          = VK_F4;
	InvVirtualKeyMap[(int)kKey::F5]          = VK_F5;
	InvVirtualKeyMap[(int)kKey::F6]          = VK_F6;
	InvVirtualKeyMap[(int)kKey::F7]          = VK_F7;
	InvVirtualKeyMap[(int)kKey::F8]          = VK_F8;
	InvVirtualKeyMap[(int)kKey::F9]          = VK_F9;
	InvVirtualKeyMap[(int)kKey::F10]         = VK_F1;
	InvVirtualKeyMap[(int)kKey::F11]         = VK_F11;
	InvVirtualKeyMap[(int)kKey::F12]         = VK_F1;

	InvVirtualKeyMap[(int)kKey::PrintScreen] = VK_SNAPSHOT;
	InvVirtualKeyMap[(int)kKey::Insert]      = VK_INSERT;
	InvVirtualKeyMap[(int)kKey::Home]        = VK_HOME;
	InvVirtualKeyMap[(int)kKey::PageUp]      = VK_PRIOR;
	InvVirtualKeyMap[(int)kKey::PageDown]    = VK_NEXT;
	InvVirtualKeyMap[(int)kKey::Delete]      = VK_DELETE;
	InvVirtualKeyMap[(int)kKey::End]         = VK_END;
	InvVirtualKeyMap[(int)kKey::Right]       = VK_RIGHT;
	InvVirtualKeyMap[(int)kKey::Left]        = VK_LEFT;
	InvVirtualKeyMap[(int)kKey::Down]        = VK_DOWN;
	InvVirtualKeyMap[(int)kKey::Up]          = VK_UP;
	InvVirtualKeyMap[(int)kKey::Divide]      = VK_DIVIDE;
	InvVirtualKeyMap[(int)kKey::Multiply]    = VK_MULTIPLY;
	InvVirtualKeyMap[(int)kKey::Plus]        = VK_ADD;
	InvVirtualKeyMap[(int)kKey::Minus]       = VK_SUBTRACT;
	InvVirtualKeyMap[(int)kKey::Period]      = VK_DECIMAL;
	InvVirtualKeyMap[(int)kKey::BackTick]    = VK_OEM_3;
	InvVirtualKeyMap[(int)kKey::Ctrl]        = VK_CONTROL;
	InvVirtualKeyMap[(int)kKey::Return]      = VK_RETURN;
	InvVirtualKeyMap[(int)kKey::Escape]      = VK_ESCAPE;
	InvVirtualKeyMap[(int)kKey::Backspace]   = VK_BACK;
	InvVirtualKeyMap[(int)kKey::Tab]         = VK_TAB;
	InvVirtualKeyMap[(int)kKey::Space]       = VK_SPACE;
	InvVirtualKeyMap[(int)kKey::Shift]       = VK_SHIFT;
}

static u32 kWin32_GetKeyModFlags(void)
{
	u32 mods = 0;
	if (GetKeyState(VK_LSHIFT) & 0x8000)
		mods |= kKeyMod_LeftShift;
	if (GetKeyState(VK_RSHIFT) & 0x8000)
		mods |= kKeyMod_RightShift;
	if (GetKeyState(VK_LCONTROL) & 0x8000)
		mods |= kKeyMod_LeftCtrl;
	if (GetKeyState(VK_RCONTROL) & 0x8000)
		mods |= kKeyMod_RightCtrl;
	if (GetKeyState(VK_LMENU) & 0x8000)
		mods |= kKeyMod_LeftAlt;
	if (GetKeyState(VK_RMENU) & 0x8000)
		mods |= kKeyMod_RightAlt;
	return mods;
}

static LRESULT kWin32_HitTestNonClientArea(kWin32Window *window, WPARAM wparam, LPARAM lparam)
{
	POINT cursor = {.x = GET_X_LPARAM(lparam), .y = GET_Y_LPARAM(lparam)};

	UINT  dpi    = GetDpiForWindow(window->Wnd);

	RECT  rc;
	GetWindowRect(window->Wnd, &rc);

	RECT frame = {0};
	AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, 0, dpi);

	USHORT row           = 1;
	USHORT col           = 1;
	bool   border_resize = false;

	if (cursor.y >= rc.top && cursor.y < rc.top + window->Border.top)
	{
		border_resize = (cursor.y < (rc.top - frame.top));
		row           = 0;
	}
	else if (cursor.y < rc.bottom && cursor.y >= rc.bottom - window->Border.bottom)
	{
		row = 2;
	}

	if (cursor.x >= rc.left && cursor.x < rc.left + window->Border.left)
	{
		col = 0;
	}
	else if (cursor.x < rc.right && cursor.x >= rc.right - window->Border.right)
	{
		col = 2;
	}

	LRESULT hit_tests[3][3] = {
		{HTTOPLEFT, border_resize ? HTTOP : HTCAPTION, HTTOPRIGHT},
		{HTLEFT, HTNOWHERE, HTRIGHT},
		{HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT},
	};

	return hit_tests[row][col];
}

static bool kWin32_CustomCaptionsWndProc(kWin32Window *window, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *result)
{
	*result      = 0;
	bool handled = DwmDefWindowProc(window->Wnd, msg, wparam, lparam, result);

	if (msg == WM_ACTIVATE)
	{
		MARGINS margins = {0};
		DwmExtendFrameIntoClientArea(window->Wnd, &margins);
		return false;
	}

	if ((msg == WM_NCCALCSIZE) && (wparam == TRUE))
	{
		*result = 0;
		return true;
	}

	if ((msg == WM_NCHITTEST) && (*result == 0))
	{
		*result = kWin32_HitTestNonClientArea(window, wparam, lparam);
		if (*result != HTNOWHERE)
		{
			return true;
		}
	}

	return handled;
}

static LRESULT kWin32_WndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifndef IMGUI_DISABLE
	if (ImGui_ImplWin32_WndProcHandler(wnd, msg, wparam, lparam))
		return 0;
#endif

	kWin32Window *window = (kWin32Window *)GetWindowLongPtrW(wnd, GWLP_USERDATA);

	if (window && window->Captions)
	{
		LRESULT res = 0;
		if (kWin32_CustomCaptionsWndProc(window, msg, wparam, lparam, &res))
			return res;
	}

	switch (msg)
	{
		case WM_ACTIVATE:
		{
			int  state   = LOWORD(wparam);
			bool focused = (state == WA_ACTIVE || state == WA_CLICKACTIVE);
			kAddWindowActivateEvent(focused);

			if (kIsCursorCaptured())
			{
				if (focused)
				{
					kWin32_ForceCaptureCursor(window);
				}
				else
				{
					kWin32_ForceReleaseCursor(window);
				}
			}

			kDebugTraceEx("Windows", "Window focus %s.", focused ? "gained" : "lost");

			return DefWindowProcW(wnd, msg, wparam, lparam);
		}

		case WM_CLOSE:
		{
			kAddWindowCloseEvent();
			return 0;
		}

		case WM_SIZE:
		{
			if (kIsCursorCaptured() && kIsWindowActive())
			{
				kWin32_ClipCursorToWindow(window);
			}

			if (wparam == SIZE_MAXIMIZED)
			{
				kAddWindowMaximizeEvent();
			}
			else if (wparam == SIZE_RESTORED)
			{
				kAddWindowRestoreEvent();
			}

			window->Resized = true;
			return 0;
		}

		case WM_MOUSELEAVE:
		{
			kAddCursorLeaveEvent();
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			if (!kIsCursorHovered())
			{
				TRACKMOUSEEVENT tme = {.cbSize = sizeof(tme), .dwFlags = TME_LEAVE, .hwndTrack = wnd};
				TrackMouseEvent(&tme);
				kAddCursorEnterEvent();
			}

			if (!kIsCursorCaptured() || !window->RawInput)
			{
				RECT rc;
				GetClientRect(window->Wnd, &rc);
				int    h      = rc.bottom - rc.top;
				int    x      = GET_X_LPARAM(lparam);
				int    y      = h - GET_Y_LPARAM(lparam);
				kVec2i cursor = kVec2i(x, y);
				kAddCursorEvent(cursor);
			}

			return 0;
		}

		case WM_INPUT:
		{
			HRAWINPUT hri      = (HRAWINPUT)lparam;

			UINT      rid_size = 0;
			GetRawInputData(hri, RID_INPUT, 0, &rid_size, sizeof(RAWINPUTHEADER));

			RAWINPUT *input = rid_size ? (RAWINPUT *)_alloca(rid_size) : nullptr;
			if (!input)
				break;

			if (GetRawInputData(hri, RID_INPUT, input, &rid_size, sizeof(RAWINPUTHEADER) != rid_size))
				break;

			if (input->header.dwType != RIM_TYPEMOUSE)
				break;

			RAWMOUSE *mouse = &input->data.mouse;
			UINT      dpi   = GetDpiForWindow(wnd);

			if ((mouse->usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE)
			{
				bool  isvirtual = (mouse->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
				int   width     = GetSystemMetricsForDpi(isvirtual ? SM_CXVIRTUALSCREEN : SM_CXSCREEN, dpi);
				int   height    = GetSystemMetricsForDpi(isvirtual ? SM_CYVIRTUALSCREEN : SM_CYSCREEN, dpi);
				int   abs_x     = (int)((mouse->lLastX / 65535.0f) * width);
				int   abs_y     = (int)((mouse->lLastY / 65535.0f) * height);
				POINT pt        = {.x = abs_x, .y = abs_y};
				ScreenToClient(wnd, &pt);

				RECT rc;
				GetClientRect(window->Wnd, &rc);
				int    wheight = rc.bottom - rc.top;
				kVec2i cursor  = kVec2i(pt.x, wheight - pt.y);
				kAddCursorEvent(cursor);
			}
			else if (mouse->lLastX != 0 || mouse->lLastY != 0)
			{
				int    rel_x = mouse->lLastX;
				int    rel_y = mouse->lLastY;
				kVec2i delta = kVec2i(rel_x, -rel_y);
				kAddCursorDeltaEvent(delta);
			}

			return 0;
		}

		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		{
			kAddButtonEvent(kButton::Left, msg == WM_LBUTTONDOWN);
			return 0;
		}

		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		{
			kAddButtonEvent(kButton::Right, msg == WM_RBUTTONDOWN);
			return 0;
		}

		case WM_MBUTTONUP:
		case WM_MBUTTONDOWN:
		{
			kAddButtonEvent(kButton::Middle, msg == WM_MBUTTONDOWN);
			return 0;
		}

		case WM_LBUTTONDBLCLK:
		{
			kAddDoubleClickEvent(kButton::Left);
			return 0;
		}

		case WM_RBUTTONDBLCLK:
		{
			kAddDoubleClickEvent(kButton::Right);
			return 0;
		}

		case WM_MBUTTONDBLCLK:
		{
			kAddDoubleClickEvent(kButton::Middle);
			return 0;
		}

		case WM_MOUSEWHEEL:
		{
			float v = (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
			kAddWheelEvent(0, v);
			return 0;
		}

		case WM_MOUSEHWHEEL:
		{
			float h = (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA;
			kAddWheelEvent(h, 0);
			return 0;
		}

		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			if (wparam < kArrayCount(VirtualKeyMap))
			{
				kKey key    = VirtualKeyMap[wparam];
				bool repeat = HIWORD(lparam) & KF_REPEAT;
				bool down   = (msg == WM_KEYDOWN);
				kAddKeyEvent(key, down, repeat);
			}
			return 0;
		}

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			if (wparam == VK_F10)
			{
				bool repeat = HIWORD(lparam) & KF_REPEAT;
				bool down   = (msg == WM_KEYDOWN);
				kAddKeyEvent(kKey::F10, down, repeat);
			}
			return DefWindowProcW(wnd, msg, wparam, lparam);
		}

		case WM_CHAR:
		{
			if (IS_HIGH_SURROGATE(wparam))
			{
				window->HighSurrogate = (u32)(wparam & 0xFFFF);
			}
			else
			{
				if (IS_LOW_SURROGATE(wparam))
				{
					if (window->HighSurrogate)
					{
						u32 codepoint = 0;
						codepoint += (window->HighSurrogate - 0xD800) << 10;
						codepoint += (u16)wparam - 0xDC00;
						codepoint += 0x10000;
						u32 mods = kWin32_GetKeyModFlags();
						kAddTextInputEvent(codepoint, mods);
					}
				}
				else
				{
					u32 mods = kWin32_GetKeyModFlags();
					kAddTextInputEvent((u32)wparam, mods);
				}
				window->HighSurrogate = 0;
			}
			return 0;
		}

		case WM_DPICHANGED:
		{
			RECT *scissor = (RECT *)lparam;
			LONG  left    = scissor->left;
			LONG  top     = scissor->top;
			LONG  width   = scissor->right - scissor->left;
			LONG  height  = scissor->bottom - scissor->top;
			SetWindowPos(wnd, 0, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
			if (window)
				window->DpiChanged = true;
			return 0;
		}

		default:
		{
			return DefWindowProcW(wnd, msg, wparam, lparam);
		}
	}

	return 0;
}

//
// Audio
//

enum kAudioCommand
{
	kAudioCommand_Update,
	kAudioCommand_Resume,
	kAudioCommand_Pause,
	kAudioCommand_Reset,
	kAudioCommand_SwitchDevice,
	kAudioCommand_Terminate,
	kAudioCommand_Count
};

enum kAudioEndpointKind
{
	kAudioEndpointKind_Render,
	kAudioEndpointKind_Capture,
	kAudioEndpointKind_Count,
};

//class kWin32NotificationClient : IMMNotificationClient
//{
//  public:
//	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
//	{
//		if (memcmp(&WIN32_IID_IUnknown, (void *)&riid, sizeof(GUID)) == 0)
//		{
//			*ppvObject = (IUnknown *)this;
//		}
//		else if (memcmp(&WIN32_IID_IMMNotificationClient, (void *)&riid, sizeof(GUID)) == 0)
//		{
//			*ppvObject = (IMMNotificationClient *)this;
//		}
//		else
//		{
//			*ppvObject = 0;
//			return E_NOINTERFACE;
//		}
//		return S_OK;
//	}
//
//	virtual ULONG STDMETHODCALLTYPE AddRef(void)
//	{
//		return 1;
//	}
//
//	virtual ULONG STDMETHODCALLTYPE Release(void)
//	{
//		return 1;
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(_In_ LPCWSTR dev_id, _In_ DWORD state)
//	{}
//
//	virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(_In_ LPCWSTR dev_id)
//	{
//		return S_OK;
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(_In_ LPCWSTR pwstrDeviceId)
//	{
//		return S_OK;
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(_In_ EDataFlow flow, _In_ ERole role,
//	                                                         _In_opt_ LPCWSTR dev_id)
//	{}
//
//	virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(_In_ LPCWSTR dev_id, _In_ const PROPERTYKEY key)
//	{}
//};
//
//static DWORD WINAPI PL_AudioThread(LPVOID param)
//{
//	kAudioEndpointKind kind   = (kAudioEndpointKind)(int)param;
//
//	HANDLE             thread = GetCurrentThread();
//	SetThreadDescription(thread, kind == kAudioEndpointKind_Render ? L"Audio Render Thread" : L"Audio Capture Thread");
//
//	DWORD             task_index;
//	HANDLE            avrt     = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);
//
//	PL_AudioEndpoint *endpoint = &Audio.Endpoints[kind];
//
//	PL_AudioEndpoint_RestartDevice(kind);
//
//	HRESULT hr;
//
//	while (1)
//	{
//		DWORD wait = WaitForMultipleObjects(kAudioCommand_Count, endpoint->Commands, FALSE, INFINITE);
//
//		if (wait == WAIT_OBJECT_0 + kAudioCommand_Update)
//		{
//			if (kind == kAudioEndpointKind_Render)
//				PL_AudioEndpoint_RenderFrames();
//			else
//				PL_AudioEndpoint_CaptureFrames();
//		}
//		else if (wait == WAIT_OBJECT_0 + kAudioCommand_Resume)
//		{
//			InterlockedExchange(&endpoint->Resumed, 1);
//			if (!endpoint->DeviceLost)
//			{
//				hr = endpoint->Client->lpVtbl->Start(endpoint->Client);
//				if (SUCCEEDED(hr) || hr == AUDCLNT_E_NOT_STOPPED)
//				{
//					PostThreadMessageW(Audio.ParentThreadId, PL_WM_AUDIO_RESUMED, PL_AudioEndpoint_Render, 0);
//				}
//				else
//				{
//					InterlockedExchange(&endpoint->DeviceLost, 1);
//				}
//			}
//		}
//		else if (wait == WAIT_OBJECT_0 + kAudioCommand_Pause)
//		{
//			InterlockedExchange(&endpoint->Resumed, 0);
//			if (!endpoint->DeviceLost)
//			{
//				hr = endpoint->Client->lpVtbl->Stop(endpoint->Client);
//				if (SUCCEEDED(hr))
//				{
//					if (!PostThreadMessageW(Audio.ParentThreadId, PL_WM_AUDIO_PAUSED, PL_AudioEndpoint_Render, 0))
//					{
//						kTriggerBreakpoint();
//					}
//				}
//				else
//				{
//					InterlockedExchange(&endpoint->DeviceLost, 1);
//				}
//			}
//		}
//		else if (wait == WAIT_OBJECT_0 + kAudioCommand_Reset)
//		{
//			LONG resumed = InterlockedExchange(&endpoint->Resumed, 0);
//			if (!endpoint->DeviceLost)
//			{
//				if (resumed)
//				{
//					endpoint->Client->lpVtbl->Stop(endpoint->Client);
//				}
//				hr = endpoint->Client->lpVtbl->Reset(endpoint->Client);
//				if (FAILED(hr))
//				{
//					InterlockedExchange(&endpoint->DeviceLost, 1);
//				}
//			}
//		}
//		else if (wait == WAIT_OBJECT_0 + kAudioCommand_SwitchDevice)
//		{
//			PL_AudioEndpoint_RestartDevice(kind);
//		}
//		else if (wait == WAIT_OBJECT_0 + kAudioCommand_Terminate)
//		{
//			break;
//		}
//	}
//
//	AvRevertMmThreadCharacteristics(avrt);
//
//	return 0;
//}

//
//
//

typedef struct kWin32Backend
{
	kWin32Window Window;
	HANDLE       AvrtHandle;
} kWin32Backend;

static kWin32Backend g_Win32;

//
//
//

static void kWin32_DestroyWindow(void)
{
	kLogInfoEx("Windows", "Destroying window.");

#ifndef IMGUI_DISABLE
	ImGui_ImplWin32_Shutdown();
#endif

	DestroyWindow(g_Win32.Window.Wnd);
	memset(&g_Win32.Window, 0, sizeof(g_Win32.Window));
}

static void *kWin32_CreateWindow(kWindowState *ws, const kWindowSpec &spec)
{
	HMODULE     instance    = GetModuleHandleW(0);

	WNDCLASSEXW wnd_class   = {0};
	HICON       icon        = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_KICON), IMAGE_ICON, 0, 0, LR_SHARED);
	HICON       icon_sm     = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_KICON), IMAGE_ICON, 0, 0, LR_SHARED);
	HICON       win_icon    = (HICON)LoadImageW(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);

	wnd_class.cbSize        = sizeof(wnd_class);
	wnd_class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wnd_class.lpfnWndProc   = kWin32_WndProc;
	wnd_class.hInstance     = instance;
	wnd_class.hIcon         = icon ? icon : win_icon;
	wnd_class.hIconSm       = icon_sm ? icon_sm : win_icon;
	wnd_class.hCursor       = (HCURSOR)LoadImageW(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
	wnd_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	wnd_class.lpszClassName = L"kWindowClass";
	RegisterClassExW(&wnd_class);

	wchar_t title[2048] = L"kWindow | Windows";

	if (spec.Title.Count)
	{
		int len    = MultiByteToWideChar(CP_UTF8, 0, (char *)spec.Title.Items, (int)spec.Title.Count, title,
		                                 kArrayCount(title) - 1);
		title[len] = 0;
	}

	kLogInfoEx("Windows", "Creating window: %S.", title);

	int width  = CW_USEDEFAULT;
	int height = CW_USEDEFAULT;

	memset(ws, 0, sizeof(*ws));

	DWORD style = kGetWindowStyleFromFlags(spec.Flags);

	if (spec.Width > 0 && spec.Height > 0)
	{
		POINT    origin   = {.x = 0, .y = 0};
		HMONITOR mprimary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
		UINT     xdpi     = 0;
		UINT     ydpi     = 0;

		GetDpiForMonitor(mprimary, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);

		RECT rect;
		rect.left   = 0;
		rect.top    = 0;
		rect.right  = spec.Width;
		rect.bottom = spec.Height;

		AdjustWindowRectExForDpi(&rect, style, FALSE, 0, ydpi);
		width  = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	g_Win32.Window.Style    = style;
	g_Win32.Window.Captions = (spec.Flags & kWindowStyle_DisableCaptions);

	g_Win32.Window.Wnd      = CreateWindowExW(WS_EX_APPWINDOW, wnd_class.lpszClassName, title, style, CW_USEDEFAULT,
	                                          CW_USEDEFAULT, width, height, 0, 0, instance, 0);

	if (!g_Win32.Window.Wnd)
	{
		kLogHresultError(GetLastError(), "Windows", "Failed to create window");
		return 0;
	}

	BOOL dark = TRUE;
	DwmSetWindowAttribute(g_Win32.Window.Wnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

	if (spec.Flags & kWindowStyle_ForceSharpCorners)
	{
		LONG corner = DWMWCP_DONOTROUND;
		DwmSetWindowAttribute(g_Win32.Window.Wnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
	}

	UINT dpi = GetDpiForWindow(g_Win32.Window.Wnd);
	SetRectEmpty(&g_Win32.Window.Border);
	AdjustWindowRectExForDpi(&g_Win32.Window.Border, g_Win32.Window.Style, FALSE, WS_EX_APPWINDOW, dpi);
	g_Win32.Window.Border.left *= -1;
	g_Win32.Window.Border.top *= -1;

	SetWindowLongPtrW(g_Win32.Window.Wnd, GWLP_USERDATA, (LONG_PTR)&g_Win32.Window);
	SetWindowPos(g_Win32.Window.Wnd, NULL, 0, 0, 0, 0,
	             SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

	ShowWindow(g_Win32.Window.Wnd, SW_SHOWNORMAL);
	UpdateWindow(g_Win32.Window.Wnd);

	if (spec.Flags & kWindowStyle_Fullscreen)
	{
		kWin32_ToggleWindowFullscreen(&g_Win32.Window);
	}

#ifndef IMGUI_DISABLE
	ImGui_ImplWin32_Init(g_Win32.Window.Wnd);
#endif

	//
	//
	//

	RECT rc = {0};
	GetClientRect(g_Win32.Window.Wnd, &rc);

	ws->Width  = rc.right - rc.left;
	ws->Height = rc.bottom - rc.top;

	if (GetActiveWindow() == g_Win32.Window.Wnd)
	{
		ws->Flags[kWindow_Active] = true;
	}

	POINT pt = {};
	GetCursorPos(&pt);
	ScreenToClient(g_Win32.Window.Wnd, &pt);

	if (PtInRect(&rc, pt))
	{
		ws->Flags[kWindow_Hovered] = true;
	}

	TRACKMOUSEEVENT tme = {.cbSize = sizeof(tme), .dwFlags = TME_LEAVE, .hwndTrack = g_Win32.Window.Wnd};
	TrackMouseEvent(&tme);

	ws->Flags[kWindow_Fullscreen] = g_Win32.Window.Fullscreen;
	ws->DpiFactor                 = (float)dpi / USER_DEFAULT_SCREEN_DPI;

	return (void *)g_Win32.Window.Wnd;
}

static u64 kWin32_GetPerformanceFrequency(void)
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

static u64 kWin32_GetPerformanceCounter(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

static int kWin32_EventLoop(void)
{
	int   status    = 0;
	float dt        = 1.0f / 60.0f;
	u64   counter   = kWin32_GetPerformanceCounter();
	u64   frequency = kWin32_GetPerformanceFrequency();

	while (1)
	{
		kClearFrame();

		MSG msg;
		while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				status = (int)msg.wParam;
				return status;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (g_Win32.Window.DpiChanged)
		{
			UINT dpi = GetDpiForWindow(g_Win32.Window.Wnd);
			SetRectEmpty(&g_Win32.Window.Border);
			AdjustWindowRectExForDpi(&g_Win32.Window.Border, g_Win32.Window.Style, FALSE, WS_EX_APPWINDOW, dpi);
			g_Win32.Window.Border.left *= -1;
			g_Win32.Window.Border.top *= -1;

			float yfactor = (float)dpi / USER_DEFAULT_SCREEN_DPI;
			kAddWindowDpiChangedEvent(yfactor);
			g_Win32.Window.DpiChanged = false;
		}

		if (g_Win32.Window.Resized)
		{
			RECT rc;
			GetClientRect(g_Win32.Window.Wnd, &rc);

			u32 width  = (u32)(rc.right - rc.left);
			u32 height = (u32)(rc.bottom - rc.top);

			kAddWindowResizeEvent(width, height, g_Win32.Window.Fullscreen);

			g_Win32.Window.Resized = false;
		}

		u32 mods = kWin32_GetKeyModFlags();
		kSetKeyModFlags(mods);

#ifndef IMGUI_DISABLE
		ImGui_ImplWin32_NewFrame();
#endif

		kUpdateFrame(dt);

		u64 new_counter = kWin32_GetPerformanceCounter();
		u64 counts      = new_counter - counter;
		counter         = new_counter;
		dt              = (float)(((1000000.0 * (double)counts) / (double)frequency) / 1000000.0);
	}

	return status;
}

//
//
//

static void kWin32_ResizeWindowImpl(u32 w, u32 h)
{
	kWin32_ResizeWindow(&g_Win32.Window, w, h);
}
static void kWin32_ToggleWindowFullscreenImpl(void)
{
	kWin32_ToggleWindowFullscreen(&g_Win32.Window);
}
static void kWin32_ReleaseCursorImpl(void)
{
	kWin32_ReleaseCursor(&g_Win32.Window);
}
static void kWin32_CaptureCursorImpl(void)
{
	kWin32_CaptureCursor(&g_Win32.Window);
}
static int kWin32_GetWindowCaptionSizeImpl(void)
{
	return kWin32_GetWindowCaptionSize(&g_Win32.Window);
}
static void kWin32_MaximizeWindowImpl(void)
{
	kWin32_MaximizeWindow(&g_Win32.Window);
}
static void kWin32_RestoreWindowImpl(void)
{
	kWin32_RestoreWindow(&g_Win32.Window);
}
static void kWin32_MinimizeWindowImpl(void)
{
	kWin32_MinimizeWindow(&g_Win32.Window);
}
static void kWin32_CloseWindowImpl(void)
{
	kWin32_CloseWindow(&g_Win32.Window);
}

//
//
//

static void kWin32_LoadKeyboardState(kKeyboardState *keyboard)
{
	for (uint key = 0; key < (uint)kKey::Count; ++key)
	{
		keyboard->Keys[key].Down = GetAsyncKeyState(InvVirtualKeyMap[key]) & 0x8000;
	}

	u32 mods = 0;
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
		mods |= kKeyMod_LeftShift;
	if (GetAsyncKeyState(VK_RSHIFT) & 0x8000)
		mods |= kKeyMod_RightShift;
	if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
		mods |= kKeyMod_LeftCtrl;
	if (GetAsyncKeyState(VK_RCONTROL) & 0x8000)
		mods |= kKeyMod_RightCtrl;
	if (GetAsyncKeyState(VK_LMENU) & 0x8000)
		mods |= kKeyMod_LeftAlt;
	if (GetAsyncKeyState(VK_RMENU) & 0x8000)
		mods |= kKeyMod_RightAlt;

	keyboard->Mods = mods;
}

static void kWin32_LoadMouseState(kMouseState *mouse)
{
	POINT pt = {0};
	GetCursorPos(&pt);
	ScreenToClient(g_Win32.Window.Wnd, &pt);
	mouse->Cursor.x                           = pt.x;
	mouse->Cursor.y                           = pt.y;
	mouse->Buttons[(int)kButton::Left].Down   = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
	mouse->Buttons[(int)kButton::Right].Down  = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	mouse->Buttons[(int)kButton::Middle].Down = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
}

//
//
//

static void kWin32_BreakLoop(int status)
{
	PostQuitMessage(status);
}

void kWin32_DestroyBackend(void)
{
	if (g_Win32.Window.Wnd)
		kWin32_DestroyWindow();

	if (g_Win32.AvrtHandle)
		AvRevertMmThreadCharacteristics(g_Win32.AvrtHandle);
	memset(&g_Win32, 0, sizeof(g_Win32));
}

void kWin32_CreateBackend(kMediaBackend *backend)
{
	DWORD task_index   = 0;
	g_Win32.AvrtHandle = AvSetMmThreadCharacteristicsW(L"Games", &task_index);

	kWin32_MapVirutalKeys();

	backend->CreateWindow           = kWin32_CreateWindow;
	backend->DestroyWindow          = kWin32_DestroyWindow;
	backend->ResizeWindow           = kWin32_ResizeWindowImpl;
	backend->ToggleWindowFullscreen = kWin32_ToggleWindowFullscreenImpl;
	backend->ReleaseCursor          = kWin32_ReleaseCursorImpl;
	backend->CaptureCursor          = kWin32_CaptureCursorImpl;
	backend->GetWindowCaptionSize   = kWin32_GetWindowCaptionSizeImpl;
	backend->MaximizeWindow         = kWin32_MaximizeWindowImpl;
	backend->RestoreWindow          = kWin32_RestoreWindowImpl;
	backend->MinimizeWindow         = kWin32_MinimizeWindowImpl;
	backend->CloseWindow            = kWin32_CloseWindowImpl;

	backend->LoadKeyboardState      = kWin32_LoadKeyboardState;
	backend->LoadMouseState         = kWin32_LoadMouseState;

	backend->EventLoop              = kWin32_EventLoop;
	backend->BreakLoop              = kWin32_BreakLoop;

	backend->Destroy                = kWin32_DestroyBackend;
}

#endif

void kCreateMediaBackend(kMediaBackend *backend)
{
	kWin32_CreateBackend(backend);
}
