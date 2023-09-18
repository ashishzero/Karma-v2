#include "kPlatform.h"

void kClearInputEx(kContext *context) {
	memset(&context->media.keyboard, 0, sizeof(context->media.keyboard));
	memset(&context->media.mouse, 0, sizeof(context->media.mouse));
}

void kNextFrameEx(kContext *context) {
	context->media.keyboard.mods = 0;

	for (uint key = 0; key < kKey_Count; ++key) {
		context->media.keyboard.keys[key].flags = 0;
		context->media.keyboard.keys[key].hits  = 0;
	}

	for (uint button = 0; button < kButton_Count; ++button) {
		context->media.mouse.buttons[button].flags = 0;
		context->media.mouse.buttons[button].hits  = 0;
	}

	memset(&context->media.mouse.wheel, 0, sizeof(context->media.mouse.wheel));

	context->media.window.resized = 0;
	context->media.window.closed  = 0;
}

void kAddEventEx(kContext *context, kEvent ev) {
	if (context->media.events.count == context->media.events.allocated) {
		int prev = context->media.events.allocated;
		int next = Max(prev ? prev << 1 : 64, prev + 1);
		context->media.events.data      = kReallocEx(&context->allocator, context->media.events.data, prev * sizeof(kEvent), next * sizeof(kEvent));
		context->media.events.allocated = next;
	}
	context->media.events.data[context->media.events.count++] = ev;
}

void kAddKeyEventEx(kContext *context, kKey key, bool down, bool repeat) {
	bool pressed  = down && !repeat;
	bool released = !down;
	kState *state = &context->media.keyboard.keys[key];
	state->down   = down;

	if (released) {
		state->flags |= kReleased;
	}

	if (pressed) {
		state->flags |= kPressed;
		state->hits += 1;
	}

	kEvent ev = {
		.kind = down ? kEvent_KeyPressed : kEvent_KeyReleased,
		.key  = { .symbol = key, .repeat = repeat }
	};

	kAddEventEx(context, ev);
}

void kAddButtonEventEx(kContext *context, kButton button, bool down) {
	kState *state = &context->media.mouse.buttons[button];
	bool previous = state->down;
	bool pressed  = down && !previous;
	bool released = !down && previous;
	state->down   = down;

	if (released) {
		state->flags |= kReleased;
		state->hits  += 1;
	}

	if (pressed) {
		state->flags |= kPressed;
	}

	kEvent ev   = {
		.kind   = down ? kEvent_ButtonPressed : kEvent_ButtonReleased,
		.button = { .symbol = button }
	};

	kAddEventEx(context, ev);
}

void kAddDoubleClickEventEx(kContext *context, kButton button) {
	kState *state = &context->media.mouse.buttons[button];
	state->flags |= kDoubleClicked;

	kEvent ev   = {
		.kind   = kEvent_DoubleClicked,
		.button = { .symbol = button }
	};

	kAddEventEx(context, ev);
}

void kAddCursorEventEx(kContext *context, kVec2i pos, kVec2 delta) {
	context->media.mouse.cursor = pos;
	context->media.mouse.delta  = delta;

	kEvent ev   = {
		.kind   = kEvent_CursorMoved,
		.cursor = { .position = pos }
	};

	kAddEventEx(context, ev);
}

void kAddWheelEventEx(kContext *context, float horz, float vert) {
	context->media.mouse.wheel.x += horz;
	context->media.mouse.wheel.y += vert;

	kEvent ev = {
		.kind  = kEvent_WheelMoved,
		.wheel = { .horizontal = horz, .vertical = vert }
	};

	kAddEventEx(context, ev);
}

void kAddWindowResizeEventEx(kContext *context, u32 width, u32 height, bool fullscreen) {
	context->media.window.width  = width;
	context->media.window.height = height;

	if (fullscreen) {
		context->media.window.flags |= kWindow_Fullscreen;
	} else {
		context->media.window.flags &= ~kWindow_Fullscreen;
	}

	context->media.window.resized = 1;

	kEvent ev    = {
		.kind    = kEvent_Resized,
		.resized = { .width = width, .height = height }
	};

	kAddEventEx(context, ev);
}

void kAddWindowFocusEventEx(kContext *context, bool focused) {
	if (focused) {
		context->media.window.flags |= kWindow_Focused;
	} else {
		context->media.window.flags &= ~kWindow_Focused;
	}

	kEvent ev = {
		.kind = focused ? kEvent_Activated : kEvent_Deactivated
	};

	kAddEventEx(context, ev);
}

void kAddWindowCloseEventEx(kContext *context) {
	context->media.window.closed = 1;

	kEvent ev = {
		.kind = kEvent_Closed
	};

	kAddEventEx(context, ev);
}

//
//
//

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"

typedef struct kWinWindow {
	HWND            wnd;
	int             monitor_w;
	int             monitor_h;
	u16             resized;
	DWORD           style;
	WINDOWPLACEMENT placement;
} kWinWindow;

static kWinWindow *kWinGetWindow(kContext *context) {
	kWindow     id     = context->media.window.handle;
	kWinWindow *window = (kWinWindow *)id.ptr;
	return window;
}

void kPlatformResizeWindow(kContext *context, u32 w, u32 h) {
	kWinWindow *window = kWinGetWindow(context);
	DWORD style = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	RECT rect   = {
		.left   = 0,
		.right  = w,
		.top    = 0,
		.bottom = h
	};
	AdjustWindowRect(&rect, style, FALSE);
	int width  = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	SetWindowPos(window->wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

void kPlatformToggleWindowFullscreen(kContext *context) {
	kWinWindow *window = kWinGetWindow(context);
	DWORD dw_style = (DWORD)GetWindowLongPtrW(window->wnd, GWL_STYLE);
	if (dw_style & WS_OVERLAPPEDWINDOW) {
		MONITORINFO mi = { .cbSize = sizeof(mi) };
		if (GetWindowPlacement(window->wnd, &window->placement) &&
			GetMonitorInfoW(MonitorFromWindow(window->wnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
			SetWindowLongPtrW(window->wnd, GWL_STYLE, dw_style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(window->wnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			window->style = dw_style;
		}
	} else {
		SetWindowLongPtrW(window->wnd, GWL_STYLE, window->style);
		SetWindowPlacement(window->wnd, &window->placement);
		SetWindowPos(window->wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

static void kWinClipCursor(kWinWindow *window, RECT *rect) {
	POINT pt  = { rect->left, rect->top };
	POINT pt2 = { rect->right, rect->bottom };
	ClientToScreen(window->wnd, &pt);
	ClientToScreen(window->wnd, &pt2);

	RECT sr;
	SetRect(&sr, pt.x, pt.y, pt2.x, pt2.y);
	ClipCursor(&sr);

	int center_x = sr.left + (sr.right - sr.left) / 2;
	int center_y = sr.top + (sr.bottom - sr.top) / 2;

	SetCursorPos(center_x, center_y);
}

void kPlatformEnableCursor(kContext *context) {
	kWinWindow *window = kWinGetWindow(context);
	CURSORINFO ci = { .cbSize = sizeof(ci) };
	if (GetCursorInfo(&ci) && (ci.flags == 0))
		return;
	ShowCursor(FALSE);
	RECT rect;
	GetClientRect(window->wnd, &rect);
	kWinClipCursor(window, &rect);
}

void kPlatformDisableCursor(kContext *io) {
	CURSORINFO ci = { .cbSize = sizeof(ci) };
	if (GetCursorInfo(&ci) && (ci.flags != 0))
		return;
	ShowCursor(TRUE);
	ClipCursor(NULL);
}

#endif
