#include "kRenderBackend.h"
#include "kPoolAllocator.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <d3d12.h>
#include <dxgi1_5.h>
#include <VersionHelpers.h>

#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "DXGI.lib")

static constexpr uint K_FRAME_COUNT       = 2;
static constexpr uint K_BACK_BUFFER_COUNT = 3;

struct kD3D12_Frame
{
	UINT                       index;
	HANDLE                     event;
	ID3D12Fence               *fence;
	UINT64                     values[K_FRAME_COUNT];
	ID3D12CommandAllocator    *allocators[K_FRAME_COUNT];
	ID3D12GraphicsCommandList *commands;
	ID3D12CommandQueue        *queue;
};

struct kD3D12_Upload
{
	ID3D12Fence               *fence;
	HANDLE                     event;
	UINT64                     counter;
	ID3D12CommandAllocator    *allocator;
	ID3D12GraphicsCommandList *commands;
	ID3D12CommandQueue        *queue;
};

struct kD3D12_Texture : kTexture
{
	ID3D12Resource *resource;
};

using kD3D12_TexturePool = kMemoryPool<kD3D12_Texture>;

struct kD3D12_SwapChain : kSwapChain
{
	uint                        index;
	IDXGISwapChain3            *native;
	ID3D12DescriptorHeap       *descriptor;
	kD3D12_Texture              targets[K_BACK_BUFFER_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE rtv[K_BACK_BUFFER_COUNT];
	kD3D12_SwapChain           *prev;
	kD3D12_SwapChain           *next;
};

struct kD3D12_Render2D
{
	uint                 vertex_sz;
	uint                 index_sz;
	ID3D12Resource      *vertex[K_FRAME_COUNT];
	ID3D12Resource      *index[K_FRAME_COUNT];
	ID3D12RootSignature *root_sig;
	ID3D12PipelineState *pipelines[kBlendMode_Count];
};

struct kD3D12_Resource
{
	kD3D12_SwapChain   swap_chains;
	kD3D12_TexturePool textures;
	kD3D12_Render2D    render2d;
};

struct kD3D12_Backend
{
	IDXGIFactory2  *factory;
	ID3D12Device   *device;
	UINT            rtv_stride;
	kD3D12_Frame    frame;
	kD3D12_Upload   upload;
	kD3D12_Resource resource;
};

static kD3D12_Backend d3d12;

template <typename T>
static void kRelease(T **o)
{
	if (*o) (*o)->Release();
	*o = nullptr;
}

//
// Mappings
//

static constexpr DXGI_FORMAT kDXGI_FormatMap[] = {
	DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM,    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_R11G11B10_FLOAT,   DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R32G32B32_UINT,     DXGI_FORMAT_R32G32_FLOAT,      DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R32G32_UINT,        DXGI_FORMAT_R8G8_UNORM,        DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_SINT,           DXGI_FORMAT_R32_UINT,          DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R8_UNORM,           DXGI_FORMAT_D32_FLOAT,         DXGI_FORMAT_D16_UNORM,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
};
static_assert(kArrayCount(kDXGI_FormatMap) == kFormat_Count, "");

//
//
//

static IDXGIAdapter1 *kD3D12_FindAdapter(IDXGIFactory2 *factory)
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

		HRESULT hr = D3D12CreateDevice(adapter_it, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device2), 0);

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

static void kD3D12_WaitForGpu(void)
{
	HRESULT hr = d3d12.frame.queue->Signal(d3d12.frame.fence, d3d12.frame.values[d3d12.frame.index]);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "Direct3D12", "Failed to signal fence");
		return;
	}
	d3d12.frame.fence->SetEventOnCompletion(d3d12.frame.values[d3d12.frame.index], d3d12.frame.event);
	WaitForSingleObjectEx(d3d12.frame.event, INFINITE, FALSE);
	d3d12.frame.values[d3d12.frame.index] += 1;
}

