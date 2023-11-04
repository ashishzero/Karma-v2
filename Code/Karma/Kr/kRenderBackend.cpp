#include "kRenderBackend.h"

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
bool kEnableDebugLayer = true;
#else
bool kEnableDebugLayer = false;
#endif

//
//
//

void kCreateSwapChainFallback(void *, const kRenderPipelineConfig &)
{}
void kDestroySwapChainFallback(void)
{}
void kResizeSwapChainFallback(uint, uint)
{}
void kPresentFallback(void)
{}

void kGetRenderPipelineConfigFallback(kRenderPipelineConfig *)
{}
void kApplyRenderPipelineConfigFallback(const kRenderPipelineConfig &)
{}

kTexture kCreateTextureFallback(const kTextureSpec &)
{
	return kTexture{};
}
void kDestroyTextureFallback(kTexture)
{}
kVec2u kGetTextureSizeFallback(kTexture)
{
	return kVec2u(0);
}
void kExecuteFrameFallback(const kRenderFrame &)
{}
void kFlushFallback(void)
{}
void kDestroyFallback(void)
{}

//
//
//

void kFallbackRenderBackend(kRenderBackend *backend)
{
	backend->CreateSwapChain           = kCreateSwapChainFallback;
	backend->DestroySwapChain          = kDestroySwapChainFallback;
	backend->ResizeSwapChain           = kResizeSwapChainFallback;
	backend->Present                   = kPresentFallback;

	backend->GetRenderPipelineConfig   = kGetRenderPipelineConfigFallback;
	backend->ApplyRenderPipelineConfig = kApplyRenderPipelineConfigFallback;

	backend->CreateTexture             = kCreateTextureFallback;
	backend->DestroyTexture            = kDestroyTextureFallback;
	backend->GetTextureSize            = kGetTextureSizeFallback;

	backend->ExecuteFrame              = kExecuteFrameFallback;
	backend->Flush                     = kFlushFallback;

	backend->Destroy                   = kDestroyFallback;
}

extern bool kD3D11_CreateRenderBackend(kRenderBackend *backend);

void        kCreateRenderBackend(kRenderBackend *backend)
{
	if (kD3D11_CreateRenderBackend(backend))
		return;
	kFallbackRenderBackend(backend);
}
