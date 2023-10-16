#pragma once
#include "kCommon.h"

typedef enum kResourceState
{
	kResourceState_Error = -1,
	kResourceState_Unready,
	kResourceState_Ready,
} kResourceState;

typedef struct kSwapChain
{
	kResourceState state;
} kSwapChain;

typedef struct kTexture
{
	kResourceState state;
} kTexture;

typedef enum kAntiAliasingMethod
{
	kAntiAliasingMethod_None,
	kAntiAliasingMethod_MSAAx2,
	kAntiAliasingMethod_MSAAx4,
	kAntiAliasingMethod_MSAAx8,
	kAntiAliasingMethod_Count
} kAntiAliasingMethod;

static const char *kAntiAliasingMethodStrings[] = {"None", "MSAAx2", "MSAAx4", "MSAAx8"};
static_assert(kArrayCount(kAntiAliasingMethodStrings) == kAntiAliasingMethod_Count, "");

typedef enum kToneMappingMethod
{
	kToneMappingMethod_SDR,
	kToneMappingMethod_HDR_AES,
	kToneMappingMethod_Count
} kToneMappingMethod;

static const char *kToneMappingMethodStrings[] = {"Standard Dynamic Range", "High Dynamic Range AES"};
static_assert(kArrayCount(kToneMappingMethodStrings) == kToneMappingMethod_Count, "");

typedef struct kRenderTargetConfig
{
	kAntiAliasingMethod antialiasing;
	kToneMappingMethod  tonemapping;
} kRenderTargetConfig;

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

enum kResourceFlags
{
	kResource_DenyShaderResource   = 0x1,
	kResource_AllowRenderTarget    = 0x2,
	kResource_AllowDepthStencil    = 0x4,
	kResource_AllowUnorderedAccess = 0x8,
};

typedef struct kTextureSpec
{
	kFormat format;
	u32     width;
	u32     height;
	u32     pitch;
	u8     *pixels;
	u32     flags;
	u32     num_samples;
} kTextureSpec;

//
//
//

#define K_MAX_TEXTURE_SLOTS 2

typedef struct kVertex2D
{
	kVec3 pos;
	kVec2 tex;
	kVec4 col;
} kVertex2D;

typedef u32 kIndex2D;

typedef struct kViewport
{
	float x, y;
	float w, h;
	float n, f;
} kViewport;

typedef struct kRenderParam2D
{
	kTexture *textures[K_MAX_TEXTURE_SLOTS];
	kMat4     transform;
	kRect     rect;
	i32       vertex;
	u32       index;
	u32       count;
} kRenderParam2D;

typedef enum kBlendMode
{
	kBlendMode_None,
	kBlendMode_Alpha,
	kBlendMode_Count,
} kBlendMode;

typedef struct kRenderShader2D
{
	kBlendMode blend;
} kRenderShader2D;

typedef enum kTextureFilter
{
	kTextureFilter_Linear,
	kTextureFilter_Point,
	kTextureFilter_Count
} kTextureFilter;

typedef struct kRenderCommand2D
{
	kTextureFilter        filter;
	kRenderShader2D       shader;
	kSpan<kRenderParam2D> params;
} kRenderCommand2D;

enum kRenderPassFlags
{
	kRenderPass_ClearColor   = 0x1,
	kRenderPass_ClearDepth   = 0x2,
	kRenderPass_ClearStencil = 0x4,
};

typedef struct kRenderClear2D
{
	kVec4 color;
	float depth;
	u8    stencil;
} kRenderClear2D;

typedef struct kRenderCommandDecoded
{
	u16 textures[2];
	u16 transform;
	u8  rect;
	u8  filter;
	u8  blend;
	u8  padding;
} kRenderCommandDecoded;

typedef struct kRenderCommand2D_Version2
{
	u64 key;
	i32 vertex;
	u32 index;
	u32 count;
} kRenderCommand2D_Version2;
