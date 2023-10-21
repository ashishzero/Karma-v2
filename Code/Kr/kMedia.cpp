#include "kArray.h"
#include "kRender.h"
#include "kMediaBackend.h"
#include "kRenderBackend.h"

static int          FallbackFontHeight = 32;
static int          FallbackFontWidth  = (3 * FallbackFontHeight) / 2;

static const kGlyph FallbackGlyph      = {
    kRect{kVec2(0), kVec2(0)},
    kVec2i(0),
    kVec2i(FallbackFontWidth, FallbackFontHeight),
    kVec2i(FallbackFontWidth + 2, FallbackFontHeight + 2),
};

typedef struct kMediaBuiltin
{
	kTexture *texture;
	kFont     font;
} kMediaBuiltin;

typedef struct kMedia
{
	kArena          *arena;
	kArray<kEvent>   events;
	kKeyboardState   keyboard;
	kMouseState      mouse;
	kWindowState     window;
	kMediaUserEvents user;
	kRenderBackend   render;
	kMediaBackend    backend;
	kMediaBuiltin    builtin;
} kMedia;

static kMedia media;

//
//
//

void          kFallbackUserLoadProc(void) {}
void          kFallbackUserReleaseProc(void) {}
void          kFallbackUserUpdateProc(float dt) {}
kSpan<kEvent> kGetEvents(void) { return media.events; }
kArena       *kGetFrameArena(void) { return media.arena; }
void         *kGetUserEventData(void) { return media.user.data; }
void          kSetUserEventData(void *data) { media.user.data = data; }
void          kGetUserEvents(kMediaUserEvents *user) { memcpy(user, &media.user, sizeof(media.user)); }

void          kSetUserEvents(const kMediaUserEvents &user)
{
	memcpy(&media.user, &user, sizeof(media.user));

	if (!media.user.load) media.user.load = kFallbackUserLoadProc;

	if (!media.user.release) media.user.release = kFallbackUserLoadProc;

	if (!media.user.update) media.user.update = kFallbackUserUpdateProc;
}

bool   kIsKeyDown(kKey key) { return media.keyboard.keys[key].down; }
bool   kKeyPressed(kKey key) { return media.keyboard.keys[key].flags & kPressed; }
bool   kKeyReleased(kKey key) { return media.keyboard.keys[key].flags & kReleased; }
u8     kKeyHits(kKey key) { return media.keyboard.keys[key].hits; }
uint   kGetKeyModFlags(void) { return media.keyboard.mods; }
bool   kIsButtonDown(kButton button) { return media.mouse.buttons[button].down; }
bool   kButtonPressed(kButton button) { return media.mouse.buttons[button].flags & kPressed; }
bool   kButtonReleased(kButton button) { return media.mouse.buttons[button].flags & kReleased; }
kVec2i kGetCursorPosition(void) { return media.mouse.cursor; }
kVec2i kGetCursorDelta(void) { return media.mouse.delta; }
float  kGetWheelHorizontal(void) { return media.mouse.wheel.x; }
float  kGetWheelVertical(void) { return media.mouse.wheel.y; }
bool   kIsWindowClosed(void) { return media.window.flags[kWindow_Closed]; }
bool   kIsWindowResized(void) { return media.window.flags[kWindow_Resized]; }
void   kIgnoreWindowCloseEvent(void) { media.window.flags[kWindow_Closed] = false; }
bool   kIsWindowActive(void) { return media.window.flags[kWindow_Active]; }
bool   kIsWindowFullscreen(void) { return media.window.flags[kWindow_Fullscreen]; }
bool   kIsWindowMaximized(void) { return media.window.flags[kWindow_Maximized]; }
kVec2i kGetWindowSize(void) { return kVec2i(media.window.width, media.window.height); }

float  kGetWindowAspectRatio(void)
{
	float y = (float)media.window.height;
	float x = (float)media.window.width;
	return x / y;
}

float kGetWindowDpiScale(void) { return media.window.yfactor; }
bool  kIsCursorCaptured(void) { return media.window.flags[kWindow_Capturing]; }
bool  kIsCursorHovered(void) { return media.window.flags[kWindow_Hovered]; }
void  kGetKeyboardState(kKeyboardState *keyboard) { memcpy(keyboard, &media.keyboard, sizeof(media.keyboard)); }

