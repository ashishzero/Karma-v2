﻿#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kImGui.h"

static struct Camera3D
{
	kVec3   Position;
	kRotor3 Orientation;
} Camera;

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

	float         sensitivity = 0.1f;

	static kVec2i cursor      = kVec2i(0);

	if (kButtonPressed(kButton::Right))
	{
		kCaptureCursor();
		cursor = kGetCursorPosition();
	}

	if (kButtonReleased(kButton::Right))
	{
		kReleaseCursor();
	}

	if (kIsButtonDown(kButton::Right))
	{
		kVec2i  delta = cursor - kGetCursorPosition();
		float   angle = delta.x * sensitivity * dt;
		kRotor3 turn  = kAngleAxisToRotor3(kVec3(0, -1, 0), angle);

		kLogTrace("Angle: %f", angle);

		Camera.Orientation *= turn;
		cursor = kGetCursorPosition();
	}

	float forward   = (float)kIsKeyDown(kKey::W) - (float)kIsKeyDown(kKey::S);
	float right     = (float)kIsKeyDown(kKey::D) - (float)kIsKeyDown(kKey::A);

	kVec3 direction = forward * kForwardDirection(Camera.Orientation) + right * kRightDirection(Camera.Orientation);

	float speed     = 20.0f;
	Camera.Position += dt * speed * kNormalizeZ(direction);

	kVec2i    size     = kGetWindowSize();
	float     ar       = kGetWindowAspectRatio();
	float     yfactor  = kGetWindowDpiScale();
	kViewport viewport = {0, 0, size.x, size.y};

	kVec2i    pos      = kGetCursorPosition();
	float     sx       = ar * 50.0f * ((float)pos.x / size.x * 2.0f - 1.0f);
	float     sy       = 50.0f * ((float)(pos.y) / size.y * 2.0f - 1.0f);

	static float fov      = 0.08f;

	{
		kMat4        tranform = kTranslation(Camera.Position) * kRotor3ToMat4(Camera.Orientation);
		kCameraView  view     = kPerspectiveView(fov, ar, 0.1f, 1000.0f, tranform);

		kRotor3      rotor    = kAngleAxisToRotor3(kNormalize(kVec3(0, 1, 1)), 0.5);

		static float angle    = 0.05f;

		static float time     = 0.0f;


		float        rot       = kSin(time);

		kMat4        modal1    = kTranslation(0, 0, 10) * kRotationX(rot) * kRotationY(angle);
		kMat4        modal2    = kTranslation(0, 0, 20) * kRotationX(0.15f) * kRotationY(angle);
		kMat4        modal3    = kTranslation(0, 0, 30) * kRotationX(0.15f) * kRotationY(angle);


		float        red      = kAbsolute(kSin(time)) * 5.0f;
		time += dt * 0.1f;

		kRender3D::kBeginScene(view, viewport);
		kRender3D::kDrawCube(modal1, kVec4(red, 3, 0, 1));
		kRender3D::kDrawCube(modal2, kVec4(3, red, 0, 1));
		kRender3D::kDrawCube(modal3, kVec4(0, 3, red, 1));
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

	ImGui::DragFloat("FOV", &fov, 0.001f, 0.03f, 0.4f);

	// ImGui::DragFloat("R", &color.x, 0.1f, 0.0f, 5000.0f);
	// ImGui::DragFloat("G", &color.y, 0.1f, 0.0f, 5000.0f);
	// ImGui::DragFloat("B", &color.z, 0.1f, 0.0f, 5000.0f);

	ImGui::End();
}

static float           g_Now;
static float           g_Accumulator;
static constexpr float FixedDeltaTime = 1.0f / 60.0f;

void                   OnLoadEvent(void *data)
{
	Camera.Position    = kVec3(0, 0, 0);
	Camera.Orientation = kRotor3();
}

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
	// spec.RenderPipeline.Hdr = kHighDynamicRange::Disabled;
	// spec.RenderPipeline.Bloom = kBloom::Disabled;

	kSetLogLevel(kLogLevel::Trace);
	kMediaUserEvents user = {.Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};
	kEventLoop(spec, user);
}
