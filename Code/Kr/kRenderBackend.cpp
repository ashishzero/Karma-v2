#include "kRenderBackend.h"
#include "kLinkedList.h"

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
static bool EnableDebugLayer = true;
#else
static bool EnableDebugLayer = false;
#endif

static const kSwapChain kFallbackSwapChain;
static const kTexture   kFallbackTexture;

//
//
//

static kSwapChain *kCreateSwapChainFallback(void *) { return (kSwapChain *)&kFallbackSwapChain; }
static void        kDestroySwapChainFallback(kSwapChain *) {}
static void        kResizeSwapChainFallback(kSwapChain *, uint, uint) {}
static kTexture   *kSwapChainRenderTargetFallback(kSwapChain *) { return (kTexture *)&kFallbackTexture; }
static void        kPresentFallback(kSwapChain *) {}

static kTexture   *kCreateTextureFallback(const kTextureSpec &) { return (kTexture *)&kFallbackTexture; }
static void        kDestroyTextureFallback(kTexture *) {}
static kVec2i      kGetTextureSizeFallback(kTexture *) { return kVec2i(0); }
static void        kResizeTextureFallback(kTexture *, u32, u32) {}

static void        kExecuteCommandsFallback(const kRenderData2D &) {}
static void        kDestroyFallback(void) {}

//
//
//

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <VersionHelpers.h>

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")

#include <d3d11_1.h>
#include <dxgi1_3.h>

constexpr uint K_BACK_BUFFER_COUNT     = 3;
constexpr uint K_TEXTURE_MAX_PER_BLOCK = 32;

typedef struct kD3D11_Texture : kTexture
{
	ID3D11ShaderResourceView  *srv;
	ID3D11RenderTargetView    *rtv;
	ID3D11DepthStencilView    *dsv;
	ID3D11UnorderedAccessView *uav;
	ID3D11Texture2D           *texture2d;
	kD3D11_Texture            *prev;
	kD3D11_Texture            *next;
} kD3D11_Texture;

typedef enum kD3D11_RenderTarget
{
	kD3D11_RenderTarget_Final,
	kD3D11_RenderTarget_MSAA,
	kD3D11_RenderTarget_Resolved,
	kD3D11_RenderTarget_Count
} kD3D11_RenderTarget;

typedef struct kD3D11_SwapChain : kSwapChain
{
	IDXGISwapChain1    *native;
	kD3D11_Texture      targets[kD3D11_RenderTarget_Count];
	kD3D11_RenderTarget rt;
	kRenderTargetSpec   rt_spec;
	kD3D11_SwapChain   *prev;
	kD3D11_SwapChain   *next;
} kD3D11_SwapChain;

typedef struct kD3D11_TextureBlock
{
	kD3D11_TextureBlock *next;
	kD3D11_Texture       data[K_TEXTURE_MAX_PER_BLOCK];
} kD3D11_TextureBlock;

typedef struct kD3D11_TexturePool
{
	kD3D11_Texture       first;
	kD3D11_Texture      *free;
	kD3D11_TextureBlock *blocks;
} kD3D11_TexturePool;

typedef struct kD3D11_Render2D
{
	uint                   vertex_sz;
	uint                   index_sz;
	ID3D11Buffer          *vertex;
	ID3D11Buffer          *index;
	ID3D11Buffer          *constant;
	ID3D11VertexShader    *vs;
	ID3D11PixelShader     *ps;
	ID3D11InputLayout     *input;
	ID3D11RasterizerState *rasterizer;
} kD3D11_Render2D;

typedef enum kD3D11_PostProcessPs
{
	kD3D11_PostProcessPs_Rect,
	kD3D11_PostProcessPs_Count,
} kD3D11_PostProcessPs;

typedef struct kD3D11_PostProcess
{
	ID3D11VertexShader *vs;
	ID3D11PixelShader  *ps[kD3D11_PostProcessPs_Count];
} kD3D11_PostProcess;

typedef struct kD3D11_Resource
{
	kD3D11_TexturePool       textures;
	kD3D11_SwapChain         swap_chains;
	ID3D11DepthStencilState *depth;
	ID3D11SamplerState      *samplers[kTextureFilter_Count];
	ID3D11BlendState        *blends[kBlendMode_Count];
	kD3D11_Render2D          render2d;
	kD3D11_PostProcess       postprocess;
} kD3D11_Resource;

typedef struct kD3D11_Backend
{
	ID3D11Device1        *device;
	ID3D11DeviceContext1 *device_context;
	IDXGIFactory2        *factory;
	kD3D11_Resource       resource;
} kD3D11_Backend;

static kD3D11_Backend d3d11;

//
//
//

template <typename T>
static void kRelease(T **o)
{
	if (*o) (*o)->Release();
	*o = nullptr;
}

static void kD3D11_Flush(void)
{
	d3d11.device_context->Flush();
	d3d11.device_context->ClearState();
}

//
// Mappings
//

