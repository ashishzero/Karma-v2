#include "kMath.h"
#include "kMedia.h"
#include "kRender.h"
#include "kStrings.h"

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

	kVec2i    size    = kGetWindowSize();
	float     ar      = kGetWindowAspectRatio();
	float     yfactor = kGetWindowDpiScale();

	kVec4     clear   = kVec4(0.01f, 0.01f, 0.01f, 1.0f);
	kTexture *rt      = kGetWindowRenderTarget();

	kString res = kFormatString(arena, "(%d, %d)\n(%f, %f)", size.x, size.y, ar, yfactor);

	kBeginRenderPass(rt, 0, kRenderPass_ClearColor, clear);

	kBeginCameraRect(0, (float)size.x, 0, (float)size.y);
	kDrawRect(kVec2(0, 100), kVec2(2000, 64.0f * yfactor), kVec4(0.1f, 0.1f, 0.1f, 1.0f));
	kDrawText(res, kVec2(50, 100), kVec4(1, 1, 1, 1), yfactor);
	kEndCamera();

	kBeginCamera(ar, -100);
	kVec2 pos1 = kVec2(ar * 50, 50) - kVec2(10);
	kVec2 pos2 = -kVec2(ar * 50, 50);
	kDrawRect(pos1, kVec2(10), kVec4(0, 1, 1, 1));
	kDrawRect(pos2, kVec2(10), kVec4(1, 1, 0, 1));
	kDrawLine(kVec2(-30), kVec2(50, 70), kVec4(1));
	kEndCamera();

	kEndRenderPass();
}

void Main(int argc, const char **argv)
{
	kMediaUserEvents user = {.update = Update};
	kEventLoop(kDefaultSpec, user);
}
