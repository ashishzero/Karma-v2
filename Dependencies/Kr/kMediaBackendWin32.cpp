#include "kMedia.h"
#include "kMediaBackend.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <windowsx.h>
#include <malloc.h>
#include <ShellScalingApi.h>
#include <dwmapi.h>
#include <avrt.h>

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

//
//
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
	kDebugTraceEx("Windows", "Resizing window to size %ux%u.\n", w, h);
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
		kLogInfoEx("Windows", "Switching to fullscreen mode.\n");

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
		kLogInfoEx("Windows", "Restoring window from fullscreen mode.\n");

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
	kDebugTraceEx("Windows", "Clipping cursor to window.\n");

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
	kDebugTraceEx("Windows", "Releasing cursor from window.\n");

	ShowCursor(TRUE);
	ClipCursor(NULL);

	RAWINPUTDEVICE rid = {.usUsagePage = 0x1, .usUsage = 0x2, .dwFlags = 0, .hwndTarget = window->Wnd};
	window->RawInput   = RegisterRawInputDevices(&rid, 1, sizeof(rid));

	kAddWindowCursorReleaseEvent();
}

static void kWin32_ForceCaptureCursor(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Capturing cursor in window.\n");

	if (GetActiveWindow() == window->Wnd)
	{
		ShowCursor(FALSE);
		kWin32_ClipCursorToWindow(window);
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
	kDebugTraceEx("Windows", "Maximizing window.\n");
	ShowWindow(window->Wnd, SW_MAXIMIZE);
}

static void kWin32_RestoreWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Restoring window.\n");
	ShowWindow(window->Wnd, SW_RESTORE);
}

static void kWin32_MinimizeWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Minimizing window.\n");
	ShowWindow(window->Wnd, SW_MINIMIZE);
}

static void kWin32_CloseWindow(kWin32Window *window)
{
	kDebugTraceEx("Windows", "Closing window.\n");
	PostMessageW(window->Wnd, WM_CLOSE, 0, 0);
}

//
//
//

static kKey  VirtualKeyMap[255];
static DWORD InvVirtualKeyMap[255];