static void kD3D12_MoveToNextFrame(void)
{
	UINT64 value = d3d12.frame.values[d3d12.frame.index];

	d3d12.frame.queue->Signal(d3d12.frame.fence, value);

	d3d12.frame.index = (d3d12.frame.index + 1) % K_FRAME_COUNT;

	if (d3d12.frame.fence->GetCompletedValue() < d3d12.frame.values[d3d12.frame.index])
	{
		d3d12.frame.fence->SetEventOnCompletion(d3d12.frame.values[d3d12.frame.index], d3d12.frame.event);
		WaitForSingleObjectEx(d3d12.frame.event, INFINITE, FALSE);
	}

	d3d12.frame.values[d3d12.frame.index] = value + 1;
}

//
// Textures
//

static kTexture *kD3D12_CreateTexture(const kTextureSpec &spec)
{
	kD3D12_Texture *r = d3d12.resource.textures.Alloc();

	if (!r)
	{
		kLogError("Direct3D11: Failed to allocate memory for texture\n");
		return (kD3D12_Texture *)&kFallbackTexture;
	}

	r->state                 = kResourceState_Unready;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment           = 0;
	desc.Width               = spec.width;
	desc.Height              = spec.height;
	desc.DepthOrArraySize    = 1;
	desc.MipLevels           = 1;
	desc.Format              = kDXGI_FormatMap[spec.format];
	desc.SampleDesc.Count    = kMax(spec.num_samples, 1u);
	desc.SampleDesc.Quality  = 0;

	UINT flags               = 0;

	if (spec.flags & kResource_AllowRenderTarget) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (spec.flags & kResource_AllowDepthStencil) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (spec.flags & kResource_AllowUnorderedAccess) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (spec.flags & kResource_DenyShaderResource) flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	desc.Flags                 = (D3D12_RESOURCE_FLAGS)flags;

	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type                  = D3D12_HEAP_TYPE_DEFAULT;

	HRESULT hr                 = d3d12.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
	                                                                   D3D12_RESOURCE_STATE_COPY_DEST, 0, IID_PPV_ARGS(&r->resource));

	if (FAILED(hr))
	{
		r->state = kResourceState_Error;
		kLogHresultError(hr, "Direct3D12", "Failed to create texture");
		return (kD3D12_Texture *)&kFallbackTexture;
	}

	if (spec.pixels)
	{
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;

		UINT64                             req_size = 0;
		d3d12.device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, 0, 0, &req_size);

		D3D12_HEAP_PROPERTIES upload_heap = {};
		upload_heap.Type                  = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC res           = {};
		res.Dimension                     = D3D12_RESOURCE_DIMENSION_BUFFER;
		res.Alignment                     = 0;
		res.Width                         = req_size;
		res.Height                        = 1;
		res.DepthOrArraySize              = 1;
		res.MipLevels                     = 1;
		res.Format                        = DXGI_FORMAT_UNKNOWN;
		res.SampleDesc.Count              = 1;
		res.SampleDesc.Quality            = 0;
		res.Layout                        = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		res.Flags                         = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource *buffer            = nullptr;
		hr = d3d12.device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &res,
		                                           D3D12_RESOURCE_STATE_GENERIC_READ, 0, IID_PPV_ARGS(&buffer));

		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create upload heap");
			return r;
		}

		kDefer { buffer->Release(); };

		u8 *data = nullptr;
		hr       = buffer->Map(0, 0, (void **)&data);

		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to map upload heap");
			return r;
		}

		u8 *src_scan = spec.pixels;
		for (UINT y = 0; y < spec.height; y++)
		{
			UINT8 *dst_scan = data + layout.Offset + y * layout.Footprint.RowPitch;
			memcpy(dst_scan, src_scan, spec.pitch);
			src_scan += spec.pitch;
		}

		buffer->Unmap(0, 0);

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource                   = r->resource;
		dst.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex            = 0;

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource                   = buffer;
		src.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint             = layout;

		D3D12_RESOURCE_BARRIER barrier  = {};
		barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource    = r->resource;
		barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_COMMON;

		d3d12.upload.allocator->Reset();
		d3d12.upload.commands->Reset(d3d12.upload.allocator, 0);
		d3d12.upload.commands->CopyTextureRegion(&dst, 0, 0, 0, &src, 0);
		d3d12.upload.commands->ResourceBarrier(1, &barrier);
		d3d12.upload.commands->Close();

		ID3D12CommandList *commands[] = {d3d12.upload.commands};
		d3d12.upload.queue->ExecuteCommandLists(kArrayCount(commands), commands);
		d3d12.upload.queue->Signal(d3d12.upload.fence, d3d12.upload.counter);
		d3d12.upload.fence->SetEventOnCompletion(d3d12.upload.counter, d3d12.upload.event);
		d3d12.upload.counter += 1;
		WaitForSingleObjectEx(d3d12.upload.event, INFINITE, FALSE);
	}

	return r;
}

