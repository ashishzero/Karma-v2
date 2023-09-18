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
	int width;
	int height;
} kResizedEvent;

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
} kTextEvent;

typedef struct kEvent {
	kEventKind kind;
	union {
		kResizedEvent resized;
		kCursorEvent  cursor;
		kButtonEvent  button;
		kKeyEvent     key;
		kWheelEvent   wheel;
		kTextEvent    text;
	};
} kEvent;

enum kStateFlags {
	kPressed        = 0x1,
	kReleased       = 0x2,
	kDoubleClicked  = 0x4,
};

enum kWindowFlags {
	kWindow_Fullscreen = 0x1,
	kWindow_Focused    = 0x2,
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
	kVec2  delta;
	kVec2  wheel;
	kState buttons[kKey_Count];
} kMouseState;

typedef struct kWindow { void *ptr; } kWindow;

typedef struct kWindowState {
	kWindow handle;
	u32     width;
	u32     height;
	u16     resized;
	u16     closed;
	u32     flags;
} kWindowState;

typedef struct kEventList {
	kEvent *data;
	int     count;
	int     allocated;
} kEventList;

typedef struct kMedia {
	kEventList     events;
	kKeyboardState keyboard;
	kMouseState    mouse;
	kWindowState   window;
} kMedia;

typedef struct kContext {
	kMedia               media;
	kAllocator           allocator;
	kRandomSource        random;
	kHandleAssertionProc assertion;
	kLogger              logger;
	kFatalErrorProc      fatal;
} kContext;
