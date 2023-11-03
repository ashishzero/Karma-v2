#pragma once

#include "Shaders/Generated/kBlit.cs.hlsl.h"
#include "Shaders/Generated/kBlit.vs.hlsl.h"
#include "Shaders/Generated/kMix.ps.hlsl.h"
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

static const kString kVertexShaderStrings[] = {"Quad", "Blit"};
static_assert(kArrayCount(kVertexShaderStrings) == kVertexShader_Count, "");

enum kPixelShaderKind
{
	kPixelShader_Quad,
	kPixelShader_Mix,
	kPixelShader_Tonemap,
	kPixelShader_Count
};

static const kString kPixelShaderStrings[] = {"Quad", "Mix", "Tonemap"};
static_assert(kArrayCount(kPixelShaderStrings) == kPixelShader_Count, "");

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

static const kString kComputeShaderStrings[] = {
	"Blit", "Threshold", "BloomDownSample", "BloomDownSampleKarisAvg", "BlurUpSample", "UpSampleMix"};
static_assert(kArrayCount(kComputeShaderStrings) == kComputeShader_Count, "");

static const kString kVertexShadersMap[] = {kString(kQuadVS), kString(kBlitVS)};
static_assert(kArrayCount(kVertexShadersMap) == kVertexShader_Count, "");

static const kString kPixelShadersMap[] = {kString(kQuadPS), kString(kMixPS), kString(kToneMapPS)};
static_assert(kArrayCount(kPixelShadersMap) == kPixelShader_Count, "");

static const kString kComputeShadersMap[] = {kString(kBlitCS),
                                             kString(kThresholdCS),
                                             kString(kBloomDownSampleCS),
                                             kString(kBloomDownSampleKarisAvgCS),
                                             kString(kBloomBlurUpSampleCS),
                                             kString(kBloomUpSampleMixCS)};
static_assert(kArrayCount(kComputeShadersMap) == kComputeShader_Count, "");
