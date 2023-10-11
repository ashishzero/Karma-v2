#include "kMedia.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kRender.h"

#include <string.h>

typedef struct kPlatformWindow kPlatformWindow;

typedef struct kWindow
{
	kWindowState     state;
	float            yfactor;
	kPlatformWindow *native;
} kWindow;

typedef struct kMedia
{
	kArena           *arena;
	kArray<kEvent>    events;
	kKeyboardState    keyboard;
	kMouseState       mouse;
	kWindow           window;
	kMediaUserEvents  user;
	kSwapChainBackend swap_chain;
	kRenderBackend    render_backend;
} kMedia;

static kMedia media;

//
//
//

void kFallbackUserLoadProc(void)
{}
void kFallbackUserReleaseProc(void)
{}
void kFallbackUserUpdateProc(float dt)
{}

kSpan<kEvent> kGetEvents(void)
{
	return media.events;
}

kArena *kGetFrameArena(void)
{
	return media.arena;
}

void *kGetUserEventData(void)
{
	return media.user.data;
}

void kSetUserEventData(void *data)
{
	media.user.data = data;
}

void kGetUserEvents(kMediaUserEvents *user)
{
	memcpy(user, &media.user, sizeof(media.user));
}

void kSetUserEvents(const kMediaUserEvents &user)
{
	memcpy(&media.user, &user, sizeof(media.user));

	if (!media.user.load)
		media.user.load = kFallbackUserLoadProc;

	if (!media.user.release)
		media.user.release = kFallbackUserLoadProc;

	if (!media.user.update)
		media.user.update = kFallbackUserUpdateProc;
}

bool kIsKeyDown(kKey key)
{
	return media.keyboard.keys[key].down;
}

bool kKeyPressed(kKey key)
{
	return media.keyboard.keys[key].flags & kPressed;
}

bool kKeyReleased(kKey key)
{
	return media.keyboard.keys[key].flags & kReleased;
}

u8 kKeyHits(kKey key)
{
	return media.keyboard.keys[key].hits;
}

uint kGetKeyModFlags(void)
{
	return media.keyboard.mods;
}

bool kIsButtonDown(kButton button)
{
	return media.mouse.buttons[button].down;
}

bool kButtonPressed(kButton button)
{
	return media.mouse.buttons[button].flags & kPressed;
}

bool kButtonReleased(kButton button)
{
	return media.mouse.buttons[button].flags & kReleased;
}

kVec2i kGetCursorPosition(void)
{
	return media.mouse.cursor;
}

kVec2i kGetCursorDelta(void)
{
	return media.mouse.delta;
}

float kGetWheelHorizontal(void)
{
	return media.mouse.wheel.x;
}

float kGetWheelVertical(void)
{
	return media.mouse.wheel.y;
}

bool kIsWindowClosed(void)
{
	return media.window.state.closed;
}

bool kIsWindowResized(void)
{
	return media.window.state.resized;
}

void kIgnoreWindowCloseEvent(void)
{
	media.window.state.closed = false;
}

bool kIsWindowFocused(void)
{
	return media.window.state.focused;
}

bool kIsWindowFullscreen(void)
{
	return media.window.state.flags & kWindow_Fullscreen;
}

bool kIsWindowMaximized(void)
{
	return media.window.state.flags & kWindow_Maximized;
}

void kGetWindowSize(u32 *w, u32 *h)
{
	*w = media.window.state.width;
	*h = media.window.state.height;
}

float kGetWindowDpiScale(void)
{
	return media.window.yfactor;
}

bool kIsCursorCaptured(void)
{
	return media.window.state.capture;
}

bool kIsCursorHovered(void)
{
	return media.window.state.hovered;
}

void kGetKeyboardState(kKeyboardState *keyboard)
{
	memcpy(keyboard, &media.keyboard, sizeof(media.keyboard));
}

void kGetKeyState(kState *state, kKey key)
{
	memcpy(state, &media.keyboard.keys[key], sizeof(media.keyboard.keys[key]));
}

void kGetMouseState(kMouseState *mouse)
{
	memcpy(mouse, &media.mouse, sizeof(media.mouse));
}

void kGetButtonState(kState *state, kButton button)
{
	memcpy(state, &media.mouse.buttons[button], sizeof(media.mouse.buttons));
}

void kGetWindowState(kWindowState *state)
{
	memcpy(state, &media.window, sizeof(media.window));
}

