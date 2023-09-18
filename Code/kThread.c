#include "kThread.h"

#if K_PLATFORM_WINDOWS == 1

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <avrt.h>

#pragma comment(lib, "Avrt.lib")

typedef struct ThreadUserData {
	kThreadProc      proc;
	void *          data;
	kThreadAttribute attr;
	HANDLE          event;
} ThreadUserData;

static void kSetThreadAttribute(kThreadAttribute attr) {
	const wchar_t *Name[] = {
		L"Audio", L"Capture", L"Distribution", L"Games", L"Playback", L"Pro Audio"
	};
	DWORD task_index = 0;
	HANDLE task_handle = AvSetMmThreadCharacteristicsW(Name[attr], &task_index);
	(void)task_handle;
}

static DWORD WINAPI kThreadStartRoutine(LPVOID thread_param) {
	ThreadUserData thrd = *(ThreadUserData *)thread_param;
	kSetThreadAttribute(thrd.attr);
	SetEvent(thrd.event);
	return thrd.proc(thrd.data);
}

kThread kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr) {
	ThreadUserData data;
	data.proc       = proc;
	data.data       = arg;
	data.attr       = attr;
	data.event      = CreateEventW(0, 0, 0, 0);
	HANDLE handle   = CreateThread(NULL, 0, kThreadStartRoutine, &data, 0, NULL);
	if (handle)
		WaitForSingleObject(data.event, INFINITE);
	CloseHandle(data.event);
	return (kThread) { .ptr = handle };
}

kThread kGetCurrentThread(void) {
	HANDLE handle = GetCurrentThread();
	return (kThread ) { .ptr = handle };
}

void kDetachThread(kThread thread) {
	CloseHandle((HANDLE)thread.ptr);
}

void kWaitThread(kThread thread) {
	WaitForSingleObject((HANDLE)thread.ptr, INFINITE);
	CloseHandle((HANDLE)thread.ptr);
}

void kTerminateThread(kThread thread, uint code) {
	TerminateThread((HANDLE)thread.ptr, code);
	CloseHandle((HANDLE)thread.ptr);
}

void kSleep(u32 millisecs) {
	Sleep(millisecs);
}

void kYield(void) {
	SwitchToThread();
}

void kInitSemaphore(kSemaphore *sem, u32 value, u32 max_value) {
	static_assert(sizeof(HANDLE) >= sizeof(kSemaphore), "");

	kAssert(max_value > 0);
	sem->_id = CreateSemaphoreW(NULL, value, max_value, NULL);
}

void kFreeSemaphore(kSemaphore *sem) {
	HANDLE handle = sem->_id;
	CloseHandle(handle);
}

u32 kReleaseSemaphore(kSemaphore *sem, u32 count) {
	LONG out;
	HANDLE handle = sem->_id;
	ReleaseSemaphore(handle, count, &out);
	return (u32)out;
}

void kWaitSemaphore(kSemaphore *sem) {
	HANDLE handle = sem->_id;
	WaitForSingleObject(handle, INFINITE);
}

int kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs) {
	HANDLE handle = sem->_id;
	DWORD res = WaitForSingleObject(handle, millisecs);
	if (res == WAIT_OBJECT_0)
		return 1;
	if (res == WAIT_TIMEOUT)
		return -1;
	return 0;
}

void kInitMutex(kMutex *mutex) {
	static_assert(sizeof(CRITICAL_SECTION) <= sizeof(kMutex), "");
	InitializeCriticalSection((CRITICAL_SECTION *)mutex);
}

void kFreeMutex(kMutex *mutex) {
	DeleteCriticalSection((CRITICAL_SECTION *)mutex);
	memset(mutex, 0, sizeof(*mutex));
}

void kLockMutex(kMutex *mutex) {
	EnterCriticalSection((CRITICAL_SECTION *)mutex);
}

bool kTryLockMutex(kMutex *mutex) {
	return TryEnterCriticalSection((CRITICAL_SECTION *)mutex);
}

void kUnlockMutex(kMutex *mutex) {
	LeaveCriticalSection((CRITICAL_SECTION *)mutex);
}

