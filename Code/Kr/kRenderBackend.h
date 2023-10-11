#pragma once
#include "kCommon.h"
#include "kRenderShared.h"

typedef void (*kSwapChainResizeCbProc)(kSwapChain, uint, uint, void *);

typedef kSwapChain (*kSwapChainCreateProc)(void *);
typedef void (*kSwapChainDestroyProc)(kSwapChain);
typedef void (*kSwapChainResizeProc)(kSwapChain, uint, uint);
typedef void (*kSwapChainResizedCbProc)(kSwapChain, kSwapChainResizeCbProc, void *);
typedef kTexture (*kSwapChainTargetProc)(kSwapChain);
typedef void (*kSwapChainPresentProc)(kSwapChain);

typedef struct kSwapChainBackend
{
	kSwapChainCreateProc    create;
	kSwapChainDestroyProc   destroy;
	kSwapChainResizeProc    resize;
	kSwapChainResizedCbProc resizecb;
	kSwapChainTargetProc    target;
	kSwapChainPresentProc   present;
} kSwapChainBackend;

typedef kTexture (*kRenderBackendTextureCreateProc)(const kTextureSpec &);
typedef void (*kkRenderBackendTextureDestroyProc)(kTexture);
typedef void (*kkRenderBackendTextureSizeProc)(kTexture, u32 *, u32 *);
typedef void (*kkRenderBackendTextureResizeProc)(kTexture, u32, u32);
typedef void (*kRenderBackendCommitProc)(const kRenderData2D &);
typedef void (*kRenderBackendDestroyProc)(void);

typedef struct kRenderBackend
{
	kSwapChainBackend                 swap_chain;

	kRenderBackendTextureCreateProc   create_texture;
	kkRenderBackendTextureDestroyProc destroy_texture;
	kkRenderBackendTextureSizeProc    texture_size;
	kkRenderBackendTextureResizeProc  resize_texture;

	kRenderBackendCommitProc          commit;

	kRenderBackendDestroyProc         destroy;
} kRenderBackend;

extern void kCreateRenderBackend(kRenderBackend *backend);
