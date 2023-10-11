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

	if (kKeyPressed(kKey_M))
	{
		if (kIsRenderFeatureEnabled(kRenderFeature_MSAA))
		{
			kDisableRenderFeatures(kRenderFeature_MSAA);
		}
		else
		{
			kEnableRenderFeatures(kRenderFeature_MSAA);
		}
	}

	if (kKeyPressed(kKey_F11))
	{
		kToggleWindowFullscreen();
	}

	kVec2i size         = kGetWindowSize();
	float  aspect_ratio = kGetWindowAspectRatio();

	kBeginDefaultRenderPass(1, kVec4(0.1f, 0.1f, 0.1f, 1.0f));

	kBeginCameraRect(0, (float)size.x, 0, (float)size.y);

	kString res = kFormatString(arena, "Resolution (%d, %d) - MSAA: %u", size.x, size.y, kGetMSAASampleCount());

	float   k   = kGetWindowDpiScale();

	// kDrawRect(kVec2(50), kVec2(2000, 64), kVec4(1, 1, 1, 0.5));
	kDrawText(res, kVec2(50, 0), kVec4(1, 1, 0, 1), k);

	kEndCamera();

	kBeginCamera(aspect_ratio, 100);
	kVec2 pos1 = kVec2(aspect_ratio * 50, 50) - kVec2(10);
	kVec2 pos2 = -kVec2(aspect_ratio * 50, 50);
	kDrawRect(pos1, kVec2(10), kVec4(1, 1, 0, 1));
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
