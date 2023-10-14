#include "kMath.h"
#include "kMedia.h"
#include "kRender.h"
#include "kStrings.h"

void Update(float dt)
{
	kArena *            arena     = kGetFrameArena();

	kRenderTargetConfig rt_config = kGetRenderTargetConfig();

	if (kKeyPressed(kKey_Escape))
	{
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey_F11))
	{
		kToggleWindowFullscreen();
	}

	if (kKeyPressed(kKey_H))
	{
		rt_config.tonemapping = (kToneMappingMethod)((rt_config.tonemapping + 1) % kToneMappingMethod_Count);
		kApplyRenderTargetConfig(rt_config);
	}

	if (kKeyPressed(kKey_A))
	{
		rt_config.antialiasing = (kAntiAliasingMethod)((rt_config.antialiasing + 1) % kAntiAliasingMethod_Count);
		kApplyRenderTargetConfig(rt_config);
	}

	kVec2i    size    = kGetWindowSize();
	float     ar      = kGetWindowAspectRatio();
	float     yfactor = kGetWindowDpiScale();

	kVec4     clear   = kVec4(0.01f, 0.01f, 0.01f, 1.0f);
	kTexture *rt      = kGetWindowRenderTarget();

	kString   tm      = kFormatString(arena, "ToneMapping : %s", kToneMappingMethodStrings[rt_config.tonemapping]);
	kString   aa      = kFormatString(arena, "AntiAliasing: %s", kAntiAliasingMethodStrings[rt_config.antialiasing]);

	kBeginRenderPass(rt, 0, kRenderPass_ClearColor, clear);

	kBeginCameraRect(0, (float)size.x, 0, (float)size.y);
	kDrawText(aa, kVec2(50, 100), kVec4(1, 1, 1, 1), yfactor);
	kDrawText(tm, kVec2(50, 150), kVec4(1, 1, 1, 1), yfactor);
	kEndCamera();

	kBeginCamera(ar, 100);
	kVec2 pos1 = kVec2(ar * 50, 50) - kVec2(10);
	kVec2 pos2 = -kVec2(ar * 50, 50);
	//kDrawRect(pos1, kVec2(10), kVec4(0, 1, 1, 1));
	//kDrawRect(pos2, kVec2(10), kVec4(1, 1, 0, 1));
	//kDrawRect(kVec2(0), kVec2(10), kVec4(50, 50, 0, 1));
	kDrawText("mjk", kVec2(0), kVec4(1, 1, 0, 1), 1.0f);
	// kDrawLine(kVec2(-30), kVec2(50, 70), kVec4(1));
	kEndCamera();

	kEndRenderPass();
}

void Main(int argc, const char **argv)
{
	kMediaUserEvents user = {.update = Update};
	kEventLoop(kDefaultSpec, user);
}
