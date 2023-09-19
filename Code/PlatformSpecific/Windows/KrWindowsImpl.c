#include "KrWindowsCommon.h"

#include <avrt.h>
#include <windowsx.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#include <VersionHelpers.h>
#include <objbase.h>
#include <shellapi.h>



//
// Files
//

//
// Window / Events Helpers
//

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
