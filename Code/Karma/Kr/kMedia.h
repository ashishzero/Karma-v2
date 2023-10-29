#pragma once
#include "kCommon.h"
#include "kRenderConfig.h"

typedef enum kKey
{
	kKey_Unknown,
	kKey_A,
	kKey_B,
	kKey_C,
	kKey_D,
	kKey_E,
	kKey_F,
	kKey_G,
	kKey_H,
	kKey_I,
	kKey_J,
	kKey_K,
	kKey_L,
	kKey_M,
	kKey_N,
	kKey_O,
	kKey_P,
	kKey_Q,
	kKey_R,
	kKey_S,
	kKey_T,
	kKey_U,
	kKey_V,
	kKey_W,
	kKey_X,
	kKey_Y,
	kKey_Z,
	kKey_0,
	kKey_1,
	kKey_2,
	kKey_3,
	kKey_4,
	kKey_5,
	kKey_6,
	kKey_7,
	kKey_8,
	kKey_9,
	kKey_Return,
	kKey_Escape,
	kKey_Backspace,
	kKey_Tab,
	kKey_Space,
	kKey_Shift,
	kKey_Ctrl,
	kKey_F1,
	kKey_F2,
	kKey_F3,
	kKey_F4,
	kKey_F5,
	kKey_F6,
	kKey_F7,
	kKey_F8,
	kKey_F9,
	kKey_F10,
	kKey_F11,
	kKey_F12,
	kKey_PrintScreen,
	kKey_Insert,
	kKey_Home,
	kKey_PageUp,
	kKey_PageDown,
	kKey_Delete,
	kKey_End,
	kKey_Right,
	kKey_Left,
	kKey_Down,
	kKey_Up,
	kKey_Divide,
	kKey_Multiply,
	kKey_Minus,
	kKey_Plus,
	kKey_Period,
	kKey_BackTick,
	kKey_Count
} kKey;

typedef enum kButton
{
	kButton_Left,
	kButton_Right,
	kButton_Middle,
	kButton_Count
} kButton;

enum kKeyModFlags
{
	kKeyMod_LeftShift  = 0x01,
	kKeyMod_RightShift = 0x02,
	kKeyMod_LeftCtrl   = 0x04,
	kKeyMod_RightCtrl  = 0x08,
	kKeyMod_LeftAlt    = 0x10,
	kKeyMod_RightAlt   = 0x20,
	kKeyMod_Ctrl       = kKeyMod_LeftCtrl | kKeyMod_RightCtrl,
	kKeyMod_Alt        = kKeyMod_LeftAlt | kKeyMod_RightAlt,
	kKeyMod_Shift      = kKeyMod_LeftShift | kKeyMod_RightShift,
};