static constexpr UINT kD3D11_SampleCountMap[] = {1, 2, 4, 8, 16};
static_assert(kArrayCount(kD3D11_SampleCountMap) == kAntiAliasingMethod_Count, "");

static constexpr DXGI_FORMAT kD3D11_FormatMap[] = {
	DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM,    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_R11G11B10_FLOAT,   DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R32G32B32_UINT,     DXGI_FORMAT_R32G32_FLOAT,      DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R32G32_UINT,        DXGI_FORMAT_R8G8_UNORM,        DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_SINT,           DXGI_FORMAT_R32_UINT,          DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R8_UNORM,           DXGI_FORMAT_D32_FLOAT,         DXGI_FORMAT_D16_UNORM,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
};
static_assert(kArrayCount(kD3D11_FormatMap) == kFormat_Count, "");

static constexpr D3D11_USAGE kD3D11_UsageMap[] = {
	D3D11_USAGE_DEFAULT,
	D3D11_USAGE_IMMUTABLE,
	D3D11_USAGE_DYNAMIC,
	D3D11_USAGE_STAGING,
};
static_assert(kArrayCount(kD3D11_UsageMap) == kUsage_Count, "");

static constexpr D3D11_FILTER kD3D11_FilterMap[] = {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_MIN_MAG_MIP_POINT};
static_assert(kArrayCount(kD3D11_FilterMap) == kTextureFilter_Count, "");

//
// Textures
//

static kD3D11_Texture *kD3D11_AllocTexture(void)
{
	kD3D11_TexturePool *pool = &d3d11.resource.textures;

	if (!pool->free)
	{
		kD3D11_TextureBlock *block = (kD3D11_TextureBlock *)kAlloc(sizeof(kD3D11_TextureBlock));

		if (!block) return nullptr;

		memset(block, 0, sizeof(*block));

		for (int i = 0; i < K_TEXTURE_MAX_PER_BLOCK - 1; ++i)
		{
			block->data[i].next = &block->data[i + 1];
		}

		pool->free   = block->data;
		block->next  = pool->blocks;
		pool->blocks = block;
	}

	kD3D11_Texture *r = pool->free;
	pool->free        = r->next;

	kDListPushBack(&pool->first, r);

	return r;
}

static void kD3D11_FreeTexture(kD3D11_Texture *r)
{
	kD3D11_TexturePool *pool = &d3d11.resource.textures;
	kDListRemove(r);
	memset(r, 0, sizeof(*r));
	r->next    = pool->free;
	pool->free = r;
}

static void kD3D11_PrepareTexture(kD3D11_Texture *texture, UINT bind_flags)
{
	texture->state = kResourceState_Unready;

	HRESULT hr     = 0;

	if (bind_flags & D3D11_BIND_SHADER_RESOURCE)
	{
		hr = d3d11.device->CreateShaderResourceView(texture->texture2d, nullptr, &texture->srv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create shader resource view");
		}
	}

	if (bind_flags & D3D11_BIND_RENDER_TARGET)
	{
		hr = d3d11.device->CreateRenderTargetView(texture->texture2d, nullptr, &texture->rtv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create render target view");
		}
	}

	if (bind_flags & D3D11_BIND_DEPTH_STENCIL)
	{
		hr = d3d11.device->CreateDepthStencilView(texture->texture2d, nullptr, &texture->dsv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create depth target view");
		}
	}

	if (bind_flags & D3D11_BIND_UNORDERED_ACCESS)
	{
		hr = d3d11.device->CreateUnorderedAccessView(texture->texture2d, nullptr, &texture->uav);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create unordered access view");
		}
	}

	texture->state = kResourceState_Ready;
}

static kTexture *kD3D11_CreateTexture(const kTextureSpec &spec)
{
	kD3D11_Texture *r = kD3D11_AllocTexture();

	if (!r)
	{
		kLogError("DirectX11: Failed to allocate memory for texture\n");
		return (kD3D11_Texture *)&kFallbackTexture;
	}

	D3D11_TEXTURE2D_DESC desc = {};

	desc.Width                = spec.width;
	desc.Height               = spec.height;
	desc.Format               = kD3D11_FormatMap[spec.format];
	desc.ArraySize            = 1;

	desc.BindFlags            = 0;
	if (spec.bind_flags & kBind_VertexBuffer) desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
	if (spec.bind_flags & kBind_IndexBuffer) desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
	if (spec.bind_flags & kBind_ConstantBuffer) desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
	if (spec.bind_flags & kBind_ShaderResource) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if (spec.bind_flags & kBind_RenderTarget) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	if (spec.bind_flags & kBind_DepthStencil) desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	if (spec.bind_flags & kBind_UnorderedAccess) desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

	desc.Usage              = kD3D11_UsageMap[spec.usage];
	desc.SampleDesc.Count   = kMax(spec.num_samples, 1u);
	desc.SampleDesc.Quality = 0;
	desc.CPUAccessFlags     = 0;
	desc.MiscFlags          = 0;
	desc.MipLevels          = 1;

	D3D11_SUBRESOURCE_DATA source;
	source.pSysMem          = spec.pixels;
	source.SysMemPitch      = spec.pitch;
	source.SysMemSlicePitch = 0;

	HRESULT hr              = d3d11.device->CreateTexture2D(&desc, spec.pixels ? &source : nullptr, &r->texture2d);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create texture");
		r->state = kResourceState_Error;
		return r;
	}

	kD3D11_PrepareTexture(r, desc.BindFlags);

	return r;
}

