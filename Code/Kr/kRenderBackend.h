#pragma once
#include "kCommon.h"
#include "kRenderShared.h"

extern bool             kEnableDebugLayer;
static const kSwapChain kFallbackSwapChain;
static const kTexture   kFallbackTexture;

typedef struct kRenderPass2D
{
	kTexture               *rt;
	kTexture               *ds;
	kViewport               viewport;
	u32                     flags;
	kRenderClear2D          clear;
	kSpan<kRenderCommand2D> commands;
} kRenderPass2D;

typedef struct kRenderPass2D_Version2
{
	kTexture               *rt;
	kTexture               *ds;
	kViewport               viewport;
	u32                     flags;
	kRenderClear2D          clear;
	kSpan<kRenderCommand2D> commands;
} kRenderPass2D_Version2;

typedef struct kRenderData2D
{
	kSpan<kRenderPass2D> passes;
	kSpan<kVertex2D>     vertices;
	kSpan<kIndex2D>      indices;
} kRenderData2D;

typedef struct kRenderData2_Version2
{
	kSpan<kRenderPass2D_Version2> passes;
	kSpan<kMat4>                  transforms;
	kSpan<kTexture *>             textures[2];
	kSpan<kVertex2D>              vertices;
	kSpan<kIndex2D>               indices;
} kRenderData2_Version2;

typedef kSwapChain *(*kSwapChainCreateProc)(void *, const kRenderTargetConfig &);
typedef void (*kSwapChainDestroyProc)(kSwapChain *);
typedef void (*kSwapChainResizeProc)(kSwapChain *, uint, uint);
typedef kTexture *(*kSwapChainTargetProc)(kSwapChain *);
typedef kRenderTargetConfig (*kSwapChainRenderTargetConfig)(kSwapChain *);
typedef void (*kApplySwapChainRenderTargetConfig)(kSwapChain *, const kRenderTargetConfig &);
typedef void (*kSwapChainPresentProc)(kSwapChain *);

typedef kTexture *(*kRenderBackendTextureCreateProc)(const kTextureSpec &);
typedef void (*kkRenderBackendTextureDestroyProc)(kTexture *);
typedef kVec2i (*kRenderBackendTextureSizeProc)(kTexture *);
typedef void (*kkRenderBackendTextureResizeProc)(kTexture *, u32, u32);

typedef void (*kRenderBackendExecuteCommandsProc)(const kRenderData2D &);
typedef void (*kRenderBackendExecuteCommandsProc2)(const kRenderData2_Version2 &);
typedef void (*kRenderBackendNextFrame)(void);

typedef void (*kRenderBackendDestroyProc)(void);

typedef struct kRenderBackend
{
	kSwapChainCreateProc              CreateSwapChain;
	kSwapChainDestroyProc             DestroySwapChain;
	kSwapChainResizeProc              ResizeSwapChain;
	kSwapChainTargetProc              SwapChainRenderTarget;
	kSwapChainRenderTargetConfig      GetRenderTargetConfig;
	kApplySwapChainRenderTargetConfig ApplyRenderTargetConfig;
	kSwapChainPresentProc             Present;

	kRenderBackendTextureCreateProc   CreateTexture;
	kkRenderBackendTextureDestroyProc DestroyTexture;
	kRenderBackendTextureSizeProc     GetTextureSize;
	kkRenderBackendTextureResizeProc  ResizeTexture;

	kRenderBackendExecuteCommandsProc ExecuteCommands;
	kRenderBackendNextFrame           NextFrame;

	kRenderBackendDestroyProc         Destroy;
} kRenderBackend;

kSwapChain         *kCreateSwapChainFallback(void *, const kRenderTargetConfig &);
void                kDestroySwapChainFallback(kSwapChain *);
void                kResizeSwapChainFallback(kSwapChain *, uint, uint);
kTexture           *kSwapChainRenderTargetFallback(kSwapChain *);
kRenderTargetConfig kGetRenderTargetConfigFallback(kSwapChain *);
void                kApplyRenderTargetConfigFallback(kSwapChain *, const kRenderTargetConfig &);
void                kPresentFallback(kSwapChain *);
kTexture           *kCreateTextureFallback(const kTextureSpec &);
void                kDestroyTextureFallback(kTexture *);
kVec2i              kGetTextureSizeFallback(kTexture *);
void                kResizeTextureFallback(kTexture *, u32, u32);
void                kExecuteCommandsFallback(const kRenderData2D &);
void                kNextFrameFallback(void);
void                kDestroyFallback(void);

void                kFallbackRenderBackend(kRenderBackend *backend);
extern void         kCreateRenderBackend(kRenderBackend *backend);