static void kD3D12_DestroyTexture(kTexture *texture)
{
	kD3D12_Texture *r = (kD3D12_Texture *)texture;
	if (r->state == kResourceState_Ready)
	{
		kRelease(&r->resource);
		d3d12.resource.textures.Free(r);
	}
}

static kVec2i kD3D12_GetTextureSize(kTexture *texture)
{
	kVec2i          size(0);
	kD3D12_Texture *r = (kD3D12_Texture *)texture;
	if (r->state == kResourceState_Ready)
	{
		D3D12_RESOURCE_DESC desc = r->resource->GetDesc();
		size.x                   = (int)desc.Width;
		size.y                   = (int)desc.Height;
	}
	return size;
}

static void kD3D12_ResizeTexture(kTexture *texture, u32 w, u32 h)
{
	kD3D12_Texture *r = (kD3D12_Texture *)texture;

	if (w == 0 || h == 0) return;

	if (r->state == kResourceState_Ready)
	{
		D3D12_RESOURCE_DESC desc = r->resource->GetDesc();

		if (w == desc.Width && h == desc.Height) return;

		D3D12_HEAP_PROPERTIES heap;
		D3D12_HEAP_FLAGS      flags;

		HRESULT               hr = r->resource->GetHeapProperties(&heap, &flags);
		if (FAILED(hr))
		{
			r->state = kResourceState_Error;
			kLogHresultError(hr, "Direct3D12", "Failed to resize texture");
			return;
		}

		r->state = kResourceState_Unready;
		kRelease(&r->resource);

		desc.Width  = w;
		desc.Height = h;

		hr          = d3d12.device->CreateCommittedResource(&heap, flags, &desc, D3D12_RESOURCE_STATE_COMMON, 0,
		                                                    IID_PPV_ARGS(&r->resource));

		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create texture");
			r->state = kResourceState_Error;
			return;
		}

		r->state = kResourceState_Ready;
	}
}

//
// Swap Chain
//

static void kD3D12_DestroySwapChainBuffers(kD3D12_SwapChain *r)
{
	kLogTrace("Direct3D12: Destroying swap chain render targets.\n");

	for (uint i = 0; i < K_BACK_BUFFER_COUNT; ++i)
	{
		kD3D12_Texture *rt = &r->targets[i];
		kRelease(&rt->resource);
		memset(rt, 0, sizeof(*rt));
	}
}

static void kD3D12_CreateSwapChainBuffers(kD3D12_SwapChain *r)
{
	kLogTrace("Direct3D12: Creating swap chain render targets.\n");

	for (uint i = 0; i < K_BACK_BUFFER_COUNT; ++i)
	{
		kD3D12_Texture *rt = &r->targets[i];
		rt->state          = kResourceState_Unready;
		HRESULT hr         = r->native->GetBuffer(0, IID_PPV_ARGS(&rt->resource));

		if (FAILED(hr))
		{
			rt->state = kResourceState_Error;
			kLogHresultError(hr, "DirectX11", "Failed to create swapchain render target (%u)\n", i);
			continue;
		}

		d3d12.device->CreateRenderTargetView(rt->resource, 0, r->rtv[i]);
		rt->state = kResourceState_Ready;
	}
}