static void kD3D11_UnprepareTexture(kD3D11_Texture *r)
{
	if (r->state == kResourceState_Ready)
	{
		kRelease(&r->rtv);
		kRelease(&r->dsv);
		kRelease(&r->srv);
		kRelease(&r->uav);
		r->state = kResourceState_Unready;
	}
}

static void kD3D11_DestroyTexture(kTexture *texture)
{
	kD3D11_Texture *r = (kD3D11_Texture *)texture;
	if (r->state == kResourceState_Ready)
	{
		kD3D11_UnprepareTexture(r);
		kRelease(&r->texture2d);
		kD3D11_FreeTexture(r);
	}
}

static kVec2i kD3D11_GetTextureSize(kTexture *texture)
{
	kD3D11_Texture *r = (kD3D11_Texture *)texture;
	if (r->state == kResourceState_Ready)
	{
		D3D11_TEXTURE2D_DESC desc;
		r->texture2d->GetDesc(&desc);
		return kVec2i((int)desc.Width, (int)desc.Height);
	}
	return kVec2i(0);
}

static void kD3D11_ResizeTexture(kTexture *texture, u32 w, u32 h)
{
	kD3D11_Texture *r = (kD3D11_Texture *)texture;

	if (w == 0 || h == 0) return;

	if (r->state == kResourceState_Ready)
	{
		D3D11_TEXTURE2D_DESC desc;
		r->texture2d->GetDesc(&desc);

		if (w == desc.Width && h == desc.Height) return;

		kD3D11_UnprepareTexture(r);
		kRelease(&r->texture2d);

		desc.Width  = w;
		desc.Height = h;

		HRESULT hr  = d3d11.device->CreateTexture2D(&desc, nullptr, &r->texture2d);

		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create texture");
			r->state = kResourceState_Error;
			return;
		}

		kD3D11_PrepareTexture(r, desc.BindFlags);
	}
}

//
// SwapChain
//

static void kD3D11_DestroySwapChainBuffers(kD3D11_SwapChain *r)
{
	for (int i = 0; i < kD3D11_RenderTarget_Count; ++i)
	{
		kD3D11_UnprepareTexture(&r->targets[i]);
		kRelease(&r->targets[i].texture2d);
		memset(&r->targets[i], 0, sizeof(r->targets[i]));
	}
}

static void kD3D11_CreateSwapChainBuffers(kD3D11_SwapChain *r)
{
	r->rt              = kD3D11_RenderTarget_Final;

	kD3D11_Texture *rt = &r->targets[kD3D11_RenderTarget_Final];
	rt->state          = kResourceState_Unready;

	HRESULT hr         = r->native->GetBuffer(0, IID_PPV_ARGS(&rt->texture2d));

	if (FAILED(hr))
	{
		rt->state = kResourceState_Error;
		kLogHresultError(hr, "DirectX11", "Failed to create swapchain render target");
		return;
	}

	kD3D11_PrepareTexture(rt, D3D11_BIND_RENDER_TARGET);

	kVec2i size = kD3D11_GetTextureSize(rt);

	if (r->rt_spec.antialiasing == kAntiAliasingMethod_None) return;

	kD3D11_Texture *msaa     = &r->targets[kD3D11_RenderTarget_MSAA];
	kD3D11_Texture *resolved = &r->targets[kD3D11_RenderTarget_Resolved];

	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width                = size.x;
		desc.Height               = size.y;
		desc.MipLevels            = 1;
		desc.ArraySize            = 1;
		desc.Format               = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.SampleDesc.Count     = kD3D11_SampleCountMap[r->rt_spec.antialiasing];
		desc.Usage                = D3D11_USAGE_DEFAULT;
		desc.BindFlags            = D3D11_BIND_RENDER_TARGET;

		hr                        = d3d11.device->CreateTexture2D(&desc, 0, &msaa->texture2d);

		if (FAILED(hr))
		{
			rt->state = kResourceState_Error;
			kLogHresultError(hr, "DirectX11", "Failed to create MSAA render target");
			return;
		}

		kD3D11_PrepareTexture(msaa, desc.BindFlags);
	}

	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width                = size.x;
		desc.Height               = size.y;
		desc.MipLevels            = 1;
		desc.ArraySize            = 1;
		desc.Format               = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.SampleDesc.Count     = 1;
		desc.Usage                = D3D11_USAGE_DEFAULT;
		desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

		hr                        = d3d11.device->CreateTexture2D(&desc, 0, &resolved->texture2d);

		if (FAILED(hr))
		{
			rt->state = kResourceState_Error;
			kLogHresultError(hr, "DirectX11", "Failed to create MSAA resolution render target");
			return;
		}

		kD3D11_PrepareTexture(resolved, desc.BindFlags);
	}

	if (msaa->state == kResourceState_Ready && resolved->state == kResourceState_Ready)
	{
		r->rt = kD3D11_RenderTarget_MSAA;
	}
}

