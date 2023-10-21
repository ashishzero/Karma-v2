#include "kRenderBackend.h"

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
bool kEnableDebugLayer = true;
#else
bool kEnableDebugLayer = false;
#endif

//
//
//

void      kCreateSwapChainFallback(void *) {}
void      kDestroySwapChainFallback(void) {}
void      kResizeSwapChainFallback(uint, uint) {}
void      kPresentFallback(void) {}

kTexture *kCreateTextureFallback(const kTextureSpec &) { return (kTexture *)&kFallbackTexture; }
void      kDestroyTextureFallback(kTexture *) {}
kVec2i    kGetTextureSizeFallback(kTexture *) { return kVec2i(0); }
void      kResizeTextureFallback(kTexture *, u32, u32) {}
void      kExecuteFrameFallback(const kRenderFrame2D &) {}
void      kNextFrameFallback(void) {}
void      kFlushFallback(void) {}
void      kDestroyFallback(void) {}

//
//
//

void kFallbackRenderBackend(kRenderBackend *backend)
{
	backend->CreateSwapChain  = kCreateSwapChainFallback;
	backend->DestroySwapChain = kDestroySwapChainFallback;
	backend->ResizeSwapChain  = kResizeSwapChainFallback;
	backend->Present          = kPresentFallback;

	backend->CreateTexture    = kCreateTextureFallback;
	backend->DestroyTexture   = kDestroyTextureFallback;
	backend->GetTextureSize   = kGetTextureSizeFallback;
	backend->ResizeTexture    = kResizeTextureFallback;

	backend->ExecuteFrame     = kExecuteFrameFallback;
	backend->NextFrame        = kNextFrameFallback;
	backend->Flush            = kFlushFallback;

	backend->Destroy          = kDestroyFallback;
}

extern bool kD3D11_CreateRenderBackend(kRenderBackend *backend);
extern bool kD3D12_CreateRenderBackend(kRenderBackend *backend);

void        kCreateRenderBackend(kRenderBackend *backend)
{
	if (kD3D12_CreateRenderBackend(backend)) return;
	if (kD3D11_CreateRenderBackend(backend)) return;
	kFallbackRenderBackend(backend);
}