static void  kWin32_MapVirutalKeys(void)
{
	VirtualKeyMap[(int)'A']            = kKey_A;
	VirtualKeyMap[(int)'B']            = kKey_B;
	VirtualKeyMap[(int)'C']            = kKey_C;
	VirtualKeyMap[(int)'D']            = kKey_D;
	VirtualKeyMap[(int)'E']            = kKey_E;
	VirtualKeyMap[(int)'F']            = kKey_F;
	VirtualKeyMap[(int)'G']            = kKey_G;
	VirtualKeyMap[(int)'H']            = kKey_H;
	VirtualKeyMap[(int)'I']            = kKey_I;
	VirtualKeyMap[(int)'J']            = kKey_J;
	VirtualKeyMap[(int)'K']            = kKey_K;
	VirtualKeyMap[(int)'L']            = kKey_L;
	VirtualKeyMap[(int)'M']            = kKey_M;
	VirtualKeyMap[(int)'N']            = kKey_N;
	VirtualKeyMap[(int)'O']            = kKey_O;
	VirtualKeyMap[(int)'P']            = kKey_P;
	VirtualKeyMap[(int)'Q']            = kKey_Q;
	VirtualKeyMap[(int)'R']            = kKey_R;
	VirtualKeyMap[(int)'S']            = kKey_S;
	VirtualKeyMap[(int)'T']            = kKey_T;
	VirtualKeyMap[(int)'U']            = kKey_U;
	VirtualKeyMap[(int)'V']            = kKey_V;
	VirtualKeyMap[(int)'W']            = kKey_W;
	VirtualKeyMap[(int)'X']            = kKey_X;
	VirtualKeyMap[(int)'Y']            = kKey_Y;
	VirtualKeyMap[(int)'Z']            = kKey_Z;

	VirtualKeyMap[(int)'0']            = kKey_0;
	VirtualKeyMap[(int)'1']            = kKey_1;
	VirtualKeyMap[(int)'2']            = kKey_2;
	VirtualKeyMap[(int)'3']            = kKey_3;
	VirtualKeyMap[(int)'4']            = kKey_4;
	VirtualKeyMap[(int)'5']            = kKey_5;
	VirtualKeyMap[(int)'6']            = kKey_6;
	VirtualKeyMap[(int)'7']            = kKey_7;
	VirtualKeyMap[(int)'8']            = kKey_8;
	VirtualKeyMap[(int)'9']            = kKey_9;

	VirtualKeyMap[VK_NUMPAD0]          = kKey_0;
	VirtualKeyMap[VK_NUMPAD1]          = kKey_1;
	VirtualKeyMap[VK_NUMPAD2]          = kKey_2;
	VirtualKeyMap[VK_NUMPAD3]          = kKey_3;
	VirtualKeyMap[VK_NUMPAD4]          = kKey_4;
	VirtualKeyMap[VK_NUMPAD5]          = kKey_5;
	VirtualKeyMap[VK_NUMPAD6]          = kKey_6;
	VirtualKeyMap[VK_NUMPAD7]          = kKey_7;
	VirtualKeyMap[VK_NUMPAD8]          = kKey_8;
	VirtualKeyMap[VK_NUMPAD9]          = kKey_9;

	VirtualKeyMap[VK_F1]               = kKey_F1;
	VirtualKeyMap[VK_F2]               = kKey_F2;
	VirtualKeyMap[VK_F3]               = kKey_F3;
	VirtualKeyMap[VK_F4]               = kKey_F4;
	VirtualKeyMap[VK_F5]               = kKey_F5;
	VirtualKeyMap[VK_F6]               = kKey_F6;
	VirtualKeyMap[VK_F7]               = kKey_F7;
	VirtualKeyMap[VK_F8]               = kKey_F8;
	VirtualKeyMap[VK_F9]               = kKey_F9;
	VirtualKeyMap[VK_F10]              = kKey_F10;
	VirtualKeyMap[VK_F11]              = kKey_F11;
	VirtualKeyMap[VK_F12]              = kKey_F12;

	VirtualKeyMap[VK_SNAPSHOT]         = kKey_PrintScreen;
	VirtualKeyMap[VK_INSERT]           = kKey_Insert;
	VirtualKeyMap[VK_HOME]             = kKey_Home;
	VirtualKeyMap[VK_PRIOR]            = kKey_PageUp;
	VirtualKeyMap[VK_NEXT]             = kKey_PageDown;
	VirtualKeyMap[VK_DELETE]           = kKey_Delete;
	VirtualKeyMap[VK_END]              = kKey_End;
	VirtualKeyMap[VK_RIGHT]            = kKey_Right;
	VirtualKeyMap[VK_LEFT]             = kKey_Left;
	VirtualKeyMap[VK_DOWN]             = kKey_Down;
	VirtualKeyMap[VK_UP]               = kKey_Up;
	VirtualKeyMap[VK_DIVIDE]           = kKey_Divide;
	VirtualKeyMap[VK_MULTIPLY]         = kKey_Multiply;
	VirtualKeyMap[VK_ADD]              = kKey_Plus;
	VirtualKeyMap[VK_SUBTRACT]         = kKey_Minus;
	VirtualKeyMap[VK_DECIMAL]          = kKey_Period;
	VirtualKeyMap[VK_OEM_3]            = kKey_BackTick;
	VirtualKeyMap[VK_CONTROL]          = kKey_Ctrl;
	VirtualKeyMap[VK_RETURN]           = kKey_Return;
	VirtualKeyMap[VK_ESCAPE]           = kKey_Escape;
	VirtualKeyMap[VK_BACK]             = kKey_Backspace;
	VirtualKeyMap[VK_TAB]              = kKey_Tab;
	VirtualKeyMap[VK_SPACE]            = kKey_Space;
	VirtualKeyMap[VK_SHIFT]            = kKey_Shift;

	InvVirtualKeyMap[kKey_A]           = 'A';
	InvVirtualKeyMap[kKey_B]           = 'B';
	InvVirtualKeyMap[kKey_C]           = 'C';
	InvVirtualKeyMap[kKey_D]           = 'D';
	InvVirtualKeyMap[kKey_E]           = 'E';
	InvVirtualKeyMap[kKey_F]           = 'F';
	InvVirtualKeyMap[kKey_G]           = 'G';
	InvVirtualKeyMap[kKey_H]           = 'H';
	InvVirtualKeyMap[kKey_I]           = 'I';
	InvVirtualKeyMap[kKey_J]           = 'J';
	InvVirtualKeyMap[kKey_K]           = 'K';
	InvVirtualKeyMap[kKey_L]           = 'L';
	InvVirtualKeyMap[kKey_M]           = 'M';
	InvVirtualKeyMap[kKey_N]           = 'N';
	InvVirtualKeyMap[kKey_O]           = 'O';
	InvVirtualKeyMap[kKey_P]           = 'P';
	InvVirtualKeyMap[kKey_Q]           = 'Q';
	InvVirtualKeyMap[kKey_R]           = 'R';
	InvVirtualKeyMap[kKey_S]           = 'S';
	InvVirtualKeyMap[kKey_T]           = 'T';
	InvVirtualKeyMap[kKey_U]           = 'U';
	InvVirtualKeyMap[kKey_V]           = 'V';
	InvVirtualKeyMap[kKey_W]           = 'W';
	InvVirtualKeyMap[kKey_X]           = 'X';
	InvVirtualKeyMap[kKey_Y]           = 'Y';
	InvVirtualKeyMap[kKey_Z]           = 'Z';

	InvVirtualKeyMap[kKey_0]           = '0';
	InvVirtualKeyMap[kKey_1]           = '1';
	InvVirtualKeyMap[kKey_2]           = '2';
	InvVirtualKeyMap[kKey_3]           = '3';
	InvVirtualKeyMap[kKey_4]           = '4';
	InvVirtualKeyMap[kKey_5]           = '5';
	InvVirtualKeyMap[kKey_6]           = '6';
	InvVirtualKeyMap[kKey_7]           = '7';
	InvVirtualKeyMap[kKey_8]           = '8';
	InvVirtualKeyMap[kKey_9]           = '9';

	InvVirtualKeyMap[kKey_0]           = VK_NUMPAD0;
	InvVirtualKeyMap[kKey_1]           = VK_NUMPAD1;
	InvVirtualKeyMap[kKey_2]           = VK_NUMPAD2;
	InvVirtualKeyMap[kKey_3]           = VK_NUMPAD3;
	InvVirtualKeyMap[kKey_4]           = VK_NUMPAD4;
	InvVirtualKeyMap[kKey_5]           = VK_NUMPAD5;
	InvVirtualKeyMap[kKey_6]           = VK_NUMPAD6;
	InvVirtualKeyMap[kKey_7]           = VK_NUMPAD7;
	InvVirtualKeyMap[kKey_8]           = VK_NUMPAD8;
	InvVirtualKeyMap[kKey_9]           = VK_NUMPAD9;

	InvVirtualKeyMap[kKey_F1]          = VK_F1;
	InvVirtualKeyMap[kKey_F2]          = VK_F2;
	InvVirtualKeyMap[kKey_F3]          = VK_F3;
	InvVirtualKeyMap[kKey_F4]          = VK_F4;
	InvVirtualKeyMap[kKey_F5]          = VK_F5;
	InvVirtualKeyMap[kKey_F6]          = VK_F6;
	InvVirtualKeyMap[kKey_F7]          = VK_F7;
	InvVirtualKeyMap[kKey_F8]          = VK_F8;
	InvVirtualKeyMap[kKey_F9]          = VK_F9;
	InvVirtualKeyMap[kKey_F10]         = VK_F1;
	InvVirtualKeyMap[kKey_F11]         = VK_F11;
	InvVirtualKeyMap[kKey_F12]         = VK_F1;

	InvVirtualKeyMap[kKey_PrintScreen] = VK_SNAPSHOT;
	InvVirtualKeyMap[kKey_Insert]      = VK_INSERT;
	InvVirtualKeyMap[kKey_Home]        = VK_HOME;
	InvVirtualKeyMap[kKey_PageUp]      = VK_PRIOR;
	InvVirtualKeyMap[kKey_PageDown]    = VK_NEXT;
	InvVirtualKeyMap[kKey_Delete]      = VK_DELETE;
	InvVirtualKeyMap[kKey_End]         = VK_END;
	InvVirtualKeyMap[kKey_Right]       = VK_RIGHT;
	InvVirtualKeyMap[kKey_Left]        = VK_LEFT;
	InvVirtualKeyMap[kKey_Down]        = VK_DOWN;
	InvVirtualKeyMap[kKey_Up]          = VK_UP;
	InvVirtualKeyMap[kKey_Divide]      = VK_DIVIDE;
	InvVirtualKeyMap[kKey_Multiply]    = VK_MULTIPLY;
	InvVirtualKeyMap[kKey_Plus]        = VK_ADD;
	InvVirtualKeyMap[kKey_Minus]       = VK_SUBTRACT;
	InvVirtualKeyMap[kKey_Period]      = VK_DECIMAL;
	InvVirtualKeyMap[kKey_BackTick]    = VK_OEM_3;
	InvVirtualKeyMap[kKey_Ctrl]        = VK_CONTROL;
	InvVirtualKeyMap[kKey_Return]      = VK_RETURN;
	InvVirtualKeyMap[kKey_Escape]      = VK_ESCAPE;
	InvVirtualKeyMap[kKey_Backspace]   = VK_BACK;
	InvVirtualKeyMap[kKey_Tab]         = VK_TAB;
	InvVirtualKeyMap[kKey_Space]       = VK_SPACE;
	InvVirtualKeyMap[kKey_Shift]       = VK_SHIFT;
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

			kDebugTraceEx("Windows", "Window focus %s.\n", focused ? "gained" : "lost");

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
				int    x      = GET_X_LPARAM(lparam);
				int    y      = GET_Y_LPARAM(lparam);
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
				kVec2i cursor = kVec2i(pt.x, pt.y);
				kAddCursorEvent(cursor);
			}
			else if (mouse->lLastX != 0 || mouse->lLastY != 0)
			{
				int    rel_x = mouse->lLastX;
				int    rel_y = mouse->lLastY;
				kVec2i delta = kVec2i(rel_x, rel_y);
				kAddCursorDeltaEvent(delta);
			}

			return 0;
		}

		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		{
			kAddButtonEvent(kButton_Left, msg == WM_LBUTTONDOWN);
			return 0;
		}

		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		{
			kAddButtonEvent(kButton_Right, msg == WM_RBUTTONDOWN);
			return 0;
		}

		case WM_MBUTTONUP:
		case WM_MBUTTONDOWN:
		{
			kAddButtonEvent(kButton_Middle, msg == WM_MBUTTONDOWN);
			return 0;
		}

		case WM_LBUTTONDBLCLK:
		{
			kAddDoubleClickEvent(kButton_Left);
			return 0;
		}

		case WM_RBUTTONDBLCLK:
		{
			kAddDoubleClickEvent(kButton_Right);
			return 0;
		}

		case WM_MBUTTONDBLCLK:
		{
			kAddDoubleClickEvent(kButton_Middle);
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
				kAddKeyEvent(kKey_F10, down, repeat);
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
	kLogInfoEx("Windows", "Destroying window.\n");

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

	kLogInfoEx("Windows", "Creating window: %S.\n", title);

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
	for (uint key = 0; key < kKey_Count; ++key)
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
	mouse->Cursor.x                     = pt.x;
	mouse->Cursor.y                     = pt.y;
	mouse->Buttons[kButton_Left].Down   = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
	mouse->Buttons[kButton_Right].Down  = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	mouse->Buttons[kButton_Middle].Down = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
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
