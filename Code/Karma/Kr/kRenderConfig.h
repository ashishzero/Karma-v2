#pragma once
#include "kCommon.h"

struct kTexture;

typedef enum kDepthTest : u8
{
	kDepthTest_Disabled,
	kDepthTest_LessEquals,
	kDepthTest_Count
} kDepthTest;

typedef enum kBlendMode : u8
{
	kBlendMode_Opaque,
	kBlendMode_Normal,
	kBlendMode_Additive,
	kBlendMode_Subtractive,
	kBlendMode_Count,
} kBlendMode;

typedef enum kTextureFilter : u8
{
	kTextureFilter_LinearWrap,
	kTextureFilter_LinearClamp,
	kTextureFilter_PointWrap,
	kTextureFilter_PointClamp,
	kTextureFilter_Count
} kTextureFilter;

enum kMultiSamplingAntiAliasing
{
	kMultiSamplingAntiAliasing_Disabled = 1,
	kMultiSamplingAntiAliasing_2        = 2,
	kMultiSamplingAntiAliasing_4        = 4,
	kMultiSamplingAntiAliasing_8        = 8,
};

enum kBloom
{
	kBloom_Disabled,
	kBloom_Enabled,
};

enum kHighDynamicRange
{
	kHighDynamicRange_Disabled,
	kHighDynamicRange_AES,
};

struct kRenderPipelineConfig
{
	kMultiSamplingAntiAliasing Msaa;
	kBloom                     Bloom;
	kHighDynamicRange          Hdr;
	kVec4                      Clear;
	float                      BloomFilterRadius;
	float                      BloomStrength;
	kVec3                      Intensity;
};

