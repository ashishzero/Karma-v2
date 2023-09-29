#include "kRenderApi.h"
#include "kLinkedList.h"

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
static bool EnableDebugLayer = true;
#else
static bool EnableDebugLayer = false;
#endif

typedef enum kResourceState
{
	kResourceState_Error = -1,
	kResourceState_Unready,
	kResourceState_Ready,
} kResourceState;

//
//
//

static kSwapChain kSwapChainCreateFallback(void *)
{
	return nullptr;
}
static void kSwapChainDestroyFallback(kSwapChain)
{}
static void kSwapChainResizeFallback(kSwapChain, uint, uint)
{}
static kTexture kSwapChainRenderTargetFallback(kSwapChain)
{
	return nullptr;
}
static void kSwapChainPresentFallback(kSwapChain)
{}

static kSwapChain kRenderBackendGetWindowSwapChainFallback(void)
{
	return nullptr;
}
static kTexture kBackendSwapChainTargetFallback(kSwapChain)
{
	return nullptr;
}
static kTexture kBackendCreateTextureFallback(const kTextureSpec &)
{
	return nullptr;
}
static void kBackendDestroyTextureFallback(kTexture)
{}
static void kBackendTextureSizeFallback(kTexture, u32 *, u32 *)
{}
static void kBackendResizeTextureFallback(kTexture, u32, u32)
{}
static void kBackendDestroyFallback(void)
{}

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

constexpr uint K_BACK_BUFFER_COUNT = 3;
constexpr uint K_TEXTURE_MAX_ALLOC = 16;

typedef struct kPlatformTexture
{
	kResourceState			   state;
	u32						   width;
	u32						   height;
	ID3D11ShaderResourceView  *shader_resource_view;
	ID3D11RenderTargetView	  *render_target_view;
	ID3D11DepthStencilView	  *depth_stencil_view;
	ID3D11UnorderedAccessView *unordered_access_view;
	ID3D11Texture2D			  *texture2d;
	D3D11_TEXTURE2D_DESC	   desc;
	kPlatformTexture		  *prev;
	kPlatformTexture		  *next;
} kPlatformTexture;

typedef struct kPlatformSwapChain
{
	IDXGISwapChain1 *native;
	kPlatformTexture texture;
} kPlatformSwapChain;

typedef struct kPlatformTexturePool
{
	kPlatformTexture  items;
	kPlatformTexture *free;
} kPlatformTexturePool;

typedef struct kPlatformBuffer2D
{
	ID3D11Buffer *vertex;
	ID3D11Buffer *index;
	ID3D11Buffer *constant;
	uint		  vertex_size;
	uint		  index_size;
} kPlatformBuffer2D;

typedef struct kPlatformRenderResource
{
	kPlatformTexturePool textures;
	kPlatformBuffer2D	 buffer2d;
} kPlatformRenderContext;

typedef struct kPlatformRenderBackend
{
	ID3D11Device1		   *device;
	ID3D11DeviceContext1   *device_context;
	IDXGIFactory2		   *factory;
	kPlatformRenderResource resource;
} kPlatformRenderBackend;

static kPlatformRenderBackend d3d11;

//
//
//

template <typename T> static void kRelease(T **o)
{
	if (*o)
		(*o)->Release();
	*o = nullptr;
}

//
// Device/Context
//

static constexpr D3D_FEATURE_LEVEL FeatureLevels[] = {
	D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,	D3D_FEATURE_LEVEL_9_2,	D3D_FEATURE_LEVEL_9_1,
};

static IDXGIAdapter1 *kD3D11_FindAdapter(IDXGIFactory2 *factory, UINT flags)
{
	IDXGIAdapter1 *adapter				= nullptr;
	size_t		   max_dedicated_memory = 0;
	IDXGIAdapter1 *adapter_it			= 0;

	u32			   it_index				= 0;
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
			adapter				 = adapter_it;
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
	kRelease(&d3d11.device);
	kRelease(&d3d11.device_context);
	kRelease(&d3d11.factory);

	memset(&d3d11, 0, sizeof(d3d11));
}

