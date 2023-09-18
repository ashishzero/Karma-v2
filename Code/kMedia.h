#pragma once
#include "kContext.h"

//
//
//

void     * kAlloc(umem size);
void     * kRealloc(void *ptr, umem prev, umem size);
void       kFree(void *ptr, umem size);

u32        kRandom(void);
u32        kRandomBound(u32 bound);
u32        kRandomRange(u32 min, u32 max);
float      kRandomFloat01(void);
float      kRandomFloatBound(float bound);
float      kRandomFloatRange(float min, float max);
float      kRandomFloat(void);
void       kRandomSourceSeed(u64 state, u64 seq);

void       kLogPrintV(kLogLevel level, const char *fmt, va_list list);
void       kLogTraceV(const char *fmt, va_list list);
void       kLogWarningV(const char *fmt, va_list list);
void       kLogErrorV(const char *fmt, va_list list);

void       kLogPrint(kLogLevel level, const char *fmt, ...);
void       kLogTrace(const char *fmt, ...);
void       kLogWarning(const char *fmt, ...);
void       kLogError(const char *fmt, ...);

void       kFatalError(const char *msg);

//
//
//

void       kFallbackUserLoadProc(void);
void       kFallbackUserReleaseProc(void);
void       kFallbackUserUpdateProc(float dt);

kEvent   * kGetEvents(int *count);

bool       kIsKeyDown(kKey key);
bool       kKeyPressed(kKey key);
bool       kKeyReleased(kKey key);
u8         kKeyHits(kKey key);
uint       kGetKeyModFlags(void);

bool       kIsButtonDown(kButton button);
bool       kButtonPressed(kButton button);
bool       kButtonReleased(kButton button);

kVec2i     kGetCursorScreenPosition(void);
kVec2      kGetCursorPosition(void);
kVec2      kGetCursorDelta(void);
float      kGetWheelHorizontal(void);
float      kGetWheelVertical(void);

bool       kIsWindowClosed(void);
bool       kIsWindowResized(void);
void       kIgnoreWindowCloseEvent(void);
bool       kIsWindowFocused(void);
bool       kIsWindowFullscreen(void);
void       kGetWindowSize(u32 *w, u32 *h);

//
//
//

void       kResizeWindow(u32 w, u32 h);
void       kToggleWindowFullscreen(void);
void       kEnableCursor(void);
void       kDisableCursor(kContext *io);

//
//
//

void       kClearInput(void);
void       kNextFrame(void);

void       kAddEvent(kEvent ev);
void       kAddKeyEvent(kKey key, bool down, bool repeat);
void       kAddButtonEvent(kButton button, bool down);
void       kAddDoubleClickEvent(kButton button);
void       kAddCursorEvent(kVec2i pos, kVec2 delta);
void       kAddWheelEvent(float horz, float vert);
void       kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen);
void       kAddWindowFocusEvent(bool focused);
void       kAddWindowCloseEvent(void);

//
//
//

u64        kGetPerformanceFrequency(void);
u64        kGetPerformanceCounter(void);
void       kTerminate(uint code);

void       kDefaultHandleAssertion(const char *file, int line, const char *proc, const char *string);
void       kDefaultFatalError(const char *message);
void       kDefaultHandleLog(void * data, kLogLevel level, const u8 *msg, imem len);
void     * kDefaultHeapAllocator(kAllocatorMode mode, void *ptr, umem prev, umem size, void *context);

//
//
//
