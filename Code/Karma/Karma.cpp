#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"

static kTexture texture;

kTexture        LoadTexture(kString filepath)
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

void Load(void)
{
	texture = LoadTexture("Resources/ColossChibi.png");
}

void Update(float dt)
{
	kArena *arena = kGetFrameArena();

	if (kKeyPressed(kKey_Escape))
	{
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey_F11))
	{
		kToggleWindowFullscreen();
	}

	kVec2i      size     = kGetWindowSize();
	float       ar       = kGetWindowAspectRatio();
	float       yfactor  = kGetWindowDpiScale();
	kViewport   viewport = {0, 0, size.x, size.y};

	kCameraView view     = kOrthographicView(ar, 100.0f);

	kBeginScene(view, viewport);
	kDrawTexture(texture, kVec2(0), kVec2(25));
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

void Main(int argc, const char **argv)
{
	kSetLogLevel(kLogLevel_Info);
	kMediaUserEvents user = {.Update = Update, .Load = Load};
	kEventLoop(kDefaultSpec, user);
}
