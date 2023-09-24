#pragma once
#include "kCommon.h"

typedef struct kWindow    { void *ptr; } kWindow;
typedef struct kSwapChain { void *ptr; } kSwapChain;
typedef struct kShader    { void *ptr; } kShader;
typedef struct kTexture   { void *ptr; } kTexture;
typedef struct kBuffer    { void *ptr; } kBuffer;

typedef enum kFormat {
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

enum kBindFlags {
	kBind_VertexBuffer    = 0x1,
	kBind_IndexBuffer     = 0x2,
	kBind_ConstantBuffer  = 0x4,
	kBind_ShaderResource  = 0x8,
	kBind_RenderTarget    = 0x10,
	kBind_DepthStencil    = 0x20,
	kBind_UnorderedAccess = 0x40,
};

typedef enum kUsage {
	kUsage_Default,
	kUsage_Immutable,
	kUsage_Dynamic,
	kUsage_Staging,
	kUsage_Count
} kUsage;

typedef struct kTextureSpec {
	kFormat   format;
	u32       width;
	u32       height;
	u32       pitch;
	u8 *      pixels;
	u32       bind_flags;
	kUsage    usage;
	u32       num_samples;
} kTextureSpec;

typedef enum kCpuAccess {
	kCpuAccess_None,
	kCpuAccess_Read,
	kCpuAccess_Write,
	kCpuAccess_ReadWrite
} kCpuAccess;

typedef struct kBufferSpec {
	u32          size;
	kUsage       usage;
	u32          bind_flags;
	kCpuAccess   cpu_access;
} kBufferSpec;

typedef enum kMap {
	kMap_Read,
	kMap_Write,
	kMap_ReadWrite,
	kMap_WriteDiscard,
	kMap_Count
} kMap;

struct kRenderBackend {
	kSwapChain  (*CreateSwapChain)(kWindow *);
	void        (*DestroySwapChain)(kSwapChain );
	void        (*ResizeSwapChain)(kSwapChain , uint, uint);
	kTexture    (*GetSwapChainRenderTarget)(kSwapChain );
	void        (*Present)(kSwapChain );

	kTexture    (*CreateTexture)(const kTextureSpec *);
	void        (*DestroyTexture)(kTexture *);
	void        (*GetTextureSize)(kTexture *, u32 *, u32 *);

	kBuffer     (*CreateBuffer)(const kBufferSpec *, void *);
	void        (*DestroyBuffer)(kBuffer );
	u32         (*GetBufferSize)(kBuffer );
	void *      (*MapBuffer)(kBuffer , kMap );
	void        (*UnmapBuffer)(kBuffer );
};
