#include "kMedia.h"
#include "kPlatform.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

static kContext context = {
	.allocator = { .proc  = kDefaultHeapAllocator },
	.random    = { .state = 0x853c49e6748fea9bULL, .inc = 0xda3e39cb94b95bdbULL },
	.assertion = kDefaultHandleAssertion,
	.logger    = { .proc = kDefaultHandleLog },
	.fatal     =  kDefaultFatalError
};

void kHandleAssertion(const char *file, int line, const char *proc, const char *desc) {
	context.assertion(file, line, proc, desc);
	kTriggerBreakpoint();
}

//
//
//

void *kAlloc(umem size) {
	return kAlloc(&context.allocator, size);
}

void *kRealloc(void *ptr, umem prev, umem size) {
	return kRealloc(&context.allocator, ptr, prev, size);
}

void kFree(void *ptr, umem size) {
	kFree(&context.allocator, ptr, size);
}

u32 kRandom(void) {
	return kRandom(&context.random);
}

u32 kRandomBound(u32 bound) {
	return kRandomBound(&context.random, bound);
}

u32 kRandomRange(u32 min, u32 max) {
	return kRandomRange(&context.random, min, max);
}

float kRandomFloat01(void) {
	return kRandomFloat01(&context.random);
}

float kRandomFloatBound(float bound) {
	return kRandomFloatBoundEx(&context.random, bound);
}

float kRandomFloatRange(float min, float max) {
	return kRandomFloatRangeEx(&context.random, min, max);
}

float kRandomFloat(void) {
	return kRandomFloatEx(&context.random);
}

void kRandomSourceSeed(u64 state, u64 seq) {
	kRandomSourceSeedEx(&context.random, state, seq);
}

void kLogPrintV(kLogLevel level, const char *fmt, va_list list) {
	if (level >= context.logger.level) {
		char buff[4096 + 2];
		int len = vsnprintf(buff, 4096, fmt, list);
		buff[len]     = '\n';
		buff[len + 1] = 0;
		context.logger.proc(context.logger.data, context.logger.level, (u8 *)buff, len + 1);
	}
}

void kLogTraceV(const char *fmt, va_list list) {
	kLogPrintV(kLogLevel_Verbose, fmt, list);
}

void kLogWarningV(const char *fmt, va_list list) {
	kLogPrintV(kLogLevel_Warning, fmt, list);
}

void kLogErrorV(const char *fmt, va_list list) {
	kLogPrintV(kLogLevel_Error, fmt, list);
}

void kLogPrint(kLogLevel level, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	kLogPrintV(level, fmt, args);
	va_end(args);
}

void kLogTrace(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	kLogTraceV(fmt, args);
	va_end(args);
}

void kLogWarning(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	kLogWarningV(fmt, args);
	va_end(args);
}

void kLogError(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	kLogErrorV(fmt, args);
	va_end(args);
}

void kFatalError(const char *msg) {
	context.fatal(msg);
}

//
//
//

void kFallbackUserLoadProc(void)        {}
void kFallbackUserReleaseProc(void)     {}
void kFallbackUserUpdateProc(float dt)  {}

kEvent *kGetEvents(int *count) {
	*count = context.media.events.count;
	return context.media.events.data;
}

bool kIsKeyDown(kKey key) {
	return context.media.keyboard.keys[key].down;
}

bool kKeyPressed(kKey key) {
	return context.media.keyboard.keys[key].flags & kPressed;
}

bool kKeyReleased(kKey key) {
	return context.media.keyboard.keys[key].flags & kReleased;
}

u8 kKeyHits(kKey key) {
	return context.media.keyboard.keys[key].hits;
}

uint kGetKeyModFlags(void) {
	return context.media.keyboard.mods;
}

bool kIsButtonDown(kButton button) {
	return context.media.mouse.buttons[button].down;
}

bool kButtonPressed(kButton button) {
	return context.media.mouse.buttons[button].flags & kPressed;
}

bool kButtonReleased(kButton button) {
	return context.media.mouse.buttons[button].flags & kReleased;
}

kVec2i kGetCursorScreenPosition(void) {
	return context.media.mouse.cursor;
}

kVec2 kGetCursorPosition(void) {
	kVec2 cursor;
	cursor.x = (float)context.media.mouse.cursor.x / context.media.window.width;
	cursor.y = (float)context.media.mouse.cursor.y / context.media.window.height;
	return cursor;
}

kVec2 kGetCursorDelta(void) {
	return context.media.mouse.delta;
}

float kGetWheelHorizontal(void) {
	return context.media.mouse.wheel.x;
}

float kGetWheelVertical(void) {
	return context.media.mouse.wheel.y;
}

