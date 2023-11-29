#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kImGui.h"

struct WorldTime
{
	float Now;
	float Delta;
	float Accumulator;
};

struct World
{
	WorldTime Time;
	kTexture  Texture;
};

//
//
//

kTexture LoadTexture(kString filepath)
{
	kString content = kReadEntireFile(filepath);

	kImage  img;
	kReadImage(content, &img, 4);

	kTextureSpec spec = {.Format = kFormat_RGBA8_UNORM_SRGB,
	                     .Width  = (u32)img.Width,
	                     .Height = (u32)img.Height,
	                     .Pitch  = (u32)img.Width * img.Channels,
	                     .Pixels = img.Pixels,
	                     .Name   = filepath};

	kTexture     t    = kCreateTexture(spec);

	kFree(content.Items, content.Count + 1);
	kFreeImage(img);

	return t;
}

void Tick(World *world, float t, float dt)
{
}

void Update(World *world, float dt, float alpha)
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

	kBeginScene(view, viewport);
	kDrawTexture(world->Texture, kVec2(0), kVec2(25));
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

void OnLoadEvent(void *data)
{
	World *world            = (World *)data;
	world->Time.Accumulator = 0;
	world->Time.Delta       = 1.0f / 60.0f;
	world->Time.Now         = 0.0f;
	world->Texture          = LoadTexture("Resources/ColossChibi.png");
}

void OnReleaseEvent(void *data)
{
	World *world = (World *)data;
	kDestroyTexture(world->Texture);
}

void OnUpdateEvent(float dt, void *data)
{
	dt               = kMin(dt, 0.25f);

	World *    world = (World *)data;
	WorldTime &time  = world->Time;
	time.Accumulator += dt;

	while (time.Accumulator >= time.Delta)
	{
		Tick(world, time.Now, time.Delta);
		time.Accumulator -= time.Delta;
		time.Now += time.Delta;
	}

	float alpha = time.Accumulator / time.Delta;

	Update(world, dt, alpha);
}

void Main(int argc, const char **argv)
{
	kSetLogLevel(kLogLevel::Trace);
	World *          world = (World *)kAlloc(sizeof(World));
	kMediaUserEvents user  = {.Data = world, .Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};
	kEventLoop(kDefaultSpec, user);
}