static void kD3D11_Flush(void)
{
	d3d11.device_context->Flush();
	d3d11.device_context->ClearState();
}

static bool kD3D11_CreateGraphicsDevice(void)
{
	UINT flags = 0;

	if (EnableDebugLayer)
		flags |= DXGI_CREATE_FACTORY_DEBUG;

	if (!d3d11.factory)
	{
		HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&d3d11.factory));
		if (FAILED(hr))
		{
			hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory));
			if (FAILED(hr))
			{
				kWinLogError(hr, "DirectX11", "Failed to create factory");
				return false;
			}
		}
	}

	UINT device_flags = 0;
	if (EnableDebugLayer)
		device_flags |= D3D11_CREATE_DEVICE_DEBUG;

	IDXGIAdapter1 *adapter = kD3D11_FindAdapter(d3d11.factory, device_flags);

	if (adapter == nullptr)
	{
		kLogError("DirectX11: Supported adaptor not found!");
		return false;
	}

	kDefer
	{
		if (adapter)
			adapter->Release();
	};

	{
		ID3D11Device *device = nullptr;
		HRESULT		  hr	 = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, device_flags, FeatureLevels,
												 kArrayCount(FeatureLevels), D3D11_SDK_VERSION, &device, nullptr, nullptr);
		kAssert(hr != E_INVALIDARG);

		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to create device");
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

	return true;
}

//
// SwapChain
//

static void kD3D11_DestroySwapChainBuffers(kSwapChain swap_chain)
{
	kPlatformSwapChain *r = swap_chain.resource;
	kRelease(&r->texture.render_target_view);
	kRelease(&r->texture.texture2d);
	memset(&r->texture, 0, sizeof(r->texture));
}

static void kD3D11_CreateSwapChainBuffers(kSwapChain swap_chain)
{
	kPlatformSwapChain *r  = swap_chain.resource;

	HRESULT				hr = r->native->GetBuffer(0, IID_PPV_ARGS(&r->texture.texture2d));

	if (FAILED(hr))
	{
		kWinLogError(hr, "DirectX11", "Failed to get buffer from swapchain");
		return;
	}

	r->texture.texture2d->GetDesc(&r->texture.desc);

	r->texture.width  = r->texture.desc.Width;
	r->texture.height = r->texture.desc.Height;

	hr				  = d3d11.device->CreateRenderTargetView(r->texture.texture2d, 0, &r->texture.render_target_view);

	if (FAILED(hr))
	{
		kWinLogError(hr, "DirectX11", "Failed to create render target view");
	}
}

