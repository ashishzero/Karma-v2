#pragma once
#include "kCommon.h"

typedef enum kKey {
	kKey_Unknown,
	kKey_A, kKey_B, kKey_C, kKey_D, kKey_E, kKey_F, kKey_G, kKey_H,
	kKey_I, kKey_J, kKey_K, kKey_L, kKey_M, kKey_N, kKey_O, kKey_P,
	kKey_Q, kKey_R, kKey_S, kKey_T, kKey_U, kKey_V, kKey_W, kKey_X,
	kKey_Y, kKey_Z,
	kKey_0, kKey_1, kKey_2, kKey_3, kKey_4,
	kKey_5, kKey_6, kKey_7, kKey_8, kKey_9,
	kKey_Return, kKey_Escape, kKey_Backspace, kKey_Tab, kKey_Space, kKey_Shift, kKey_Ctrl,
	kKey_F1, kKey_F2, kKey_F3, kKey_F4, kKey_F5, kKey_F6,
	kKey_F7, kKey_F8, kKey_F9, kKey_F10, kKey_F11, kKey_F12,
	kKey_PrintScreen, kKey_Insert, kKey_Home,
	kKey_PageUp, kKey_PageDown, kKey_Delete, kKey_End,
	kKey_Right, kKey_Left, kKey_Down, kKey_Up,
	kKey_Divide, kKey_Multiply, kKey_Minus, kKey_Plus,
	kKey_Period, kKey_BackTick,
	kKey_Count
} kKey;

typedef enum kButton {
	kButton_Left,
	kButton_Right,
	kButton_Middle,
	kButton_Count
} kButton;

enum kKeyModFlags {
	kKeyMod_LeftShift  = 0x01,
	kKeyMod_RightShift = 0x02,
	kKeyMod_LeftCtrl   = 0x04,
	kKeyMod_RightCtrl  = 0x08,
	kKeyMod_LeftAlt    = 0x10,
	kKeyMod_RightAlt   = 0x20,
	kKeyMod_Ctrl       = kKeyMod_LeftCtrl  | kKeyMod_RightCtrl,
	kKeyMod_Alt        = kKeyMod_LeftAlt   | kKeyMod_RightAlt,
	kKeyMod_Shift      = kKeyMod_LeftShift | kKeyMod_RightShift,
};

typedef enum kEventKind {
	kEvent_Activated,
	kEvent_Deactivated,
	kEvent_Resized,
	kEvent_Closed,
	kEvent_DpiChanged,
	kEvent_CursorEnter,
	kEvent_CursorLeave,
	kEvent_CursorMoved,
	kEvent_ButtonPressed,
	kEvent_ButtonReleased,
	kEvent_DoubleClicked,
	kEvent_WheelMoved,
	kEvent_KeyPressed,
	kEvent_KeyReleased,
	kEvent_TextInput,
} kEventKind;

typedef struct kResizedEvent {
	u32 width;
	u32 height;
} kResizedEvent;

typedef struct kDpiChangedEvent {
	float scale;
} kDpiChangedEvent;

typedef struct kCursorEvent {
	kVec2i position;
} kCursorEvent;

typedef struct kButtonEvent {
	kButton symbol;
} kButtonEvent;

typedef struct kKeyEvent {
	kKey symbol;
	int  repeat;
} kKeyEvent;

typedef struct kWheelEvent {
	float horizontal;
	float vertical;
} kWheelEvent;

typedef struct kTextEvent {
	u32 codepoint;
	u32 mods;
} kTextEvent;

typedef struct kEvent {
	kEventKind kind;
	union {
		kResizedEvent    resized;
		kDpiChangedEvent dpi;
		kCursorEvent     cursor;
		kButtonEvent     button;
		kKeyEvent        key;
		kWheelEvent      wheel;
		kTextEvent       text;
	};
} kEvent;

enum kStateFlags {
	kPressed       = 0x1,
	kReleased      = 0x2,
	kDoubleClicked = 0x4,
};