static void kD3D11_ResizeSwapChainBuffers(kSwapChain *swap_chain, uint w, uint h)
{
	if (!w || !h) return;

	kD3D11_Flush();

	kD3D11_SwapChain *r = (kD3D11_SwapChain *)swap_chain;
	if (r->state == kResourceState_Ready)
	{
		kD3D11_DestroySwapChainBuffers(r);
		r->native->ResizeBuffers(K_BACK_BUFFER_COUNT, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		kD3D11_CreateSwapChainBuffers(r);
	}
}

static void kD3D11_DestroySwapChain(kSwapChain *swap_chain)
{
	kD3D11_Flush();

	kD3D11_SwapChain *r = (kD3D11_SwapChain *)swap_chain;
	if (r->state == kResourceState_Ready)
	{
		kDListRemove(r);
		kD3D11_DestroySwapChainBuffers(r);
		kRelease(&r->native);
		kFree(r, sizeof(*r));
	}
}

static kSwapChain *kD3D11_CreateSwapChain(void *_window)
{
	HWND                  wnd             = (HWND)_window;

	DXGI_SWAP_EFFECT      swap_effect     = IsWindows10OrGreater()  ? DXGI_SWAP_EFFECT_FLIP_DISCARD
	                                        : IsWindows8OrGreater() ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
	                                                                : DXGI_SWAP_EFFECT_DISCARD;

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.Width                 = 0;
	swap_chain_desc.Height                = 0;
	swap_chain_desc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.SampleDesc.Count      = 1;
	swap_chain_desc.SampleDesc.Quality    = 0;
	swap_chain_desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount           = K_BACK_BUFFER_COUNT;
	swap_chain_desc.SwapEffect            = swap_effect;
	swap_chain_desc.Scaling               = DXGI_SCALING_STRETCH;
	swap_chain_desc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;

	IDXGISwapChain1 *swapchain1           = nullptr;
	HRESULT          hr = d3d11.factory->CreateSwapChainForHwnd(d3d11.device, wnd, &swap_chain_desc, 0, 0, &swapchain1);
	kAssert(hr != DXGI_ERROR_INVALID_CALL);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create swap chain");
		return (kSwapChain *)&kFallbackSwapChain;
	}

	kD3D11_SwapChain *r = (kD3D11_SwapChain *)kAlloc(sizeof(kD3D11_SwapChain));

	if (!r)
	{
		kLogError("DirectX12: Failed RemoveAllocate memory for create swap chain\n");
		swapchain1->Release();
		return (kSwapChain *)&kFallbackSwapChain;
	}

	memset(r, 0, sizeof(*r));

	r->native = swapchain1;
	r->state  = kResourceState_Ready;

	// todo: param!!
	r->rt_spec.antialiasing = kAntiAliasingMethod_MSAAx4;
	kD3D11_CreateSwapChainBuffers(r);
	kDListPushBack(&d3d11.resource.swap_chains, r);

	return r;
}