static void kD3D11_ResizeSwapChainBuffers(kSwapChain swap_chain, uint w, uint h)
{
	if (!w || !h)
		return;

	kD3D11_Flush();

	kPlatformSwapChain *r = swap_chain.resource;
	kD3D11_DestroySwapChainBuffers(swap_chain);
	r->native->ResizeBuffers(K_BACK_BUFFER_COUNT, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	kD3D11_CreateSwapChainBuffers(swap_chain);
}

static void kD3D11_DestroySwapChain(kSwapChain swap_chain)
{
	kD3D11_Flush();

	kPlatformSwapChain *r = swap_chain.resource;
	kD3D11_DestroySwapChainBuffers(swap_chain);
	kRelease(&r->native);
	kFree(r, sizeof(*r));
}

static kSwapChain kD3D11_CreateSwapChain(void *_window)
{
	HWND				  wnd			  = (HWND)_window;

	DXGI_SWAP_EFFECT	  swap_effect	  = IsWindows10OrGreater()	? DXGI_SWAP_EFFECT_FLIP_DISCARD
											: IsWindows8OrGreater() ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
																	: DXGI_SWAP_EFFECT_DISCARD;

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.Width				  = 0;
	swap_chain_desc.Height				  = 0;
	swap_chain_desc.Format				  = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.SampleDesc.Count	  = 1;
	swap_chain_desc.SampleDesc.Quality	  = 0;
	swap_chain_desc.BufferUsage			  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount			  = K_BACK_BUFFER_COUNT;
	swap_chain_desc.SwapEffect			  = swap_effect;
	swap_chain_desc.AlphaMode			  = DXGI_ALPHA_MODE_IGNORE;

	IDXGISwapChain1 *swapchain1			  = nullptr;
	HRESULT			 hr = d3d11.factory->CreateSwapChainForHwnd(d3d11.device, wnd, &swap_chain_desc, 0, 0, &swapchain1);
	kAssert(hr != DXGI_ERROR_INVALID_CALL);

	if (FAILED(hr))
	{
		kWinLogError(hr, "DirectX11", "Failed to create swap chain");
		return nullptr;
	}

	kPlatformSwapChain *r = (kPlatformSwapChain *)kAlloc(sizeof(kPlatformSwapChain));

	if (!r)
	{
		kLogError("DirectX12: Failed RemoveAllocate memory for create swap chain");
		swapchain1->Release();
		return nullptr;
	}

	memset(r, 0, sizeof(*r));

	r->native = swapchain1;
	kD3D11_CreateSwapChainBuffers(r);

	return r;
}

static void kD3D11_Present(kSwapChain swap_chain)
{
	kPlatformSwapChain *r = swap_chain.resource;
	r->native->Present(1, 0);
}

static kTexture kD3D11_GetSwapChainRenderTarget(kSwapChain swap_chain)
{
	kPlatformSwapChain *r = swap_chain.resource;
	return &r->texture;
}

//
// Mappings
//

static constexpr DXGI_FORMAT kD3D11_FormatMap[] = {
	DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM,	   DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R32G32B32_FLOAT,	DXGI_FORMAT_R11G11B10_FLOAT,   DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R32G32B32_UINT,		DXGI_FORMAT_R32G32_FLOAT,	   DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R32G32_UINT,		DXGI_FORMAT_R8G8_UNORM,		   DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_SINT,			DXGI_FORMAT_R32_UINT,		   DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R8_UNORM,			DXGI_FORMAT_D32_FLOAT,		   DXGI_FORMAT_D16_UNORM,
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

//
// Textures
//

static kPlatformTexture *kD3D11_AllocTexture(void)
{
	if (!d3d11.resource.textures.free)
	{
		kPlatformTexture *items = (kPlatformTexture *)kAlloc(sizeof(kPlatformTexture) * K_TEXTURE_MAX_ALLOC);
		if (!items)
			return nullptr;

		memset(items, 0, sizeof(kPlatformTexture) * K_TEXTURE_MAX_ALLOC);

		for (int i = 0; i < K_TEXTURE_MAX_ALLOC - 1; ++i)
		{
			items[i].next = &items[i + 1];
		}

		d3d11.resource.textures.free = items;
	}

	kPlatformTexture *r			 = d3d11.resource.textures.free;
	d3d11.resource.textures.free = r->next;

	kDoublyLinkedListPushBack(&d3d11.resource.textures.items, r);

	return r;
}

static void kD3D11_FreeTexture(kPlatformTexture *r)
{
	kDoublyLinkedListRemove(r);

	memset(r, 0, sizeof(*r));

	r->next						 = d3d11.resource.textures.free;
	d3d11.resource.textures.free = r;
}

static void kD3D11_PrepareTexture(kPlatformTexture *texture, D3D11_SUBRESOURCE_DATA *data)
{
	D3D11_TEXTURE2D_DESC &desc = texture->desc;
	texture->state			   = kResourceState_Unready;

	HRESULT hr				   = d3d11.device->CreateTexture2D(&desc, data, &texture->texture2d);

	if (FAILED(hr))
	{
		kWinLogError(hr, "DirectX11", "Failed to create texture");
		texture->state = kResourceState_Error;
		return;
	}

	if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		hr = d3d11.device->CreateShaderResourceView(texture->texture2d, nullptr, &texture->shader_resource_view);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to create shader resource view");
		}
	}

	if (desc.BindFlags & D3D11_BIND_RENDER_TARGET)
	{
		hr = d3d11.device->CreateRenderTargetView(texture->texture2d, nullptr, &texture->render_target_view);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to create render target view");
		}
	}

	if (desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		hr = d3d11.device->CreateDepthStencilView(texture->texture2d, nullptr, &texture->depth_stencil_view);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to create depth target view");
		}
	}

	if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		hr = d3d11.device->CreateUnorderedAccessView(texture->texture2d, nullptr, &texture->unordered_access_view);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to create unordered access view");
		}
	}

	texture->width	= desc.Width;
	texture->height = desc.Height;
	texture->state	= kResourceState_Ready;
}

