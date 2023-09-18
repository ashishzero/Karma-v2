#pragma once
#include "kCommon.h"

typedef enum kThreadAttribute {
	kThreadAttribute_Audio,
	kThreadAttribute_Capture,
	kThreadAttribute_Distribution,
	kThreadAttribute_Games,
	kThreadAttribute_Playback,
	kThreadAttribute_ProAudio,
} kThreadAttribute;

typedef struct kSemaphore { void *_id; }              kSemaphore;
typedef struct kMutex     { u64   _id; u32 _mem[7]; } kMutex;

typedef struct kCondVar {
#if K_PLATFORM_WASM == 1 || K_PLATFORM_LINUX == 1 || PLATFORM_MACOS == 1
	u32 _mem[12];
#else
	void *_id;
#endif
} kCondVar;

typedef struct kThread { void *ptr; } kThread;
typedef int(*kThreadProc)(void *arg);

kThread  kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr);;
kThread  kGetCurrentThread(void);
void     kDetachThread(kThread thread);
void     kWaitThread(kThread thread);
void     kTerminateThread(kThread thread, uint code);
void     kSleep(u32 millisecs);
void     kYield(void);

void     kInitSemaphore(kSemaphore *sem, u32 value, u32 max);
void     kFreeSemaphore(kSemaphore *sem);
u32      kReleaseSemaphore(kSemaphore *sem, u32 count);
void     kWaitSemaphore(kSemaphore *sem);
int      kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs);
void     kInitMutex(kMutex *mutex);
void     kFreeMutex(kMutex *mutex);
void     kLockMutex(kMutex *mutex);
bool     kTryLockMutex(kMutex *mutex);
void     kUnlockMutex(kMutex *mutex);
void     kInitCondVar(kCondVar *cond);
void     kFreeCondVar(kCondVar *cond);
void     kSignalCondVar(kCondVar *cond);
void     kBroadcastCondVar(kCondVar *cond);
bool     kWaitCondVar(kCondVar *cond, kMutex *mutex);
int      kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs);

