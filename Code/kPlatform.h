#pragma once
#include "kContext.h"

void kClearInputEx(kContext *context);
void kNextFrameEx(kContext *context);

void kAddEventEx(kContext *context, kEvent ev);
void kAddKeyEventEx(kContext *context, kKey key, bool down, bool repeat);
void kAddButtonEventEx(kContext *context, kButton button, bool down);
void kAddDoubleClickEventEx(kContext *context, kButton button);
void kAddCursorEventEx(kContext *context, kVec2i pos, kVec2 delta);
void kAddWheelEventEx(kContext *context, float horz, float vert);
void kAddWindowResizeEventEx(kContext *context, u32 width, u32 height, bool fullscreen);
void kAddWindowFocusEventEx(kContext *context, bool focused);
void kAddWindowCloseEventEx(kContext *context);

void kPlatformResizeWindow(kContext *context, u32 w, u32 h);
void kPlatformToggleWindowFullscreen(kContext *context);
void kPlatformEnableCursor(kContext *context);
void kPlatformDisableCursor(kContext *io);

//typedef void(*kMediaUserLoadProc)(void);
//typedef void(*kMediaUserReleaseProc)(void);
//typedef void(*kMediaUserUpdateProc)(float);
//
//typedef struct kMediaUserProc {
//	kMediaUserUpdateProc  update;
//	kMediaUserLoadProc    load;
//	kMediaUserReleaseProc release;
//} kMediaUserProc;
//
//typedef struct kMediaUser {
//	void *         data;
//	kMediaUserProc proc;
//} kMediaUser;
//
//typedef struct kMediaSpec {
//	const char *title;
//	u32         width;
//	u32         height;
//	u32         flags;
//} kMediaSpec;
//
//