static kTexture kD3D11_CreateTexture(const kTextureSpec &spec)
{
	kPlatformTexture *r = kD3D11_AllocTexture();

	if (!r)
	{
		kLogError("DirectX11: Failed to allocate memory for texture");
		return nullptr;
	}

	r->desc.Width	  = spec.width;
	r->desc.Height	  = spec.height;
	r->desc.Format	  = kD3D11_FormatMap[spec.format];
	r->desc.ArraySize = 1;

	r->desc.BindFlags = 0;
	if (spec.bind_flags & kBind_VertexBuffer)
		r->desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
	if (spec.bind_flags & kBind_IndexBuffer)
		r->desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
	if (spec.bind_flags & kBind_ConstantBuffer)
		r->desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
	if (spec.bind_flags & kBind_ShaderResource)
		r->desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if (spec.bind_flags & kBind_RenderTarget)
		r->desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	if (spec.bind_flags & kBind_DepthStencil)
		r->desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	if (spec.bind_flags & kBind_UnorderedAccess)
		r->desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

	r->desc.Usage			   = kD3D11_UsageMap[spec.usage];
	r->desc.SampleDesc.Count   = kMax(spec.num_samples, 1u);
	r->desc.SampleDesc.Quality = 0;
	r->desc.CPUAccessFlags	   = 0;
	r->desc.MiscFlags		   = 0;
	r->desc.MipLevels		   = 1;

	D3D11_SUBRESOURCE_DATA source;
	source.pSysMem			= spec.pixels;
	source.SysMemPitch		= spec.pitch;
	source.SysMemSlicePitch = 0;

	kD3D11_PrepareTexture(r, spec.pixels ? &source : nullptr);

	return r;
}

static void kD3D11_UnprepareTexture(kPlatformTexture *r)
{
	kRelease(&r->render_target_view);
	kRelease(&r->depth_stencil_view);
	kRelease(&r->shader_resource_view);
	kRelease(&r->unordered_access_view);
	kRelease(&r->texture2d);

	r->width  = 0;
	r->height = 0;
	r->state  = kResourceState_Unready;
}

static void kD3D11_DestroyTexture(kTexture texture)
{
	kPlatformTexture *r = texture.resource;
	kD3D11_UnprepareTexture(r);
	kD3D11_FreeTexture(r);
}

static void kD3D11_GetTextureSize(kTexture texture, u32 *width, u32 *height)
{
	kPlatformTexture *r = texture.resource;
	*width				= r->width;
	*height				= r->height;
}

static void kD3D11_ResizeTexture(kTexture texture, u32 w, u32 h)
{
	kPlatformTexture *r = texture.resource;

	if (w == 0 || h == 0)
		return;

	if (w == r->width && h == r->height)
		return;

	kD3D11_UnprepareTexture(r);

	r->desc.Width  = w;
	r->desc.Height = h;

	kD3D11_PrepareTexture(r, nullptr);
}

//
//
//

