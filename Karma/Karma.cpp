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

	kVec2i    size     = kGetWindowSize();
	float     ar       = kGetWindowAspectRatio();
	float     yfactor  = kGetWindowDpiScale();
	kViewport viewport = {0, 0, size.x, size.y};

	kVec2i    pos      = kGetCursorPosition();
	float     sx       = ar * 50.0f * ((float)pos.x / size.x * 2.0f - 1.0f);
	float     sy       = 50.0f * ((float)(pos.y) / size.y * 2.0f - 1.0f);

	{
		kMat4        tranform = kIdentity();
		kCameraView  view     = kPerspectiveView(0.16f, ar, 0.1f, 1000.0f, tranform);

		kRotor3      rotor    = kAngleAxisToRotor3(kNormalize(kVec3(0, 1, 1)), 0.5);

		static float angle    = 0.05f;

		kMat4        modal    = kTranslation(0, 0, 10) * kRotationX(0.15f) * kRotationY(angle);

		kRender3D::kBeginScene(view, viewport);
		kRender3D::kDrawCube(modal, kVec4(5, 3, 0, 1));
		kRender3D::kEndScene();

		angle += dt * 0.1f;
	}

	kCameraView view = kOrthographicView(ar, 100.0f);

	kRender2D::kLineThickness(0.1f);

	kRender2D::kBeginScene(view, viewport);
	kRender2D::kDrawCircle(kVec2(sx, sy), 15, kVec4(2, 20, 20, 1));

	kRender2D::kLineThickness(2);

	kRender2D::kDrawBezierCubic(kVec2(0), kVec2(20, -50), kVec2(50, -10), kVec2(80, -40), kVec4(3, 20, 3, 1));

	kRender2D::kEndScene();

	kCameraView  ui    = KOrthographicView(0, (float)size.x, 0, (float)size.y);

	static kVec4 color = kVec4(1);

	kRender2D::kBeginScene(ui, viewport);
	kRender2D::kDrawText("HDR Bloom", kVec2(50, 100), color, 8);
	kRender2D::kEndScene();

	kString FPS = kFormatString(arena, "FPS: %d (%.2fms)", (int)(1.0f / dt), 1000.0f * dt);

	kRender2D::kBeginRenderPass(kRenderPass::HUD);
	kRender2D::kBeginScene(ui, viewport);
	kRender2D::kPushOutLineStyle(kVec3(0.1f), 0.4f);
	kRender2D::kDrawText(FPS, kVec2(4), kVec4(1, 1, 1, 1), 0.5f * yfactor);
	kRender2D::kPopOutLineStyle();
	kRender2D::kEndScene();
	kRender2D::kEndRenderPass();

	ImGui::Begin("Config");

	ImGui::DragFloat("R", &color.x, 0.1f, 0.0f, 5000.0f);
	ImGui::DragFloat("G", &color.y, 0.1f, 0.0f, 5000.0f);
	ImGui::DragFloat("B", &color.z, 0.1f, 0.0f, 5000.0f);

	ImGui::End();
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
	auto spec = kDefaultSpec;
	//spec.RenderPipeline.Hdr = kHighDynamicRange::Disabled;
	//spec.RenderPipeline.Bloom = kBloom::Disabled;

	kSetLogLevel(kLogLevel::Trace);
	kMediaUserEvents user = {.Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};
	kEventLoop(spec, user);
}
