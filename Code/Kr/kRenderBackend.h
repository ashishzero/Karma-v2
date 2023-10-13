#pragma once
#include "kCommon.h"
#include "kRenderShared.h"

typedef struct kRenderPass2D
{
	kTexture               *rt;
	kTexture               *ds;
	kViewport               viewport;
	u32                     flags;
	kRenderClear2D          clear;
	kSpan<kRenderCommand2D> commands;
} kRenderPass2D;

typedef struct kRenderData2D
{
	kSpan<kRenderPass2D> passes;
	kSpan<kVertex2D>     vertices;
	kSpan<kIndex2D>      indices;
} kRenderData2D;

typedef kSwapChain *(*kSwapChainCreateProc)(void *, const kRenderTargetConfig &);
typedef void (*kSwapChainDestroyProc)(kSwapChain *);
typedef void (*kSwapChainResizeProc)(kSwapChain *, uint, uint);
typedef kTexture *(*kSwapChainTargetProc)(kSwapChain *);
typedef kRenderTargetConfig(*kSwapChainRenderTargetConfig)(kSwapChain *);
typedef void (*kApplySwapChainRenderTargetConfig)(kSwapChain *, const kRenderTargetConfig &);
typedef void (*kSwapChainPresentProc)(kSwapChain *);

typedef kTexture *(*kRenderBackendTextureCreateProc)(const kTextureSpec &);
typedef void (*kkRenderBackendTextureDestroyProc)(kTexture *);
typedef kVec2i (*kRenderBackendTextureSizeProc)(kTexture *);
typedef void (*kkRenderBackendTextureResizeProc)(kTexture *, u32, u32);

typedef void (*kRenderBackendExecuteCommandsProc)(const kRenderData2D &);
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

	kRenderBackendDestroyProc         Destroy;
} kRenderBackend;

void        kFallbackRenderBackend(kRenderBackend *backend);
extern void kCreateRenderBackend(kRenderBackend *backend);