void  kGetKeyState(kKeyState *state, kKey key)
{
	memcpy(state, &media.keyboard.keys[key], sizeof(media.keyboard.keys[key]));
}

void kGetMouseState(kMouseState *mouse) { memcpy(mouse, &media.mouse, sizeof(media.mouse)); }

void kGetButtonState(kKeyState *state, kButton button)
{
	memcpy(state, &media.mouse.buttons[button], sizeof(media.mouse.buttons));
}

void kGetWindowState(kWindowState *state) { memcpy(state, &media.window, sizeof(media.window)); }
void kSetKeyboardState(const kKeyboardState &keyboard) { memcpy(&media.keyboard, &keyboard, sizeof(media.keyboard)); }

void kSetKeyState(const kKeyState &state, kKey key)
{
	memcpy(&media.keyboard.keys[key], &state, sizeof(media.keyboard.keys[key]));
}

void kSetKeyModFlags(uint mods) { media.keyboard.mods = mods; }

void kSetMouseState(const kMouseState &mouse) { memcpy(&media.mouse, &mouse, sizeof(media.mouse)); }

void kSetButtonState(const kKeyState &state, kButton button)
{
	memcpy(&media.mouse.buttons[button], &state, sizeof(media.mouse.buttons[button]));
}

void kClearInput(void)
{
	media.events.Reset();
	memset(&media.keyboard, 0, sizeof(media.keyboard));
	memset(&media.mouse, 0, sizeof(media.mouse));
}

void kClearFrame(void)
{
	kResetArena(media.arena);

	media.events.count  = 0;
	media.keyboard.mods = 0;

	for (uint key = 0; key < kKey_Count; ++key)
	{
		media.keyboard.keys[key].flags = 0;
		media.keyboard.keys[key].hits  = 0;
	}

	for (uint button = 0; button < kButton_Count; ++button)
	{
		media.mouse.buttons[button].flags = 0;
		media.mouse.buttons[button].hits  = 0;
	}

	memset(&media.mouse.wheel, 0, sizeof(media.mouse.wheel));

	media.window.flags[kWindow_Resized] = false;
	media.window.flags[kWindow_Closed]  = false;
}

void kAddEvent(const kEvent &ev) { media.events.Add(ev); }

void kAddKeyEvent(kKey key, bool down, bool repeat)
{
	bool       pressed  = down && !repeat;
	bool       released = !down;
	kKeyState *state    = &media.keyboard.keys[key];
	state->down         = down;
	if (released)
	{
		state->flags |= kReleased;
	}
	if (pressed)
	{
		state->flags |= kPressed;
		state->hits += 1;
	}
	kEvent ev = {.kind = down ? kEvent_KeyPressed : kEvent_KeyReleased, .key = {.symbol = key, .repeat = repeat}};
	kAddEvent(ev);
}

void kAddButtonEvent(kButton button, bool down)
{
	kKeyState *state    = &media.mouse.buttons[button];
	bool       previous = state->down;
	bool       pressed  = down && !previous;
	bool       released = !down && previous;
	state->down         = down;
	if (released)
	{
		state->flags |= kReleased;
		state->hits += 1;
	}
	if (pressed)
	{
		state->flags |= kPressed;
	}
	kEvent ev = {.kind = down ? kEvent_ButtonPressed : kEvent_ButtonReleased, .button = {.symbol = button}};
	kAddEvent(ev);
}

void kAddTextInputEvent(u32 codepoint, u32 mods)
{
	kEvent ev = {.kind = kEvent_TextInput, .text = {.codepoint = codepoint, .mods = mods}};
	kAddEvent(ev);
}

void kAddDoubleClickEvent(kButton button)
{
	kKeyState *state = &media.mouse.buttons[button];
	state->flags |= kDoubleClicked;
	kEvent ev = {.kind = kEvent_DoubleClicked, .button = {.symbol = button}};
	kAddEvent(ev);
}

void kAddCursorEvent(kVec2i pos)
{
	int xdel = pos.x - media.mouse.cursor.x;
	int ydel = pos.y - media.mouse.cursor.y;
	media.mouse.delta.x += xdel;
	media.mouse.delta.y += ydel;
	media.mouse.cursor = pos;
	kEvent ev          = {.kind = kEvent_CursorMoved, .cursor = {.position = pos}};
	kAddEvent(ev);
}

