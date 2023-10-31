#pragma once
#include "kCommon.h"

struct kTexture;

typedef enum kDepthTest : u8
{
	kDepthTest_Disabled,
	kDepthTest_LessEquals,
	kDepthTest_Count
} kDepthTest;

static const kString kDepthTestStrings[] = {"Depth_Disabled", "Depth_Enabled"};
static_assert(kArrayCount(kDepthTestStrings) == kDepthTest_Count, "");

typedef enum kBlendMode : u8
{
	kBlendMode_Opaque,
	kBlendMode_Normal,
	kBlendMode_Additive,
	kBlendMode_Subtractive,
	kBlendMode_Count,
} kBlendMode;

static const kString kBlendModeStrings[] = {"BlendMode_Opaque", "BlendMode_Normal", "BlendMode_Additive",
                                            "BlendMode_Subtractive"};
static_assert(kArrayCount(kBlendModeStrings) == kBlendMode_Count, "");

typedef enum kTextureFilter : u8
{
	kTextureFilter_LinearWrap,
	kTextureFilter_LinearClamp,
	kTextureFilter_PointWrap,
	kTextureFilter_PointClamp,
	kTextureFilter_Count
} kTextureFilter;

static const kString kTextureFilterStrings[] = {"TextureFilter_LinearWrap", "TextureFilter_LinearClamp",
                                                "TextureFilter_PointWrap", "TextureFilter_PointClamp"};
static_assert(kArrayCount(kTextureFilterStrings) == kTextureFilter_Count, "");

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
