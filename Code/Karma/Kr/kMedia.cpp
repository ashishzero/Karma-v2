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
	kTexture Texture;
	kFont    Font;
} kMediaBuiltin;

typedef struct kMedia
{
	kArena          *Arena;
	kArray<kEvent>   Events;
	kKeyboardState   Keyboard;
	kMouseState      Mouse;
	kWindowState     Window;
	kMediaUserEvents User;
	kRenderBackend   Render;
	kMediaBackend    Backend;
	kMediaBuiltin    Builtin;
} kMedia;

static kMedia g_Media;

//
//
//

void kFallbackUserLoadProc(void)
{}
void kFallbackUserReleaseProc(void)
{}
void kFallbackUserUpdateProc(float dt)
{}
kSpan<kEvent> kGetEvents(void)
{
	return g_Media.Events;
}
kArena *kGetFrameArena(void)
{
	return g_Media.Arena;
}
void *kGetUserEventData(void)
{
	return g_Media.User.Data;
}
void kSetUserEventData(void *data)
{
	g_Media.User.Data = data;
}
void kGetUserEvents(kMediaUserEvents *user)
{
	memcpy(user, &g_Media.User, sizeof(g_Media.User));
}

void kSetUserEvents(const kMediaUserEvents &user)
{
	memcpy(&g_Media.User, &user, sizeof(g_Media.User));
	if (!g_Media.User.Load)
		g_Media.User.Load = kFallbackUserLoadProc;
	if (!g_Media.User.Release)
		g_Media.User.Release = kFallbackUserLoadProc;
	if (!g_Media.User.Update)
		g_Media.User.Update = kFallbackUserUpdateProc;
}

bool kIsKeyDown(kKey key)
{
	return g_Media.Keyboard.Keys[key].Down;
}
bool kKeyPressed(kKey key)
{
	return g_Media.Keyboard.Keys[key].Flags & kPressed;
}
bool kKeyReleased(kKey key)
{
	return g_Media.Keyboard.Keys[key].Flags & kReleased;
}
u8 kKeyHits(kKey key)
{
	return g_Media.Keyboard.Keys[key].Hits;
}
uint kGetKeyModFlags(void)
{
	return g_Media.Keyboard.Mods;
}
bool kIsButtonDown(kButton button)
{
	return g_Media.Mouse.Buttons[button].Down;
}
bool kButtonPressed(kButton button)
{
	return g_Media.Mouse.Buttons[button].Flags & kPressed;
}
bool kButtonReleased(kButton button)
{
	return g_Media.Mouse.Buttons[button].Flags & kReleased;
}
kVec2i kGetCursorPosition(void)
{
	return g_Media.Mouse.Cursor;
}
kVec2i kGetCursorDelta(void)
{
	return g_Media.Mouse.Delta;
}
float kGetWheelHorizontal(void)
{
	return g_Media.Mouse.Wheel.x;
}
float kGetWheelVertical(void)
{
	return g_Media.Mouse.Wheel.y;
}
bool kIsWindowClosed(void)
{
	return g_Media.Window.Flags[kWindow_Closed];
}
bool kIsWindowResized(void)
{
	return g_Media.Window.Flags[kWindow_Resized];
}
void kIgnoreWindowCloseEvent(void)
{
	g_Media.Window.Flags[kWindow_Closed] = false;
}
bool kIsWindowActive(void)
{
	return g_Media.Window.Flags[kWindow_Active];
}
bool kIsWindowFullscreen(void)
{
	return g_Media.Window.Flags[kWindow_Fullscreen];
}
bool kIsWindowMaximized(void)
{
	return g_Media.Window.Flags[kWindow_Maximized];
}
kVec2i kGetWindowSize(void)
{
	return kVec2i(g_Media.Window.Width, g_Media.Window.Height);
}

float kGetWindowAspectRatio(void)
{
	float y = (float)g_Media.Window.Height;
	float x = (float)g_Media.Window.Width;
	return x / y;
}

float kGetWindowDpiScale(void)
{
	return g_Media.Window.DpiFactor;
}
bool kIsCursorCaptured(void)
{
	return g_Media.Window.Flags[kWindow_Capturing];
}
bool kIsCursorHovered(void)
{
	return g_Media.Window.Flags[kWindow_Hovered];
}
void kGetKeyboardState(kKeyboardState *keyboard)
{
	memcpy(keyboard, &g_Media.Keyboard, sizeof(g_Media.Keyboard));
}

