#include "kMedia.h"
#include "kContext.h"

#include <string.h>

typedef struct kEventList {
	kEvent *data;
	int     count;
	int     allocated;
} kEventList;

typedef struct kPlatformWindow kPlatformWindow;

enum kWindowFlagsEx {
	kWindowEx_Closed         = 0x01,
	kWindowEx_Resized        = 0x02,
	kWindowEx_Focused        = 0x04,
	kWindowEx_CursorDisabled = 0x08,
	kWindowEx_CursorHovered  = 0x10,
};

typedef struct kWindow {
	kWindowState     state;
	float            yfactor;
	u32              exflags;
	kPlatformWindow *native;
} kWindow;

typedef struct kMedia {
	kEventList       events;
	kKeyboardState   keyboard;
	kMouseState      mouse;
	kWindow          window;
	kMediaUserEvents user;
} kMedia;

static kMedia        media;

//
//
//

void kFallbackUserLoadProc(void) {}
void kFallbackUserReleaseProc(void) {}
void kFallbackUserUpdateProc(float dt) {}

void *kGetUserEventData(void) {
	return media.user.data;
}

void kSetUserEventData(void *data) {
	media.user.data = data;
}

void kGetUserEvents(kMediaUserEvents *user) {
	memcpy(user, &media.user, sizeof(media.user));
}

void kSetUserEvents(kMediaUserEvents *user) {
	memcpy(&media.user, user, sizeof(media.user));

	if (!media.user.load)
		media.user.load    = kFallbackUserLoadProc;

	if (!media.user.release)
		media.user.release = kFallbackUserLoadProc;

	if (!media.user.update)
		media.user.update  = kFallbackUserUpdateProc;
}

kEvent *kGetEvents(int *count) {
	*count = media.events.count;
	return media.events.data;
}

bool kIsKeyDown(kKey key) {
	return media.keyboard.keys[key].down;
}

bool kKeyPressed(kKey key) {
	return media.keyboard.keys[key].flags & kPressed;
}

bool kKeyReleased(kKey key) {
	return media.keyboard.keys[key].flags & kReleased;
}

u8 kKeyHits(kKey key) {
	return media.keyboard.keys[key].hits;
}

uint kGetKeyModFlags(void) {
	return media.keyboard.mods;
}

bool kIsButtonDown(kButton button) {
	return media.mouse.buttons[button].down;
}

bool kButtonPressed(kButton button) {
	return media.mouse.buttons[button].flags & kPressed;
}

bool kButtonReleased(kButton button) {
	return media.mouse.buttons[button].flags & kReleased;
}

kVec2i kGetCursorPosition(void) {
	return media.mouse.cursor;
}

kVec2i kGetCursorDelta(void) {
	return media.mouse.delta;
}

float kGetWheelHorizontal(void) {
	return media.mouse.wheel.x;
}

float kGetWheelVertical(void) {
	return media.mouse.wheel.y;
}

bool kIsWindowClosed(void) {
	return media.window.exflags & kWindowEx_Closed;
}

bool kIsWindowResized(void) {
	return media.window.exflags & kWindowEx_Resized;
}

void kIgnoreWindowCloseEvent(void) {
	media.window.exflags &= ~kWindowEx_Closed;
}

bool kIsWindowFocused(void) {
	return media.window.exflags & kWindowEx_Focused;
}

bool kIsWindowFullscreen(void) {
	return media.window.state.flags & kWindow_Fullscreen;
}

bool kIsWindowMaximized(void) {
	return media.window.state.flags & kWindow_Maximized;
}

void kGetWindowSize(u32 *w, u32 *h) {
	*w = media.window.state.width;
	*h = media.window.state.height;
}

float kGetWindowDpiScale(void) {
	return media.window.yfactor;
}

bool kIsCursorEnabled(void) {
	return (media.window.exflags & kWindowEx_CursorDisabled) == 0;
}

bool kIsCursorHovered(void) {
	return media.window.exflags & kWindowEx_CursorHovered;
}

void kGetKeyboardState(kKeyboardState *keyboard) {
	memcpy(keyboard, &media.keyboard, sizeof(media.keyboard));
}

void kGetKeyState(kState *state, kKey key) {
	memcpy(state, &media.keyboard.keys[key], sizeof(media.keyboard.keys[key]));
}

void kGetMouseState(kMouseState *mouse) {
	memcpy(mouse, &media.mouse, sizeof(media.mouse));
}

void kGetButtonState(kState *state, kButton button) {
	memcpy(state, &media.mouse.buttons[button], sizeof(media.mouse.buttons));
}

void kGetWindowState(kWindowState *state) {
	memcpy(state, &media.window, sizeof(media.window));
}