typedef enum kEventKind
{
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

typedef struct kResizedEvent
{
	u32 Width;
	u32 Height;
} kResizedEvent;

typedef struct kDpiChangedEvent
{
	float Factor;
} kDpiChangedEvent;

typedef struct kCursorEvent
{
	kVec2i Position;
} kCursorEvent;

typedef struct kButtonEvent
{
	kButton Symbol;
} kButtonEvent;

typedef struct kKeyEvent
{
	kKey Symbol;
	int  Repeat;
} kKeyEvent;

typedef struct kWheelEvent
{
	float Horz;
	float Vert;
} kWheelEvent;

typedef struct kTextEvent
{
	u32 Codepoint;
	u32 Mods;
} kTextEvent;

typedef struct kEvent
{
	kEventKind Kind;
	union
	{
		kResizedEvent    Resized;
		kDpiChangedEvent Dpi;
		kCursorEvent     Cursor;
		kButtonEvent     Button;
		kKeyEvent        Key;
		kWheelEvent      Wheel;
		kTextEvent       Text;
	};
} kEvent;

enum kStateFlags
{
	kPressed       = 0x1,
	kReleased      = 0x2,
	kDoubleClicked = 0x4,
};

typedef struct kKeyState
{
	u8 Down;
	u8 Flags;
	u8 Hits;
} kKeyState;

typedef struct kKeyboardState
{
	uint      Mods;
	kKeyState Keys[kKey_Count];
} kKeyboardState;

typedef struct kMouseState
{
	kVec2i    Cursor;
	kVec2i    Delta;
	kVec2     Wheel;
	kKeyState Buttons[kKey_Count];
} kMouseState;

typedef enum kWindowFlag
{
	kWindow_Active,
	kWindow_Hovered,
	kWindow_Fullscreen,
	kWindow_Resized,
	kWindow_Closed,
	kWindow_Capturing,
	kWindow_Maximized,
	kWindow_FlagCount
} kWindowFlag;

typedef struct kWindowState
{
	u32   Width;
	u32   Height;
	float DpiFactor;
	u32   Flags[kWindow_FlagCount];
} kWindowState;

typedef struct kMediaUserEvents
{
	void *Data;
	void (*Update)(float);
	void (*Load)(void);
	void (*Release)(void);
} kMediaUserEvents;

//
//
//

void kFallbackUserLoadProc(void);
void kFallbackUserReleaseProc(void);
void kFallbackUserUpdateProc(float dt);

//
//
//

kSpan<kEvent> kGetEvents(void);
kArena       *kGetFrameArena(void);

void         *kGetUserEventData(void);
void          kSetUserEventData(void *);
void          kGetUserEvents(kMediaUserEvents *user);
void          kSetUserEvents(const kMediaUserEvents &user);

bool          kIsKeyDown(kKey key);
bool          kKeyPressed(kKey key);
bool          kKeyReleased(kKey key);
u8            kKeyHits(kKey key);
uint          kGetKeyModFlags(void);

bool          kIsButtonDown(kButton button);
bool          kButtonPressed(kButton button);
bool          kButtonReleased(kButton button);

kVec2i        kGetCursorPosition(void);
kVec2i        kGetCursorDelta(void);
float         kGetWheelHorizontal(void);
float         kGetWheelVertical(void);

bool          kIsWindowClosed(void);
bool          kIsWindowResized(void);
void          kIgnoreWindowCloseEvent(void);
bool          kIsWindowActive(void);
bool          kIsWindowFullscreen(void);
bool          kIsWindowMaximized(void);
kVec2i        kGetWindowSize(void);
float         kGetWindowAspectRatio(void);
float         kGetWindowDpiScale(void);

bool          kIsCursorCaptured(void);
bool          kIsCursorHovered(void);

void          kGetKeyboardState(kKeyboardState *keyboard);
void          kGetKeyState(kKeyState *state, kKey key);
void          kGetMouseState(kMouseState *mouse);
void          kGetButtonState(kKeyState *state, kButton button);
void          kGetWindowState(kWindowState *state);

void          kSetKeyboardState(const kKeyboardState &keyboard);
void          kSetKeyState(const kKeyState &state, kKey key);
void          kSetKeyModFlags(uint mods);
void          kSetMouseState(const kMouseState &mouse);
void          kSetButtonState(const kKeyState &state, kButton button);

//
//
//

void kClearInput(void);
void kClearFrame(void);

void kAddEvent(const kEvent &ev);
void kAddKeyEvent(kKey key, bool down, bool repeat);
void kAddButtonEvent(kButton button, bool down);
void kAddTextInputEvent(u32 codepoint, u32 mods);
void kAddDoubleClickEvent(kButton button);
void kAddCursorEvent(kVec2i pos);
void kAddCursorDeltaEvent(kVec2i delta);
void kAddWheelEvent(float horz, float vert);
void kAddCursorEnterEvent(void);
void kAddCursorLeaveEvent(void);
void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen);
void kAddWindowActivateEvent(bool focused);
void kAddWindowCloseEvent(void);
void kAddWindowMaximizeEvent(void);
void kAddWindowRestoreEvent(void);
void kAddWindowCursorCaptureEvent(void);
void kAddWindowCursorReleaseEvent(void);
void kAddWindowDpiChangedEvent(float scale);

//
//
//

void kGetRenderPipelineConfig(kRenderPipelineConfig *config);
void kApplyRenderPipelineConfig(const kRenderPipelineConfig &config);

//
//
//

void kResizeWindow(u32 w, u32 h);
void kToggleWindowFullscreen(void);
void kReleaseCursor(void);
void kCaptureCursor(void);
int  kGetWindowCaptionSize(void);
void kMaximizeWindow(void);
void kRestoreWindow(void);
void kMinimizeWindow(void);
void kCloseWindow(void);

//
//
//

void kUserLoad(void);
void kUserUpdate(float dt);
void kUserRelease(void);
void kUpdateFrame(float dt);

//
//
//

enum kWindowStyleFlags
{
	kWindowStyle_Fullscreen        = 0x1,
	kWindowStyle_FixedBorders      = 0x2,
	kWindowStyle_ForceSharpCorners = 0x4,
	kWindowStyle_DisableCaptions   = 0x8,
};

typedef struct kWindowSpec
{
	kString Title;
	u32     Width;
	u32     Height;
	uint    Flags;
} kWindowSpec;

typedef struct kMediaSpec
{
	kWindowSpec           Window;
	kArenaSpec            Arena;
	kRenderPipelineConfig RenderPipeline;
} kMediaSpec;

static const kArenaSpec            kDefaultArena          = {.Alignment = sizeof(8), .Capacity = kMegaByte * 64};
static const kRenderPipelineConfig kDefaultRenderPipeline = {.Msaa              = kMultiSamplingAntiAliasing_8,
                                                             .Bloom             = kBloom_Enabled,
                                                             .Hdr               = kHighDynamicRange_AES,
                                                             .BloomFilterRadius = 0.005f,
                                                             .BloomStrength     = 0.15f,
                                                             .Intensity         = kVec3(1)};

static const kMediaSpec            kDefaultSpec = {.Arena = kDefaultArena, .RenderPipeline = kDefaultRenderPipeline};

int                                kEventLoop(const kMediaSpec &spec, const kMediaUserEvents &user);
void                               kBreakLoop(int status);
