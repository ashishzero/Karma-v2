#include "kMedia.h"
#include "kRender.h"

void Update(float dt)
{
	if (kKeyPressed(kKey_Escape))
	{
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey_F11))
	{
		kToggleWindowFullscreen();
	}

	u32 w, h;
	kGetWindowSize(&w, &h);

	float aspect_ratio = (float)w / (float)h;

	kBeginDefaultRenderPass();

	kBeginCameraRect(0, (float)w, 0, (float)h);

	kString text = "Karma";

	//float r = 0.5f * kCalculateText(text, 50);
	
	kDrawText("Karma", kVec2(200), kVec4(1, 1, 0, 1), 256);

	kEndCamera();

	kEndRenderPass();
}

void Main(int argc, const char **argv)
{
	kMediaUserEvents user = {.update = Update};
	kEventLoop(kDefaultSpec, user);
}