//typedef enum kKey {
//	kKey_Unknown,
//	kKey_A, kKey_B, kKey_C, kKey_D, kKey_E, kKey_F, kKey_G, kKey_H,
//	kKey_I, kKey_J, kKey_K, kKey_L, kKey_M, kKey_N, kKey_O, kKey_P,
//	kKey_Q, kKey_R, kKey_S, kKey_T, kKey_U, kKey_V, kKey_W, kKey_X,
//	kKey_Y, kKey_Z,
//	kKey_0, kKey_1, kKey_2, kKey_3, kKey_4,
//	kKey_5, kKey_6, kKey_7, kKey_8, kKey_9,
//	kKey_Return, kKey_Escape, kKey_Backspace, kKey_Tab, kKey_Space, kKey_Shift, kKey_Ctrl,
//	kKey_F1, kKey_F2, kKey_F3, kKey_F4, kKey_F5, kKey_F6,
//	kKey_F7, kKey_F8, kKey_F9, kKey_F10, kKey_F11, kKey_F12,
//	kKey_PrintScreen, kKey_Insert, kKey_Home,
//	kKey_PageUp, kKey_PageDown, kKey_Delete, kKey_End,
//	kKey_Right, kKey_Left, kKey_Down, kKey_Up,
//	kKey_Divide, kKey_Multiply, kKey_Minus, kKey_Plus,
//	kKey_Period, kKey_BackTick,
//	kKey_Count
//} kKey;
//
//typedef enum kButton {
//	kButton_Left,
//	kButton_Right,
//	kButton_Middle,
//	kButton_Count
//} kButton;
//
//enum kKeyModFlags {
//	kKeyMod_LeftShift  = 0x01,
//	kKeyMod_RightShift = 0x02,
//	kKeyMod_LeftCtrl   = 0x04,
//	kKeyMod_RightCtrl  = 0x08,
//	kKeyMod_LeftAlt    = 0x10,
//	kKeyMod_RightAlt   = 0x20,
//	kKeyMod_Ctrl       = kKeyMod_LeftCtrl  | kKeyMod_RightCtrl,
//	kKeyMod_Alt        = kKeyMod_LeftAlt   | kKeyMod_RightAlt,
//	kKeyMod_Shift      = kKeyMod_LeftShift | kKeyMod_RightShift,
//};
//
//enum kStateFlags {
//	kPressed        = 0x1,
//	kReleased       = 0x2,
//	kDoubleClicked  = 0x4,
//};
//
//enum kWindowFlags {
//	kWindow_Fullscreen   = 0x1,
//	kWindow_Focused      = 0x2,
//	kWindow_FixedBorder  = 0x4, // remove this
//	kWindow_CursorHidden   = 0x8,
//};
//
//typedef struct kState {
//	u8 down;
//	u8 flags;
//	u8 hits;
//} kState;
//
//typedef struct kTextInput {
//	u32 count;
//	u8  data[64];
//} kTextInput;
//
//typedef struct kKeyboardState {
//	uint       mods;
//	kState     keys[kKey_Count];
//	kTextInput text;
//} kKeyboardState;
//
//typedef struct kMouseState {
//	kVec2i cursor;
//	kVec2  delta;
//	kVec2  wheel;
//	kState buttons[kKey_Count];
//} kMouseState;
//
//typedef struct kWindow { void *ptr; } kWindow;
//
//typedef struct kWindowState {
//	kWindow handle;
//	u32     width;
//	u32     height;
//	u32     resized;
//	u32     closed;
//	u32     flags;
//} kWindowState;
//
//struct kMediaIo;
//
//typedef void(*kMediaUserLoadProc)(struct kMediaIo *);
//typedef void(*kMediaUserReleaseProc)(struct kMediaIo *);
//typedef void(*kMediaUserUpdateProc)(struct kMediaIo *, float );
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
//typedef void(*kMediaPlatformBreakEventLoop)(struct kMediaIo *, int);
//typedef void(*kMediaPlatformResizeWindow)(struct kMediaIo *, kWindow , u32, u32);
//typedef void(*kMediaPlatformToggleFullscreen)(struct kMediaIo *, kWindow );
//typedef void(*kMediaPlatformMakeWindowBorderFixed)(struct kMediaIo *, kWindow );
//typedef void(*kMediaPlatformMakeWindowBorderResiable)(struct kMediaIo *, kWindow );
//typedef void(*kMediaPlatformHideCursor)(struct kMediaIo *);
//typedef void(*kMediaPlatformShowCursor)(struct kMediaIo *);
//
//typedef struct kMediaPlatform {
//	void *                                 data;
//	kMediaPlatformBreakEventLoop           break_event_loop;
//	kMediaPlatformResizeWindow             resize_window;
//	kMediaPlatformToggleFullscreen         toggle_fullscreen;
//	kMediaPlatformMakeWindowBorderFixed    make_window_border_fixed;
//	kMediaPlatformMakeWindowBorderResiable make_window_border_resizable;
//	kMediaPlatformHideCursor               hide_cursor;
//	kMediaPlatformShowCursor               show_cursor;
//} kMediaPlatform;
//
//typedef struct kMediaIo {
//	kKeyboardState keyboard;
//	kMouseState    mouse;
//	kWindowState   window;
//	kMediaUser     user;
//	kMediaPlatform platform;
//} kMediaIo;
//
////
////
////
//
//void     kFallbackUserLoad(struct kMediaIo *io);
//void     kFallbackUserRelease(struct kMediaIo *io);
//void     kFallbackUserUpdate(struct kMediaIo *io, float dt);
//
//void     kClearInput(kMediaIo *io);
//void     kNextFrame(kMediaIo *io);
//
//void     kPushKeyEvent(kMediaIo *io, kKey key, bool down, bool repeat);
//void     kPushButtonEvent(kMediaIo *io, kButton button, bool down);
//void     kPushDoubleClickEvent(kMediaIo *io, kButton button);
//void     kPushCursorEvent(kMediaIo *io, kVec2i pos, kVec2 delta);
//void     kPushWheelEvent(kMediaIo *io, float horz, float vert);
//void     kPushWindowResizeEvent(kMediaIo *io, u32 width, u32 height, bool fullscreen);
//void     kPushWindowBorderEvent(kMediaIo *io, bool resizable);
//void     kPushWindowFocusEvent(kMediaIo *io, bool focused);
//void     kPushWindowCursorEvent(kMediaIo *io, bool shown);
//void     kPushWindowCloseEvent(kMediaIo *io);
//
//void     kBreakEventLoop(kMediaIo *io, int status);
//bool     kIsWindowClosed(kMediaIo *io);
//bool     kIsWindowResized(kMediaIo *io);
//void     kIgnoreWindowCloseEvent(kMediaIo *io);
//void     kGetWindowSize(kMediaIo *io, u32 *w, u32 *h);
//void     kSetWindowSize(kMediaIo *io, u32 w, u32 h);
//uint     kGetWindowFlags(kMediaIo *io);
//void     kToggleWindowFullscreen(kMediaIo *io);
//void     kMakeWindowBorderFixed(kMediaIo *io);
//void     kMakeWindowBorderResizable(kMediaIo *io);
//void     kDisableCursor(kMediaIo *io);
//void     kEnableCursor(kMediaIo *io);
//
//bool     kIsKeyDown(kMediaIo *io, kKey key);
//bool     kKeyPressed(kMediaIo *io, kKey key);
//bool     kKeyReleased(kMediaIo *io, kKey key);
//u8       kKeyHits(kMediaIo *io, kKey key);
//
//bool     kIsButtonDown(kMediaIo *io, kButton button);
//bool     kButtonPressed(kMediaIo *io, kButton button);
//bool     kButtonReleased(kMediaIo *io, kButton button);
//
//kVec2i   kGetCursorScreenPosition(kMediaIo *io);
//kVec2    kGetCursorPosition(kMediaIo *io);
//kVec2    kGetCursorDelta(kMediaIo *io);
//float    kGetWheelHorizontal(kMediaIo *io);
//float    kGetWheelVertical(kMediaIo *io);
//
////
////
////
//
//int      kEventLoop(const kMediaUser user, const kMediaSpec *spec);
