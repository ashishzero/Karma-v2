#pragma once
#include "kCommon.h"
#include "kRenderShared.h"

enum class kKey
{
	Unknown,
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	N0,
	N1,
	N2,
	N3,
	N4,
	N5,
	N6,
	N7,
	N8,
	N9,
	Return,
	Escape,
	Backspace,
	Tab,
	Space,
	Shift,
	Ctrl,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	PrintScreen,
	Insert,
	Home,
	PageUp,
	PageDown,
	Delete,
	End,
	Right,
	Left,
	Down,
	Up,
	Divide,
	Multiply,
	Minus,
	Plus,
	Period,
	BackTick,
	Count
};

enum class kButton
{
	Left,
	Right,
	Middle,
	Count
};

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

enum class kEventKind
{
	Activated,
	Deactivated,
	Resized,
	Closed,
	DpiChanged,
	CursorEnter,
	CursorLeave,
	CursorMoved,
	ButtonPressed,
	ButtonReleased,
	DoubleClicked,
	WheelMoved,
	KeyPressed,
	KeyReleased,
	TextInput,
};

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

struct kEvent
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
};

enum class kAudioDeviceState
{
	Resume,
	Pause,
	Lost
};

enum class kAudioEndpoint
{
	Render,
	Capture
};

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
	kKeyState Keys[(int)kKey::Count];
} kKeyboardState;

typedef struct kMouseState
{
	kVec2i    Cursor;
	kVec2i    Delta;
	kVec2     Wheel;
	kKeyState Buttons[(int)kKey::Count];
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
	void (*Update)(float, void *);
	void (*Load)(void *);
	void (*Release)(void *);
} kMediaUserEvents;

//
//
//

void kFallbackUserLoadProc(void *);
void kFallbackUserReleaseProc(void *);
void kFallbackUserUpdateProc(float dt, void *);

//
//
//

kSpan<kEvent>     kGetEvents(void);
kArena           *kGetFrameArena(void);

void              kGetUserEvents(kMediaUserEvents *user);
void              kSetUserEvents(const kMediaUserEvents &user);

bool              kIsKeyDown(kKey key);
bool              kKeyPressed(kKey key);
bool              kKeyReleased(kKey key);
u8                kKeyHits(kKey key);
uint              kGetKeyModFlags(void);

bool              kIsButtonDown(kButton button);
bool              kButtonPressed(kButton button);
bool              kButtonReleased(kButton button);

kVec2i            kGetCursorPosition(void);
kVec2i            kGetCursorDelta(void);
float             kGetWheelHorizontal(void);
float             kGetWheelVertical(void);

bool              kIsWindowClosed(void);
bool              kIsWindowResized(void);
void              kIgnoreWindowCloseEvent(void);
bool              kIsWindowActive(void);
bool              kIsWindowFullscreen(void);
bool              kIsWindowMaximized(void);
kVec2i            kGetWindowSize(void);
float             kGetWindowAspectRatio(void);
float             kGetWindowDpiScale(void);

bool              kIsCursorCaptured(void);
bool              kIsCursorHovered(void);

void              kGetKeyboardState(kKeyboardState *keyboard);
void              kGetKeyState(kKeyState *state, kKey key);
void              kGetMouseState(kMouseState *mouse);
void              kGetButtonState(kKeyState *state, kButton button);
void              kGetWindowState(kWindowState *state);

void              kSetKeyboardState(const kKeyboardState &keyboard);
void              kSetKeyState(const kKeyState &state, kKey key);
void              kSetKeyModFlags(uint mods);
void              kSetMouseState(const kMouseState &mouse);
void              kSetButtonState(const kKeyState &state, kButton button);

kAudioDeviceState kGetAudioDeviceState(kAudioEndpoint endpoint);
void              kResumeAudioRender(void);
void              kPauseAudioRender(void);
void              kResetAudioRender(void);
void              kUpdateAudioRender(void);
void              kResumeAudioCapture(void);
void              kPauseAudioCapture(void);
void              kResetAudioCapture(void);
void              kUpdateAudioCapture(void);

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

void     kGetRenderPipelineConfig(kRenderPipelineConfig *config);
void     kApplyRenderPipelineConfig(const kRenderPipelineConfig &config);
kTexture kCreateTexture(const kTextureSpec &spec);
void     kDestroyTexture(kTexture texture);

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
static const kRenderPipelineConfig kDefaultRenderPipeline = {.Msaa              = kMultiSamplingAntiAliasing::x8,
                                                             .Bloom             = kBloom::Enabled,
                                                             .Hdr               = kHighDynamicRange::AES,
                                                             .Clear             = kVec4(0.1f, 0.1f, 0.1f, 1.0f),
                                                             .BloomFilterRadius = 0.005f,
                                                             .BloomStrength     = 0.05f,
                                                             .Intensity         = kVec3(1)};
static const kWindowSpec           kDefaultWindow         = {.Width = 1280, .Height = 720};

static const kMediaSpec            kDefaultSpec           = {.Window         = kDefaultWindow,
                                                             .Arena          = kDefaultArena,
                                                             .RenderPipeline = kDefaultRenderPipeline};

//
//
//

int  kEventLoop(const kMediaSpec &spec, const kMediaUserEvents &user);
void kRestartLoop(void);
void kBreakLoop(int status);