static void kD3D11_Present(kSwapChain *swap_chain)
{
	kD3D11_SwapChain *r = (kD3D11_SwapChain *)swap_chain;
	if (r->state != kResourceState_Ready) return;

	if (r->rt == kD3D11_RenderTarget_MSAA)
	{
		kD3D11_Texture *src = &r->targets[kD3D11_RenderTarget_MSAA];
		kD3D11_Texture *dst = &r->targets[kD3D11_RenderTarget_Resolved];
		d3d11.device_context->ResolveSubresource(dst->texture2d, 0, src->texture2d, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

		kD3D11_Texture           *rt          = &r->targets[kD3D11_RenderTarget_Final];
		ID3D11ShaderResourceView *resources[] = {dst->srv};

		D3D11_TEXTURE2D_DESC      desc;
		rt->texture2d->GetDesc(&desc);

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width    = (float)desc.Width;
		viewport.Height   = (float)desc.Height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;

		float clear[]     = {0.0f, 0.0f, 0.0f, 0.0f};
		d3d11.device_context->ClearRenderTargetView(rt->rtv, clear);

		d3d11.device_context->OMSetRenderTargets(1, &rt->rtv, 0);
		d3d11.device_context->RSSetViewports(1, &viewport);
		d3d11.device_context->VSSetShader(d3d11.resource.postprocess.vs, 0, 0);
		d3d11.device_context->PSSetShader(d3d11.resource.postprocess.ps[kD3D11_PostProcessPs_Rect], 0, 0);
		d3d11.device_context->PSSetShaderResources(0, 1, resources);
		d3d11.device_context->PSSetSamplers(0, 1, &d3d11.resource.samplers[kTextureFilter_Linear]);
		d3d11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3d11.device_context->Draw(6, 0);

		d3d11.device_context->ClearState();
	}

	r->native->Present(1, 0);
}

static kTexture *kD3D11_GetSwapChainRenderTarget(kSwapChain *swap_chain)
{
	kD3D11_SwapChain *r = (kD3D11_SwapChain *)swap_chain;
	return &r->targets[r->rt];
}

//
// Render2D
//

static void kD3D11_ExecuteCommands(const kRenderData2D &data)
{
	if (data.vertices.count == 0 || data.indices.count == 0) return;

	uint vb_size = (uint)kArrSizeInBytes(data.vertices);
	uint ib_size = (uint)kArrSizeInBytes(data.indices);

	if (vb_size > d3d11.resource.render2d.vertex_sz)
	{
		kRelease(&d3d11.resource.render2d.vertex);
		d3d11.resource.render2d.vertex_sz = 0;

		D3D11_BUFFER_DESC vb_desc         = {};
		vb_desc.ByteWidth                 = vb_size;
		vb_desc.Usage                     = D3D11_USAGE_DYNAMIC;
		vb_desc.BindFlags                 = D3D11_BIND_VERTEX_BUFFER;
		vb_desc.CPUAccessFlags            = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr                        = d3d11.device->CreateBuffer(&vb_desc, 0, &d3d11.resource.render2d.vertex);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to allocate vertex buffer");
			return;
		}

		d3d11.resource.render2d.vertex_sz = vb_size;
	}

	if (ib_size > d3d11.resource.render2d.index_sz)
	{
		kRelease(&d3d11.resource.render2d.index);
		d3d11.resource.render2d.index_sz = 0;

		D3D11_BUFFER_DESC ib_desc        = {};
		ib_desc.ByteWidth                = ib_size;
		ib_desc.Usage                    = D3D11_USAGE_DYNAMIC;
		ib_desc.BindFlags                = D3D11_BIND_INDEX_BUFFER;
		ib_desc.CPUAccessFlags           = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr                       = d3d11.device->CreateBuffer(&ib_desc, 0, &d3d11.resource.render2d.index);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to allocate index buffer");
			return;
		}

		d3d11.resource.render2d.index_sz = ib_size;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;

	ID3D11DeviceContext1    *dc = d3d11.device_context;
	ID3D11Buffer            *vb = d3d11.resource.render2d.vertex;
	ID3D11Buffer            *ib = d3d11.resource.render2d.index;
	ID3D11Buffer            *cb = d3d11.resource.render2d.constant;
	HRESULT                  hr = S_OK;

	hr                          = dc->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to map vertex2d buffer");
		return;
	}
	memcpy(mapped.pData, data.vertices.data, vb_size);
	dc->Unmap(vb, 0);

	hr = dc->Map(ib, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to map index2d buffer");
		return;
	}
	memcpy(mapped.pData, data.indices.data, ib_size);
	dc->Unmap(ib, 0);

	UINT        stride = sizeof(kVertex2D);
	UINT        offset = 0;
	DXGI_FORMAT format = sizeof(kIndex2D) == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	dc->IASetVertexBuffers(0, 1, &d3d11.resource.render2d.vertex, &stride, &offset);
	dc->IASetIndexBuffer(d3d11.resource.render2d.index, format, 0);
	dc->VSSetShader(d3d11.resource.render2d.vs, nullptr, 0);
	dc->PSSetShader(d3d11.resource.render2d.ps, nullptr, 0);
	dc->IASetInputLayout(d3d11.resource.render2d.input);
	dc->RSSetState(d3d11.resource.render2d.rasterizer);
	dc->OMSetDepthStencilState(d3d11.resource.depth, 0);
	dc->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const kRenderPass2D &pass : data.passes)
	{
		kD3D11_Texture *rt = (kD3D11_Texture *)pass.rt;
		kD3D11_Texture *ds = (kD3D11_Texture *)pass.ds;

		if (rt->state != kResourceState_Ready) continue;
		if (ds && ds->state != kResourceState_Ready) continue;

		if (pass.flags & kRenderPass_ClearColor)
		{
			dc->ClearRenderTargetView(rt->rtv, pass.clear.color.m);
		}

		if (ds)
		{
			UINT flags = 0;
			if (pass.flags & kRenderPass_ClearDepth) flags |= D3D11_CLEAR_DEPTH;
			if (pass.flags & kRenderPass_ClearStencil) flags |= D3D11_CLEAR_STENCIL;

			if (flags)
			{
				dc->ClearDepthStencilView(ds->dsv, flags, pass.clear.depth, pass.clear.stencil);
			}
		}

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = pass.viewport.x;
		viewport.TopLeftY = pass.viewport.y;
		viewport.Width    = pass.viewport.w;
		viewport.Height   = pass.viewport.h;
		viewport.MinDepth = pass.viewport.n;
		viewport.MaxDepth = pass.viewport.f;

		dc->OMSetRenderTargets(1, &rt->rtv, ds ? ds->dsv : nullptr);
		dc->RSSetViewports(1, &viewport);

		for (const kRenderCommand2D &cmd : pass.commands)
		{
			dc->OMSetBlendState(d3d11.resource.blends[cmd.shader.blend], 0, 0xffffffff);

			ID3D11ShaderResourceView *ps_resources[2];

			for (const kRenderParam2D &param : cmd.params)
			{
				hr = dc->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
				if (FAILED(hr))
				{
					kLogHresultError(hr, "DirectX11", "Failed to map constant buffer");
					continue;
				}
				memcpy(mapped.pData, param.transform.m, sizeof(kMat4));
				dc->Unmap(cb, 0);

				kD3D11_Texture *r1 = (kD3D11_Texture *)param.textures[0];
				kD3D11_Texture *r2 = (kD3D11_Texture *)param.textures[1];

				if (r1->state != kResourceState_Ready || r2->state != kResourceState_Ready) continue;

				ps_resources[0] = r1->srv;
				ps_resources[1] = r2->srv;

				D3D11_RECT rect;
				rect.left   = (LONG)(param.rect.min.x);
				rect.top    = (LONG)(param.rect.min.y);
				rect.right  = (LONG)(param.rect.max.x);
				rect.bottom = (LONG)(param.rect.max.y);

				dc->VSSetConstantBuffers(0, 1, &cb);
				dc->PSSetShaderResources(0, kArrayCount(ps_resources), ps_resources);
				dc->PSSetSamplers(0, 1, &d3d11.resource.samplers[cmd.filter]);
				dc->RSSetScissorRects(1, &rect);
				dc->DrawIndexed(param.count, param.index, param.vertex);
			}
		}

		dc->PSSetSamplers(0, 0, nullptr);
		dc->PSSetShaderResources(0, 0, nullptr);
	}

	dc->ClearState();
}

