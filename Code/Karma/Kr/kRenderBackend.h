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
	u8     *Pixels;
} kTextureSpec;

extern bool           kEnableDebugLayer;
static const kTexture kFallbackTexture;

typedef void (*kSwapChainCreateProc)(void *, const kRenderPipelineConfig &);
typedef void (*kSwapChainDestroyProc)(void);
typedef void (*kSwapChainResizeProc)(uint, uint);
typedef void (*kSwapChainPresentProc)(void);

typedef void (*kGetRenderPipelineConfigProc)(kRenderPipelineConfig *);
typedef void (*kApplyRenderPipelineConfigProc)(const kRenderPipelineConfig &);

typedef kTexture *(*kRenderBackendTextureCreateProc)(const kTextureSpec &);
typedef void (*kkRenderBackendTextureDestroyProc)(kTexture *);
typedef kVec2u (*kRenderBackendTextureSizeProc)(kTexture *);
typedef void (*kkRenderBackendTextureResizeProc)(kTexture *, u32, u32);

typedef void (*kRenderBackendExecuteFrameProc)(const kRenderFrame &);
typedef void (*kRenderBackendNextFrame)(void);
typedef void (*kRenderBackendFlush)(void);

typedef void (*kRenderBackendDestroyProc)(void);

typedef struct kRenderBackend
{
	kSwapChainCreateProc              CreateSwapChain;
	kSwapChainDestroyProc             DestroySwapChain;
	kSwapChainResizeProc              ResizeSwapChain;
	kSwapChainPresentProc             Present;

	kGetRenderPipelineConfigProc      GetRenderPipelineConfig;
	kApplyRenderPipelineConfigProc    ApplyRenderPipelineConfig;

	kRenderBackendTextureCreateProc   CreateTexture;
	kkRenderBackendTextureDestroyProc DestroyTexture;
	kRenderBackendTextureSizeProc     GetTextureSize;
	kkRenderBackendTextureResizeProc  ResizeTexture;

	kRenderBackendExecuteFrameProc    ExecuteFrame;
	kRenderBackendNextFrame           NextFrame;
	kRenderBackendFlush               Flush;

	kRenderBackendDestroyProc         Destroy;
} kRenderBackend;

void        kCreateSwapChainFallback(void *, const kRenderPipelineConfig &);
void        kDestroySwapChainFallback(void);
void        kResizeSwapChainFallback(uint, uint);
void        kPresentFallback(void);

void        kGetRenderPipelineConfigFallback(kRenderPipelineConfig *);
void        kApplyRenderPipelineConfigFallback(const kRenderPipelineConfig &);

kTexture   *kCreateTextureFallback(const kTextureSpec &);
void        kDestroyTextureFallback(kTexture *);
kVec2u      kGetTextureSizeFallback(kTexture *);
void        kResizeTextureFallback(kTexture *, u32, u32);
void        kExecuteFrameFallback(const kRenderFrame &);
void        kNextFrameFallback(void);
void        kFlushFallback(void);
void        kDestroyFallback(void);

void        kFallbackRenderBackend(kRenderBackend *backend);
extern void kCreateRenderBackend(kRenderBackend *backend);