static void kD3D11_ExecuteRenderCommands(const kRenderData2D &data)
{
	if (data.vertices.count == 0 || data.indices.count == 0)
		return;

	uint vertex_size = (uint)kArrSizeInBytes(data.vertices);
	uint index_size	 = (uint)kArrSizeInBytes(data.indices);

	if (vertex_size > d3d11.resource.buffer2d.vertex_size)
	{
		kRelease(&d3d11.resource.buffer2d.vertex);

		D3D11_BUFFER_DESC vertex_buff_desc = {};
		vertex_buff_desc.ByteWidth		   = vertex_size;
		vertex_buff_desc.Usage			   = D3D11_USAGE_DYNAMIC;
		vertex_buff_desc.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;
		vertex_buff_desc.CPUAccessFlags	   = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = d3d11.device->CreateBuffer(&vertex_buff_desc, 0, &d3d11.resource.buffer2d.vertex);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to allocate vertex buffer for render context 2d");
			return;
		}
	}

	if (index_size > d3d11.resource.buffer2d.index_size)
	{
		kRelease(&d3d11.resource.buffer2d.index);

		D3D11_BUFFER_DESC index_buff_desc = {};
		index_buff_desc.ByteWidth		  = index_size;
		index_buff_desc.Usage			  = D3D11_USAGE_DYNAMIC;
		index_buff_desc.BindFlags		  = D3D11_BIND_INDEX_BUFFER;
		index_buff_desc.CPUAccessFlags	  = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = d3d11.device->CreateBuffer(&index_buff_desc, 0, &d3d11.resource.buffer2d.index);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to allocate index buffer for render context 2d");
			return;
		}
	}

	if (!d3d11.resource.buffer2d.constant)
	{
		D3D11_BUFFER_DESC const_buff_desc = {};
		const_buff_desc.ByteWidth		  = sizeof(kMat4);
		const_buff_desc.Usage			  = D3D11_USAGE_DEFAULT;
		const_buff_desc.BindFlags		  = D3D11_BIND_CONSTANT_BUFFER;
		const_buff_desc.CPUAccessFlags	  = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = d3d11.device->CreateBuffer(&const_buff_desc, 0, &d3d11.resource.buffer2d.constant);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to allocate constant buffer for render context 2d");
			return;
		}
	}

	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = d3d11.device_context->Map(d3d11.resource.buffer2d.vertex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to map vertex2d buffer");
			return;
		}

		memcpy(mapped.pData, data.vertices.data, vertex_size);

		d3d11.device_context->Unmap(d3d11.resource.buffer2d.vertex, 0);
	}

	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = d3d11.device_context->Map(d3d11.resource.buffer2d.index, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to map index2d buffer");
			return;
		}

		memcpy(mapped.pData, data.indices.data, index_size);

		d3d11.device_context->Unmap(d3d11.resource.buffer2d.index, 0);
	}

	ID3D11DeviceContext1 *dc = d3d11.device_context;
	ID3D11Buffer		 *cb = d3d11.resource.buffer2d.constant;

	/*-------------------------------------------------------------- */

	UINT		stride = sizeof(kVertex2D);
	UINT		offset = 0;
	DXGI_FORMAT format = sizeof(kIndex2D) == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	dc->IASetVertexBuffers(0, 1, &d3d11.resource.buffer2d.vertex, &stride, &offset);
	dc->IASetIndexBuffer(d3d11.resource.buffer2d.index, format, 0);

	/*-------------------------------------------------------------- */

	for (const kRenderPass2D &pass : data.passes)
	{
		kPlatformTexture *render_target = pass.render_target.resource;

		if (render_target->state != kResourceState_Ready)
			continue;

		if (pass.flags & kRenderPass_ClearColor)
		{
			dc->ClearRenderTargetView(pass.render_target.resource->render_target_view, pass.clear.color.m);
		}

		kPlatformTexture *depth_stencil = pass.depth_stencil.resource;

		if (depth_stencil && depth_stencil->state == kResourceState_Ready)
		{
			UINT clear_flags = 0;
			if (pass.flags & kRenderPass_ClearDepth)
				clear_flags |= D3D11_CLEAR_DEPTH;
			if (pass.flags & kRenderPass_ClearStencil)
				clear_flags |= D3D11_CLEAR_STENCIL;

			if (clear_flags)
			{
				dc->ClearDepthStencilView(depth_stencil->depth_stencil_view, clear_flags, pass.clear.depth,
										  pass.clear.stencil);
			}
		}

		dc->OMSetRenderTargets(1, &render_target->render_target_view, 0);

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = pass.viewport.x;
		viewport.TopLeftY = render_target->height - pass.viewport.y;
		viewport.Width	  = pass.viewport.w;
		viewport.Height	  = pass.viewport.h;
		viewport.MinDepth = pass.viewport.n;
		viewport.MaxDepth = pass.viewport.f;

		dc->RSSetViewports(1, &viewport);

		for (const kRenderCommand2D &cmd : pass.commands)
		{
			kUnimplemented(); // TODO: something with shader

			ID3D11ShaderResourceView *ps_resources[2];
			D3D11_MAPPED_SUBRESOURCE  mapped;

			for (const kRenderParam2D &param : cmd.params)
			{
				HRESULT hr = dc->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
				if (FAILED(hr))
				{
					kWinLogError(hr, "DirectX11", "Failed to map constant buffer for render context 2d");
					continue;
				}

				memcpy(mapped.pData, param.transform.m, sizeof(kMat4));

				dc->Unmap(cb, 0);

				/*-------------------------------------------------------------- */

				kPlatformTexture *r1 = param.textures[0].resource;
				kPlatformTexture *r2 = param.textures[1].resource;

				if (r1->state != kResourceState_Ready || r2->state != kResourceState_Ready)
					continue;

				ps_resources[0] = r1->shader_resource_view;
				ps_resources[1] = r2->shader_resource_view;

				dc->PSSetShaderResources(0, kArrayCount(ps_resources), ps_resources);

				/*-------------------------------------------------------------- */

				D3D11_RECT rect;
				rect.left	= (LONG)(param.rect.min.x);
				rect.top	= (LONG)(render_target->height - param.rect.max.y);
				rect.right	= (LONG)(param.rect.max.x);
				rect.bottom = (LONG)(render_target->height - param.rect.min.y);

				dc->RSSetScissorRects(1, &rect);

				/*-------------------------------------------------------------- */

				dc->DrawIndexed(param.count, param.index, param.vertex);
			}
		}

		dc->PSSetShaderResources(0, 0, nullptr);
	}

	dc->ClearState();
}

