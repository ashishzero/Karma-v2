#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kImGui.h"

void Tick(float t, float dt)
{}

void Update(float dt, float alpha)
{
	kArena *arena = kGetFrameArena();

	if (kKeyPressed(kKey::Escape))
	{
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey::F11))
	{
		kToggleWindowFullscreen();
	}

	if (kKeyPressed(kKey::R) && (kGetKeyModFlags() & (kKeyMod_Shift | kKeyMod_Ctrl)))
	{
		kRestartLoop();
	}

	kVec2i      size     = kGetWindowSize();
	float       ar       = kGetWindowAspectRatio();
	float       yfactor  = kGetWindowDpiScale();
	kViewport   viewport = {0, 0, size.x, size.y};

	kCameraView view     = kOrthographicView(ar, 100.0f);

	kVec2i      pos      = kGetCursorPosition();
	float       sx       = ar * 50.0f * ((float)pos.x / size.x * 2.0f - 1.0f);
	float       sy       = 50.0f * ((float)(pos.y) / size.y * 2.0f - 1.0f);

	kLineThickness(0.1f);

	kBeginScene(view, viewport);

	kDrawCircle(kVec2(sx, sy), 0.5f, kVec4(2, 2, 1, 1));

	kEndScene();

	kCameraView ui = KOrthographicView(0, (float)size.x, 0, (float)size.y);

	kBeginRenderPass(kRenderPass_HUD);
	kBeginScene(ui, viewport);
	kPushOutLineStyle(kVec3(0.1f), 0.4f);
	kString FPS = kFormatString(arena, "FPS: %d (%.2fms)", (int)(1.0f / dt), 1000.0f * dt);
	kDrawText(FPS, kVec2(4), kVec4(1, 1, 1, 1), 0.5f * yfactor);
	kPopOutLineStyle();
	kEndScene();
	kEndRenderPass();
}

static float           g_Now;
static float           g_Accumulator;
static constexpr float FixedDeltaTime = 1.0f / 60.0f;

void                   OnLoadEvent(void *data)
{}

void OnReleaseEvent(void *data)
{}

void OnUpdateEvent(float dt, void *data)
{
	dt = kMin(dt, 0.25f);

	g_Accumulator += dt;

	while (g_Accumulator >= FixedDeltaTime)
	{
		Tick(g_Now, FixedDeltaTime);
		g_Accumulator -= FixedDeltaTime;
		g_Now += FixedDeltaTime;
	}

	float alpha = g_Accumulator / FixedDeltaTime;

	Update(dt, alpha);
}

void Main(int argc, const char **argv)
{
	kSetLogLevel(kLogLevel::Trace);
	kMediaUserEvents user = {.Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};
	kEventLoop(kDefaultSpec, user);
}