void kSetKeyboardState(const kKeyboardState &keyboard)
{
	memcpy(&media.keyboard, &keyboard, sizeof(media.keyboard));
}

void kSetKeyState(const kState &state, kKey key)
{
	memcpy(&media.keyboard.keys[key], &state, sizeof(media.keyboard.keys[key]));
}

void kSetMouseState(const kMouseState &mouse)
{
	memcpy(&media.mouse, &mouse, sizeof(media.mouse));
}

void kSetButtonState(const kState &state, kButton button)
{
	memcpy(&media.mouse.buttons[button], &state, sizeof(media.mouse.buttons[button]));
}

void kSetWindowState(const kWindowState &state)
{
	kResizeWindow(state.width, state.height);
	bool fullscreen = kIsWindowFullscreen();

	if (state.flags & kWindow_Fullscreen)
	{
		if (!fullscreen)
		{
			kToggleWindowFullscreen();
		}
	}
	else
	{
		if (fullscreen)
		{
			kToggleWindowFullscreen();
		}
	}
}

void kClearInput(void)
{
	media.events.Reset();
	memset(&media.keyboard, 0, sizeof(media.keyboard));
	memset(&media.mouse, 0, sizeof(media.mouse));
}

void kClearFrame(void)
{
	kResetArena(media.arena);

	media.events.count  = 0;
	media.keyboard.mods = 0;

	for (uint key = 0; key < kKey_Count; ++key)
	{
		media.keyboard.keys[key].flags = 0;
		media.keyboard.keys[key].hits  = 0;
	}

	for (uint button = 0; button < kButton_Count; ++button)
	{
		media.mouse.buttons[button].flags = 0;
		media.mouse.buttons[button].hits  = 0;
	}

	memset(&media.mouse.wheel, 0, sizeof(media.mouse.wheel));

	media.window.state.resized = 0;
}

void kAddEvent(const kEvent &ev)
{
	media.events.Add(ev);
}

void kAddKeyEvent(kKey key, bool down, bool repeat)
{
	bool    pressed  = down && !repeat;
	bool    released = !down;
	kState *state    = &media.keyboard.keys[key];
	state->down      = down;

	if (released)
	{
		state->flags |= kReleased;
	}

	if (pressed)
	{
		state->flags |= kPressed;
		state->hits += 1;
	}

	kEvent ev = {.kind = down ? kEvent_KeyPressed : kEvent_KeyReleased, .key = {.symbol = key, .repeat = repeat}};

	kAddEvent(ev);
}

void kAddButtonEvent(kButton button, bool down)
{
	kState *state    = &media.mouse.buttons[button];
	bool    previous = state->down;
	bool    pressed  = down && !previous;
	bool    released = !down && previous;
	state->down      = down;

	if (released)
	{
		state->flags |= kReleased;
		state->hits += 1;
	}

	if (pressed)
	{
		state->flags |= kPressed;
	}

	kEvent ev = {.kind = down ? kEvent_ButtonPressed : kEvent_ButtonReleased, .button = {.symbol = button}};

	kAddEvent(ev);
}

void kAddTextInputEvent(u32 codepoint, u32 mods)
{
	kEvent ev = {.kind = kEvent_TextInput, .text = {.codepoint = codepoint, .mods = mods}};
	kAddEvent(ev);
}

void kAddDoubleClickEvent(kButton button)
{
	kState *state = &media.mouse.buttons[button];
	state->flags |= kDoubleClicked;

	kEvent ev = {.kind = kEvent_DoubleClicked, .button = {.symbol = button}};

	kAddEvent(ev);
}

void kAddCursorEvent(kVec2i pos)
{
	int xdel = pos.x - media.mouse.cursor.x;
	int ydel = pos.y - media.mouse.cursor.y;

	media.mouse.delta.x += xdel;
	media.mouse.delta.y += ydel;
	media.mouse.cursor = pos;

	kEvent ev          = {.kind = kEvent_CursorMoved, .cursor = {.position = pos}};
	kAddEvent(ev);
}

void kAddCursorDeltaEvent(kVec2i delta)
{
	int xdel = delta.x;
	int ydel = delta.y;

	media.mouse.delta.x += xdel;
	media.mouse.delta.y += ydel;

	media.mouse.cursor.x += xdel;
	media.mouse.cursor.y += ydel;

	kEvent ev = {.kind = kEvent_CursorMoved, .cursor = {.position = media.mouse.cursor}};
	kAddEvent(ev);
}