void kGetKeyState(kKeyState *state, kKey key)
{
	memcpy(state, &g_Media.Keyboard.Keys[key], sizeof(g_Media.Keyboard.Keys[key]));
}

void kGetMouseState(kMouseState *mouse)
{
	memcpy(mouse, &g_Media.Mouse, sizeof(g_Media.Mouse));
}

void kGetButtonState(kKeyState *state, kButton button)
{
	memcpy(state, &g_Media.Mouse.Buttons[button], sizeof(g_Media.Mouse.Buttons));
}

void kGetWindowState(kWindowState *state)
{
	memcpy(state, &g_Media.Window, sizeof(g_Media.Window));
}
void kSetKeyboardState(const kKeyboardState &keyboard)
{
	memcpy(&g_Media.Keyboard, &keyboard, sizeof(g_Media.Keyboard));
}

void kSetKeyState(const kKeyState &state, kKey key)
{
	memcpy(&g_Media.Keyboard.Keys[key], &state, sizeof(g_Media.Keyboard.Keys[key]));
}

void kSetKeyModFlags(uint mods)
{
	g_Media.Keyboard.Mods = mods;
}

void kSetMouseState(const kMouseState &mouse)
{
	memcpy(&g_Media.Mouse, &mouse, sizeof(g_Media.Mouse));
}

void kSetButtonState(const kKeyState &state, kButton button)
{
	memcpy(&g_Media.Mouse.Buttons[button], &state, sizeof(g_Media.Mouse.Buttons[button]));
}

void kClearInput(void)
{
	g_Media.Events.Reset();
	memset(&g_Media.Keyboard, 0, sizeof(g_Media.Keyboard));
	memset(&g_Media.Mouse, 0, sizeof(g_Media.Mouse));
}

void kClearFrame(void)
{
	kResetArena(g_Media.Arena);

	g_Media.Events.Count  = 0;
	g_Media.Keyboard.Mods = 0;

	for (uint key = 0; key < kKey_Count; ++key)
	{
		g_Media.Keyboard.Keys[key].Flags = 0;
		g_Media.Keyboard.Keys[key].Hits  = 0;
	}

	for (uint button = 0; button < kButton_Count; ++button)
	{
		g_Media.Mouse.Buttons[button].Flags = 0;
		g_Media.Mouse.Buttons[button].Hits  = 0;
	}

	memset(&g_Media.Mouse.Wheel, 0, sizeof(g_Media.Mouse.Wheel));

	g_Media.Window.Flags[kWindow_Resized] = false;
	g_Media.Window.Flags[kWindow_Closed]  = false;
}

void kAddEvent(const kEvent &ev)
{
	g_Media.Events.Add(ev);
}

void kAddKeyEvent(kKey key, bool down, bool repeat)
{
	bool       pressed  = down && !repeat;
	bool       released = !down;
	kKeyState *state    = &g_Media.Keyboard.Keys[key];
	state->Down         = down;
	if (released)
	{
		state->Flags |= kReleased;
	}
	if (pressed)
	{
		state->Flags |= kPressed;
		state->Hits += 1;
	}
	kEvent ev = {.Kind = down ? kEvent_KeyPressed : kEvent_KeyReleased, .Key = {.Symbol = key, .Repeat = repeat}};
	kAddEvent(ev);
}

void kAddButtonEvent(kButton button, bool down)
{
	kKeyState *state    = &g_Media.Mouse.Buttons[button];
	bool       previous = state->Down;
	bool       pressed  = down && !previous;
	bool       released = !down && previous;
	state->Down         = down;
	if (released)
	{
		state->Flags |= kReleased;
		state->Hits += 1;
	}
	if (pressed)
	{
		state->Flags |= kPressed;
	}
	kEvent ev = {.Kind = down ? kEvent_ButtonPressed : kEvent_ButtonReleased, .Button = {.Symbol = button}};
	kAddEvent(ev);
}

void kAddTextInputEvent(u32 codepoint, u32 mods)
{
	kEvent ev = {.Kind = kEvent_TextInput, .Text = {.Codepoint = codepoint, .Mods = mods}};
	kAddEvent(ev);
}

void kAddDoubleClickEvent(kButton button)
{
	kKeyState *state = &g_Media.Mouse.Buttons[button];
	state->Flags |= kDoubleClicked;
	kEvent ev = {.Kind = kEvent_DoubleClicked, .Button = {.Symbol = button}};
	kAddEvent(ev);
}