//
// Resources
//

#include "Generated/kRender2D.hlsl.h"
#include "Generated/kPostProcess.hlsl.h"

static void kD3D11_DestroyRenderResources2D(void)
{
	kRelease(&d3d11.resource.render2d.vertex);
	kRelease(&d3d11.resource.render2d.index);
	kRelease(&d3d11.resource.render2d.constant);
	kRelease(&d3d11.resource.render2d.rasterizer);
	kRelease(&d3d11.resource.render2d.input);
	kRelease(&d3d11.resource.render2d.vs);
	kRelease(&d3d11.resource.render2d.ps);

	d3d11.resource.render2d.vertex_sz = 0;
	d3d11.resource.render2d.index_sz  = 0;
}

static bool kD3D11_CreateRenderResources2D(void)
{
	kString           vs      = kString(kQuadVS, kArrayCount(kQuadVS));
	kString           ps      = kString(kQuadPS, kArrayCount(kQuadPS));

	D3D11_BUFFER_DESC cb_desc = {};
	cb_desc.ByteWidth         = sizeof(kMat4);
	cb_desc.Usage             = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr                = d3d11.device->CreateBuffer(&cb_desc, 0, &d3d11.resource.render2d.constant);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to allocate constant buffer");
		return false;
	}

	D3D11_RASTERIZER_DESC ras_desc = {};
	ras_desc.FillMode              = D3D11_FILL_SOLID;
	ras_desc.CullMode              = D3D11_CULL_NONE;
	ras_desc.ScissorEnable         = TRUE;
	ras_desc.MultisampleEnable     = TRUE;
	ras_desc.AntialiasedLineEnable = TRUE;

	hr = d3d11.device->CreateRasterizerState(&ras_desc, &d3d11.resource.render2d.rasterizer);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create rasterizer");
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC attrs[] = {
		{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	hr = d3d11.device->CreateInputLayout(attrs, kArrayCount(attrs), vs.data, vs.count, &d3d11.resource.render2d.input);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create input layout");
		return false;
	}

	hr = d3d11.device->CreateVertexShader(vs.data, vs.count, 0, &d3d11.resource.render2d.vs);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create vertex shader");
		return false;
	}

	hr = d3d11.device->CreatePixelShader(ps.data, ps.count, 0, &d3d11.resource.render2d.ps);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create pixel shader");
		return false;
	}

	return true;
}

static void kD3D11_DestroyPostProcessResources(void)
{
	kRelease(&d3d11.resource.postprocess.vs);
	for (int i = 0; i < kD3D11_PostProcessPs_Count; ++i)
	{
		kRelease(&d3d11.resource.postprocess.ps[i]);
	}
}