enum kWindowFlags {
	kWindow_Fullscreen = 0x1,
	kWindow_Maximized  = 0x2,
	kWindow_Resizable  = 0x4,
	kWindow_NoTitleBar = 0x8,
};

typedef struct kState {
	u8 down;
	u8 flags;
	u8 hits;
} kState;

typedef struct kKeyboardState {
	uint       mods;
	kState     keys[kKey_Count];
} kKeyboardState;

typedef struct kMouseState {
	kVec2i cursor;
	kVec2i delta;
	kVec2  wheel;
	kState buttons[kKey_Count];
} kMouseState;

typedef struct kWindowState {
	u32     width;
	u32     height;
	u32     flags;
} kWindowState;

typedef struct kMediaUserEvents {
	void  *data;
	void (*update)(float);
	void (*load)(void);
	void (*release)(void);
} kMediaUserEvents;

//
//
//

struct kPlatformFile;
using kFile = kHandle<kPlatformFile>;

typedef enum kFileAccess {
	kFileAccess_Read,
	kFileAccess_Write,
	kFileAccess_ReadWrite
} kFileAccess;

typedef enum kFileShareMode {
	kFileShareMode_Exclusive,
	kFileShareMode_Read,
	kFileShareMode_Write,
	kFileShareMode_ReadWrite
} kFileShareMode;

typedef enum kFileMethod {
	kFileMethod_CreateAlways,
	kFileMethod_CreateNew,
	kFileMethod_OpenAlways,
	kFileMethod_OpenExisting
} kFileMethod;

typedef enum kFileAttribute {
	kFileAttribute_Archive    = 0x1,
	kFileAttribute_Compressed = 0x2,
	kFileAttribute_Directory  = 0x4,
	kFileAttribute_Encrypted  = 0x8,
	kFileAttribute_Hidden     = 0x10,
	kFileAttribute_Normal     = 0x20,
	kFileAttribute_Offline    = 0x40,
	kFileAttribute_ReadOnly   = 0x80,
	kFileAttribute_System     = 0x100,
	kFileAttribute_Temporary  = 0x200,
} kFileAttribute;

//
//
//

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

struct kPlatformThread;
using kThread = kHandle<kPlatformThread>;
typedef int(*kThreadProc)(void *arg);

//
//
//

void       kFallbackUserLoadProc(void);
void       kFallbackUserReleaseProc(void);
void       kFallbackUserUpdateProc(float dt);

//
//
//

kSlice<kEvent> kGetEvents(void);

void     * kGetUserEventData(void);
void       kSetUserEventData(void *);
void       kGetUserEvents(kMediaUserEvents *user);
void       kSetUserEvents(kMediaUserEvents *user);

bool       kIsKeyDown(kKey key);
bool       kKeyPressed(kKey key);
bool       kKeyReleased(kKey key);
u8         kKeyHits(kKey key);
uint       kGetKeyModFlags(void);

bool       kIsButtonDown(kButton button);
bool       kButtonPressed(kButton button);
bool       kButtonReleased(kButton button);

kVec2i     kGetCursorPosition(void);
kVec2i     kGetCursorDelta(void);
float      kGetWheelHorizontal(void);
float      kGetWheelVertical(void);

bool       kIsWindowClosed(void);
bool       kIsWindowResized(void);
void       kIgnoreWindowCloseEvent(void);
bool       kIsWindowFocused(void);
bool       kIsWindowFullscreen(void);
bool       kIsWindowMaximized(void);
void       kGetWindowSize(u32 *w, u32 *h);
float      kGetWindowDpiScale(void);

bool       kIsCursorEnabled(void);
bool       kIsCursorHovered(void);

void       kGetKeyboardState(kKeyboardState *keyboard);
void       kGetKeyState(kState *state, kKey key);
void       kGetMouseState(kMouseState *mouse);
void       kGetButtonState(kState *state, kButton button);
void       kGetWindowState(kWindowState *state);