void kAddCursorEvent(kVec2i pos)
{
	int xdel = pos.x - g_Media.Mouse.Cursor.x;
	int ydel = pos.y - g_Media.Mouse.Cursor.y;
	g_Media.Mouse.Delta.x += xdel;
	g_Media.Mouse.Delta.y += ydel;
	g_Media.Mouse.Cursor = pos;
	kEvent ev            = {.Kind = kEvent_CursorMoved, .Cursor = {.Position = pos}};
	kAddEvent(ev);
}

void kAddCursorDeltaEvent(kVec2i delta)
{
	int xdel = delta.x;
	int ydel = delta.y;
	g_Media.Mouse.Delta.x += xdel;
	g_Media.Mouse.Delta.y += ydel;
	g_Media.Mouse.Cursor.x += xdel;
	g_Media.Mouse.Cursor.y += ydel;
	kEvent ev = {.Kind = kEvent_CursorMoved, .Cursor = {.Position = g_Media.Mouse.Cursor}};
	kAddEvent(ev);
}

void kAddWheelEvent(float horz, float vert)
{
	g_Media.Mouse.Wheel.x += horz;
	g_Media.Mouse.Wheel.y += vert;

	kEvent ev = {.Kind = kEvent_WheelMoved, .Wheel = {.Horz = horz, .Vert = vert}};

	kAddEvent(ev);
}

void kAddCursorEnterEvent(void)
{
	g_Media.Window.Flags[kWindow_Hovered] = true;
	kEvent ev                             = {.Kind = kEvent_CursorEnter};
	kAddEvent(ev);
}

void kAddCursorLeaveEvent(void)
{
	g_Media.Window.Flags[kWindow_Hovered] = false;
	kEvent ev                             = {.Kind = kEvent_CursorLeave};
	kAddEvent(ev);
}

void kAddWindowResizeEvent(u32 width, u32 height, bool fullscreen)
{
	g_Media.Window.Width                     = width;
	g_Media.Window.Height                    = height;
	g_Media.Window.Flags[kWindow_Resized]    = true;
	g_Media.Window.Flags[kWindow_Fullscreen] = fullscreen;
	kEvent ev                                = {.Kind = kEvent_Resized, .Resized = {.Width = width, .Height = height}};
	kAddEvent(ev);

	g_Media.Render.ResizeSwapChain(width, height);
}

void kAddWindowActivateEvent(bool active)
{
	g_Media.Window.Flags[kWindow_Active] = active;
	kEvent ev                            = {.Kind = active ? kEvent_Activated : kEvent_Deactivated};
	kAddEvent(ev);
}

void kAddWindowCloseEvent(void)
{
	g_Media.Window.Flags[kWindow_Closed] = true;
	kEvent ev                            = {.Kind = kEvent_Closed};
	kAddEvent(ev);
}

void kAddWindowMaximizeEvent(void)
{
	g_Media.Window.Flags[kWindow_Maximized] = true;
}
void kAddWindowRestoreEvent(void)
{
	g_Media.Window.Flags[kWindow_Maximized] = false;
}
void kAddWindowCursorCaptureEvent(void)
{
	g_Media.Window.Flags[kWindow_Capturing] = true;
}
void kAddWindowCursorReleaseEvent(void)
{
	g_Media.Window.Flags[kWindow_Capturing] = false;
}

void kAddWindowDpiChangedEvent(float yfactor)
{
	g_Media.Window.DpiFactor = yfactor;
	kEvent ev                = {.Kind = kEvent_DpiChanged};
	kAddEvent(ev);
}

//
//
//

void kGetRenderPipelineConfig(kRenderPipelineConfig *config)
{
	g_Media.Render.GetRenderPipelineConfig(config);
}

void kApplyRenderPipelineConfig(const kRenderPipelineConfig &config)
{
	g_Media.Render.ApplyRenderPipelineConfig(config);
}

kTexture kCreateTexture(const kTextureSpec &spec)
{
	return g_Media.Render.CreateTexture(spec);
}

void kDestroyTexture(kTexture texture)
{
	g_Media.Render.DestroyTexture(texture);
}

//
//
//