static bool kD3D11_CreatePostProcessResources(void)
{
	HRESULT hr = d3d11.device->CreateVertexShader(kRectVS, kArrayCount(kRectVS), 0, &d3d11.resource.postprocess.vs);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create post processing vertex shader");
		return false;
	}

	hr = d3d11.device->CreatePixelShader(kRectPS, kArrayCount(kRectPS), 0,
	                                     &d3d11.resource.postprocess.ps[kD3D11_PostProcessPs_Rect]);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create post processing pixel shader");
		return false;
	}

	return true;
}

static void kD3D11_DestroyRenderResources(void)
{
	kD3D11_DestroyPostProcessResources();
	kD3D11_DestroyRenderResources2D();

	{
		kD3D11_TexturePool *pool = &d3d11.resource.textures;
		while (!kDListIsEmpty(&pool->first))
		{
			kD3D11_Texture *r = kDListPopFront(&pool->first);
			kD3D11_FreeTexture(r);
		}

		kD3D11_TextureBlock *prev  = nullptr;
		kD3D11_TextureBlock *block = pool->blocks;

		while (block)
		{
			prev  = block;
			block = prev->next;
			kFree(prev, sizeof(*prev));
		}
	}

	for (int i = 0; i < kTextureFilter_Count; ++i)
	{
		kRelease(&d3d11.resource.samplers[i]);
	}

	kRelease(&d3d11.resource.depth);

	for (int i = 0; i < kBlendMode_Count; ++i)
	{
		kRelease(&d3d11.resource.blends[i]);
	}
}

static bool kD3D11_CreateRenderResources(void)
{
	for (int i = 0; i < kTextureFilter_Count; ++i)
	{
		if (d3d11.resource.samplers[i]) continue;

		D3D11_SAMPLER_DESC sampler = {};
		sampler.Filter             = kD3D11_FilterMap[i];
		sampler.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.MipLODBias         = 0.0f;
		sampler.MaxAnisotropy      = 1;
		sampler.ComparisonFunc     = D3D11_COMPARISON_NEVER;
		sampler.MinLOD             = -FLT_MAX;
		sampler.MaxLOD             = FLT_MAX;

		HRESULT hr                 = d3d11.device->CreateSamplerState(&sampler, &d3d11.resource.samplers[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create sampler");
			return false;
		}
	}

	if (!d3d11.resource.depth)
	{
		D3D11_DEPTH_STENCIL_DESC depth = {};
		depth.DepthEnable              = TRUE;
		depth.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
		depth.DepthFunc                = D3D11_COMPARISON_LESS_EQUAL;

		HRESULT hr                     = d3d11.device->CreateDepthStencilState(&depth, &d3d11.resource.depth);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create depth stencil state");
			return false;
		}
	}

	if (!d3d11.resource.blends[kBlendMode_None])
	{
		D3D11_BLEND_DESC blend                      = {};
		blend.RenderTarget[0].RenderTargetWriteMask = 0xf;

		HRESULT hr = d3d11.device->CreateBlendState(&blend, &d3d11.resource.blends[kBlendMode_None]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create blend state");
			return false;
		}
	}

	if (!d3d11.resource.blends[kBlendMode_Alpha])
	{
		D3D11_BLEND_DESC blend                      = {};
		blend.RenderTarget[0].BlendEnable           = TRUE;
		blend.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
		blend.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
		blend.RenderTarget[0].RenderTargetWriteMask = 0xf;

		HRESULT hr = d3d11.device->CreateBlendState(&blend, &d3d11.resource.blends[kBlendMode_Alpha]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create alpha blend state");
			return false;
		}
	}

	if (!kD3D11_CreateRenderResources2D()) return false;
	if (!kD3D11_CreatePostProcessResources()) return false;

	return true;
}

//
// Device/Context
//

static constexpr D3D_FEATURE_LEVEL FeatureLevels[] = {
	D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,  D3D_FEATURE_LEVEL_9_1,
};

static IDXGIAdapter1 *kD3D11_FindAdapter(IDXGIFactory2 *factory, UINT flags)
{
	IDXGIAdapter1 *adapter              = nullptr;
	size_t         max_dedicated_memory = 0;
	IDXGIAdapter1 *adapter_it           = 0;

	u32            it_index             = 0;
	while (factory->EnumAdapters1(it_index, &adapter_it) != DXGI_ERROR_NOT_FOUND)
	{
		it_index += 1;

		DXGI_ADAPTER_DESC1 desc;
		adapter_it->GetDesc1(&desc);

		HRESULT hr = D3D11CreateDevice(adapter_it, D3D_DRIVER_TYPE_UNKNOWN, 0, flags, FeatureLevels,
		                               kArrayCount(FeatureLevels), D3D11_SDK_VERSION, NULL, NULL, NULL);

		if (SUCCEEDED(hr) && desc.DedicatedVideoMemory > max_dedicated_memory)
		{
			max_dedicated_memory = desc.DedicatedVideoMemory;
			adapter              = adapter_it;
		}
		else
		{
			adapter_it->Release();
		}
	}

	return adapter;
}