void       kSetKeyboardState(kKeyboardState *keyboard);
void       kSetKeyState(kState *state, kKey key);
void       kSetMouseState(kMouseState *mouse);
void       kSetButtonState(kState *state, kButton button);
void       kSetWindowState(kWindowState *state);

//
//
//

void       kClearInput(void);
void       kNextFrame(void);

void       kAddEvent(const kEvent &ev);
void       kAddKeyEvent(kKey key, bool down, bool repeat);
void       kAddButtonEvent(kButton button, bool down);
void       kAddTextInputEvent(u32 codepoint, u32 mods);
void       kAddDoubleClickEvent(kButton button);
void       kAddCursorEvent(kVec2i pos);
void       kAddCursorDeltaEvent(kVec2i delta);
void       kAddWheelEvent(float horz, float vert);
void       kAddCursorEnterEvent(void);
void       kAddCursorLeaveEvent(void);
void       kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen);
void       kAddWindowFocusEvent(bool focused);
void       kAddWindowCloseEvent(void);
void       kAddWindowDpiChangedEvent(float scale);

//
//
//

u64        kGetPerformanceFrequency(void);
u64        kGetPerformanceCounter(void);
void       kTerminate(uint code);

//
//
//

kFile      kOpenFile(const char *mb_filepath, kFileAccess paccess, kFileShareMode pshare, kFileMethod method);
void       kCloseFile(kFile handle);
umem       kReadFile(kFile handle, u8 *buffer, umem size);
umem       kWriteFile(kFile handle, u8 *buff, umem size);
umem       kGetFileSize(kFile handle);
u8 *       kReadEntireFile(const char *filepath, umem *file_size);
bool       kWriteEntireFile(const char *filepath, u8 *buffer, umem size);
uint       kGetFileAttributes(const char *mb_filepath);

//
//
//

kThread    kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr);;
kThread    kGetCurrentThread(void);
void       kDetachThread(kThread thread);
void       kWaitThread(kThread thread);
void       kTerminateThread(kThread thread, uint code);
void       kSleep(u32 millisecs);
void       kYield(void);

void       kInitSemaphore(kSemaphore *sem, u32 value, u32 max);
void       kFreeSemaphore(kSemaphore *sem);
u32        kReleaseSemaphore(kSemaphore *sem, u32 count);
void       kWaitSemaphore(kSemaphore *sem);
int        kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs);
void       kInitMutex(kMutex *mutex);
void       kFreeMutex(kMutex *mutex);
void       kLockMutex(kMutex *mutex);
bool       kTryLockMutex(kMutex *mutex);
void       kUnlockMutex(kMutex *mutex);
void       kInitCondVar(kCondVar *cond);
void       kFreeCondVar(kCondVar *cond);
void       kSignalCondVar(kCondVar *cond);
void       kBroadcastCondVar(kCondVar *cond);
bool       kWaitCondVar(kCondVar *cond, kMutex *mutex);
int        kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs);

//
//
//

void       kResizeWindow(u32 w, u32 h);
void       kToggleWindowFullscreen(void);
void       kEnableCursor(void);
void       kDisableCursor(void);
void       kMaximizeWindow(void);
void       kRestoreWindow(void);
void       kMinimizeWindow(void);
void       kCloseWindow(void);

//
//
//

typedef struct kMediaWindowSpec {
	const char *title;
	u32         width;
	u32         height;
	u32         flags;
} kMediaWindowSpec;

typedef struct kMediaThreadSpec {
	kThreadAttribute attribute;
} kMediaThreadSpec;

typedef struct kMediaSpec {
	kMediaWindowSpec window;
	kMediaThreadSpec thread;
} kMediaSpec;

static const kMediaSpec kDefaultSpec = {
	.window = {
		.flags = kWindow_Resizable
	},
	.thread = {
		.attribute = kThreadAttribute_Games
	}
};

int        kEventLoop(const kMediaSpec *spec, kMediaUserEvents user);
void       kBreakLoop(int status);
