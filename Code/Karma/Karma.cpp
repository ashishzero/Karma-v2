#include "kMath.h"
#include "kMedia.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"

void            Update(float dt)
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
	kCameraView view     = KOrthographicView(0, (float)size.x, 0, (float)size.y);
	kViewport   viewport = {0, 0, size.x, size.y};

	kBeginRenderPass(kRenderPass_HUD);
	kBeginScene(view, viewport);
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
	kMediaUserEvents user   = {.Update = Update};
	kEventLoop(kDefaultSpec, user);
}