void kResizeWindow(u32 w, u32 h)
{
	g_Media.Backend.ResizeWindow(w, h);
}
void kToggleWindowFullscreen(void)
{
	g_Media.Backend.ToggleWindowFullscreen();
}
void kReleaseCursor(void)
{
	g_Media.Backend.ReleaseCursor();
}
void kCaptureCursor(void)
{
	g_Media.Backend.CaptureCursor();
}
int kGetWindowCaptionSize(void)
{
	return g_Media.Backend.GetWindowCaptionSize();
}
void kMaximizeWindow(void)
{
	g_Media.Backend.MaximizeWindow();
}
void kRestoreWindow(void)
{
	g_Media.Backend.RestoreWindow();
}
void kMinimizeWindow(void)
{
	g_Media.Backend.MinimizeWindow();
}
void kCloseWindow(void)
{
	g_Media.Backend.CloseWindow();
}

//
//
//

void kUserLoad(void)
{
	g_Media.User.Load();
}
void kUserUpdate(float dt)
{
	g_Media.User.Update(dt);
}
void kUserRelease(void)
{
	g_Media.User.Release();
}

namespace ImGui
{
void NewFrame();
} // namespace ImGui

void kUpdateFrame(float dt)
{
	g_Media.Render.NextFrame();

#ifndef IMGUI_DISABLE
	ImGui::NewFrame();
#endif

	kResetFrame();

	kUserUpdate(dt);

	if (kIsWindowClosed())
	{
		kBreakLoop(0);
	}

	kRenderFrame frame;
	kGetFrameData(&frame.Frame2D);

	g_Media.Render.ExecuteFrame(frame);
	g_Media.Render.Present();
}

//
//
//

#include "Font/kFont.h"

static void kCreateBuiltinResources(void)
{
	{
		u8           pixels[]   = {0xff, 0xff, 0xff, 0xff};

		kTextureSpec spec       = {};
		spec.Width              = 1;
		spec.Height             = 1;
		spec.Pitch              = 1 * sizeof(u32);
		spec.Format             = kFormat_RGBA8_UNORM;
		spec.Pixels             = pixels;
		spec.Name               = "White";

		g_Media.Builtin.Texture = g_Media.Render.CreateTexture(spec);
	}

	{
		kFont       *font = &g_Media.Builtin.Font;

		kTextureSpec spec = {};
		spec.Format       = kFormat_R8_UNORM;
		spec.Width        = kEmFontAtlasWidth;
		spec.Height       = kEmFontAtlasHeight;
		spec.Pitch        = kEmFontAtlasWidth;
		spec.Pixels       = (u8 *)kEmFontAtlasPixels;
		spec.Name         = "BuiltinFont";

		font->Atlas       = g_Media.Render.CreateTexture(spec);

		if (!font->Atlas.ID.Index)
		{
			font->Atlas        = g_Media.Builtin.Texture;
			font->CodepointMap = nullptr;
			font->Glyphs       = nullptr;
			font->Fallback     = (kGlyph *)&FallbackGlyph;
			font->MinCodepoint = 0;
			font->MaxCodepoint = 0;
			font->GlyphsCount  = 0;
			font->Height       = (i16)FallbackFontHeight;
			font->Ascent       = (i16)FallbackFontHeight;
			font->Descent      = (i16)FallbackFontHeight;
			font->Linegap      = (i16)FallbackFontHeight;
			return;
		}

		font->CodepointMap = (u16 *)kEmFontGlyphMap;
		font->Glyphs       = (kGlyph *)kEmFontGlyphs;
		font->Fallback     = (kGlyph *)&font->Glyphs[kEmFontGlyphCount - 1];
		font->MinCodepoint = kEmFontMinCodepoint;
		font->MaxCodepoint = kEmFontMaxCodepoint;
		font->GlyphsCount  = kEmFontGlyphCount;
		font->Height       = (i16)kEmFontSize;
		font->Ascent       = (i16)kEmFontAscent;
		font->Descent      = (i16)kEmFontDescent;
		font->Linegap      = (i16)kEmFontLineGap;
	}
}

static void kDestroyBuiltinResources(void)
{
	if (g_Media.Builtin.Font.Atlas != g_Media.Builtin.Texture)
	{
		g_Media.Render.DestroyTexture(g_Media.Builtin.Font.Atlas);
	}

	if (g_Media.Builtin.Texture.ID.Index)
	{
		g_Media.Render.DestroyTexture(g_Media.Builtin.Texture);
	}
}

//
//
//

#ifndef IMGUI_DISABLE

#include "ImGui/imgui.h"