static void kD3D12_ResizeSwapChainBuffers(kSwapChain *swap_chain, uint w, uint h)
{
	if (!w || !h) return;

	kD3D12_WaitForGpu();

	kD3D12_SwapChain *r = (kD3D12_SwapChain *)swap_chain;
	if (r->state == kResourceState_Ready)
	{
		kD3D12_Texture     *rt   = &r->targets[0];
		D3D12_RESOURCE_DESC desc = rt->resource->GetDesc();

		if (desc.Width == w && desc.Height == h)
		{
			return;
		}

		kLogTrace("Direct3D12: Resizing swap chain render targets to %ux%u.\n", w, h);

		kD3D12_DestroySwapChainBuffers(r);
		r->native->ResizeBuffers(K_BACK_BUFFER_COUNT, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		kD3D12_CreateSwapChainBuffers(r);
	}
}

static void kD3D12_DestroySwapChain(kSwapChain *swap_chain)
{
	kLogTrace("Destroying swap chain.\n");

	kD3D12_WaitForGpu();

	kD3D12_SwapChain *r = (kD3D12_SwapChain *)swap_chain;
	if (r->state == kResourceState_Ready)
	{
		kDListRemove(r);
		kD3D12_DestroySwapChainBuffers(r);
		kRelease(&r->descriptor);
		kRelease(&r->native);
		kFree(r, sizeof(*r));
	}
}

static kSwapChain *kD3D12_CreateSwapChain(void *_window, const kRenderTargetConfig &config)
{
	kLogTrace("Direct3D12: Creating swap chain.\n");

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
	HRESULT hr = d3d12.factory->CreateSwapChainForHwnd(d3d12.frame.queue, wnd, &swap_chain_desc, 0, 0, &swapchain1);
	kAssert(hr != DXGI_ERROR_INVALID_CALL);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX12", "Failed to create swap chain");
		return (kSwapChain *)&kFallbackSwapChain;
	}

	IDXGISwapChain3 *swapchain3 = nullptr;
	if (FAILED(swapchain1->QueryInterface(IID_PPV_ARGS(&swapchain3))))
	{
		kLogHresultError(hr, "DirectX12", "Failed to create d3d12 supported swap chain");
		return (kSwapChain *)&kFallbackSwapChain;
	}

	kRelease(&swapchain1);

	D3D12_DESCRIPTOR_HEAP_DESC heap  = {};
	heap.Type                        = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap.NumDescriptors              = K_BACK_BUFFER_COUNT;

	ID3D12DescriptorHeap *descriptor = nullptr;
	hr                               = d3d12.device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&descriptor));

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create descriptor heap for swap chain");
		swapchain3->Release();
		return (kSwapChain *)&kFallbackSwapChain;
	}

	kD3D12_SwapChain *r = (kD3D12_SwapChain *)kAlloc(sizeof(kD3D12_SwapChain));

	if (!r)
	{
		kLogError("DirectX12: Failed to allocate memory for create swap chain\n");
		swapchain3->Release();
		descriptor->Release();
		return (kSwapChain *)&kFallbackSwapChain;
	}

	memset(r, 0, sizeof(*r));

	r->native     = swapchain3;
	r->descriptor = descriptor;
	r->state      = kResourceState_Ready;

	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv = r->descriptor->GetCPUDescriptorHandleForHeapStart();

		for (uint i = 0; i < K_BACK_BUFFER_COUNT; ++i)
		{
			r->rtv[i] = rtv;
			rtv.ptr += d3d12.rtv_stride;
		}
	}

	r->index = r->native->GetCurrentBackBufferIndex();

	kD3D12_CreateSwapChainBuffers(r);
	kDListPushBack(&d3d12.resource.swap_chains, r);

	return r;
}

static kTexture *kD3D12_GetSwapChainRenderTarget(kSwapChain *swap_chain)
{
	kD3D12_SwapChain *r = (kD3D12_SwapChain *)swap_chain;
	return &r->targets[r->index];
}

static void kD3D12_Present(kSwapChain *swap_chain)
{
	kD3D12_SwapChain *r = (kD3D12_SwapChain *)swap_chain;
	r->native->Present(1, 0);
	r->index = r->native->GetCurrentBackBufferIndex();
}

//
// Resources
//

#include "Shaders/Generated/kRender2D.hlsl.h"

static void kD3D12_DestroyRenderResources(void)
{
	kLogTrace("Direct3D12: Destroying resources.\n");

	for (int i = 0; i < K_FRAME_COUNT; ++i)
	{
		kRelease(&d3d12.resource.render2d.vertex[i]);
		kRelease(&d3d12.resource.render2d.index[i]);
	}

	kRelease(&d3d12.resource.render2d.root_sig);

	for (int i = 0; i < kArrayCount(d3d12.resource.render2d.pipelines); ++i)
		kRelease(&d3d12.resource.render2d.pipelines[i]);

	d3d12.resource.render2d.vertex_sz = 0;
	d3d12.resource.render2d.index_sz  = 0;

	// TODO: release rest of other objects!!
}