void kAddCursorDeltaEvent(kVec2i delta)
{
	int xdel = delta.x;
	int ydel = delta.y;
	media.mouse.delta.x += xdel;
	media.mouse.delta.y += ydel;
	media.mouse.cursor.x += xdel;
	media.mouse.cursor.y += ydel;
	kEvent ev = {.kind = kEvent_CursorMoved, .cursor = {.position = media.mouse.cursor}};
	kAddEvent(ev);
}

void kAddWheelEvent(float horz, float vert)
{
	media.mouse.wheel.x += horz;
	media.mouse.wheel.y += vert;

	kEvent ev = {.kind = kEvent_WheelMoved, .wheel = {.horizontal = horz, .vertical = vert}};

	kAddEvent(ev);
}

void kAddCursorEnterEvent(void)
{
	media.window.flags[kWindow_Hovered] = true;
	kEvent ev                           = {.kind = kEvent_CursorEnter};
	kAddEvent(ev);
}

void kAddCursorLeaveEvent(void)
{
	media.window.flags[kWindow_Hovered] = false;
	kEvent ev                           = {.kind = kEvent_CursorLeave};
	kAddEvent(ev);
}

void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen)
{
	media.window.width                     = width;
	media.window.height                    = height;
	media.window.flags[kWindow_Resized]    = true;
	media.window.flags[kWindow_Fullscreen] = fullscreen;
	kEvent ev                              = {.kind = kEvent_Resized, .resized = {.width = width, .height = height}};
	kAddEvent(ev);

	media.render.ResizeSwapChain(width, height);
}

void kAddWindowActivateEvent(bool active)
{
	media.window.flags[kWindow_Active] = active;
	kEvent ev                          = {.kind = active ? kEvent_Activated : kEvent_Deactivated};
	kAddEvent(ev);
}

void kAddWindowCloseEvent(void)
{
	media.window.flags[kWindow_Closed] = true;
	kEvent ev                          = {.kind = kEvent_Closed};
	kAddEvent(ev);
}

void kAddWindowMaximizeEvent(void) { media.window.flags[kWindow_Maximized] = true; }
void kAddWindowRestoreEvent(void) { media.window.flags[kWindow_Maximized] = false; }
void kAddWindowCursorCaptureEvent(void) { media.window.flags[kWindow_Capturing] = true; }
void kAddWindowCursorReleaseEvent(void) { media.window.flags[kWindow_Capturing] = false; }

void kAddWindowDpiChangedEvent(float yfactor)
{
	media.window.yfactor = yfactor;
	kEvent ev            = {.kind = kEvent_DpiChanged};
	kAddEvent(ev);
}

//
//
//

void kResizeWindow(u32 w, u32 h) { media.backend.ResizeWindow(w, h); }
void kToggleWindowFullscreen(void) { media.backend.ToggleWindowFullscreen(); }
void kReleaseCursor(void) { media.backend.ReleaseCursor(); }
void kCaptureCursor(void) { media.backend.CaptureCursor(); }
int  kGetWindowCaptionSize(void) { return media.backend.GetWindowCaptionSize(); }
void kMaximizeWindow(void) { media.backend.MaximizeWindow(); }
void kRestoreWindow(void) { media.backend.RestoreWindow(); }
void kMinimizeWindow(void) { media.backend.MinimizeWindow(); }
void kCloseWindow(void) { media.backend.CloseWindow(); }

//
//
//

void kUserLoad(void) { media.user.load(); }
void kUserUpdate(float dt) { media.user.update(dt); }
void kUserRelease(void) { media.user.release(); }

void kUpdateFrame(float dt)
{
	kResetFrame();

	kUserUpdate(dt);

	if (kIsWindowClosed())
	{
		kBreakLoop(0);
	}

	kRenderFrame2D frame;
	kGetFrameData(&frame);

	media.render.ExecuteFrame(frame);
	media.render.Present();
	media.render.NextFrame();
}

//
//
//

#include "Font/kFont.h"

