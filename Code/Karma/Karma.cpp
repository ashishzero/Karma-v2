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

	kBeginCamera(aspect_ratio, 600);

	kDrawRectCentered(kVec2(0), kVec2(200.0f), kVec4(1, 1, 0, 1));

	kEndCamera();

	kEndRenderPass();
}

void Main(int argc, const char **argv)
{
	kMediaUserEvents user = {.update = Update};
	kEventLoop(kDefaultSpec, user);
}