static ID3D12RootSignature *kD3D12_RootSignature2D(void)
{
	if (!d3d12.resource.render2d.root_sig)
	{
		D3D12_ROOT_PARAMETER params[3]           = {};
		params[0].ParameterType                  = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		params[0].Constants.Num32BitValues       = 16;
		params[0].Constants.ShaderRegister       = 0;
		params[0].Constants.RegisterSpace        = 0;
		params[0].ShaderVisibility               = D3D12_SHADER_VISIBILITY_VERTEX;

		params[1].ParameterType                  = D3D12_ROOT_PARAMETER_TYPE_SRV;
		params[1].Descriptor.ShaderRegister      = 0;
		params[1].Descriptor.RegisterSpace       = 0;
		params[1].ShaderVisibility               = D3D12_SHADER_VISIBILITY_PIXEL;

		params[2].ParameterType                  = D3D12_ROOT_PARAMETER_TYPE_SRV;
		params[2].Descriptor.ShaderRegister      = 0;
		params[2].Descriptor.RegisterSpace       = 1;
		params[2].ShaderVisibility               = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
		desc.Version                             = D3D_ROOT_SIGNATURE_VERSION_1;
		desc.Desc_1_0.Flags                      = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		desc.Desc_1_0.NumParameters              = kArrayCount(params);
		desc.Desc_1_0.pParameters                = params;

		ID3DBlob *rootsig, *err;
		HRESULT   hr = D3D12SerializeVersionedRootSignature(&desc, &rootsig, &err);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create root signature: %s\n", err->GetBufferPointer());
			kRelease(&err);
		}

		hr = d3d12.device->CreateRootSignature(0, rootsig->GetBufferPointer(), rootsig->GetBufferSize(),
		                                       IID_PPV_ARGS(&d3d12.resource.render2d.root_sig));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create root signature");
		}
	}
	return d3d12.resource.render2d.root_sig;
}

static ID3D12PipelineState *kD3D12_PipelineState2D(kBlendMode blend)
{
	if (!d3d12.resource.render2d.pipelines[blend])
	{
		ID3D12RootSignature *rootsig = kD3D12_RootSignature2D();
		if (!rootsig) return nullptr;

		D3D12_INPUT_ELEMENT_DESC input_elements[] = {
			{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc               = {};
		desc.pRootSignature                                   = rootsig;
		desc.VS.BytecodeLength                                = kArrayCount(kQuadVS);
		desc.VS.pShaderBytecode                               = kQuadVS;
		desc.PS.BytecodeLength                                = kArrayCount(kQuadPS);
		desc.PS.pShaderBytecode                               = kQuadPS;
		desc.BlendState.AlphaToCoverageEnable                 = FALSE;
		desc.BlendState.IndependentBlendEnable                = FALSE;
		desc.SampleMask                                       = 0xFFFFFFFF;
		desc.RasterizerState.FillMode                         = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode                         = D3D12_CULL_MODE_NONE;
		desc.RasterizerState.FrontCounterClockwise            = TRUE;
		desc.RasterizerState.MultisampleEnable                = TRUE;
		desc.RasterizerState.AntialiasedLineEnable            = FALSE;
		desc.DepthStencilState.DepthEnable                    = TRUE;
		desc.DepthStencilState.DepthWriteMask                 = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthStencilState.DepthFunc                      = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc.InputLayout.NumElements                          = kArrayCount(input_elements);
		desc.InputLayout.pInputElementDescs                   = input_elements;
		desc.PrimitiveTopologyType                            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets                                 = 1;
		desc.RTVFormats[0]                                    = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.DSVFormat                                        = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count                                 = 1;
		desc.SampleDesc.Quality                               = 0;

		desc.BlendState.RenderTarget[0].BlendEnable           = blend != kBlendMode_None;
		desc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
		desc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
		desc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
		desc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_SRC_ALPHA;
		desc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
		desc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
		desc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0xf;

		HRESULT hr =
			d3d12.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&d3d12.resource.render2d.pipelines[blend]));

		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create graphics pipeline state");
		}
	}
	return d3d12.resource.render2d.pipelines[blend];
}