void kAddWheelEvent(float horz, float vert)
{
	media.mouse.wheel.x += horz;
	media.mouse.wheel.y += vert;

	kEvent ev = {.kind = kEvent_WheelMoved, .wheel = {.horizontal = horz, .vertical = vert}};

	kAddEvent(ev);
}

void kAddCursorEnterEvent(void)
{
	media.window.state.hovered = 1;

	kEvent ev                  = {.kind = kEvent_CursorEnter};
	kAddEvent(ev);
}

void kAddCursorLeaveEvent(void)
{
	media.window.state.hovered = 0;

	kEvent ev                  = {.kind = kEvent_CursorLeave};
	kAddEvent(ev);
}

void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen)
{
	media.window.state.width   = width;
	media.window.state.height  = height;

	media.window.state.resized = 1;

	if (fullscreen)
	{
		media.window.state.flags |= kWindow_Fullscreen;
	}
	else
	{
		media.window.state.flags &= ~kWindow_Fullscreen;
	}

	kEvent ev = {.kind = kEvent_Resized, .resized = {.width = width, .height = height}};

	kAddEvent(ev);
}

void kAddWindowFocusEvent(bool focused)
{
	media.window.state.focused = focused;

	kEvent ev                  = {.kind = focused ? kEvent_Activated : kEvent_Deactivated};

	kAddEvent(ev);
}

void kAddWindowCloseEvent(void)
{
	media.window.state.closed = 1;

	kEvent ev                 = {.kind = kEvent_Closed};

	kAddEvent(ev);
}

void kAddWindowMaximizeEvent(void)
{
	media.window.state.flags |= kWindow_Maximized;
}

void kAddWindowRestoreEvent(void)
{
	media.window.state.flags &= ~kWindow_Maximized;
}

void kAddWindowCursorCaptureEvent(void)
{
	media.window.state.capture = 1;
}

void kAddWindowCursorReleaseEvent(void)
{
	media.window.state.capture = 0;
}

void kAddWindowDpiChangedEvent(float yfactor)
{
	media.window.yfactor = yfactor;

	kEvent ev            = {.kind = kEvent_DpiChanged};
	kAddEvent(ev);
}

//
//
//

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <windowsx.h>
#include <malloc.h>
#include <ShellScalingApi.h>
#include <dwmapi.h>
#include <avrt.h>

#include "kResource.h"

#ifndef IDI_KICON
#define IDI_KICON 101
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3
#endif

#pragma comment(lib, "Avrt.lib")   // AvSetMmThreadCharacteristicsW
#pragma comment(lib, "Shcore.lib") // GetDpiForMonitor
#pragma comment(lib, "Dwmapi.lib") // DwmSetWindowAttribute

//
//
//

typedef struct kPlatformWindow
{
	HWND            wnd;
	kSwapChain      swap_chain;
	u32             high_surrogate;
	bool            request_raw_input;
	bool            request_resize;
	bool            request_notitlebar;
	bool            fullscreen;
	WINDOWPLACEMENT placement;
} kPlatformWindow;

static DWORD kGetWindowStyleFromFlags(u32 flags)
{
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if (flags & kWindow_Resizable)
	{
		style |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
	}
	return style;
}

