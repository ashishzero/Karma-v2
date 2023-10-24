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

static kMedia g_Media;

//
//
//

void          kFallbackUserLoadProc(void) {}
void          kFallbackUserReleaseProc(void) {}
void          kFallbackUserUpdateProc(float dt) {}
kSpan<kEvent> kGetEvents(void) { return g_Media.events; }
kArena       *kGetFrameArena(void) { return g_Media.arena; }
void         *kGetUserEventData(void) { return g_Media.user.data; }
void          kSetUserEventData(void *data) { g_Media.user.data = data; }
void          kGetUserEvents(kMediaUserEvents *user) { memcpy(user, &g_Media.user, sizeof(g_Media.user)); }

void          kSetUserEvents(const kMediaUserEvents &user)
{
	memcpy(&g_Media.user, &user, sizeof(g_Media.user));

	if (!g_Media.user.load) g_Media.user.load = kFallbackUserLoadProc;

	if (!g_Media.user.release) g_Media.user.release = kFallbackUserLoadProc;

	if (!g_Media.user.update) g_Media.user.update = kFallbackUserUpdateProc;
}

bool   kIsKeyDown(kKey key) { return g_Media.keyboard.keys[key].down; }
bool   kKeyPressed(kKey key) { return g_Media.keyboard.keys[key].flags & kPressed; }
bool   kKeyReleased(kKey key) { return g_Media.keyboard.keys[key].flags & kReleased; }
u8     kKeyHits(kKey key) { return g_Media.keyboard.keys[key].hits; }
uint   kGetKeyModFlags(void) { return g_Media.keyboard.mods; }
bool   kIsButtonDown(kButton button) { return g_Media.mouse.buttons[button].down; }
bool   kButtonPressed(kButton button) { return g_Media.mouse.buttons[button].flags & kPressed; }
bool   kButtonReleased(kButton button) { return g_Media.mouse.buttons[button].flags & kReleased; }
kVec2i kGetCursorPosition(void) { return g_Media.mouse.cursor; }
kVec2i kGetCursorDelta(void) { return g_Media.mouse.delta; }
float  kGetWheelHorizontal(void) { return g_Media.mouse.wheel.x; }
float  kGetWheelVertical(void) { return g_Media.mouse.wheel.y; }
bool   kIsWindowClosed(void) { return g_Media.window.flags[kWindow_Closed]; }
bool   kIsWindowResized(void) { return g_Media.window.flags[kWindow_Resized]; }
void   kIgnoreWindowCloseEvent(void) { g_Media.window.flags[kWindow_Closed] = false; }
bool   kIsWindowActive(void) { return g_Media.window.flags[kWindow_Active]; }
bool   kIsWindowFullscreen(void) { return g_Media.window.flags[kWindow_Fullscreen]; }
bool   kIsWindowMaximized(void) { return g_Media.window.flags[kWindow_Maximized]; }
kVec2i kGetWindowSize(void) { return kVec2i(g_Media.window.width, g_Media.window.height); }

float  kGetWindowAspectRatio(void)
{
	float y = (float)g_Media.window.height;
	float x = (float)g_Media.window.width;
	return x / y;
}

float kGetWindowDpiScale(void) { return g_Media.window.yfactor; }
bool  kIsCursorCaptured(void) { return g_Media.window.flags[kWindow_Capturing]; }
bool  kIsCursorHovered(void) { return g_Media.window.flags[kWindow_Hovered]; }
void  kGetKeyboardState(kKeyboardState *keyboard) { memcpy(keyboard, &g_Media.keyboard, sizeof(g_Media.keyboard)); }

void  kGetKeyState(kKeyState *state, kKey key)
{
	memcpy(state, &g_Media.keyboard.keys[key], sizeof(g_Media.keyboard.keys[key]));
}

void kGetMouseState(kMouseState *mouse) { memcpy(mouse, &g_Media.mouse, sizeof(g_Media.mouse)); }

void kGetButtonState(kKeyState *state, kButton button)
{
	memcpy(state, &g_Media.mouse.buttons[button], sizeof(g_Media.mouse.buttons));
}

void kGetWindowState(kWindowState *state) { memcpy(state, &g_Media.window, sizeof(g_Media.window)); }
void kSetKeyboardState(const kKeyboardState &keyboard) { memcpy(&g_Media.keyboard, &keyboard, sizeof(g_Media.keyboard)); }

void kSetKeyState(const kKeyState &state, kKey key)
{
	memcpy(&g_Media.keyboard.keys[key], &state, sizeof(g_Media.keyboard.keys[key]));
}

void kSetKeyModFlags(uint mods) { g_Media.keyboard.mods = mods; }

void kSetMouseState(const kMouseState &mouse) { memcpy(&g_Media.mouse, &mouse, sizeof(g_Media.mouse)); }