//
//
//

bool kD3D11_CreateRenderBackend(kRenderBackend *backend, kSwapChainBackend *swap_chain)
{
	if (!kD3D11_CreateGraphicsDevice())
		return false;

	kInitDoublyLinkedList(&d3d11.resource.textures.items);

	backend->window_swap_chain = kRenderBackendGetWindowSwapChainFallback;
	backend->swap_chain_target = kD3D11_GetSwapChainRenderTarget;
	backend->create_texture	   = kD3D11_CreateTexture;
	backend->destroy_texture   = kD3D11_DestroyTexture;
	backend->texture_size	   = kD3D11_GetTextureSize;
	backend->resize_texture	   = kD3D11_ResizeTexture;
	backend->destroy		   = kD3D11_DestroyGraphicsDevice;

	swap_chain->create		   = kD3D11_CreateSwapChain;
	swap_chain->destroy		   = kD3D11_DestroySwapChain;
	swap_chain->resize		   = kD3D11_ResizeSwapChainBuffers;
	swap_chain->present		   = kD3D11_Present;
	swap_chain->render_target  = kD3D11_GetSwapChainRenderTarget;

	return true;
}

#endif

//
//
//

void kCreateRenderBackend(kRenderBackend *backend, kSwapChainBackend *swap_chain)
{
	if (kD3D11_CreateRenderBackend(backend, swap_chain))
		return;

	swap_chain->create		   = kSwapChainCreateFallback;
	swap_chain->destroy		   = kSwapChainDestroyFallback;
	swap_chain->resize		   = kSwapChainResizeFallback;
	swap_chain->render_target  = kSwapChainRenderTargetFallback;
	swap_chain->present		   = kSwapChainPresentFallback;

	backend->window_swap_chain = kRenderBackendGetWindowSwapChainFallback;
	backend->swap_chain_target = kBackendSwapChainTargetFallback;
	backend->create_texture	   = kBackendCreateTextureFallback;
	backend->destroy_texture   = kBackendDestroyTextureFallback;
	backend->texture_size	   = kBackendTextureSizeFallback;
	backend->resize_texture	   = kBackendResizeTextureFallback;
	backend->destroy		   = kBackendDestroyFallback;
}
