#pragma once
#include "kCommon.h"
#include "kRenderShared.h"

typedef enum kResourceState
{
	kResourceState_Error = -1,
	kResourceState_Unready,
	kResourceState_Ready,
} kResourceState;

typedef struct kTexture
{
	kResourceState state;
} kTexture;

// typedef enum kAntiAliasingMethod
//{
//	kAntiAliasingMethod_None,
//	kAntiAliasingMethod_MSAAx2,
//	kAntiAliasingMethod_MSAAx4,
//	kAntiAliasingMethod_MSAAx8,
//	kAntiAliasingMethod_Count
// } kAntiAliasingMethod;
//
// static const char *kAntiAliasingMethodStrings[] = {"None", "MSAAx2", "MSAAx4", "MSAAx8"};
// static_assert(kArrayCount(kAntiAliasingMethodStrings) == kAntiAliasingMethod_Count, "");
//
// typedef enum kToneMappingMethod
//{
//	kToneMappingMethod_SDR,
//	kToneMappingMethod_HDR_AES,
//	kToneMappingMethod_Count
// } kToneMappingMethod;
//
// static const char *kToneMappingMethodStrings[] = {"Standard Dynamic Range", "High Dynamic Range AES"};
// static_assert(kArrayCount(kToneMappingMethodStrings) == kToneMappingMethod_Count, "");

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

extern bool           kEnableDebugLayer;
static const kTexture kFallbackTexture;

typedef void (*kSwapChainCreateProc)(void *);
typedef void (*kSwapChainDestroyProc)(void);
typedef void (*kSwapChainResizeProc)(uint, uint);
typedef void (*kSwapChainPresentProc)(void);

typedef kTexture *(*kRenderBackendTextureCreateProc)(const kTextureSpec &);
typedef void (*kkRenderBackendTextureDestroyProc)(kTexture *);
typedef kVec2i (*kRenderBackendTextureSizeProc)(kTexture *);
typedef void (*kkRenderBackendTextureResizeProc)(kTexture *, u32, u32);

typedef void (*kRenderBackendExecuteFrameProc)(const kRenderFrame2D &);
typedef void (*kRenderBackendNextFrame)(void);
typedef void (*kRenderBackendFlush)(void);

typedef void (*kRenderBackendDestroyProc)(void);

typedef struct kRenderBackend
{
	kSwapChainCreateProc              CreateSwapChain;
	kSwapChainDestroyProc             DestroySwapChain;
	kSwapChainResizeProc              ResizeSwapChain;
	kSwapChainPresentProc             Present;

	kRenderBackendTextureCreateProc   CreateTexture;
	kkRenderBackendTextureDestroyProc DestroyTexture;
	kRenderBackendTextureSizeProc     GetTextureSize;
	kkRenderBackendTextureResizeProc  ResizeTexture;

	kRenderBackendExecuteFrameProc    ExecuteFrame;
	kRenderBackendNextFrame           NextFrame;
	kRenderBackendFlush               Flush;

	kRenderBackendDestroyProc         Destroy;
} kRenderBackend;

void        kCreateSwapChainFallback(void *);
void        kDestroySwapChainFallback(void);
void        kResizeSwapChainFallback(uint, uint);
void        kPresentFallback(void);

kTexture   *kCreateTextureFallback(const kTextureSpec &);
void        kDestroyTextureFallback(kTexture *);
kVec2i      kGetTextureSizeFallback(kTexture *);
void        kResizeTextureFallback(kTexture *, u32, u32);
void        kExecuteFrameFallback(const kRenderFrame2D &);
void        kNextFrameFallback(void);
void        kFlushFallback(void);
void        kDestroyFallback(void);

void        kFallbackRenderBackend(kRenderBackend *backend);
extern void kCreateRenderBackend(kRenderBackend *backend);