void kSetButtonState(const kKeyState &state, kButton button)
{
	memcpy(&g_Media.mouse.buttons[button], &state, sizeof(g_Media.mouse.buttons[button]));
}

void kClearInput(void)
{
	g_Media.events.Reset();
	memset(&g_Media.keyboard, 0, sizeof(g_Media.keyboard));
	memset(&g_Media.mouse, 0, sizeof(g_Media.mouse));
}

void kClearFrame(void)
{
	kResetArena(g_Media.arena);

	g_Media.events.count  = 0;
	g_Media.keyboard.mods = 0;

	for (uint key = 0; key < kKey_Count; ++key)
	{
		g_Media.keyboard.keys[key].flags = 0;
		g_Media.keyboard.keys[key].hits  = 0;
	}

	for (uint button = 0; button < kButton_Count; ++button)
	{
		g_Media.mouse.buttons[button].flags = 0;
		g_Media.mouse.buttons[button].hits  = 0;
	}

	memset(&g_Media.mouse.wheel, 0, sizeof(g_Media.mouse.wheel));

	g_Media.window.flags[kWindow_Resized] = false;
	g_Media.window.flags[kWindow_Closed]  = false;
}

void kAddEvent(const kEvent &ev) { g_Media.events.Add(ev); }

void kAddKeyEvent(kKey key, bool down, bool repeat)
{
	bool       pressed  = down && !repeat;
	bool       released = !down;
	kKeyState *state    = &g_Media.keyboard.keys[key];
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
	kKeyState *state    = &g_Media.mouse.buttons[button];
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
	kKeyState *state = &g_Media.mouse.buttons[button];
	state->flags |= kDoubleClicked;
	kEvent ev = {.kind = kEvent_DoubleClicked, .button = {.symbol = button}};
	kAddEvent(ev);
}

void kAddCursorEvent(kVec2i pos)
{
	int xdel = pos.x - g_Media.mouse.cursor.x;
	int ydel = pos.y - g_Media.mouse.cursor.y;
	g_Media.mouse.delta.x += xdel;
	g_Media.mouse.delta.y += ydel;
	g_Media.mouse.cursor = pos;
	kEvent ev          = {.kind = kEvent_CursorMoved, .cursor = {.position = pos}};
	kAddEvent(ev);
}

void kAddCursorDeltaEvent(kVec2i delta)
{
	int xdel = delta.x;
	int ydel = delta.y;
	g_Media.mouse.delta.x += xdel;
	g_Media.mouse.delta.y += ydel;
	g_Media.mouse.cursor.x += xdel;
	g_Media.mouse.cursor.y += ydel;
	kEvent ev = {.kind = kEvent_CursorMoved, .cursor = {.position = g_Media.mouse.cursor}};
	kAddEvent(ev);
}

void kAddWheelEvent(float horz, float vert)
{
	g_Media.mouse.wheel.x += horz;
	g_Media.mouse.wheel.y += vert;

	kEvent ev = {.kind = kEvent_WheelMoved, .wheel = {.horizontal = horz, .vertical = vert}};

	kAddEvent(ev);
}

void kAddCursorEnterEvent(void)
{
	g_Media.window.flags[kWindow_Hovered] = true;
	kEvent ev                           = {.kind = kEvent_CursorEnter};
	kAddEvent(ev);
}

void kAddCursorLeaveEvent(void)
{
	g_Media.window.flags[kWindow_Hovered] = false;
	kEvent ev                           = {.kind = kEvent_CursorLeave};
	kAddEvent(ev);
}

void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen)
{
	g_Media.window.width                     = width;
	g_Media.window.height                    = height;
	g_Media.window.flags[kWindow_Resized]    = true;
	g_Media.window.flags[kWindow_Fullscreen] = fullscreen;
	kEvent ev                              = {.kind = kEvent_Resized, .resized = {.width = width, .height = height}};
	kAddEvent(ev);

	g_Media.render.ResizeSwapChain(width, height);
}

void kAddWindowActivateEvent(bool active)
{
	g_Media.window.flags[kWindow_Active] = active;
	kEvent ev                          = {.kind = active ? kEvent_Activated : kEvent_Deactivated};
	kAddEvent(ev);
}

void kAddWindowCloseEvent(void)
{
	g_Media.window.flags[kWindow_Closed] = true;
	kEvent ev                          = {.kind = kEvent_Closed};
	kAddEvent(ev);
}

void kAddWindowMaximizeEvent(void) { g_Media.window.flags[kWindow_Maximized] = true; }
void kAddWindowRestoreEvent(void) { g_Media.window.flags[kWindow_Maximized] = false; }
void kAddWindowCursorCaptureEvent(void) { g_Media.window.flags[kWindow_Capturing] = true; }
void kAddWindowCursorReleaseEvent(void) { g_Media.window.flags[kWindow_Capturing] = false; }