static void kD3D11_DestroyGraphicsDevice(void)
{
	kD3D11_DestroyRenderResources();

	kRelease(&d3d11.device);
	kRelease(&d3d11.device_context);
	kRelease(&d3d11.factory);

	memset(&d3d11, 0, sizeof(d3d11));
}

static bool kD3D11_CreateGraphicsDevice(void)
{
	IDXGIAdapter1 *adapter      = nullptr;
	UINT           device_flags = 0;
	UINT           flags        = 0;

	for (int i = 0; i < 2; ++i)
	{
		if (EnableDebugLayer) flags |= DXGI_CREATE_FACTORY_DEBUG;

		if (!d3d11.factory)
		{
			HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&d3d11.factory));
			if (FAILED(hr))
			{
				hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory));
				if (FAILED(hr))
				{
					kLogHresultError(hr, "DirectX11", "Failed to create factory");
					return false;
				}
			}
		}

		device_flags = 0;
		if (EnableDebugLayer) device_flags |= D3D11_CREATE_DEVICE_DEBUG;

		adapter = kD3D11_FindAdapter(d3d11.factory, device_flags);

		if (EnableDebugLayer && !adapter)
		{
			EnableDebugLayer = false;
			kLogWarning("DirectX11: Debug Layer SDK not present. Falling back to adaptor without debug layer\n");
		}
	}

	if (adapter == nullptr)
	{
		kLogError("DirectX11: Supported adaptor not found!\n");
		return false;
	}

	kDefer
	{
		if (adapter) adapter->Release();
	};

	{
		ID3D11Device *device = nullptr;
		HRESULT       hr     = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, device_flags, FeatureLevels,
		                                         kArrayCount(FeatureLevels), D3D11_SDK_VERSION, &device, nullptr, nullptr);
		kAssert(hr != E_INVALIDARG);

		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create device");
			return false;
		}

		hr = device->QueryInterface(IID_PPV_ARGS(&d3d11.device));
		kAssert(SUCCEEDED(hr));
		device->Release();
	}

	if (device_flags & D3D11_CREATE_DEVICE_DEBUG)
	{
		ID3D11Debug *debug = 0;

		if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&debug))))
		{
			ID3D11InfoQueue *info_queue = nullptr;
			if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&info_queue))))
			{
				kLogTrace("DirectX11: ID3D11Debug enabled.");
				info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
				info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
				info_queue->Release();
			}
		}
		debug->Release();
	}

	d3d11.device->GetImmediateContext1(&d3d11.device_context);

	if (!kD3D11_CreateRenderResources())
	{
		kD3D11_DestroyGraphicsDevice();
		return false;
	}

	return true;
}

//
// Backend
//

static bool kD3D11_CreateRenderBackend(kRenderBackend *backend)
{
	if (!kD3D11_CreateGraphicsDevice()) return false;

	kDListInit(&d3d11.resource.textures.first);
	kDListInit(&d3d11.resource.swap_chains);

	backend->CreateSwapChain       = kD3D11_CreateSwapChain;
	backend->DestroySwapChain      = kD3D11_DestroySwapChain;
	backend->ResizeSwapChain       = kD3D11_ResizeSwapChainBuffers;
	backend->Present               = kD3D11_Present;
	backend->SwapChainRenderTarget = kD3D11_GetSwapChainRenderTarget;

	backend->CreateTexture         = kD3D11_CreateTexture;
	backend->DestroyTexture        = kD3D11_DestroyTexture;
	backend->GetTextureSize        = kD3D11_GetTextureSize;
	backend->ResizeTexture         = kD3D11_ResizeTexture;

	backend->ExecuteCommands       = kD3D11_ExecuteCommands;
	backend->Destroy               = kD3D11_DestroyGraphicsDevice;

	return true;
}

#endif

//
//
//

void kFallbackRenderBackend(kRenderBackend *backend)
{
	backend->CreateSwapChain       = kCreateSwapChainFallback;
	backend->DestroySwapChain      = kDestroySwapChainFallback;
	backend->ResizeSwapChain       = kResizeSwapChainFallback;
	backend->SwapChainRenderTarget = kSwapChainRenderTargetFallback;
	backend->Present               = kPresentFallback;

	backend->CreateTexture         = kCreateTextureFallback;
	backend->DestroyTexture        = kDestroyTextureFallback;
	backend->GetTextureSize        = kGetTextureSizeFallback;
	backend->ResizeTexture         = kResizeTextureFallback;
	backend->ExecuteCommands       = kExecuteCommandsFallback;
	backend->Destroy               = kDestroyFallback;
}

void kCreateRenderBackend(kRenderBackend *backend)
{
	if (kD3D11_CreateRenderBackend(backend)) return;
	kFallbackRenderBackend(backend);
}
