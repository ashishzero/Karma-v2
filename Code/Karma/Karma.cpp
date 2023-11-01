﻿#include "kMath.h"
#include "kMedia.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"

static float         stabilized_dt = 0;

static kArray<kVec2> Rects;
static kArray<kVec2> Dissappear;
static kArray<u8>    Chars;
static kArray<u8>    DissappearChars;
static kArray<float> Angles;
static kArray<float> DissappearAngles;
static kArray<float> Rotations;
static kArray<float> DissappearRotations;
static kArray<kVec4> Colors;
static kArray<kVec4> DissappearColors;
static kArray<float> TFactors;
static kArray<float> DissappearTFactors;

void                 Update(float dt)
{
	kArena *arena = kGetFrameArena();

	if (kKeyPressed(kKey_Escape))
	{
		kBreakLoop(0);
	}

	if (kIsKeyDown(kKey_Plus) || kKeyPressed(kKey_Up))
	{
		float x = kRandomFloatBound(100.0f) - 50.0f;
		float y = kRandomFloatBound(100.0f) - 50.0f;
		Rects.Add(kVec2(x, y));

		float r = kRandomFloat01();
		float g = kRandomFloat01();
		float b = kRandomFloat01();
		float a = 0.0f;

		Colors.Add(kVec4(r, b, g, a));
		TFactors.Add(0.0f);

		u8 ch = (u8)kRandomRange(32, 127);
		Chars.Add(ch);

		float theta = kRandomFloat01();
		Angles.Add(theta);

		float rotation = kRandomFloat01();
		Rotations.Add(rotation);
	}

	if (kIsKeyDown(kKey_Minus) || kKeyPressed(kKey_Down))
	{
		if (Rects.Count)
		{
			Dissappear.Add(Rects.Last());
			DissappearColors.Add(Colors.Last());
			DissappearTFactors.Add(TFactors.Last());
			DissappearChars.Add(Chars.Last());
			DissappearAngles.Add(Angles.Last());
			DissappearRotations.Add(Rotations.Last());
			Rects.Pop();
			Colors.Pop();
			TFactors.Pop();
			Chars.Pop();
			Angles.Pop();
			Rotations.Pop();
		}
	}

	if (kKeyPressed(kKey_F11))
	{
		kToggleWindowFullscreen();
	}

	kVec2i size    = kGetWindowSize();
	float  ar      = kGetWindowAspectRatio();
	float  yfactor = kGetWindowDpiScale();

	kVec4  clear   = kVec4(0.01f, 0.01f, 0.01f, 1.0f);
	// kTexture *rt      = kGetWindowRenderTarget();

	// kString   tm      = kFormatString(arena, "ToneMapping : %s", kToneMappingMethodStrings[rt_config.tonemapping]);
	// kString   aa      = kFormatString(arena, "AntiAliasing: %s", kAntiAliasingMethodStrings[rt_config.antialiasing]);

	stabilized_dt = kLerp(dt, stabilized_dt, 0.9f);

	kRenderMemory used_mem, alloc_mem;
	kGetRenderMemoryUsage(&used_mem);

	kGetRenderMemoryCaps(&alloc_mem);

	int     fps  = (int)(1.0f / stabilized_dt);
	kString pf   = kFormatString(arena, "FPS: %d (%.2fms)\nRender Memory: %.3f / %.3f MB", fps, 1000 * stabilized_dt,
	                             used_mem.MB, alloc_mem.MB);

	kRect   rect = kRect(0.0f, 0.0f, (float)size.x, (float)size.y);

	while (DissappearTFactors.Count && DissappearTFactors.Last() < 0.001f)
	{
		Dissappear.Pop();
		DissappearColors.Pop();
		DissappearTFactors.Pop();
		DissappearChars.Pop();
		DissappearAngles.Pop();
		DissappearRotations.Pop();
	}

	static float time = 0.0f;

	time += dt;

	kBeginScene(ar, 100, rect);

	float chfactor = kSin(time);

	float dim      = 15.0f;
	float tdim     = 0.5f;
	for (imem i = 0; i < Rects.Count; ++i)
	{
		float t     = kEaseInOutBounce(TFactors[i]);
		Colors[i].w = kEaseInCirc(TFactors[i]);
		kMat3 rot   = kRotation3x3(kLerp(0.0f, Angles[i], t));
		kPushTransform(rot);
		float rf = Rotations[i];
		kDrawTextQuadCenteredRotated(Chars[i], kLerp(kVec2(0), Rects[i], t), rf * chfactor, Colors[i], tdim);
		kPopTransform();
		TFactors[i] = kLerp(TFactors[i], 1.0f, 0.09f);
	}

	for (imem i = 0; i < Dissappear.Count; ++i)
	{
		float t               = kEaseOutExpo(DissappearTFactors[i]);
		DissappearColors[i].w = kEaseOutCirc(DissappearTFactors[i]);
		kMat3 rot             = kRotation3x3(kLerp(0.0f, DissappearAngles[i], t));
		kPushTransform(rot);
		float rf = DissappearRotations[i];
		kDrawTextQuadCenteredRotated(DissappearChars[i], kLerp(kVec2(0), Dissappear[i], t), rf * chfactor,
		                             DissappearColors[i], tdim);
		kPopTransform();
		DissappearTFactors[i] = kLerp(DissappearTFactors[i], 0.0f, 0.09f);
	}

	kLineThickness(2.0f);

	kDrawRectCentered(kVec2(-30, 0), kVec2(10), kVec4(1, 1, 1, 1));
	kDrawRectCentered(kVec2(0, 0), kVec2(10), kVec4(1, 1, 1, 1));
	kDrawRectCentered(kVec2(30, 0), kVec2(10), kVec4(1, 1, 1, 1));
	kDrawRectCentered(kVec2(0, -30), kVec2(10), kVec4(1, 1, 1, 1));
	kDrawRectCentered(kVec2(0, 30), kVec2(10), kVec4(1, 1, 1, 1));

	kDrawLine(kVec2(0), kVec2(20, 20), kVec4(50, 50, 0, 1));

	kEndScene();

	kBeginScene(0, (float)size.x, 0, (float)size.y, -1.0f, 1.0f, rect);

	kPushOutLineStyle(kVec3(1), 0.5f * kMap01(-1.0f, 1.0f, chfactor));

	kVec2 dd = kCalculateText(pf, 0.75f * yfactor);
	kDrawRect(kVec2(0), dd, kVec4(0, 0, 0, 0.75f));
	kDrawText(pf, kVec2(0), kVec4(1, 0, 0, 1), 0.75f * yfactor);

	kDrawRect(kVec2(0), kVec2(10), kVec4(1, 1, 0, 1));

	kPopOutLineStyle();

	kEndScene();
}

void Main(int argc, const char **argv)
{
	kSetLogLevel(kLogLevel_Info);
	kMediaUserEvents user = {.Update = Update};
	kMediaSpec       spec = kDefaultSpec;

	// spec.Window.Width = 1024;
	// spec.Window.Height = 1024;

	// spec.RenderPipeline.BloomFilterRadius = 0;
	spec.RenderPipeline.Intensity = kVec3(1);
	// spec.RenderPipeline.Clear = kVec4(1, 0, 1, 1);
	kEventLoop(spec, user);
}