static ID3D12Resource *kD3D12_VertexBuffer2D(uint sz)
{
	uint frame = d3d12.frame.index;

	if (sz < d3d12.resource.render2d.vertex_sz) return d3d12.resource.render2d.vertex[frame];

	kD3D12_WaitForGpu();

	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type                  = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc   = {};
	desc.Dimension             = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment             = 0;
	desc.Width                 = sz;
	desc.Height                = 1;
	desc.DepthOrArraySize      = 1;
	desc.MipLevels             = 1;
	desc.Format                = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count      = 1;
	desc.SampleDesc.Quality    = 0;
	desc.Layout                = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags                 = D3D12_RESOURCE_FLAG_NONE;

	for (int i = 0; i < K_FRAME_COUNT; ++i)
	{
		kRelease(&d3d12.resource.render2d.vertex[i]);
		HRESULT hr =
			d3d12.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, 0,
		                                          IID_PPV_ARGS(&d3d12.resource.render2d.vertex[i]));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create vertex shader (%d)", i);
		}
	}

	d3d12.resource.render2d.vertex_sz = sz;
	return d3d12.resource.render2d.vertex[frame];
}

static ID3D12Resource *kD3D12_IndexBuffer2D(uint sz)
{
	uint frame = d3d12.frame.index;

	if (sz < d3d12.resource.render2d.index_sz) return d3d12.resource.render2d.index[frame];

	kD3D12_WaitForGpu();

	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type                  = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc   = {};
	desc.Dimension             = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment             = 0;
	desc.Width                 = sz;
	desc.Height                = 1;
	desc.DepthOrArraySize      = 1;
	desc.MipLevels             = 1;
	desc.MipLevels             = 1;
	desc.Format                = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count      = 1;
	desc.SampleDesc.Quality    = 0;
	desc.Layout                = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags                 = D3D12_RESOURCE_FLAG_NONE;

	for (int i = 0; i < K_FRAME_COUNT; ++i)
	{
		kRelease(&d3d12.resource.render2d.index[i]);
		HRESULT hr =
			d3d12.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, 0,
		                                          IID_PPV_ARGS(&d3d12.resource.render2d.index[i]));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create index shader (%d)", i);
		}
	}

	d3d12.resource.render2d.index_sz = sz;
	return d3d12.resource.render2d.index[frame];
}

//
// Device
//

static void kD3D12_DestroyGraphicsDevice(void)
{
	kD3D12_WaitForGpu();

	kD3D12_DestroyRenderResources();

	CloseHandle(d3d12.frame.event);
	kRelease(&d3d12.frame.fence);

	for (int i = 0; i < K_FRAME_COUNT; ++i)
		kRelease(&d3d12.frame.allocators[i]);
	kRelease(&d3d12.frame.commands);
	kRelease(&d3d12.frame.queue);

	kRelease(&d3d12.upload.commands);
	kRelease(&d3d12.upload.queue);

	kRelease(&d3d12.device);
	kRelease(&d3d12.factory);

	memset(&d3d12, 0, sizeof(d3d12));
}