void kResizeWindow(u32 w, u32 h)
{
	kPlatformWindow *window = media.window.native;
	DWORD            style  = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	RECT             rect;
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = w;
	rect.bottom = h;
	UINT dpi    = GetDpiForWindow(window->wnd);
	AdjustWindowRectExForDpi(&rect, style, FALSE, 0, dpi);
	int width  = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	SetWindowPos(window->wnd, NULL, 0, 0, width, height,
	             SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

	media.window.state.width  = w;
	media.window.state.height = h;
}

void kToggleWindowFullscreen(void)
{
	kPlatformWindow *window = media.window.native;
	if (!media.window.native->fullscreen)
	{
		DWORD       style = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
		MONITORINFO mi    = {.cbSize = sizeof(mi)};
		if (GetWindowPlacement(window->wnd, &window->placement) &&
		    GetMonitorInfoW(MonitorFromWindow(window->wnd, MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			SetWindowLongPtrW(window->wnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(window->wnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
			             mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
			             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
		media.window.native->request_resize = true;
		media.window.native->fullscreen     = true;
	}
	else
	{
		DWORD style = kGetWindowStyleFromFlags(media.window.state.flags);
		SetWindowLongPtrW(window->wnd, GWL_STYLE, style);
		SetWindowPlacement(window->wnd, &window->placement);
		SetWindowPos(window->wnd, NULL, 0, 0, 0, 0,
		             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		media.window.native->request_resize = true;
		media.window.native->fullscreen     = false;
	}
}

static void kClipCursorToWindow(void)
{
	kPlatformWindow *window = media.window.native;

	RECT             rect;
	GetClientRect(window->wnd, &rect);

	POINT pt  = {rect.left, rect.top};
	POINT pt2 = {rect.right, rect.bottom};
	ClientToScreen(window->wnd, &pt);
	ClientToScreen(window->wnd, &pt2);

	SetRect(&rect, pt.x, pt.y, pt2.x, pt2.y);
	ClipCursor(&rect);

	POINT center_pt = {.x = (rect.right - rect.left) / 2, .y = (rect.bottom - rect.top) / 2};
	ClientToScreen(window->wnd, &center_pt);
	SetCursorPos(center_pt.x, center_pt.y);
}

static void kForceReleaseCursor(void)
{
	ShowCursor(TRUE);
	ClipCursor(NULL);

	RAWINPUTDEVICE rid = {.usUsagePage = 0x1, .usUsage = 0x2, .dwFlags = 0, .hwndTarget = media.window.native->wnd};
	media.window.native->request_raw_input = RegisterRawInputDevices(&rid, 1, sizeof(rid));

	kAddWindowCursorReleaseEvent();
}

static void kForceCaptureCursor(void)
{
	kPlatformWindow *window = media.window.native;
	if (GetActiveWindow() == window->wnd)
	{
		ShowCursor(FALSE);
		kClipCursorToWindow();
	}

	RAWINPUTDEVICE rid = {.usUsagePage = 0x1,
	                      .usUsage     = 0x2,
	                      .dwFlags     = RIDEV_REMOVE,
	                      .hwndTarget  = media.window.native->wnd};
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	kAddWindowCursorCaptureEvent();
	media.window.native->request_raw_input = false;
}

void kReleaseCursor(void)
{
	if (!kIsCursorCaptured())
		return;
	kForceReleaseCursor();
}

void kCaptureCursor(void)
{
	if (kIsCursorCaptured())
		return;
	kForceCaptureCursor();
}

void kMaximizeWindow(void)
{
	ShowWindow(media.window.native->wnd, SW_MAXIMIZE);
}

void kRestoreWindow(void)
{
	ShowWindow(media.window.native->wnd, SW_RESTORE);
}

void kMinimizeWindow(void)
{
	ShowWindow(media.window.native->wnd, SW_MINIMIZE);
}

void kCloseWindow(void)
{
	PostMessageW(media.window.native->wnd, WM_CLOSE, 0, 0);
}

//
// https://github.com/cmuratori/dtc/blob/main/dtc.cpp
//

#define K_HWND_MSG_CREATE (WM_USER + 0x1337)
#define K_HWND_MSG_DESTROY (WM_USER + 0x1338)

struct kWindowEventDispatcher
{
	DWORD  parent;
	HANDLE ready;
	HANDLE thread;
	HWND   window;
};

static kWindowEventDispatcher ev_dispatcher;
static RECT                   ThickFrameBorderRect;

static LRESULT CALLBACK       kHandleServiceWindowEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT res = 0;

	switch (msg)
	{
		case K_HWND_MSG_CREATE:
		{
			CREATESTRUCTW *cs = (CREATESTRUCTW *)wparam;
			res = (LRESULT)CreateWindowExW(cs->dwExStyle, cs->lpszClass, cs->lpszName, cs->style, cs->x, cs->y, cs->cx,
			                               cs->cy, cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams);
		}
		break;

		case K_HWND_MSG_DESTROY:
		{
			DestroyWindow((HWND)wparam);
		}
		break;

		default:
		{
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		}
		break;
	}

	return res;
}

static LRESULT kHitTestNonClientArea(HWND wnd, WPARAM wparam, LPARAM lparam)
{
	POINT cursor = {.x = GET_X_LPARAM(lparam), .y = GET_Y_LPARAM(lparam)};

	UINT  dpi    = GetDpiForWindow(wnd);

	RECT  rc;
	GetWindowRect(wnd, &rc);

	RECT frame = {0};
	AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, 0, dpi);

	USHORT row           = 1;
	USHORT col           = 1;
	bool   border_resize = false;

	RECT   border        = ThickFrameBorderRect;

	if (cursor.y >= rc.top && cursor.y < rc.top + border.top)
	{
		border_resize = (cursor.y < (rc.top - frame.top));
		row           = 0;
	}
	else if (cursor.y < rc.bottom && cursor.y >= rc.bottom - border.bottom)
	{
		row = 2;
	}

	if (cursor.x >= rc.left && cursor.x < rc.left + border.left)
	{
		col = 0;
	}
	else if (cursor.x < rc.right && cursor.x >= rc.right - border.right)
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

static bool kHandleCustomCaptionEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *result)
{
	*result      = 0;
	bool handled = DwmDefWindowProc(wnd, msg, wparam, lparam, result);

	if (msg == WM_ACTIVATE)
	{
		MARGINS margins = {0};
		DwmExtendFrameIntoClientArea(wnd, &margins);
		return false;
	}

	if ((msg == WM_NCCALCSIZE) && (wparam == TRUE))
	{
		*result = 0;
		return true;
	}

	if ((msg == WM_NCHITTEST) && (*result == 0))
	{
		*result = kHitTestNonClientArea(wnd, wparam, lparam);
		if (*result != HTNOWHERE)
		{
			return true;
		}
	}

	return handled;
}

static LRESULT CALLBACK kDispatchWindowEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT          res    = 0;

	kPlatformWindow *window = (kPlatformWindow *)GetWindowLongPtrW(wnd, GWLP_USERDATA);

	if (window && window->request_notitlebar)
	{
		if (kHandleCustomCaptionEvent(wnd, msg, wparam, lparam, &res))
			return res;
	}

	switch (msg)
	{
		case WM_CLOSE:
		{
			PostThreadMessageW(ev_dispatcher.parent, msg, (WPARAM)wnd, lparam);
		}
		break;

		case WM_ACTIVATE:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			PostThreadMessageW(ev_dispatcher.parent, msg, (WPARAM)wnd, lparam);
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		}
		break;

		case WM_SIZE:
		case WM_MOUSELEAVE:
		case WM_MOUSEMOVE:
		case WM_INPUT:
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_DPICHANGED:
		{
			PostThreadMessageW(ev_dispatcher.parent, msg, wparam, lparam);
		}
		break;

		default:
		{
			res = DefWindowProcW(wnd, msg, wparam, lparam);
		}
		break;
	}

	return res;
}

static DWORD WINAPI kWindowsEventDispatchThread(LPVOID param)
{
	HINSTANCE instance = GetModuleHandleW(0);

	{
		WNDCLASSEXW wnd_class   = {0};
		HICON       icon        = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_KICON), IMAGE_ICON, 0, 0, LR_SHARED);
		HICON       icon_sm     = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_KICON), IMAGE_ICON, 0, 0, LR_SHARED);
		HICON       win_icon    = (HICON)LoadImageW(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);

		wnd_class.cbSize        = sizeof(wnd_class);
		wnd_class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wnd_class.lpfnWndProc   = kDispatchWindowEvent;
		wnd_class.hInstance     = instance;
		wnd_class.hIcon         = icon ? icon : win_icon;
		wnd_class.hIconSm       = icon_sm ? icon_sm : win_icon;
		wnd_class.hCursor       = (HCURSOR)LoadImageW(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
		wnd_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
		wnd_class.lpszClassName = L"kWindowClass";
		RegisterClassExW(&wnd_class);
	}

	WNDCLASSEXW wnd_class   = {};
	wnd_class.cbSize        = sizeof(wnd_class);
	wnd_class.lpfnWndProc   = &kHandleServiceWindowEvent;
	wnd_class.hInstance     = instance;
	wnd_class.hIcon         = LoadIconW(NULL, IDI_APPLICATION);
	wnd_class.hCursor       = LoadCursorW(NULL, IDC_ARROW);
	wnd_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	wnd_class.lpszClassName = L"kServiceWindow";
	RegisterClassExW(&wnd_class);

	ev_dispatcher.window = CreateWindowExW(0, wnd_class.lpszClassName, 0, 0, 0, 0, 0, 0, 0, 0, wnd_class.hInstance, 0);
	SetEvent(ev_dispatcher.ready);

	while (1)
	{
		MSG msg;
		GetMessageW(&msg, 0, 0, 0);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

static void kCreateWindowEventDispatcher(void)
{
	DWORD style = kGetWindowStyleFromFlags(kWindow_NoTitleBar);
	SetRectEmpty(&ThickFrameBorderRect);
	AdjustWindowRectEx(&ThickFrameBorderRect, style, FALSE, WS_EX_APPWINDOW);
	ThickFrameBorderRect.left *= -1;
	ThickFrameBorderRect.top *= -1;

	ev_dispatcher.parent = GetCurrentThreadId();
	ev_dispatcher.ready  = CreateEventW(0, TRUE, 0, 0);
	ev_dispatcher.thread = CreateThread(0, 0, kWindowsEventDispatchThread, 0, 0, 0);
	WaitForSingleObject(ev_dispatcher.ready, INFINITE);
}

static void kDestroyWindowEventDispatcher(void)
{
	DestroyWindow(ev_dispatcher.window);
	TerminateThread(ev_dispatcher.thread, 0);
	CloseHandle(ev_dispatcher.thread);
	CloseHandle(ev_dispatcher.ready);
	memset(&ev_dispatcher, 0, sizeof(ev_dispatcher));
}

static HWND kDispatchCreateWindowExW(DWORD ex_style, LPCWSTR name, DWORD style, int x, int y, int cx, int cy,
                                     HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
{
	CREATESTRUCTW cs  = {};
	cs.dwExStyle      = ex_style;
	cs.lpszClass      = L"kWindowClass";
	cs.lpszName       = name;
	cs.style          = style;
	cs.x              = x;
	cs.y              = y;
	cs.cx             = cx;
	cs.cy             = cy;
	cs.hwndParent     = parent;
	cs.hMenu          = menu;
	cs.hInstance      = instance;
	cs.lpCreateParams = param;
	HWND wnd          = (HWND)SendMessageW(ev_dispatcher.window, K_HWND_MSG_CREATE, (WPARAM)&cs, 0);
	return wnd;
}

static void kDispatchDestroyWindow(HWND hwnd)
{
	SendMessageW(ev_dispatcher.window, K_HWND_MSG_DESTROY, (WPARAM)hwnd, 0);
}

//
//
//

static kKey  VirtualKeyMap[255];
static DWORD InvVirtualKeyMap[255];

static void  kMapVirutalKeys(void)
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

static u32 kGetVirtualKeyModFlags(void)
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

static LRESULT kHandleWindowEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_ACTIVATE:
		{
			int  state   = LOWORD(wparam);
			bool focused = (state == WA_ACTIVE || state == WA_CLICKACTIVE);
			kAddWindowFocusEvent(focused);

			if (kIsCursorCaptured())
			{
				if (focused)
				{
					kForceCaptureCursor();
				}
				else
				{
					kForceReleaseCursor();
				}
			}

			return 0;
		}

		case WM_CLOSE:
		{
			kAddWindowCloseEvent();
			return 0;
		}

		case WM_SIZE:
		{
			if (kIsCursorCaptured() && kIsWindowFocused())
			{
				kClipCursorToWindow();
			}

			if (wparam == SIZE_MAXIMIZED)
			{
				kAddWindowMaximizeEvent();
			}
			else if (wparam == SIZE_RESTORED)
			{
				kAddWindowRestoreEvent();
			}

			media.window.native->request_resize = true;
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

			if (!kIsCursorCaptured() || !media.window.native->request_raw_input)
			{
				RECT rc;
				GetClientRect(wnd, &rc);
				int    height = rc.bottom - rc.top;
				int    x      = GET_X_LPARAM(lparam);
				int    y      = GET_Y_LPARAM(lparam);
				kVec2i cursor = kVec2i(x, height - y);
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
				RECT rc;
				GetClientRect(wnd, &rc);
				bool  isvirtual = (mouse->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
				int   width     = GetSystemMetricsForDpi(isvirtual ? SM_CXVIRTUALSCREEN : SM_CXSCREEN, dpi);
				int   height    = GetSystemMetricsForDpi(isvirtual ? SM_CYVIRTUALSCREEN : SM_CYSCREEN, dpi);
				int   abs_x     = (int)((mouse->lLastX / 65535.0f) * width);
				int   abs_y     = (int)((mouse->lLastY / 65535.0f) * height);
				POINT pt        = {.x = abs_x, .y = abs_y};
				int   client_h  = rc.bottom - rc.top;
				ScreenToClient(wnd, &pt);
				kVec2i cursor = kVec2i(pt.x, client_h - pt.y);
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
			return 0;
		}

		case WM_CHAR:
		{
			if (IS_HIGH_SURROGATE(wparam))
			{
				media.window.native->high_surrogate = (u32)(wparam & 0xFFFF);
			}
			else
			{
				if (IS_LOW_SURROGATE(wparam))
				{
					if (media.window.native->high_surrogate)
					{
						u32 codepoint = 0;
						codepoint += (media.window.native->high_surrogate - 0xD800) << 10;
						codepoint += (u16)wparam - 0xDC00;
						codepoint += 0x10000;
						u32 mods = kGetVirtualKeyModFlags();
						kAddTextInputEvent(codepoint, mods);
					}
				}
				else
				{
					u32 mods = kGetVirtualKeyModFlags();
					kAddTextInputEvent((u32)wparam, mods);
				}
				media.window.native->high_surrogate = 0;
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

			UINT  dpi     = HIWORD(wparam);
			float yfactor = (float)dpi / USER_DEFAULT_SCREEN_DPI;
			kAddWindowDpiChangedEvent(yfactor);
			return 0;
		}
	}

	return 0;
}

static void kDestroyWindow(void)
{
	kPlatformWindow *window = media.window.native;

	if (window)
	{
		if (window->swap_chain)
		{
			media.swap_chain.destroy(window->swap_chain);
		}
		kDispatchDestroyWindow(window->wnd);
		kFree(window, sizeof(*window));
	}

	media.window.native = nullptr;
}

static void kCreateWindow(kString mb_title, uint w, uint h, uint flags)
{
	HMODULE instance    = GetModuleHandleW(0);

	wchar_t title[2048] = L"kWindow | Windows";

	if (mb_title.count)
	{
		int len =
			MultiByteToWideChar(CP_UTF8, 0, (char *)mb_title.data, (int)mb_title.count, title, kArrayCount(title) - 1);
		title[len] = 0;
	}

	int   width  = CW_USEDEFAULT;
	int   height = CW_USEDEFAULT;

	DWORD style  = kGetWindowStyleFromFlags(flags);

	if (w > 0 && h > 0)
	{
		POINT    origin   = {.x = 0, .y = 0};
		HMONITOR mprimary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
		UINT     xdpi     = 0;
		UINT     ydpi     = 0;

		GetDpiForMonitor(mprimary, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);

		RECT rect;
		rect.left   = 0;
		rect.top    = 0;
		rect.right  = w;
		rect.bottom = h;

		AdjustWindowRectExForDpi(&rect, style, FALSE, 0, ydpi);
		width  = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	kPlatformWindow *window = (kPlatformWindow *)kAlloc(sizeof(kPlatformWindow));
	if (!window)
	{
		kLogError("Windows: Failed to create window: out of memory");
		return;
	}

	memset(window, 0, sizeof(*window));
	media.window.native      = window;
	media.window.state.flags = flags;

	HWND hwnd = kDispatchCreateWindowExW(WS_EX_APPWINDOW, title, style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0,
	                                     0, instance, 0);

	if (!hwnd)
	{
		kLogHresultError(GetLastError(), "Windows", "Failed to create window");
		return;
	}

	BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

	LONG corner = DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

	window->wnd = hwnd;

	if (flags & kWindow_NoTitleBar)
	{
		window->request_notitlebar = true;
	}

	SetWindowLongPtrW(window->wnd, GWLP_USERDATA, (LONG_PTR)window);
	SetWindowPos(window->wnd, NULL, 0, 0, 0, 0,
	             SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

	ShowWindow(window->wnd, SW_SHOWNORMAL);
	UpdateWindow(window->wnd);

	if (flags & kWindow_Fullscreen)
	{
		kToggleWindowFullscreen();
	}

	window->swap_chain = media.swap_chain.create(media.window.native->wnd);
}

static void kInitMediaState(void)
{
	RECT rc = {0};
	GetClientRect(media.window.native->wnd, &rc);

	media.window.state.width  = rc.right - rc.left;
	media.window.state.height = rc.bottom - rc.top;

	if (GetActiveWindow() == media.window.native->wnd)
	{
		media.window.state.focused = 1;
	}

	{
		HMONITOR hmonitor = MonitorFromWindow(media.window.native->wnd, MONITOR_DEFAULTTOPRIMARY);
		UINT     xdpi     = 0;
		UINT     ydpi     = 0;
		GetDpiForMonitor(hmonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
		media.window.yfactor = (float)ydpi / USER_DEFAULT_SCREEN_DPI;
	}

	POINT pt = {0};
	GetCursorPos(&pt);
	ScreenToClient(media.window.native->wnd, &pt);

	media.mouse.cursor.x = pt.x;
	media.mouse.cursor.y = media.window.state.height - pt.y;

	if (PtInRect(&rc, pt))
	{
		media.window.state.hovered = 1;
	}

	media.mouse.buttons[kButton_Left].down   = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
	media.mouse.buttons[kButton_Right].down  = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	media.mouse.buttons[kButton_Middle].down = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

	for (uint key = 0; key < kKey_Count; ++key)
	{
		media.keyboard.keys[key].down = GetAsyncKeyState(InvVirtualKeyMap[key]) & 0x8000;
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

	media.keyboard.mods = mods;

	media.events.Reserve(64);
}

static int kWindowsEventLoop(void)
{
	int   status    = 0;
	float dt        = 1.0f / 60.0f;
	u64   counter   = kGetPerformanceCounter();
	u64   frequency = kGetPerformanceFrequency();

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
			kHandleWindowEvent(media.window.native->wnd, msg.message, msg.wParam, msg.lParam);
		}

		if (media.window.native->request_resize)
		{
			RECT rc;
			GetClientRect(media.window.native->wnd, &rc);

			media.window.state.width  = (u32)(rc.right - rc.left);
			media.window.state.height = (u32)(rc.bottom - rc.top);

			media.swap_chain.resize(media.window.native->swap_chain, media.window.state.width,
			                        media.window.state.height);

			kAddWindowResizeEvent(media.window.state.width, media.window.state.height, media.window.native->fullscreen);

			media.window.native->request_resize = false;
		}

		media.keyboard.mods = kGetVirtualKeyModFlags();

		{
			kBeginFrame();

			media.user.update(dt);

			if (kIsWindowClosed())
				break;

			kEndFrame();
			media.swap_chain.present(media.window.native->swap_chain);
		}

		u64 new_counter = kGetPerformanceCounter();
		u64 counts      = new_counter - counter;
		counter         = new_counter;
		dt              = (float)(((1000000.0 * (double)counts) / (double)frequency) / 1000000.0);
	}

	return status;
}

static kSwapChain kGetWindowSwapChain(void)
{
	return media.window.native->swap_chain;
}

extern void kCreateRenderBackend(kRenderBackend *backend, kSwapChainBackend *swap_chain);

//
//
//

int kEventLoop(const kMediaSpec &spec, const kMediaUserEvents &user)
{
	DWORD  task_index  = 0;
	HANDLE avrt_handle = AvSetMmThreadCharacteristicsW(L"Games", &task_index);

	kMapVirutalKeys();
	kCreateWindowEventDispatcher();

	//
	//
	//

	memset(&media, 0, sizeof(media));

	kCreateRenderBackend(&media.render_backend, &media.swap_chain);

	media.render_backend.window_swap_chain = kGetWindowSwapChain;

	kCreateWindow(spec.window.title, spec.window.width, spec.window.height, spec.window.flags);

	if (!media.window.native)
	{
		kFatalError("Failed to create window");
	}

	kInitMediaState();
	kSetUserEvents(user);

	{
		kAllocator *allocator  = kGetContextAllocator();

		kArenaSpec  arena_spec = {};
		arena_spec.alignment   = sizeof(umem);
		arena_spec.capacity    = spec.frame.arena_size;
		arena_spec.flags       = 0;

		media.arena            = kAllocArena(arena_spec, allocator);

		if (media.arena == &kFallbackArena)
		{
			kLogWarning("Failed to allocate frame arena\n");
		}
	}

	kCreateRenderContext(media.render_backend, spec.features.render);
	media.user.load();

	int status = kWindowsEventLoop();

	media.user.release();
	kDestroyRenderContext();

	kDestroyWindow();
	media.render_backend.destroy();

	//
	//
	//

	if (avrt_handle)
		AvRevertMmThreadCharacteristics(avrt_handle);
	kDestroyWindowEventDispatcher();

	return status;
}

void kBreakLoop(int status)
{
	PostQuitMessage(status);
}

#endif