bool kIsWindowClosed(void) {
	return context.media.window.closed;
}

bool kIsWindowResized(void) {
	return context.media.window.resized;
}

void kIgnoreWindowCloseEvent(void) {
	return context.media.window.closed = false;
}

bool kIsWindowFocused(void) {
	return context.media.window.flags & kWindow_Focused;
}

bool kIsWindowFullscreen(void) {
	return context.media.window.flags & kWindow_Fullscreen;
}

void kGetWindowSize(u32 *w, u32 *h) {
	*w = context.media.window.width;
	*h = context.media.window.height;
}

void kResizeWindow(u32 w, u32 h) {
	kPlatformResizeWindow(&context, w, h);
}

void kToggleWindowFullscreen(void) {
	kPlatformToggleWindowFullscreen(&context);
}

void kEnableCursor(void) {
	kPlatformEnableCursor(&context);
}

void kDisableCursor(kContext *io) {
	kPlatformDisableCursor(&context);
}

void kClearInput(void) {
	kClearInputEx(&context);
}

void kNextFrame(void) {
	kNextFrameEx(&context);
}

void kAddEvent(kEvent ev) {
	kAddEventEx(&context, ev);
}

void kAddKeyEvent(kKey key, bool down, bool repeat) {
	kAddKeyEventEx(&context, key, down, repeat);
}

void kAddButtonEvent(kButton button, bool down) {
	kAddButtonEventEx(&context, button, down);
}

void kAddDoubleClickEvent(kButton button) {
	kAddDoubleClickEventEx(&context, button);
}

void kAddCursorEvent(kVec2i pos, kVec2 delta) {
	kAddCursorEventEx(&context, pos, delta);
}

void kAddWheelEvent(float horz, float vert) {
	kAddWheelEventEx(&context, horz, vert);
}

void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen) {
	kAddWindowResizeEventEx(&context, width, height, fullscreen);
}

void kAddWindowFocusEvent(bool focused) {
	kAddWindowFocusEventEx(&context, focused);
}

void kAddWindowCloseEvent(void) {
	kAddWindowCloseEventEx(&context);
}

//
//
//

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"

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

void kDefaultHandleAssertion(const char *file, int line, const char *proc, const char *string) {
	kLogError("Assertion Failed: %(%): % ; Procedure: %", file, line, string, proc);
	kTriggerBreakpoint();
}

void kDefaultFatalError(const char *message) {
	int wlen = MultiByteToWideChar(CP_UTF8, 0, message, (int)strlen(message), NULL, 0);
	wchar_t *msg = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, (wlen + 1) * sizeof(wchar_t));
	if (msg) {
		MultiByteToWideChar(CP_UTF8, 0, message, (int)strlen(message), msg, wlen + 1);
		msg[wlen] = 0;
	}
	FatalAppExitW(0, msg);
}

void kDefaultHandleLog(void *data, kLogLevel level, const u8 *msg, imem msg_len) {
	static kAtomic Guard = { 0 };

	static const WORD ColorsMap[] = {
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_GREEN,
		FOREGROUND_RED
	};
	static_assert(kArrayCount(ColorsMap) == kLogLevel_Error + 1, "");

	wchar_t buff[4096];

	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)msg, (int)msg_len, buff, kArrayCount(buff) - 1);
	buff[len] = 0;

	kAtomicLock(&Guard);

	HANDLE handle = (level == kLogLevel_Error) ?
		GetStdHandle(STD_ERROR_HANDLE) :
		GetStdHandle(STD_OUTPUT_HANDLE);

	if (handle != INVALID_HANDLE_VALUE) {
		CONSOLE_SCREEN_BUFFER_INFO buffer_info;
		GetConsoleScreenBufferInfo(handle, &buffer_info);
		SetConsoleTextAttribute(handle, ColorsMap[level]);
		DWORD written = 0;
		WriteConsoleW(handle, buff, len, &written, 0);
		SetConsoleTextAttribute(handle, buffer_info.wAttributes);
	}

	if (IsDebuggerPresent())
		OutputDebugStringW(buff);

	kAtomicUnlock(&Guard);
}

void *kDefaultHeapAllocator(kAllocatorMode mode, void *ptr, umem prev, umem size, void *ctx) {
	HANDLE heap = GetProcessHeap();
	if (mode == kAllocatorMode_Alloc)
		return HeapAlloc(heap, 0, size);
	else if (mode == kAllocatorMode_Realloc)
		return ptr ? HeapReAlloc(heap, 0, ptr, size) : HeapAlloc(heap, 0, size);
	else
		HeapFree(heap, 0, ptr);
	return 0;
}

#endif