static bool kD3D12_CreateGraphicsDevice(void)
{
	UINT    flags = 0;
	HRESULT hr    = S_OK;

	if (kEnableDebugLayer)
	{
		ID3D12Debug  *debug;
		ID3D12Debug1 *debug1;

		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		if (SUCCEEDED(hr))
		{
			kLogTrace("Direct3D12: Debug layer enabled.\n");
			debug->EnableDebugLayer();
			flags |= DXGI_CREATE_FACTORY_DEBUG;
		}
		else
		{
			kLogTrace("Direct3D12: Failed to enable debug layer.\n");
		}

		hr = debug->QueryInterface(IID_PPV_ARGS(&debug1));
		if (SUCCEEDED(hr))
		{
			kLogTrace("Direct3D12: GPU validation enabled.\n");
			debug1->SetEnableGPUBasedValidation(true);
		}
	}

	hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&d3d12.factory));
	if (FAILED(hr))
	{
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d12.factory));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX12", "Failed to create factory");
			return false;
		}
	}

	IDXGIAdapter1 *adapter = kD3D12_FindAdapter(d3d12.factory);

	if (adapter == nullptr)
	{
		kLogError("Direct3D12: Supported adaptor not found!\n");
		return false;
	}

	kDefer
	{
		if (adapter) adapter->Release();
	};

	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d12.device));
	kAssert(hr != E_INVALIDARG);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX12", "Failed to create device");
		return false;
	}

	{
		D3D12_COMMAND_QUEUE_DESC queue = {};
		queue.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queue.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;

		hr                             = d3d12.device->CreateCommandQueue(&queue, IID_PPV_ARGS(&d3d12.frame.queue));

		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create command queue");
			return false;
		}
	}

	{
		D3D12_COMMAND_QUEUE_DESC queue = {};
		queue.Type                     = D3D12_COMMAND_LIST_TYPE_COPY;
		queue.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;

		hr                             = d3d12.device->CreateCommandQueue(&queue, IID_PPV_ARGS(&d3d12.upload.queue));

		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create command queue");
			return false;
		}

		hr = d3d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&d3d12.upload.allocator));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create command allocator");
			return false;
		}

		hr = d3d12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, d3d12.upload.allocator, 0,
		                                     IID_PPV_ARGS(&d3d12.upload.commands));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create command list");
			return false;
		}

		hr = d3d12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12.upload.fence));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direc3D12", "Failed to create fence");
			return false;
		}

		d3d12.upload.event   = CreateEventW(0, 0, 0, 0);
		d3d12.upload.counter = 1;

		d3d12.upload.commands->Close();
	}

	d3d12.frame.index = 0;
	d3d12.frame.event = CreateEventW(0, FALSE, 0, 0);

	hr                = d3d12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12.frame.fence));
	if (FAILED(hr))
	{
		kLogHresultError(hr, "Direct3D12", "Failed to create fence");
		return false;
	}

	memset(d3d12.frame.values, 0, sizeof(d3d12.frame.values));

	d3d12.frame.values[0] = 1;

	for (int i = 0; i < K_FRAME_COUNT; ++i)
	{
		hr = d3d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		                                          IID_PPV_ARGS(&d3d12.frame.allocators[i]));
		if (FAILED(hr))
		{
			kLogHresultError(hr, "Direct3D12", "Failed to create command allocator (%d)", i);
			return false;
		}
	}

	hr = d3d12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12.frame.allocators[0], 0,
	                                     IID_PPV_ARGS(&d3d12.frame.commands));
	if (FAILED(hr))
	{
		kLogHresultError(hr, "Direct3D12", "Failed to create command list");
		return false;
	}

	d3d12.frame.commands->Close();

	kDListInit(&d3d12.resource.swap_chains);

	d3d12.rtv_stride = d3d12.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	return true;
}

bool kD3D12_CreateRenderBackend(kRenderBackend *backend)
{
	kLogTrace("Direct3D12: Creating graphics devices\n");
	if (!kD3D12_CreateGraphicsDevice())
	{
		kD3D12_DestroyGraphicsDevice();
		return false;
	}

	kLogTrace("Direct3D12: Registering render backend.\n");

	backend->CreateSwapChain         = kD3D12_CreateSwapChain;
	backend->DestroySwapChain        = kD3D12_DestroySwapChain;
	backend->ResizeSwapChain         = kD3D12_ResizeSwapChainBuffers;
	backend->SwapChainRenderTarget   = kD3D12_GetSwapChainRenderTarget;
	backend->GetRenderTargetConfig   = kGetRenderTargetConfigFallback;
	backend->ApplyRenderTargetConfig = kApplyRenderTargetConfigFallback;
	backend->Present                 = kD3D12_Present;

	backend->CreateTexture           = kD3D12_CreateTexture;
	backend->DestroyTexture          = kD3D12_DestroyTexture;
	backend->GetTextureSize          = kD3D12_GetTextureSize;
	backend->ResizeTexture           = kD3D12_ResizeTexture;

	backend->ExecuteCommands         = kExecuteCommandsFallback;
	backend->NextFrame               = kD3D12_MoveToNextFrame;

	backend->Destroy                 = kD3D12_DestroyGraphicsDevice;

	return true;
}

#endif