static ImVec4 ImMix(ImVec4 a, ImVec4 b, float t)
{
	ImVec4 r;
	r.x = (1.0f - t) * a.x + t * b.x;
	r.y = (1.0f - t) * a.y + t * b.y;
	r.z = (1.0f - t) * a.z + t * b.z;
	r.w = (1.0f - t) * a.w + t * b.w;
	return r;
}

void ImCreateContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

	io.IniFilename                         = "kImGui.ini";
	io.LogFilename                         = "kImGuiLogs.txt";

	ImGuiStyle &style                      = ImGui::GetStyle();
	ImVec4     *colors                     = style.Colors;

	colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	colors[ImGuiCol_PopupBg]               = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	colors[ImGuiCol_Border]                = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	colors[ImGuiCol_BorderShadow]          = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	colors[ImGuiCol_FrameBg]               = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgActive]         = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.09f, 0.12f, 0.75f);
	colors[ImGuiCol_TitleBgActive]         = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.80f, 1.0f, 1.0f);
	colors[ImGuiCol_SliderGrab]            = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	colors[ImGuiCol_Button]                = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	colors[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	colors[ImGuiCol_ButtonActive]          = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	colors[ImGuiCol_Header]                = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	colors[ImGuiCol_HeaderHovered]         = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	colors[ImGuiCol_HeaderActive]          = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	colors[ImGuiCol_ResizeGrip]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	colors[ImGuiCol_PlotLines]             = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram]         = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	colors[ImGuiCol_Separator]             = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive]       = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);

	colors[ImGuiCol_DockingPreview]        = ImVec4(1.5f, 1.0f, 1.0f, 1.0f);
	colors[ImGuiCol_DockingEmptyBg]        = ImVec4(1.5f, 1.0f, 1.0f, 1.00f);
	colors[ImGuiCol_TabHovered]            = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_Tab]                   = ImMix(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
	colors[ImGuiCol_TabActive]          = ImMix(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
	colors[ImGuiCol_TabUnfocused]       = ImMix(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabUnfocusedActive] = ImMix(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
}
#else
void ImCreateContext()
{}
#endif

int kEventLoop(const kMediaSpec &spec, const kMediaUserEvents &user)
{
	ImCreateContext();

	memset(&g_Media, 0, sizeof(g_Media));

	kCreateMediaBackend(&g_Media.Backend);

	kLogInfoEx("Windows", "Creating render backend.\n");
	kCreateRenderBackend(&g_Media.Render);

	void *window = g_Media.Backend.CreateWindow(&g_Media.Window, spec.Window);
	g_Media.Render.CreateSwapChain(window, spec.RenderPipeline);

	g_Media.Events.Reserve(64);
	g_Media.Backend.LoadMouseState(&g_Media.Mouse);
	g_Media.Backend.LoadKeyboardState(&g_Media.Keyboard);

	kSetUserEvents(user);

	kAllocator *allocator = kGetContextAllocator();
	g_Media.Arena         = kAllocArena(spec.Arena, allocator);

	if (g_Media.Arena != &kFallbackArena)
	{
		kLogInfoEx("Windows", "Allocated frame arena of size: %zu bytes\n", (u64)g_Media.Arena->Cap);
	}
	else
	{
		kLogWarningEx("Windows", "Failed to allocate frame arena.\n");
	}

	kCreateBuiltinResources();

	kTexture textures[kTextureType_Count];
	for (int i = 0; i < kTextureType_Count; ++i)
		textures[i] = g_Media.Builtin.Texture;

	kCreateRenderContext(kDefaultRenderSpec, textures, &g_Media.Builtin.Font);

	kLogInfoEx("Windows", "Calling user load.\n");
	g_Media.User.Load();

	int status = g_Media.Backend.EventLoop();

	g_Media.Render.Flush();

	kLogInfoEx("Windows", "Calling user release.\n");
	g_Media.User.Release();

	kFreeArena(g_Media.Arena, allocator);
	kDestroyRenderContext();
	kDestroyBuiltinResources();

	g_Media.Render.DestroySwapChain();
	g_Media.Backend.DestroyWindow();
	g_Media.Render.Destroy();

#ifndef IMGUI_DISABLE
	ImGui::DestroyContext();
#endif

	memset(&g_Media, 0, sizeof(g_Media));

	return status;
}

void kBreakLoop(int status)
{
	g_Media.Backend.BreakLoop(status);
}