void kAddWindowDpiChangedEvent(float yfactor)
{
	g_Media.window.yfactor = yfactor;
	kEvent ev            = {.kind = kEvent_DpiChanged};
	kAddEvent(ev);
}

//
//
//

void kResizeWindow(u32 w, u32 h) { g_Media.backend.ResizeWindow(w, h); }
void kToggleWindowFullscreen(void) { g_Media.backend.ToggleWindowFullscreen(); }
void kReleaseCursor(void) { g_Media.backend.ReleaseCursor(); }
void kCaptureCursor(void) { g_Media.backend.CaptureCursor(); }
int  kGetWindowCaptionSize(void) { return g_Media.backend.GetWindowCaptionSize(); }
void kMaximizeWindow(void) { g_Media.backend.MaximizeWindow(); }
void kRestoreWindow(void) { g_Media.backend.RestoreWindow(); }
void kMinimizeWindow(void) { g_Media.backend.MinimizeWindow(); }
void kCloseWindow(void) { g_Media.backend.CloseWindow(); }

//
//
//

void kUserLoad(void) { g_Media.user.load(); }
void kUserUpdate(float dt) { g_Media.user.update(dt); }
void kUserRelease(void) { g_Media.user.release(); }

void kUpdateFrame(float dt)
{
	kResetFrame();

	kUserUpdate(dt);

	if (kIsWindowClosed())
	{
		kBreakLoop(0);
	}

	kRenderFrame frame;
	kGetFrameData(&frame.render2d);

	g_Media.render.ExecuteFrame(frame);
	g_Media.render.Present();
	g_Media.render.NextFrame();
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

		g_Media.builtin.texture = g_Media.render.CreateTexture(spec);
	}

	{
		kFont       *font = &g_Media.builtin.font;

		kTextureSpec spec = {};
		spec.num_samples  = 1;
		spec.format       = kFormat_R8_UNORM;
		spec.flags        = 0;
		spec.width        = kEmFontAtlasWidth;
		spec.height       = kEmFontAtlasHeight;
		spec.pitch        = kEmFontAtlasWidth;
		spec.pixels       = (u8 *)kEmFontAtlasPixels;

		font->texture     = g_Media.render.CreateTexture(spec);

		if (!font->texture)
		{
			font->texture  = g_Media.builtin.texture;
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
	if (g_Media.builtin.font.texture != g_Media.builtin.texture)
	{
		g_Media.render.DestroyTexture(g_Media.builtin.font.texture);
	}

	if (g_Media.builtin.texture)
	{
		g_Media.render.DestroyTexture(g_Media.builtin.texture);
	}
}

//
//
//

int kEventLoop(const kMediaSpec &spec, const kMediaUserEvents &user)
{
	memset(&g_Media, 0, sizeof(g_Media));

	kCreateMediaBackend(&g_Media.backend);

	kLogInfoEx("Windows", "Creating render backend.\n");
	kCreateRenderBackend(&g_Media.render);

	void *window = g_Media.backend.CreateWindow(&g_Media.window, spec.window);
	g_Media.render.CreateSwapChain(window);

	g_Media.events.Reserve(64);
	g_Media.backend.LoadMouseState(&g_Media.mouse);
	g_Media.backend.LoadKeyboardState(&g_Media.keyboard);

	kSetUserEvents(user);

	kAllocator *allocator = kGetContextAllocator();
	g_Media.arena           = kAllocArena(spec.arena, allocator);

	if (g_Media.arena != &kFallbackArena)
	{
		kLogInfoEx("Windows", "Allocated frame arena of size: %zu bytes\n", (u64)g_Media.arena->cap);
	}
	else
	{
		kLogWarningEx("Windows", "Failed to allocate frame arena.\n");
	}

	kCreateBuiltinResources();

	kTexture *textures[kTextureType_Count];
	for (int i = 0; i < kTextureType_Count; ++i)
		textures[i] = g_Media.builtin.texture;

	kCreateRenderContext(kDefaultRenderSpec, textures, &g_Media.builtin.font);

	kLogInfoEx("Windows", "Calling user load.\n");
	g_Media.user.load();

	int status = g_Media.backend.EventLoop();

	g_Media.render.Flush();

	kLogInfoEx("Windows", "Calling user release.\n");
	g_Media.user.release();

	kFreeArena(g_Media.arena, allocator);
	kDestroyRenderContext();
	kDestroyBuiltinResources();

	g_Media.render.DestroySwapChain();
	g_Media.backend.DestroyWindow();
	g_Media.render.Destroy();

	memset(&g_Media, 0, sizeof(g_Media));

	return status;
}

void kBreakLoop(int status) { g_Media.backend.BreakLoop(status); }