void kInitCondVar(kCondVar *cond) {
	static_assert(sizeof(CONDITION_VARIABLE) <= sizeof(kCondVar), "");
	InitializeConditionVariable((CONDITION_VARIABLE *)cond);
}

void kFreeCondVar(kCondVar *cond) {
}

void kSignalCondVar(kCondVar *cond) {
	WakeConditionVariable((CONDITION_VARIABLE *)cond);
}

void kBroadcastCondVar(kCondVar *cond) {
	WakeAllConditionVariable((CONDITION_VARIABLE *)cond);
}

bool kWaitCondVar(kCondVar *cond, kMutex *mutex) {
	return SleepConditionVariableCS((CONDITION_VARIABLE *)cond, (CRITICAL_SECTION *)mutex, INFINITE);
}

int kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs) {
	if (SleepConditionVariableCS((CONDITION_VARIABLE *)cond, (CRITICAL_SECTION *)mutex, millisecs))
		return 1;
	if (GetLastError() == ERROR_TIMEOUT)
		return -1;
	return 0;
}

#endif

//#include "KrPlatform.h"
//
//#include <string.h>
//
//void kFallbackUserLoad(struct kMediaIo *io)             {}
//void kFallbackUserRelease(struct kMediaIo *io)          {}
//void kFallbackUserUpdate(struct kMediaIo *io, float dt) {}
//
//void kClearInput(kMediaIo *io) {
//	memset(&io->keyboard, 0, sizeof(io->keyboard));
//	memset(&io->mouse, 0, sizeof(io->mouse));
//}
//
//void kNextFrame(kMediaIo *io) {
//	io->keyboard.mods = 0;
//
//	memset(&io->keyboard.text, 0, sizeof(io->keyboard.text));
//
//	for (uint key = 0; key < kKey_Count; ++key) {
//		io->keyboard.keys[key].flags = 0;
//		io->keyboard.keys[key].hits  = 0;
//	}
//
//	for (uint button = 0; button < kButton_Count; ++button) {
//		io->mouse.buttons[button].flags = 0;
//		io->mouse.buttons[button].hits  = 0;
//	}
//
//	memset(&io->mouse.wheel, 0, sizeof(io->mouse.wheel));
//
//	io->window.resized = 0;
//	io->window.closed  = 0;
//}
//
//void kPushKeyEvent(kMediaIo *io, kKey key, bool down, bool repeat) {
//	bool pressed  = down && !repeat;
//	bool released = !down;
//	kState *state = &io->keyboard.keys[key];
//	state->down   = down;
//
//	if (released) {
//		state->flags |= kReleased;
//	}
//
//	if (pressed) {
//		state->flags |= kPressed;
//		state->hits += 1;
//	}
//}
//
//void kPushButtonEvent(kMediaIo *io, kButton button, bool down) {
//	kState *state = &io->mouse.buttons[button];
//	bool previous = state->down;
//	bool pressed  = down && !previous;
//	bool released = !down && previous;
//	state->down   = down;
//
//	if (released) {
//		state->flags |= kReleased;
//		state->hits  += 1;
//	}
//
//	if (pressed) {
//		state->flags |= kPressed;
//	}
//}
//
//void kPushDoubleClickEvent(kMediaIo *io, kButton button) {
//	kState *state = &io->mouse.buttons[button];
//	state->flags |= kDoubleClicked;
//}
//
//void kPushCursorEvent(kMediaIo *io, kVec2i pos, kVec2 delta) {
//	io->mouse.cursor = pos;
//	io->mouse.delta  = delta;
//}
//
//void kPushWheelEvent(kMediaIo *io, float horz, float vert) {
//	io->mouse.wheel.x += horz;
//	io->mouse.wheel.y += vert;
//}
//
//void kPushWindowResizeEvent(kMediaIo *io, u32 width, u32 height, bool fullscreen) {
//	io->window.width  = width;
//	io->window.height = height;
//
//	if (fullscreen) {
//		io->window.flags |= kWindow_Fullscreen;
//	} else {
//		io->window.flags &= ~kWindow_Fullscreen;
//	}
//
//	io->window.resized = 1;
//}
//
//void kPushWindowBorderEvent(kMediaIo *io, bool resizable) {
//	if (resizable)
//		io->window.flags &= ~kWindow_FixedBorder;
//	else
//		io->window.flags |= kWindow_FixedBorder;
//}
//
//void kPushWindowFocusEvent(kMediaIo *io, bool focused) {
//	if (focused) {
//		io->window.flags |= kWindow_Focused;
//	} else {
//		io->window.flags &= ~kWindow_Focused;
//	}
//}
//
//void kPushWindowCursorEvent(kMediaIo *io, bool shown) {
//	if (shown) {
//		io->window.flags &= ~kWindow_CursorHidden;
//	} else {
//		io->window.flags |= kWindow_CursorHidden;
//	}
//}
//
//void kPushWindowCloseEvent(kMediaIo *io) {
//	io->window.closed = 1;
//}
//
//void kBreakEventLoop(kMediaIo *io, int status) {
//	io->platform.break_event_loop(io, status);
//}
//
//bool kIsWindowClosed(kMediaIo *io) {
//	return io->window.closed;
//}
//
//bool kIsWindowResized(kMediaIo *io) {
//	return io->window.resized;
//}
//
//void kIgnoreWindowCloseEvent(kMediaIo *io) {
//	io->window.closed = false;
//}
//
//bool kIsWindowFocused(kMediaIo *io) {
//	return io->window.flags & kWindow_Focused;
//}
//
//bool kIsWindowFullscreen(kMediaIo *io) {
//	return io->window.flags & kWindow_Fullscreen;
//}
//
//void kGetWindowSize(kMediaIo *io, u32 *w, u32 *h) {
//	*w = io->window.width;
//	*h = io->window.height;
//}
//
//void kSetWindowSize(kMediaIo *io, u32 w, u32 h) {
//	io->platform.resize_window(io, io->window.handle, w, h);
//}
//
//uint kGetWindowFlags(kMediaIo *io) {
//	return io->window.flags;
//}
//
//void kToggleWindowFullscreen(kMediaIo *io) {
//	io->platform.toggle_fullscreen(io, io->window.handle);
//}
//
//void kMakeWindowBorderFixed(kMediaIo *io) {
//	io->platform.make_window_border_fixed(io, io->window.handle);
//}
//
//void kMakeWindowBorderResizable(kMediaIo *io) {
//	io->platform.make_window_border_resizable(io, io->window.handle);
//}
//
//void kDisableCursor(kMediaIo *io) {
//	io->platform.hide_cursor(io);
//}
//
//void kEnableCursor(kMediaIo *io) {
//	io->platform.show_cursor(io);
//}
//
//bool kIsKeyDown(kMediaIo *io, kKey key) {
//	return io->keyboard.keys[key].down;
//}
//
//bool kKeyPressed(kMediaIo *io, kKey key) {
//	return io->keyboard.keys[key].flags & kPressed;
//}
//
//bool kKeyReleased(kMediaIo *io, kKey key) {
//	return io->keyboard.keys[key].flags & kReleased;
//}
//
//u8 kKeyHits(kMediaIo *io, kKey key) {
//	return io->keyboard.keys[key].hits;
//}
//
//bool kIsButtonDown(kMediaIo *io, kButton button) {
//	return io->mouse.buttons[button].down;
//}
//
//bool kButtonPressed(kMediaIo *io, kButton button) {
//	return io->mouse.buttons[button].flags & kPressed;
//}
//
//bool kButtonReleased(kMediaIo *io, kButton button) {
//	return io->mouse.buttons[button].flags & kReleased;
//}
//
//kVec2i kGetCursorScreenPosition(kMediaIo *io) {
//	return io->mouse.cursor;
//}
//
//kVec2 kGetCursorPosition(kMediaIo *io) {
//	kVec2 cursor;
//	cursor.x = (float)io->mouse.cursor.x / io->window.width;
//	cursor.y = (float)io->mouse.cursor.y / io->window.height;
//	return cursor;
//}
//
//kVec2 kGetCursorDelta(kMediaIo *io) {
//	return io->mouse.delta;
//}
//
//float kGetWheelHorizontal(kMediaIo *io) {
//	return io->mouse.wheel.x;
//}
//
//float kGetWheelVertical(kMediaIo *io) {
//	return io->mouse.wheel.y;
//}
