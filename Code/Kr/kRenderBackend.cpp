#include "kRenderBackend.h"

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
bool kEnableDebugLayer = true;
#else
bool kEnableDebugLayer = false;
#endif

//
//
//

kSwapChain *kCreateSwapChainFallback(void *, const kRenderTargetConfig &) { return (kSwapChain *)&kFallbackSwapChain; }
void        kDestroySwapChainFallback(kSwapChain *) {}
void        kResizeSwapChainFallback(kSwapChain *, uint, uint) {}
kTexture *  kSwapChainRenderTargetFallback(kSwapChain *) { return (kTexture *)&kFallbackTexture; }
kRenderTargetConfig kGetRenderTargetConfigFallback(kSwapChain *) { return kRenderTargetConfig{}; }
bool                kApplyRenderTargetConfigFallback(kSwapChain *, const kRenderTargetConfig &) { return false; }
void                kPresentFallback(kSwapChain *) {}
kTexture *          kCreateTextureFallback(const kTextureSpec &) { return (kTexture *)&kFallbackTexture; }
void                kDestroyTextureFallback(kTexture *) {}
kVec2i              kGetTextureSizeFallback(kTexture *) { return kVec2i(0); }
void                kResizeTextureFallback(kTexture *, u32, u32) {}
void                kExecuteFrameFallback(const kRenderFrame2D &) {}
void                kNextFrameFallback(void) {}
void                kDestroyFallback(void) {}

//
//
//

void kFallbackRenderBackend(kRenderBackend *backend)
{
	backend->CreateSwapChain         = kCreateSwapChainFallback;
	backend->DestroySwapChain        = kDestroySwapChainFallback;
	backend->ResizeSwapChain         = kResizeSwapChainFallback;
	backend->SwapChainRenderTarget   = kSwapChainRenderTargetFallback;
	backend->GetRenderTargetConfig   = kGetRenderTargetConfigFallback;
	backend->ApplyRenderTargetConfig = kApplyRenderTargetConfigFallback;
	backend->Present                 = kPresentFallback;

	backend->CreateTexture           = kCreateTextureFallback;
	backend->DestroyTexture          = kDestroyTextureFallback;
	backend->GetTextureSize          = kGetTextureSizeFallback;
	backend->ResizeTexture           = kResizeTextureFallback;
	backend->ExecuteFrame            = kExecuteFrameFallback;
	backend->NextFrame               = kNextFrameFallback;
	backend->Destroy                 = kDestroyFallback;
}

extern bool kD3D11_CreateRenderBackend(kRenderBackend *backend);
extern bool kD3D12_CreateRenderBackend(kRenderBackend *backend);

void        kCreateRenderBackend(kRenderBackend *backend)
{
	if (kD3D12_CreateRenderBackend(backend)) return;
	if (kD3D11_CreateRenderBackend(backend)) return;
	kFallbackRenderBackend(backend);
}
