#pragma once
#include "kCommon.h"
#include "kRenderCommand.h"

extern bool kEnableDebugLayer;

typedef void (*kSwapChainCreateProc)(void *, const kRenderPipelineConfig &);
typedef void (*kSwapChainDestroyProc)(void);
typedef void (*kSwapChainResizeProc)(uint, uint);
typedef void (*kSwapChainPresentProc)(void);

typedef void (*kGetRenderPipelineConfigProc)(kRenderPipelineConfig *);
typedef void (*kApplyRenderPipelineConfigProc)(const kRenderPipelineConfig &);

typedef kTexture (*kRenderBackendTextureCreateProc)(const kTextureSpec &);
typedef void (*kkRenderBackendTextureDestroyProc)(kTexture);
typedef kVec2u (*kRenderBackendTextureSizeProc)(kTexture);

typedef void (*kRenderBackendExecuteFrameProc)(const kRenderFrame &);
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

	kRenderBackendExecuteFrameProc    ExecuteFrame;
	kRenderBackendFlush               Flush;

	kRenderBackendDestroyProc         Destroy;
} kRenderBackend;

void        kCreateSwapChainFallback(void *, const kRenderPipelineConfig &);
void        kDestroySwapChainFallback(void);
void        kResizeSwapChainFallback(uint, uint);
void        kPresentFallback(void);

void        kGetRenderPipelineConfigFallback(kRenderPipelineConfig *);
void        kApplyRenderPipelineConfigFallback(const kRenderPipelineConfig &);

kTexture    kCreateTextureFallback(const kTextureSpec &);
void        kDestroyTextureFallback(kTexture);
kVec2u      kGetTextureSizeFallback(kTexture);
void        kExecuteFrameFallback(const kRenderFrame &);
void        kFlushFallback(void);
void        kDestroyFallback(void);

void        kFallbackRenderBackend(kRenderBackend *backend);
extern void kCreateRenderBackend(kRenderBackend *backend);
