#pragma once

#include "Shaders/Generated/kBlit.vs.hlsl.h"
#include "Shaders/Generated/kBlit.ps.hlsl.h"
#include "Shaders/Generated/kQuad.vs.hlsl.h"
#include "Shaders/Generated/kQuad.ps.hlsl.h"
#include "Shaders/Generated/kToneMap.ps.hlsl.h"
#include "Shaders/Generated/kThreshold.cs.hlsl.h"
#include "Shaders/Generated/kBloomDownSample.cs.hlsl.h"

enum kVertexShaderKind
{
	kVertexShader_Quad,
	kVertexShader_Blit,
	kVertexShader_Count
};

enum kPixelShaderKind
{
	kPixelShader_Quad,
	kPixelShader_Blit,
	kPixelShader_Tonemap,
	kPixelShader_Count
};

enum kComputeShaderKind
{
	kComputeShader_Threshold,
	kComputeShader_Bloom,
	kComputeShader_Count
};

static const kString VertexShadersMap[] = {kString(kQuadVS), kString(kBlitVS)};
static_assert(kArrayCount(VertexShadersMap) == kVertexShader_Count, "");

static const kString PixelShadersMap[] = {kString(kQuadPS), kString(kBlitPS), kString(kToneMapPS)};
static_assert(kArrayCount(PixelShadersMap) == kPixelShader_Count, "");

static const kString ComputeShadersMap[] = {kString(kThresholdCS), kString(kBloomDownSampleCS)};
static_assert(kArrayCount(ComputeShadersMap) == kComputeShader_Count, "");