void kSetKeyboardState(kKeyboardState *keyboard) {
	memcpy(&media.keyboard, keyboard, sizeof(media.keyboard));
}

void kSetKeyState(kState *state, kKey key) {
	memcpy(&media.keyboard.keys[key], state, sizeof(media.keyboard.keys[key]));
}

void kSetMouseState(kMouseState *mouse) {
	memcpy(&media.mouse, mouse, sizeof(media.mouse));
}

void kSetButtonState(kState *state, kButton button) {
	memcpy(&media.mouse.buttons[button], state, sizeof(media.mouse.buttons[button]));
}

void kSetWindowState(kWindowState *state) {
	kResizeWindow(state->width, state->height);
	bool fullscreen = kIsWindowFullscreen();

	if (state->flags & kWindow_Fullscreen) {
		if (!fullscreen) {
			kToggleWindowFullscreen();
		}
	}
	else {
		if (fullscreen) {
			kToggleWindowFullscreen();
		}
	}
}

void kClearInput(void) {
	media.events.count = 0;
	memset(&media.keyboard, 0, sizeof(media.keyboard));
	memset(&media.mouse, 0, sizeof(media.mouse));
}

void kNextFrame(void) {
	media.events.count = 0;
	media.keyboard.mods = 0;

	for (uint key = 0; key < kKey_Count; ++key) {
		media.keyboard.keys[key].flags = 0;
		media.keyboard.keys[key].hits = 0;
	}

	for (uint button = 0; button < kButton_Count; ++button) {
		media.mouse.buttons[button].flags = 0;
		media.mouse.buttons[button].hits = 0;
	}

	memset(&media.mouse.wheel, 0, sizeof(media.mouse.wheel));

	u32 reset_flags = kWindowEx_Closed | kWindowEx_Resized;
	media.window.exflags &= ~reset_flags;
}

void kAddEvent(kEvent *ev) {
	if (media.events.count == media.events.allocated) {
		int prev = media.events.allocated;
		int next = kMax(prev ? prev << 1 : 64, prev + 1);
		media.events.data = kRealloc(media.events.data, prev * sizeof(kEvent), next * sizeof(kEvent));
		media.events.allocated = next;
	}
	media.events.data[media.events.count++] = *ev;
}

void kAddKeyEvent(kKey key, bool down, bool repeat) {
	bool pressed = down && !repeat;
	bool released = !down;
	kState *state = &media.keyboard.keys[key];
	state->down = down;

	if (released) {
		state->flags |= kReleased;
	}

	if (pressed) {
		state->flags |= kPressed;
		state->hits += 1;
	}

	kEvent ev = {
		.kind = down ? kEvent_KeyPressed : kEvent_KeyReleased,
		.key = {.symbol = key, .repeat = repeat }
	};

	kAddEvent(&ev);
}

void kAddButtonEvent(kButton button, bool down) {
	kState *state = &media.mouse.buttons[button];
	bool previous = state->down;
	bool pressed = down && !previous;
	bool released = !down && previous;
	state->down = down;

	if (released) {
		state->flags |= kReleased;
		state->hits += 1;
	}

	if (pressed) {
		state->flags |= kPressed;
	}

	kEvent ev = {
		.kind = down ? kEvent_ButtonPressed : kEvent_ButtonReleased,
		.button = {.symbol = button }
	};

	kAddEvent(&ev);
}

void kAddTextInputEvent(u32 codepoint, u32 mods) {
	kEvent ev = {
		.kind = kEvent_TextInput,
		.text = {.codepoint = codepoint, .mods = mods }
	};
	kAddEvent(&ev);
}

void kAddDoubleClickEvent(kButton button) {
	kState *state = &media.mouse.buttons[button];
	state->flags |= kDoubleClicked;

	kEvent ev = {
		.kind = kEvent_DoubleClicked,
		.button = {.symbol = button }
	};

	kAddEvent(&ev);
}

void kAddCursorEvent(kVec2i pos) {
	int xdel = pos.x - media.mouse.cursor.x;
	int ydel = pos.y - media.mouse.cursor.y;

	media.mouse.delta.x += xdel;
	media.mouse.delta.y += ydel;
	media.mouse.cursor   = pos;

	kEvent ev = {
		.kind = kEvent_CursorMoved,
		.cursor = {.position = pos }
	};
	kAddEvent(&ev);
}

void kAddCursorDeltaEvent(kVec2i delta) {
	int xdel = delta.x;
	int ydel = delta.y;

	media.mouse.delta.x += xdel;
	media.mouse.delta.y += ydel;

	media.mouse.cursor.x += xdel;
	media.mouse.cursor.y += ydel;

	kEvent ev = {
		.kind   = kEvent_CursorMoved,
		.cursor = { .position = media.mouse.cursor }
	};
	kAddEvent(&ev);
}