static void kCreateBuiltinResources(void)
{
	{
		u8           pixels[] = {0xff, 0xff, 0xff, 0xff};

		kTextureSpec spec     = {};
		spec.width            = 1;
		spec.height           = 1;
		spec.num_samples      = 1;
		spec.pitch            = 1 * sizeof(u32);
		spec.format           = kFormat_RGBA8_UNORM;
		spec.flags            = 0;
		spec.pixels           = pixels;

		media.builtin.texture = media.render.CreateTexture(spec);
	}

	{
		kFont       *font = &media.builtin.font;

		kTextureSpec spec = {};
		spec.num_samples  = 1;
		spec.format       = kFormat_R8_UNORM;
		spec.flags        = 0;
		spec.width        = kEmFontAtlasWidth;
		spec.height       = kEmFontAtlasHeight;
		spec.pitch        = kEmFontAtlasWidth;
		spec.pixels       = (u8 *)kEmFontAtlasPixels;

		font->texture     = media.render.CreateTexture(spec);

		if (!font->texture)
		{
			font->texture  = media.builtin.texture;
			font->map      = nullptr;
			font->glyphs   = nullptr;
			font->fallback = (kGlyph *)&FallbackGlyph;
			font->mincp    = 0;
			font->maxcp    = 0;
			font->count    = 0;
			font->size     = (i16)FallbackFontHeight;
			font->ascent   = (i16)FallbackFontHeight;
			font->descent  = (i16)FallbackFontHeight;
			font->linegap  = (i16)FallbackFontHeight;
			return;
		}

		font->map      = (u16 *)kEmFontGlyphMap;
		font->glyphs   = (kGlyph *)kEmFontGlyphs;
		font->fallback = (kGlyph *)&font->glyphs[kEmFontGlyphCount - 1];
		font->mincp    = kEmFontMinCodepoint;
		font->maxcp    = kEmFontMaxCodepoint;
		font->count    = kEmFontGlyphCount;
		font->size     = (i16)kEmFontSize;
		font->ascent   = (i16)kEmFontAscent;
		font->descent  = (i16)kEmFontDescent;
		font->linegap  = (i16)kEmFontLineGap;
	}
}

static void kDestroyBuiltinResources(void)
{
	if (media.builtin.font.texture != media.builtin.texture)
	{
		media.render.DestroyTexture(media.builtin.font.texture);
	}

	if (media.builtin.texture)
	{
		media.render.DestroyTexture(media.builtin.texture);
	}
}

//
//
//

int kEventLoop(const kMediaSpec &spec, const kMediaUserEvents &user)
{
	memset(&media, 0, sizeof(media));

	kCreateMediaBackend(&media.backend);

	kLogInfoEx("Windows", "Creating render backend.\n");
	kCreateRenderBackend(&media.render);

	void *window = media.backend.CreateWindow(&media.window, spec.window);
	media.render.CreateSwapChain(window);

	media.events.Reserve(64);
	media.backend.LoadMouseState(&media.mouse);
	media.backend.LoadKeyboardState(&media.keyboard);

	kSetUserEvents(user);

	kAllocator *allocator = kGetContextAllocator();
	media.arena           = kAllocArena(spec.arena, allocator);

	if (media.arena != &kFallbackArena)
	{
		kLogInfoEx("Windows", "Allocated frame arena of size: %zu bytes\n", (u64)media.arena->cap);
	}
	else
	{
		kLogWarningEx("Windows", "Failed to allocate frame arena.\n");
	}

	kCreateBuiltinResources();

	kTexture *textures[kTextureType_Count];
	for (int i = 0; i < kTextureType_Count; ++i)
		textures[i] = media.builtin.texture;

	kCreateRenderContext(kDefaultRenderSpec, textures, &media.builtin.font);

	kLogInfoEx("Windows", "Calling user load.\n");
	media.user.load();

	int status = media.backend.EventLoop();

	media.render.Flush();

	kLogInfoEx("Windows", "Calling user release.\n");
	media.user.release();

	kFreeArena(media.arena, allocator);
	kDestroyRenderContext();
	kDestroyBuiltinResources();

	media.render.DestroySwapChain();
	media.backend.DestroyWindow();
	media.render.Destroy();

	memset(&media, 0, sizeof(media));

	return status;
}

void kBreakLoop(int status) { media.backend.BreakLoop(status); }
