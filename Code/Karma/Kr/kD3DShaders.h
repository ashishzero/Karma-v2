#pragma once

#include "Shaders/Generated/kBlit.cs.hlsl.h"
#include "Shaders/Generated/kBlit.vs.hlsl.h"
#include "Shaders/Generated/kBlit.ps.hlsl.h"
#include "Shaders/Generated/kQuad.vs.hlsl.h"
#include "Shaders/Generated/kQuad.ps.hlsl.h"
#include "Shaders/Generated/kToneMap.ps.hlsl.h"
#include "Shaders/Generated/kThreshold.cs.hlsl.h"
#include "Shaders/Generated/kBloomDownSample.cs.hlsl.h"
#include "Shaders/Generated/kBloomDownSampleKarisAvg.cs.hlsl.h"
#include "Shaders/Generated/kBloomUpSampleMix.cs.hlsl.h"
#include "Shaders/Generated/kBloomBlurUpSample.cs.hlsl.h"

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
	kComputeShader_Blit,
	kComputeShader_Threshold,
	kComputeShader_BloomDownSample,
	kComputeShader_BloomDownSampleKarisAvg,
	kComputeShader_BloomBlurUpSample,
	kComputeShader_BloomUpSampleMix,
	kComputeShader_Count
};

static const kString VertexShadersMap[] = {kString(kQuadVS), kString(kBlitVS)};
static_assert(kArrayCount(VertexShadersMap) == kVertexShader_Count, "");

static const kString PixelShadersMap[] = {kString(kQuadPS), kString(kBlitPS), kString(kToneMapPS)};
static_assert(kArrayCount(PixelShadersMap) == kPixelShader_Count, "");

static const kString ComputeShadersMap[] = {kString(kBlitCS), kString(kThresholdCS), kString(kBloomDownSampleCS), kString(kBloomDownSampleKarisAvgCS), kString(kBloomBlurUpSampleCS), kString(kBloomUpSampleMixCS)};
static_assert(kArrayCount(ComputeShadersMap) == kComputeShader_Count, "");
