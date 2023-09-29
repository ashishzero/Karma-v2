#pragma once
#include "kCommon.h"

//
//
//

struct kPlatformWindow;
struct kPlatformSwapChain;
struct kPlatformTexture;

using kSwapChain = kHandle<kPlatformSwapChain>;
using kTexture	 = kHandle<kPlatformTexture>;

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

typedef struct kRect
{
	kVec2 min;
	kVec2 max;
} kRect;

typedef struct kViewport
{
	float x, y;
	float w, h;
	float n, f;
} kViewport;

typedef struct kGlyph
{
	kRect rect;
	kVec2 bearing;
	kVec2 size;
	float advance;
} kGlyph;

typedef struct kFont
{
	kTexture texture;
	u16		*map;
	kGlyph	*glyphs;
	kGlyph	*fallback;
	u32		 largest;
	u32		 count;
} kFont;

typedef struct kRenderParam2D
{
	kTexture textures[K_MAX_TEXTURE_SLOTS];
	kMat4	 transform;
	kRect	 rect;
	i32		 vertex;
	u32		 index;
	u32		 count;
} kRenderParam2D;

typedef enum kRenderMode2D
{
	kRenderMode2D_Blend,
	kRenderMode2D_Depth,
	kRenderMode2D_Count,
} kRenderMode2D;

enum kBlend
{
	PL_Blend_Zero,
	PL_Blend_One,
	PL_Blend_SrcColor,
	PL_Blend_InvSrcColor,
	PL_Blend_SrcAlpha,
	PL_Blend_InvSrcAlpha,
	PL_Blend_DestAlpha,
	PL_Blend_InvDestAlpha,
	PL_Blend_DestColor,
	PL_Blend_InvDestColor,
	PL_Blend_Count
};

enum kBlendOp
{
	PL_BlendOp_Add,
	PL_BlendOp_Subtract,
	PL_BlendOp_RevSubtract,
	PL_BlendOp_Min,
	PL_BlendOp_Max,
	PL_BlendOp_Count
};

struct kBlendFunction
{
	kBlend	 src;
	kBlend	 dest;
	kBlendOp op;
};

typedef struct kBlendSpec
{
	kBlendFunction color;
	kBlendFunction alpha;
} kBlendSpec;

typedef enum kTextureFilter
{
	kTextureFilter_Linear,
	kTextureFilter_Point,
	kTextureFilter_Count
} kTextureFilter;

typedef struct kRenderCommand2D
{
	u8					   modes[kRenderMode2D_Count];
	kBlendSpec			   blend;
	kTextureFilter		   filter;
	kSlice<kRenderParam2D> params;
} kRenderCommand2D;

enum kRenderPassFlags
{
	kRenderPass_ClearColor	 = 0x1,
	kRenderPass_ClearDepth	 = 0x2,
	kRenderPass_ClearStencil = 0x4,
};

typedef struct kRenderClear2D
{
	kVec4 color;
	float depth;
	u8	  stencil;
} kRenderClear2D;

typedef struct kRenderPass2D
{
	kTexture				 render_target;
	kTexture				 depth_stencil;
	kViewport				 viewport;
	u32						 flags;
	kRenderClear2D			 clear;
	kSlice<kRenderCommand2D> commands;
} kRenderPass2D;

typedef struct kRenderData2D
{
	kSlice<kRenderPass2D> passes;
	kSlice<kVertex2D>	  vertices;
	kSlice<kIndex2D>	  indices;
} kRenderData2D;

//
//
//

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
	kBind_VertexBuffer	  = 0x1,
	kBind_IndexBuffer	  = 0x2,
	kBind_ConstantBuffer  = 0x4,
	kBind_ShaderResource  = 0x8,
	kBind_RenderTarget	  = 0x10,
	kBind_DepthStencil	  = 0x20,
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
	u32		width;
	u32		height;
	u32		pitch;
	u8	   *pixels;
	u32		bind_flags;
	kUsage	usage;
	u32		num_samples;
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
	u32		   size;
	kUsage	   usage;
	u32		   bind_flags;
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

typedef kSwapChain (*kSwapChainCreateProc)(void *);
typedef void (*kSwapChainDestroyProc)(kSwapChain);
typedef void (*kSwapChainResizeProc)(kSwapChain, uint, uint);
typedef kTexture (*kSwapChainRenderTargetProc)(kSwapChain);
typedef void (*kSwapChainPresentProc)(kSwapChain);

typedef struct kSwapChainBackend
{
	kSwapChainCreateProc	   create;
	kSwapChainDestroyProc	   destroy;
	kSwapChainResizeProc	   resize;
	kSwapChainRenderTargetProc render_target;
	kSwapChainPresentProc	   present;
} kSwapChainBackend;

typedef kSwapChain (*kRenderBackendGetWindowSwapChainProc)(void);
typedef kTexture (*kRenderBackendSwapChainTargetProc)(kSwapChain);
typedef kTexture (*kRenderBackendCreateTextureProc)(const kTextureSpec &);
typedef void (*kRenderBackendDestroyTextureProc)(kTexture);
typedef void (*kRenderBackendTextureSizeProc)(kTexture, u32 *, u32 *);
typedef void (*kRenderBackendResizeTextureProc)(kTexture, u32, u32);
typedef void (*kRenderBackendDestroyProc)(void);
typedef void (*kRenderBackendExecuteCommandsProc)(const kRenderData2D &);

typedef struct kRenderBackend
{
	kRenderBackendGetWindowSwapChainProc window_swap_chain;
	kRenderBackendSwapChainTargetProc	 swap_chain_target;
	kRenderBackendCreateTextureProc		 create_texture;
	kRenderBackendDestroyTextureProc	 destroy_texture;
	kRenderBackendTextureSizeProc		 texture_size;
	kRenderBackendResizeTextureProc		 resize_texture;
	kRenderBackendExecuteCommandsProc	 execute_commands;
	kRenderBackendDestroyProc			 destroy;
} kRenderBackend;