void kAddWheelEvent(float horz, float vert) {
	media.mouse.wheel.x += horz;
	media.mouse.wheel.y += vert;

	kEvent ev = {
		.kind = kEvent_WheelMoved,
		.wheel = {.horizontal = horz, .vertical = vert }
	};

	kAddEvent(&ev);
}

void kAddCursorEnterEvent(void) {
	media.window.exflags |= kWindowEx_CursorHovered;

	kEvent ev = {
		.kind = kEvent_CursorEnter
	};
	kAddEvent(&ev);
}

void kAddCursorLeaveEvent(void) {
	media.window.exflags &= ~kWindowEx_CursorHovered;

	kEvent ev = {
		.kind = kEvent_CursorLeave
	};
	kAddEvent(&ev);
}

void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen) {
	media.window.state.width = width;
	media.window.state.height = height;

	if (fullscreen) {
		media.window.state.flags |= kWindow_Fullscreen;
	}
	else {
		media.window.state.flags &= ~kWindow_Fullscreen;
	}

	media.window.exflags |= kWindowEx_Resized;

	kEvent ev = {
		.kind    = kEvent_Resized,
		.resized = {.width = width, .height = height }
	};

	kAddEvent(&ev);
}

void kAddWindowFocusEvent(bool focused) {
	media.window.exflags |= kWindowEx_Focused;

	kEvent ev = {
		.kind = focused ? kEvent_Activated : kEvent_Deactivated
	};

	kAddEvent(&ev);
}

void kAddWindowCloseEvent(void) {
	media.window.exflags |= kWindowEx_Closed;

	kEvent ev = {
		.kind = kEvent_Closed
	};

	kAddEvent(&ev);
}

void kAddWindowDpiChangedEvent(float yfactor) {
	media.window.yfactor = yfactor;

	kEvent ev = {
		.kind = kEvent_DpiChanged
	};
	kAddEvent(&ev);
}

//
//
//

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <windowsx.h>
#include <malloc.h>
#include <objbase.h>
#include <shellapi.h>
#include <ShellScalingApi.h>
#include <dwmapi.h>

#include "kResource.h"

#ifndef IDI_KICON
#define IDI_KICON   101
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#pragma comment(lib, "Shell32.lib")    // CommandLineToArgvW
#pragma comment(lib, "Shcore.lib")     // GetDpiForMonitor
#pragma comment(lib, "Ole32.lib")      // COM
#pragma comment(lib, "Dwmapi.lib")     // DwmSetWindowAttribute

