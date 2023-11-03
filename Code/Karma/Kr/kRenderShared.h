#pragma once
#include "kCommon.h"

typedef enum kResourceState
{
	kResourceState_Error = -1,
	kResourceState_Unready,
	kResourceState_Ready,
} kResourceState;

struct kResourceID
{
	u16 Index;
	u16 GenID;
};

struct kTexture
{
	kResourceID ID;

	kTexture()
	{
		ID.Index = 0;
		ID.GenID = 0;
	}

	kTexture(kResourceID id) : ID(id)
	{}
};

bool operator==(kTexture a, kTexture b);
bool operator!=(kTexture a, kTexture b);

typedef enum kFormat
{
	kFormat_RGBA32_FLOAT,
	kFormat_RGBA32_SINT,
	kFormat_RGBA32_UINT,
	kFormat_RGBA16_FLOAT,
	kFormat_RGBA8_UNORM,
	kFormat_RGBA8_UNORM_SRGB,
	kFormat_RGB32_FLOAT,
	kFormat_R11G11B10_FLOAT,
	kFormat_RGB32_SINT,
	kFormat_RGB32_UINT,
	kFormat_RG32_FLOAT,
	kFormat_RG32_SINT,
	kFormat_RG32_UINT,
	kFormat_RG8_UNORM,
	kFormat_R32_FLOAT,
	kFormat_R32_SINT,
	kFormat_R32_UINT,
	kFormat_R16_UINT,
	kFormat_R8_UNORM,
	kFormat_D32_FLOAT,
	kFormat_D16_UNORM,
	kFormat_D24_UNORM_S8_UINT,
	kFormat_Count
} kFormat;

typedef struct kTextureSpec
{
	kFormat Format;
	u32     Width;
	u32     Height;
	u32     Pitch;
	u8 *    Pixels;
	kString Name;
} kTextureSpec;



//
//
//

typedef enum kDepthTest : u8
{
	kDepthTest_Disabled,
	kDepthTest_LessEquals,
	kDepthTest_Count
} kDepthTest;

static const kString kDepthTestStrings[] = {"Disabled", "Enabled"};
static_assert(kArrayCount(kDepthTestStrings) == kDepthTest_Count, "");

typedef enum kBlendMode : u8
{
	kBlendMode_Opaque,
	kBlendMode_Normal,
	kBlendMode_Additive,
	kBlendMode_Subtractive,
	kBlendMode_Count,
} kBlendMode;

static const kString kBlendModeStrings[] = {"Opaque", "Normal", "Additive", "Subtractive"};
static_assert(kArrayCount(kBlendModeStrings) == kBlendMode_Count, "");

typedef enum kTextureFilter : u8
{
	kTextureFilter_LinearWrap,
	kTextureFilter_LinearClamp,
	kTextureFilter_PointWrap,
	kTextureFilter_PointClamp,
	kTextureFilter_Count
} kTextureFilter;

static const kString kTextureFilterStrings[] = {"LinearWrap", "LinearClamp", "PointWrap", "PointClamp"};
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
