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
	kAntiAliasingMethod_MSAAx16,
	kAntiAliasingMethod_Count,
} kAntiAliasingMethod;

typedef struct kRenderTargetSpec
{
	kAntiAliasingMethod antialiasing;
} kRenderTargetSpec;

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

enum kBindFlags
{
	kBind_VertexBuffer    = 0x1,
	kBind_IndexBuffer     = 0x2,
	kBind_ConstantBuffer  = 0x4,
	kBind_ShaderResource  = 0x8,
	kBind_RenderTarget    = 0x10,
	kBind_DepthStencil    = 0x20,
	kBind_UnorderedAccess = 0x40,
};

typedef enum kUsage
{
	kUsage_Default,
	kUsage_Immutable,
	kUsage_Dynamic,
	kUsage_Staging,
	kUsage_Count
} kUsage;

typedef struct kTextureSpec
{
	kFormat format;
	u32     width;
	u32     height;
	u32     pitch;
	u8     *pixels;
	u32     bind_flags;
	kUsage  usage;
	u32     num_samples;
} kTextureSpec;

typedef enum kCpuAccess
{
	kCpuAccess_None,
	kCpuAccess_Read,
	kCpuAccess_Write,
	kCpuAccess_ReadWrite
} kCpuAccess;

typedef struct kBufferSpec
{
	u32        size;
	kUsage     usage;
	u32        bind_flags;
	kCpuAccess cpu_access;
} kBufferSpec;

typedef enum kMap
{
	kMap_Read,
	kMap_Write,
	kMap_ReadWrite,
	kMap_WriteDiscard,
	kMap_Count
} kMap;

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