u64 kGetPerformanceFrequency(void) {
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

u64 kGetPerformanceCounter(void) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

void kTerminate(uint code) {
	ExitProcess(code);
}

//
//
//

typedef struct kPlatformWindow {
	HWND            wnd;
	u32             high_surrogate;
	u16             raw_input;
	u32             dpi;
	WINDOWPLACEMENT placement;
} kPlatformWindow;

static DWORD kWinGetWindowStyle(u32 flags) {
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if (flags & kWindow_Resizable) {
		style |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
	}
	return style;
}

void kResizeWindow(u32 w, u32 h) {
	kPlatformWindow *window = media.window.native;
	DWORD style = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	RECT rect = {
		.left = 0,
		.right = w,
		.top = 0,
		.bottom = h
	};
	AdjustWindowRectExForDpi(&rect, style, FALSE, 0, window->dpi);
	int width  = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	SetWindowPos(window->wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

	media.window.state.width = w;
	media.window.state.height = h;
}

void kToggleWindowFullscreen(void) {
	kPlatformWindow *window = media.window.native;
	DWORD dw_style = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	if ((media.window.state.flags & kWindow_Fullscreen) == 0) {
		MONITORINFO mi = { .cbSize = sizeof(mi) };
		if (GetWindowPlacement(window->wnd, &window->placement) &&
			GetMonitorInfoW(MonitorFromWindow(window->wnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
			SetWindowLongPtrW(window->wnd, GWL_STYLE, 0);
			SetWindowPos(window->wnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
		media.window.state.flags |= kWindow_Fullscreen;
	}
	else {
		DWORD style = kWinGetWindowStyle(media.window.state.flags);
		SetWindowLongPtrW(window->wnd, GWL_STYLE, style);
		SetWindowPlacement(window->wnd, &window->placement);
		SetWindowPos(window->wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		media.window.state.flags &= ~kWindow_Fullscreen;
	}
}

static void kWinClipCursor(void) {
	kPlatformWindow *window = media.window.native;

	RECT rect;
	GetClientRect(window->wnd, &rect);

	POINT pt = { rect.left, rect.top };
	POINT pt2 = { rect.right, rect.bottom };
	ClientToScreen(window->wnd, &pt);
	ClientToScreen(window->wnd, &pt2);

	SetRect(&rect, pt.x, pt.y, pt2.x, pt2.y);
	ClipCursor(&rect);

	POINT center_pt = {
		.x = (rect.right - rect.left) / 2,
		.y = (rect.bottom - rect.top) / 2
	};
	ClientToScreen(window->wnd, &center_pt);
	SetCursorPos(center_pt.x, center_pt.y);
}

static void kWinForceEnableCursor(void) {
	ShowCursor(TRUE);
	ClipCursor(NULL);

	RAWINPUTDEVICE rid = {
		.usUsagePage   = 0x1,
		.usUsage       = 0x2,
		.dwFlags       = 0,
		.hwndTarget    = media.window.native->wnd
	};
	media.window.native->raw_input = RegisterRawInputDevices(&rid, 1, sizeof(rid));

	media.window.exflags &= ~kWindowEx_CursorDisabled;
}

static void kWinForceDisableCursor(void) {
	kPlatformWindow *window = media.window.native;
	if (GetActiveWindow() == window->wnd) {
		ShowCursor(FALSE);
		kWinClipCursor();
	}

	RAWINPUTDEVICE rid = {
		.usUsagePage   = 0x1,
		.usUsage       = 0x2,
		.dwFlags       = RIDEV_REMOVE,
		.hwndTarget    = media.window.native->wnd
	};
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	media.window.exflags |= ~kWindowEx_CursorDisabled;
	media.window.native->raw_input = false;
}

void kEnableCursor(void) {
	if (kIsCursorEnabled())
		return;
	kWinForceEnableCursor();
}

void kDisableCursor(void) {
	if (!kIsCursorEnabled())
		return;
	kWinForceDisableCursor();
}

void kMaximizeWindow(void) {
	ShowWindow(media.window.native->wnd, SW_MAXIMIZE);
}

void kRestoreWindow(void) {
	ShowWindow(media.window.native->wnd, SW_RESTORE);
}

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

	[VK_F1]  = kKey_F1,  [VK_F2]  = kKey_F2,
	[VK_F3]  = kKey_F3,  [VK_F4]  = kKey_F4,
	[VK_F5]  = kKey_F5,  [VK_F6]  = kKey_F6,
	[VK_F7]  = kKey_F7,  [VK_F8]  = kKey_F8,
	[VK_F9]  = kKey_F9,  [VK_F10] = kKey_F10,
	[VK_F11] = kKey_F11, [VK_F12] = kKey_F12,

	[VK_SNAPSHOT] = kKey_PrintScreen, [VK_INSERT]  = kKey_Insert,
	[VK_HOME]     = kKey_Home,        [VK_PRIOR]   = kKey_PageUp,
	[VK_NEXT]     = kKey_PageDown,    [VK_DELETE]  = kKey_Delete,
	[VK_END]      = kKey_End,         [VK_RIGHT]   = kKey_Right,
	[VK_LEFT]     = kKey_Left,        [VK_DOWN]    = kKey_Down,
	[VK_UP]       = kKey_Up,          [VK_DIVIDE]  = kKey_Divide,
	[VK_MULTIPLY] = kKey_Multiply,    [VK_ADD]     = kKey_Plus,
	[VK_SUBTRACT] = kKey_Minus,       [VK_DECIMAL] = kKey_Period,
	[VK_OEM_3]    = kKey_BackTick,    [VK_CONTROL] = kKey_Ctrl,
	[VK_RETURN]   = kKey_Return,      [VK_ESCAPE]  = kKey_Escape,
	[VK_BACK]     = kKey_Backspace,   [VK_TAB]     = kKey_Tab,
	[VK_SPACE]    = kKey_Space,       [VK_SHIFT]   = kKey_Shift,
};

static const DWORD InvVirtualKeyMap[] = {
	[kKey_A] = 'A', [kKey_B] = 'B',
	[kKey_C] = 'C', [kKey_D] = 'D',
	[kKey_E] = 'E', [kKey_F] = 'F',
	[kKey_G] = 'G', [kKey_H] = 'H',
	[kKey_I] = 'I', [kKey_J] = 'J',
	[kKey_K] = 'K', [kKey_L] = 'L',
	[kKey_M] = 'M', [kKey_N] = 'N',
	[kKey_O] = 'O', [kKey_P] = 'P',
	[kKey_Q] = 'Q', [kKey_R] = 'R',
	[kKey_S] = 'S', [kKey_T] = 'T',
	[kKey_U] = 'U', [kKey_V] = 'V',
	[kKey_W] = 'W', [kKey_X] = 'X',
	[kKey_Y] = 'Y', [kKey_Z] = 'Z',

	[kKey_0] = '0', [kKey_1] = '1',
	[kKey_2] = '2', [kKey_3] = '3',
	[kKey_4] = '4', [kKey_5] = '5',
	[kKey_6] = '6', [kKey_7] = '7',
	[kKey_8] = '8', [kKey_9] = '9',

	[kKey_0] = VK_NUMPAD0, [kKey_1] = VK_NUMPAD1,
	[kKey_2] = VK_NUMPAD2, [kKey_3] = VK_NUMPAD3,
	[kKey_4] = VK_NUMPAD4, [kKey_5] = VK_NUMPAD5,
	[kKey_6] = VK_NUMPAD6, [kKey_7] = VK_NUMPAD7,
	[kKey_8] = VK_NUMPAD8, [kKey_9] = VK_NUMPAD9,

	[kKey_F1]  = VK_F1,  [kKey_F2]  = VK_F2,
	[kKey_F3]  = VK_F3,  [kKey_F4]  = VK_F4,
	[kKey_F5]  = VK_F5,  [kKey_F6]  = VK_F6,
	[kKey_F7]  = VK_F7,  [kKey_F8]  = VK_F8,
	[kKey_F9]  = VK_F9,  [kKey_F10] = VK_F1,
	[kKey_F11] = VK_F11, [kKey_F12] = VK_F1,

	[kKey_PrintScreen] = VK_SNAPSHOT, [kKey_Insert] = VK_INSERT,
	[kKey_Home]        = VK_HOME,     [kKey_PageUp] = VK_PRIOR,
	[kKey_PageDown]    = VK_NEXT,     [kKey_Delete] = VK_DELETE,
	[kKey_End]         = VK_END,      [kKey_Right]  = VK_RIGHT,
	[kKey_Left]        = VK_LEFT,     [kKey_Down]   = VK_DOWN,
	[kKey_Up]          = VK_UP,       [kKey_Divide] = VK_DIVIDE,
	[kKey_Multiply]    = VK_MULTIPLY, [kKey_Plus]   = VK_ADD,
	[kKey_Minus]       = VK_SUBTRACT, [kKey_Period] = VK_DECIMAL,
	[kKey_BackTick]    = VK_OEM_3,    [kKey_Ctrl]   = VK_CONTROL,
	[kKey_Return]      = VK_RETURN,   [kKey_Escape] = VK_ESCAPE,
	[kKey_Backspace]   = VK_BACK,     [kKey_Tab]    = VK_TAB,
	[kKey_Space]       = VK_SPACE,    [kKey_Shift]  = VK_SHIFT,
};

static u32 kWinGetKeyModFlags(void) {
	u32 mods = 0;
	if (GetKeyState(VK_LSHIFT)   & 0x8000) mods |= kKeyMod_LeftShift;
	if (GetKeyState(VK_RSHIFT)   & 0x8000) mods |= kKeyMod_RightShift;
	if (GetKeyState(VK_LCONTROL) & 0x8000) mods |= kKeyMod_LeftCtrl;
	if (GetKeyState(VK_RCONTROL) & 0x8000) mods |= kKeyMod_RightCtrl;
	if (GetKeyState(VK_LMENU)    & 0x8000) mods |= kKeyMod_LeftAlt;
	if (GetKeyState(VK_RMENU)    & 0x8000) mods |= kKeyMod_RightAlt;
	return mods;
}

//static LRESULT wWinHitTestNCA(HWND wnd, WPARAM wparam, LPARAM lparam) {
//	POINT cursor = {
//		.x = GET_X_LPARAM(lparam),
//		.y = GET_Y_LPARAM(lparam)
//	};
//
//	RECT rc;
//	GetWindowRect(wnd, &rc);
//
//	RECT frame = { 0 };
//	AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL, media.window.native->dpi);
//
//	USHORT row = 1;
//	USHORT col = 1;
//	bool border_resize = false;
//
//	if (cursor.y >= rc.top && cursor.y < rc.top + TOPEXTENDWIDTH) {
//		border_resize = (cursor.y < (rc.top - frame.top));
//		row = 0;
//	} else if (cursor.y < rc.bottom && cursor.y >= rc.bottom - BOTTOMEXTENDWIDTH) {
//		row = 2;
//	}
//
//	if (cursor.x >= rc.left && cursor.x < rc.left + LEFTEXTENDWIDTH) {
//		col = 0;
//	} else if (cursor.x < rc.right && cursor.x >= rc.right - RIGHTEXTENDWIDTH) {
//		col = 2;
//	}
//
//	LRESULT hit_tests[3][3] =
//	{
//		{ HTTOPLEFT, border_resize ? HTTOP : HTCAPTION, HTTOPRIGHT },
//		{ HTLEFT, HTNOWHERE, HTRIGHT },
//		{ HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
//	};
//
//	return hit_tests[row][col];
//}

static bool kWinHandleCustomCaptionEvents(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *result) {
	*result      = 0;
	bool forward = !DwmDefWindowProc(wnd, msg, wparam, lparam, result);

	if (msg == WM_CREATE) {
		RECT rc;
		GetWindowRect(wnd, &rc);

		int x = rc.left;
		int y = rc.top;
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		SetWindowPos(wnd, 0, x, y, w, h, SWP_FRAMECHANGED);

		return true;
	}

	if (msg == WM_ACTIVATE) {
		MARGINS margins = { 0 };
		DwmExtendFrameIntoClientArea(wnd, &margins);
		return true;
	}

	if (msg == WM_PAINT) {
	/*	HDC hdc;
		PAINTSTRUCT ps;
		hdc = BeginPaint(wnd, &ps);
		PaintCustomCaption(wnd, hdc);
		EndPaint(wnd, &ps);*/
		return true;
	}

	if ((msg == WM_NCCALCSIZE) && (wparam == TRUE)) {
		*result = 0;
		return false;
	}

	//if ((msg == WM_NCHITTEST) && (*result == 0)) {
	//	*result = kWinHitTestNCA(wnd, wparam, lparam);

	//	if (*result != HTNOWHERE) {
	//		return false;
	//	}
	//}

	return forward;
}

static LRESULT kWinHandleEvents(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_ACTIVATE:
		{
			int state = LOWORD(wparam);
			bool focused = (state == WA_ACTIVE || state == WA_CLICKACTIVE);
			kAddWindowFocusEvent(focused);

			if (!kIsCursorEnabled()) {
				if (focused) {
					kWinForceDisableCursor();
				}
				else {
					kWinForceEnableCursor();
				}
			}

			return DefWindowProcW(wnd, msg, wparam, lparam);
		}

		case WM_CLOSE:
		{
			kAddWindowCloseEvent();
			return 0;
		}

		case WM_SIZE:
		{
			media.window.exflags |= kWindowEx_Resized;

			if (!kIsCursorEnabled() && kIsWindowFocused()) {
				kWinClipCursor();
			}

			if (wparam == SIZE_MAXIMIZED) {
				media.window.state.flags |= kWindow_Maximized;
			} else if (wparam == SIZE_RESTORED) {
				media.window.state.flags &= ~kWindow_Maximized;
			}

			return DefWindowProcW(wnd, msg, wparam, lparam);
		}

		case WM_MOUSELEAVE:
		{
			kAddCursorLeaveEvent();
			return 0;
		} break;

		case WM_MOUSEMOVE:
		{
			if (!kIsCursorHovered()) {
				TRACKMOUSEEVENT tme = {
					.cbSize    = sizeof(tme),
					.dwFlags   = TME_LEAVE,
					.hwndTrack = wnd
				};
				TrackMouseEvent(&tme);
				kAddCursorEnterEvent();
			}

			if (kIsCursorEnabled() || !media.window.native->raw_input) {
				int x         = GET_X_LPARAM(lparam);
				int y         = GET_Y_LPARAM(lparam);
				kVec2i cursor = { .x = x, .y = media.window.state.height - y };
				kAddCursorEvent(cursor);
			}

			return 0;
		}

		case WM_INPUT:
		{
			HRAWINPUT hri = (HRAWINPUT)lparam;

			UINT rid_size = 0;
			GetRawInputData(hri, RID_INPUT, 0, &rid_size, sizeof(RAWINPUTHEADER));

			RAWINPUT *input = rid_size ? alloca(rid_size) : 0;
			if (!input) break;

			if (GetRawInputData(hri, RID_INPUT, input, &rid_size, sizeof(RAWINPUTHEADER) != rid_size))
				break;

			if (input->header.dwType != RIM_TYPEMOUSE)
				break;

			RAWMOUSE *mouse = &input->data.mouse;
			UINT     dpi    = media.window.native->dpi;

			if ((mouse->usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE) {
				bool virtual  = (mouse->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
				int width     = GetSystemMetricsForDpi(virtual ? SM_CXVIRTUALSCREEN : SM_CXSCREEN, dpi);
				int height    = GetSystemMetricsForDpi(virtual ? SM_CYVIRTUALSCREEN : SM_CYSCREEN, dpi);
				int abs_x     = (int)((mouse->lLastX / 65535.0f) * width);
				int abs_y     = (int)((mouse->lLastY / 65535.0f) * height);
				POINT pt      = { .x = abs_x, .y = abs_y };
				ScreenToClient(wnd, &pt);
				kVec2i cursor = { .x = pt.x, .y = media.window.state.height - pt.y };
				kAddCursorEvent(cursor);
			} else if (mouse->lLastX != 0 || mouse->lLastY != 0) {
				int rel_x     = mouse->lLastX;
				int rel_y     = mouse->lLastY;
				kVec2i delta  = { .x = rel_x, .y = -rel_y };
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
			if (wparam < kArrayCount(VirtualKeyMap)) {
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
			if (wparam == VK_F10) {
				bool repeat = HIWORD(lparam) & KF_REPEAT;
				bool down   = (msg == WM_KEYDOWN);
				kAddKeyEvent(kKey_F10, down, repeat);
			}
			return DefWindowProcW(wnd, msg, wparam, lparam);
		}

		case WM_CHAR:
		{
			if (IS_HIGH_SURROGATE(wparam)) {
				media.window.native->high_surrogate = (u32)(wparam & 0xFFFF);
			} else {
				if (IS_LOW_SURROGATE(wparam)) {
					if (media.window.native->high_surrogate) {
						u32 codepoint = 0;
						codepoint += (media.window.native->high_surrogate - 0xD800) << 10;
						codepoint += (u16)wparam - 0xDC00;
						codepoint += 0x10000;
						u32 mods   = kWinGetKeyModFlags();
						kAddTextInputEvent(codepoint, mods);
					}
				} else {
					u32 mods = kWinGetKeyModFlags();
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

			media.window.native->dpi = HIWORD(wparam);
			float yfactor = (float)media.window.native->dpi / USER_DEFAULT_SCREEN_DPI;
			kAddWindowDpiChangedEvent(yfactor);
			return 0;
		}
	}

	return DefWindowProcW(wnd, msg, wparam, lparam);
}

static LRESULT CALLBACK kWinProcessWindowsEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if ((media.window.state.flags & kWindow_NoTitleBar)) {
		LRESULT result = 0;
		if (!kWinHandleCustomCaptionEvents(wnd, msg, wparam, lparam, &result))
			return result;
	}
	return kWinHandleEvents(wnd, msg, wparam, lparam);
}

static void kWinDestroyWindow(void) {
	kPlatformWindow *window = media.window.native;
	DestroyWindow(window->wnd);
	kFree(window, sizeof(*window));
}

static void kWinCreateWindow(const char *mb_title, uint w, uint h, uint flags) {
	HMODULE     instance           = GetModuleHandleW(0);
	WNDCLASSEXW window_class       = { 0 };

	{
		HICON   icon     = LoadImageW(instance, MAKEINTRESOURCEW(IDI_KICON), IMAGE_ICON, 0, 0, LR_SHARED);
		HICON   icon_sm  = LoadImageW(instance, MAKEINTRESOURCEW(IDI_KICON), IMAGE_ICON, 0, 0, LR_SHARED);
		HICON   win_icon = LoadImageW(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);

		window_class.cbSize        = sizeof(window_class);
		window_class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		window_class.lpfnWndProc   = kWinProcessWindowsEvent;
		window_class.hInstance     = instance;
		window_class.hIcon         = icon ? icon : win_icon;
		window_class.hIconSm       = icon_sm ? icon_sm : win_icon;
		window_class.hCursor       = LoadImageW(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
		window_class.hbrBackground = GetStockObject(BLACK_BRUSH);
		window_class.lpszClassName = L"KrWindowClass";
		RegisterClassExW(&window_class);
	}

	wchar_t title[2048] = L"KrWindow | Windows";

	if (mb_title) {
		MultiByteToWideChar(CP_UTF8, 0, (char *)mb_title, -1, title, kArrayCount(title));
	}

	int width      = CW_USEDEFAULT;
	int height     = CW_USEDEFAULT;

	DWORD style    = kWinGetWindowStyle(flags);

	if (w > 0 && h > 0) {
		POINT    origin   = { .x = 0, .y = 0 };
		HMONITOR mprimary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
		UINT     xdpi     = 0;
		UINT     ydpi     = 0;
		UINT     dpi      = GetDpiForMonitor(mprimary, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);

		RECT rect   = {
			.left   = 0,
			.right  = w,
			.top    = 0,
			.bottom = h
		};

		AdjustWindowRectExForDpi(&rect, style, FALSE, 0, ydpi);
		width  = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	// Set the flags before CreateWindow is called, since WndProc uses kWindow_NoTitleBar flag
	media.window.state.flags = flags;

	HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, window_class.lpszClassName, title, style,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, instance, 0);

	if (!hwnd) {
		kWinLogError(GetLastError(), "Windows", "Failed to create window");
		return;
	}

	BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

	DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

	kPlatformWindow *window = kAlloc(sizeof(kPlatformWindow));
	if (!window) {
		kLogError("Windows: Failed to create window: out of memory");
		return;
	}

	memset(window, 0, sizeof(*window));

	window->wnd       = hwnd;

	SetWindowLongPtrW(window->wnd, GWLP_USERDATA, (LONG_PTR)window);

	ShowWindow(window->wnd, SW_SHOWNORMAL);
	UpdateWindow(window->wnd);

	media.window.native = window;

	if (flags & kWindow_Fullscreen) {
		kToggleWindowFullscreen();
	}
}

static void kWinLoadInitialState(void) {
	RECT rc = {0};
	GetClientRect(media.window.native->wnd, &rc);

	media.window.state.width  = rc.right - rc.left;
	media.window.state.height = rc.bottom - rc.top;

	if (GetActiveWindow() == media.window.native->wnd) {
		media.window.exflags |= kWindowEx_Focused;
	}

	POINT pt = {0};
	GetCursorPos(&pt);
	ScreenToClient(media.window.native->wnd, &pt);

	media.mouse.cursor.x = pt.x;
	media.mouse.cursor.y = media.window.state.height - pt.y;

	if (PtInRect(&rc, pt)) {
		media.window.exflags |= kWindowEx_CursorHovered;
	}

	media.mouse.buttons[kButton_Left].down   = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
	media.mouse.buttons[kButton_Right].down  = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	media.mouse.buttons[kButton_Middle].down = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

	for (uint key = 0; key < kKey_Count; ++key) {
		media.keyboard.keys[key].down = GetAsyncKeyState(InvVirtualKeyMap[key]) & 0x8000;
	}

	u32 mods = 0;
	if (GetAsyncKeyState(VK_LSHIFT)   & 0x8000) mods |= kKeyMod_LeftShift;
	if (GetAsyncKeyState(VK_RSHIFT)   & 0x8000) mods |= kKeyMod_RightShift;
	if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) mods |= kKeyMod_LeftCtrl;
	if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) mods |= kKeyMod_RightCtrl;
	if (GetAsyncKeyState(VK_LMENU)    & 0x8000) mods |= kKeyMod_LeftAlt;
	if (GetAsyncKeyState(VK_RMENU)    & 0x8000) mods |= kKeyMod_RightAlt;

	media.keyboard.mods    = mods;
	media.events.count     = 0;
	media.events.allocated = 64;
	media.events.data      = kAlloc(sizeof(kEvent) * media.events.allocated);
}

static int kWinRunEventLoop(void) {
	int   status       = 0;
	float dt           = 1.0f / 60.0f;
	u64   counter      = kGetPerformanceCounter();
	u64   frequency    = kGetPerformanceFrequency();

	while (1) {
		kNextFrame();

		MSG msg;
		while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				status = (int)msg.wParam;
				return status;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (media.window.exflags & kWindowEx_Resized) {
			RECT rc;
			GetClientRect(media.window.native->wnd, &rc);

			media.window.state.width  = (u32)(rc.right - rc.left);
			media.window.state.height = (u32)(rc.bottom - rc.top);

			kEvent ev    = {
				.kind    = kEvent_Resized,
				.resized = { .width = media.window.state.width, .height = media.window.state.height }
			};
			kAddEvent(&ev);
		}

		media.keyboard.mods = kWinGetKeyModFlags();

		media.user.update(dt);

		if (kIsWindowClosed())
			break;

		u64 new_counter = kGetPerformanceCounter();
		u64 counts      = new_counter - counter;
		counter         = new_counter;
		dt              = (float)(((1000000.0 * (double)counts) / (double)frequency) / 1000000.0);
	}

	return status;
}

int kEventLoop(const kMediaSpec *spec, kMediaUserEvents user) {
	memset(&media, 0, sizeof(media));

	kWinCreateWindow(spec->window.title, spec->window.width, spec->window.height, spec->window.flags);
	kWinLoadInitialState();

	kSetUserEvents(&user);

	media.user.load();

	int status = kWinRunEventLoop();

	media.user.release();

	kWinDestroyWindow();

	return status;
}

void kBreakLoop(int status) {
	PostQuitMessage(status);
}

//
// Main
//

extern void Main(int argc, const char **argv);

static int kMain(void) {
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

#endif
