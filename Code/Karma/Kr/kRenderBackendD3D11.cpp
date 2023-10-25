#include "kRenderBackend.h"
#include "kLinkedList.h"
#include "kMath.h"
#include "kPoolAllocator.h"
#include <math.h>

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <VersionHelpers.h>

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")

#include <d3d11_1.h>
#include <dxgi1_3.h>

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

#include "kD3DShaders.h"

constexpr uint                  K_BACK_BUFFER_COUNT = 3;

static ID3D11Device1           *g_Device;
static ID3D11DeviceContext1    *g_DeviceContext;
static IDXGIFactory2           *g_Factory;

static ID3D11RasterizerState   *g_RasterizerState;
static ID3D11DepthStencilState *g_DepthStencilStates[kDepthTest_Count];
static ID3D11SamplerState      *g_SamplerStates[kTextureFilter_Count];
static ID3D11BlendState        *g_BlendStates[kBlendMode_Count];
static ID3D11VertexShader      *g_VertexShaders[kVertexShader_Count];
static ID3D11PixelShader       *g_PixelShaders[kPixelShader_Count];
static ID3D11ComputeShader     *g_ComputeShaders[kComputeShader_Count];

//
// Mappings
//

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

static constexpr D3D11_FILTER kD3D11_FilterMap[] = {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_MIN_MAG_MIP_POINT};
static_assert(kArrayCount(kD3D11_FilterMap) == kTextureFilter_Count, "");

//
//
//

template <typename T>
static void kRelease(T **o)
{
	if (*o)
	{
		IUnknown *unknown = (*o);
		unknown->Release();
	}
	*o = nullptr;
}

static void kD3D11_Flush(void)
{
	g_DeviceContext->Flush();
	g_DeviceContext->ClearState();
}

//
// Device and State
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
	kD3D11_Flush();

	kLogInfoEx("D3D11", "Destroying graphics devices.\n");

	kRelease(&g_RasterizerState);
	for (ID3D11DepthStencilState *&dss : g_DepthStencilStates)
		kRelease(&dss);
	for (ID3D11SamplerState *&ss : g_SamplerStates)
		kRelease(&ss);
	for (ID3D11BlendState *&bs : g_BlendStates)
		kRelease(&bs);
	for (ID3D11VertexShader *&vs : g_VertexShaders)
		kRelease(&vs);
	for (ID3D11PixelShader *&ps : g_PixelShaders)
		kRelease(&ps);
	for (ID3D11ComputeShader *&cs : g_ComputeShaders)
		kRelease(&cs);

	kRelease(&g_Device);
	kRelease(&g_DeviceContext);
	kRelease(&g_Factory);
}

static bool kD3D11_CreateRasterizerState(void)
{
	D3D11_RASTERIZER_DESC ras_desc = {};
	ras_desc.FillMode              = D3D11_FILL_SOLID;
	ras_desc.CullMode              = D3D11_CULL_NONE;
	ras_desc.ScissorEnable         = TRUE;
	ras_desc.MultisampleEnable     = TRUE;
	ras_desc.AntialiasedLineEnable = TRUE;

	HRESULT hr                     = g_Device->CreateRasterizerState(&ras_desc, &g_RasterizerState);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create rasterizer");
		return false;
	}

	return true;
}

static bool kD3D11_CreateDepthStencilState(void)
{
	{
		D3D11_DEPTH_STENCIL_DESC depth = {};
		depth.DepthEnable              = FALSE;
		depth.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;

		HRESULT hr = g_Device->CreateDepthStencilState(&depth, &g_DepthStencilStates[kDepthTest_Disabled]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create depth stencil state");
			return false;
		}
	}

	{
		D3D11_DEPTH_STENCIL_DESC depth = {};
		depth.DepthEnable              = TRUE;
		depth.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
		depth.DepthFunc                = D3D11_COMPARISON_LESS_EQUAL;

		HRESULT hr = g_Device->CreateDepthStencilState(&depth, &g_DepthStencilStates[kDepthTest_LessEquals]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create depth stencil state");
			return false;
		}
	}
	return true;
}

static bool kD3D11_CreateSamplerState(void)
{
	for (int filter = 0; filter < kTextureFilter_Count; ++filter)
	{
		D3D11_SAMPLER_DESC sampler = {};
		sampler.Filter             = kD3D11_FilterMap[filter];
		sampler.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.MipLODBias         = 0.0f;
		sampler.MaxAnisotropy      = 1;
		sampler.ComparisonFunc     = D3D11_COMPARISON_NEVER;
		sampler.MinLOD             = -FLT_MAX;
		sampler.MaxLOD             = FLT_MAX;

		HRESULT hr                 = g_Device->CreateSamplerState(&sampler, &g_SamplerStates[filter]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create sampler");
			return false;
		}
	}

	return true;
}

static bool kD3D11_CreateBlendState(void)
{
	D3D11_BLEND_DESC BlendOpaque                           = {};
	BlendOpaque.RenderTarget[0].BlendEnable                = FALSE;
	BlendOpaque.RenderTarget[0].SrcBlend                   = D3D11_BLEND_ONE;
	BlendOpaque.RenderTarget[0].DestBlend                  = D3D11_BLEND_ZERO;
	BlendOpaque.RenderTarget[0].BlendOp                    = D3D11_BLEND_OP_ADD;
	BlendOpaque.RenderTarget[0].SrcBlendAlpha              = D3D11_BLEND_ONE;
	BlendOpaque.RenderTarget[0].DestBlendAlpha             = D3D11_BLEND_ZERO;
	BlendOpaque.RenderTarget[0].BlendOpAlpha               = D3D11_BLEND_OP_ADD;
	BlendOpaque.RenderTarget[0].RenderTargetWriteMask      = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC BlendNormal                           = {};
	BlendNormal.RenderTarget[0].BlendEnable                = TRUE;
	BlendNormal.RenderTarget[0].SrcBlend                   = D3D11_BLEND_SRC_ALPHA;
	BlendNormal.RenderTarget[0].DestBlend                  = D3D11_BLEND_INV_SRC_ALPHA;
	BlendNormal.RenderTarget[0].BlendOp                    = D3D11_BLEND_OP_ADD;
	BlendNormal.RenderTarget[0].SrcBlendAlpha              = D3D11_BLEND_SRC_ALPHA;
	BlendNormal.RenderTarget[0].DestBlendAlpha             = D3D11_BLEND_INV_SRC_ALPHA;
	BlendNormal.RenderTarget[0].BlendOpAlpha               = D3D11_BLEND_OP_ADD;
	BlendNormal.RenderTarget[0].RenderTargetWriteMask      = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC BlendAdditive                         = {};
	BlendAdditive.RenderTarget[0].BlendEnable              = TRUE;
	BlendAdditive.RenderTarget[0].SrcBlend                 = D3D11_BLEND_SRC_ALPHA;
	BlendAdditive.RenderTarget[0].DestBlend                = D3D11_BLEND_ONE;
	BlendAdditive.RenderTarget[0].BlendOp                  = D3D11_BLEND_OP_ADD;
	BlendAdditive.RenderTarget[0].SrcBlendAlpha            = D3D11_BLEND_SRC_ALPHA;
	BlendAdditive.RenderTarget[0].DestBlendAlpha           = D3D11_BLEND_ONE;
	BlendAdditive.RenderTarget[0].BlendOpAlpha             = D3D11_BLEND_OP_ADD;
	BlendAdditive.RenderTarget[0].RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC BlendSubtractive                      = {};
	BlendSubtractive.RenderTarget[0].BlendEnable           = TRUE;
	BlendSubtractive.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
	BlendSubtractive.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
	BlendSubtractive.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_SUBTRACT;
	BlendSubtractive.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
	BlendSubtractive.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
	BlendSubtractive.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_SUBTRACT;
	BlendSubtractive.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC *BlendModeDesc[] = {&BlendOpaque, &BlendNormal, &BlendAdditive, &BlendSubtractive};

	static_assert(kArrayCount(BlendModeDesc) == kBlendMode_Count, "");

	for (int mode = 0; mode < kBlendMode_Count; ++mode)
	{
		HRESULT hr = g_Device->CreateBlendState(BlendModeDesc[mode], &g_BlendStates[mode]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create alpha blend state");
			return false;
		}
	}

	return true;
}

static bool kD3D11_CreateVertexShaders(void)
{
	kLogInfoEx("D3D11", "Compiling vertex shaders.\n");

	for (int i = 0; i < kVertexShader_Count; ++i)
	{
		kString vs = VertexShadersMap[i];
		HRESULT hr = g_Device->CreateVertexShader(vs.data, vs.count, 0, &g_VertexShaders[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create vertex shader (%d)", i);
			return false;
		}
	}

	return true;
}

static bool kD3D11_CreatePixelShaders(void)
{
	kLogInfoEx("D3D11", "Compiling pixel shaders.\n");

	for (int i = 0; i < kPixelShader_Count; ++i)
	{
		kString ps = PixelShadersMap[i];
		HRESULT hr = g_Device->CreatePixelShader(ps.data, ps.count, 0, &g_PixelShaders[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create pixel shader (%d)", i);
			return false;
		}
	}

	return true;
}

static bool kD3D11_CreateComputeShaders(void)
{
	kLogInfoEx("D3D11", "Compiling compute shaders.\n");

	for (int i = 0; i < kComputeShader_Count; ++i)
	{
		kString cs = ComputeShadersMap[i];
		HRESULT hr = g_Device->CreateComputeShader(cs.data, cs.count, 0, &g_ComputeShaders[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create compute shader (%d)", i);
			return false;
		}
	}

	return true;
}

static bool kD3D11_TryCreateGraphicsDevice(void)
{
	IDXGIAdapter1 *adapter      = nullptr;
	UINT           device_flags = 0;
	UINT           flags        = 0;

	for (int i = 0; i < 2; ++i)
	{
		if (kEnableDebugLayer) flags |= DXGI_CREATE_FACTORY_DEBUG;

		if (!g_Factory)
		{
			HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&g_Factory));
			if (FAILED(hr))
			{
				hr = CreateDXGIFactory1(IID_PPV_ARGS(&g_Factory));
				if (FAILED(hr))
				{
					kLogHresultError(hr, "DirectX11", "Failed to create factory");
					return false;
				}
			}
		}

		device_flags = 0;
		if (kEnableDebugLayer) device_flags |= D3D11_CREATE_DEVICE_DEBUG;

		adapter = kD3D11_FindAdapter(g_Factory, device_flags);

		if (kEnableDebugLayer && !adapter)
		{
			kEnableDebugLayer = false;
			kLogWarningEx("D3D11", "Debug Layer SDK not present. Falling back to adaptor without debug layer.\n");
		}
	}

	if (adapter == nullptr)
	{
		kLogWarningEx("D3D11", "Supported adaptor not found!\n");
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

		hr = device->QueryInterface(IID_PPV_ARGS(&g_Device));
		kAssert(SUCCEEDED(hr));
		device->Release();
	}

	if (device_flags & D3D11_CREATE_DEVICE_DEBUG)
	{
		ID3D11Debug *debug = 0;
		if (SUCCEEDED(g_Device->QueryInterface(IID_PPV_ARGS(&debug))))
		{
			ID3D11InfoQueue *info_queue = nullptr;
			if (SUCCEEDED(g_Device->QueryInterface(IID_PPV_ARGS(&info_queue))))
			{
				kLogTraceEx("D3D11", "ID3D11Debug enabled.");
				info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
				info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
				kRelease(&info_queue);
			}
		}
		kRelease(&debug);
	}

	g_Device->GetImmediateContext1(&g_DeviceContext);

	if (!kD3D11_CreateRasterizerState()) return false;
	if (!kD3D11_CreateDepthStencilState()) return false;
	if (!kD3D11_CreateBlendState()) return false;
	if (!kD3D11_CreateSamplerState()) return false;
	if (!kD3D11_CreateVertexShaders()) return false;
	if (!kD3D11_CreatePixelShaders()) return false;
	if (!kD3D11_CreateComputeShaders()) return false;

	return true;
}

static bool kD3D11_CreateGraphicsDevice(void)
{
	if (!kD3D11_TryCreateGraphicsDevice())
	{
		kD3D11_DestroyGraphicsDevice();
		return false;
	}
	return true;
}

//
//
//

typedef struct kD3D11_Texture : kTexture
{
	kVec2u                    size;
	ID3D11ShaderResourceView *srv;
	ID3D11Texture2D          *texture2d;
} kD3D11_Texture;

struct kD3D11_RenderTarget
{
	kVec2u                     size;
	ID3D11RenderTargetView    *rtv;
	ID3D11DepthStencilView    *dsv;
	ID3D11ShaderResourceView  *srv;
	ID3D11UnorderedAccessView *uav;
	ID3D11Texture2D           *texture2d;
};

typedef enum kD3D11_RenderTargetKind
{
	kD3D11_RenderTarget_Final,
	kD3D11_RenderTarget_Depth,
	kD3D11_RenderTarget_MASS,
	kD3D11_RenderTarget_ResolveMASS,
	kD3D11_RenderTarget_BloomDown,
	// kD3D11_RenderTarget_BloomUp,
	kD3D11_RenderTarget_Tonemap,
	kD3D11_RenderTarget_Count,
} kD3D11_RenderTargetKind;

typedef void (*kD3D11_RenderPassProc)(const struct kD3D11_RenderPass &, const kRenderFrame &);

enum kD3D11_RenderPassFlags
{
	kD3D11_RenderPass_ClearColor = 0x1,
	kD3D11_RenderPass_ClearDepth = 0x2,
	kD3D11_RenderPass_Compute    = 0x10,
	kD3D11_RenderPass_Disabled   = 0x20,
};

typedef struct kD3D11_RenderPass
{
	kD3D11_RenderTarget  *rt;
	kD3D11_RenderTarget  *ds;
	UINT                  flags;
	kVec4                 color;
	float                 depth;
	DXGI_FORMAT           format;
	kD3D11_RenderTarget  *resolve;
	kD3D11_RenderTarget  *params[2];
	kD3D11_RenderPassProc Execute;
} kD3D11_RenderPass;

typedef struct kD3D11_SwapChain
{
	IDXGISwapChain1    *native;
	kD3D11_RenderTarget targets[kD3D11_RenderTarget_Count];
	kD3D11_RenderPass   passes[kRenderPass_Count];
	ID3D11Buffer       *csbuffers[kComputeShader_Count];
} kD3D11_SwapChain;

typedef struct kD3D11_Render2D
{
	uint               vertex_sz;
	uint               index_sz;
	ID3D11Buffer      *vertex;
	ID3D11Buffer      *index;
	ID3D11Buffer      *xform;
	ID3D11InputLayout *input;
} kD3D11_Render2D;

typedef kMemoryPool<kD3D11_Texture> kD3D11_TexturePool;

typedef struct kD3D11_Resource
{
	kD3D11_TexturePool textures;
	kD3D11_Render2D    render2d;
} kD3D11_Resource;

typedef struct kD3D11_Backend
{
	kD3D11_SwapChain swapchain;
	kD3D11_Resource  resource;
} kD3D11_Backend;

static kD3D11_Backend d3d11;

//
//
//

//
// TODO: CLEANUP
//

static void kD3D11_RenderPassFallback(const kD3D11_RenderPass &, const kRenderFrame &) {}

static UINT kD3D11_ComputeBufferSizes[] = {sizeof(kVec4), sizeof(u32), 0};
static_assert(kArrayCount(kD3D11_ComputeBufferSizes) == kComputeShader_Count, "");

//
// Textures
//

static void kD3D11_PrepareRenderTarget(kD3D11_RenderTarget *target)
{
	D3D11_TEXTURE2D_DESC desc;
	target->texture2d->GetDesc(&desc);

	HRESULT hr = 0;

	if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		hr = g_Device->CreateShaderResourceView(target->texture2d, nullptr, &target->srv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create shader resource view");
		}
	}

	if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		hr = g_Device->CreateUnorderedAccessView(target->texture2d, nullptr, &target->uav);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create unordered access view");
		}
	}

	if (desc.BindFlags & D3D11_BIND_RENDER_TARGET)
	{
		hr = g_Device->CreateRenderTargetView(target->texture2d, 0, &target->rtv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create unordered access view");
		}
	}

	if (desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		hr = g_Device->CreateDepthStencilView(target->texture2d, 0, &target->dsv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create unordered access view");
		}
	}

	target->size.x = (int)desc.Width;
	target->size.y = (int)desc.Height;
}

static kTexture *kD3D11_CreateTexture(const kTextureSpec &spec)
{
	kD3D11_Texture *r = d3d11.resource.textures.Alloc();

	if (!r)
	{
		kLogErrorEx("D3D11", "Failed to allocate memory for texture\n");
		return (kD3D11_Texture *)&kFallbackTexture;
	}

	D3D11_TEXTURE2D_DESC desc = {};

	desc.Width                = spec.width;
	desc.Height               = spec.height;
	desc.Format               = kD3D11_FormatMap[spec.format];
	desc.ArraySize            = 1;
	desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage                = D3D11_USAGE_DEFAULT;
	desc.SampleDesc.Count     = 1;
	desc.SampleDesc.Quality   = 0;
	desc.CPUAccessFlags       = 0;
	desc.MiscFlags            = 0;
	desc.MipLevels            = 1;

	D3D11_SUBRESOURCE_DATA source;
	source.pSysMem          = spec.pixels;
	source.SysMemPitch      = spec.pitch;
	source.SysMemSlicePitch = 0;

	HRESULT hr              = g_Device->CreateTexture2D(&desc, spec.pixels ? &source : nullptr, &r->texture2d);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create texture");
		r->state = kResourceState_Error;
		return r;
	}

	hr = g_Device->CreateShaderResourceView(r->texture2d, 0, &r->srv);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create texture view");
		r->state = kResourceState_Error;
		return r;
	}

	r->size  = kVec2u(spec.width, spec.height);
	r->state = kResourceState_Ready;

	return r;
}

static void kD3D11_UnprepareRenderTarget(kD3D11_RenderTarget *r)
{
	kRelease(&r->srv);
	kRelease(&r->uav);
	kRelease(&r->rtv);
	kRelease(&r->dsv);
	r->size = kVec2u(0);
}

static void kD3D11_DestroyTexture(kTexture *texture)
{
	kD3D11_Texture *r = (kD3D11_Texture *)texture;
	if (r->state == kResourceState_Ready)
	{
		kRelease(&r->srv);
		kRelease(&r->texture2d);
		d3d11.resource.textures.Free(r);
	}
}

static kVec2i kD3D11_GetTextureSize(kTexture *texture)
{
	kD3D11_Texture *r = (kD3D11_Texture *)texture;
	return r->size;
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

		kRelease(&r->srv);
		kRelease(&r->texture2d);

		desc.Width  = w;
		desc.Height = h;

		HRESULT hr  = g_Device->CreateTexture2D(&desc, nullptr, &r->texture2d);

		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create texture");
			r->state = kResourceState_Error;
			return;
		}

		hr = g_Device->CreateShaderResourceView(r->texture2d, 0, &r->srv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create texture view");
			r->state = kResourceState_Error;
			return;
		}

		r->size  = kVec2u(w, h);
		r->state = kResourceState_Ready;
	}
}

//
// Render Passes
//

static ID3D11Buffer *kD3D11_UploadVertices2D(kSpan<kVertex2D> vertices)
{
	uint vb_size = (uint)vertices.Size();

	if (vb_size > d3d11.resource.render2d.vertex_sz)
	{
		kRelease(&d3d11.resource.render2d.vertex);
		d3d11.resource.render2d.vertex_sz = 0;

		D3D11_BUFFER_DESC vb_desc         = {};
		vb_desc.ByteWidth                 = vb_size;
		vb_desc.Usage                     = D3D11_USAGE_DYNAMIC;
		vb_desc.BindFlags                 = D3D11_BIND_VERTEX_BUFFER;
		vb_desc.CPUAccessFlags            = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr                        = g_Device->CreateBuffer(&vb_desc, 0, &d3d11.resource.render2d.vertex);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to allocate vertex buffer");
			return nullptr;
		}

		d3d11.resource.render2d.vertex_sz = vb_size;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = g_DeviceContext->Map(d3d11.resource.render2d.vertex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to map vertex2d buffer");
		return nullptr;
	}
	memcpy(mapped.pData, vertices.data, vb_size);
	g_DeviceContext->Unmap(d3d11.resource.render2d.vertex, 0);

	return d3d11.resource.render2d.vertex;
}

static ID3D11Buffer *kD3D11_UploadIndices2D(kSpan<kIndex2D> indices)
{
	uint ib_size = (uint)indices.Size();

	if (ib_size > d3d11.resource.render2d.index_sz)
	{
		kRelease(&d3d11.resource.render2d.index);
		d3d11.resource.render2d.index_sz = 0;

		D3D11_BUFFER_DESC ib_desc        = {};
		ib_desc.ByteWidth                = ib_size;
		ib_desc.Usage                    = D3D11_USAGE_DYNAMIC;
		ib_desc.BindFlags                = D3D11_BIND_INDEX_BUFFER;
		ib_desc.CPUAccessFlags           = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr                       = g_Device->CreateBuffer(&ib_desc, 0, &d3d11.resource.render2d.index);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to allocate index buffer");
			return nullptr;
		}

		d3d11.resource.render2d.index_sz = ib_size;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = g_DeviceContext->Map(d3d11.resource.render2d.index, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to map index2d buffer");
		return nullptr;
	}
	memcpy(mapped.pData, indices.data, ib_size);
	g_DeviceContext->Unmap(d3d11.resource.render2d.index, 0);

	return d3d11.resource.render2d.index;
}

static void kD3D11_RenderPassQuad2D(const kD3D11_RenderPass &pass, const kRenderFrame &render)
{
	const kRenderFrame2D &frame = render.render2d;

	if (frame.vertices.count == 0 || frame.indices.count == 0) return;

	ID3D11Buffer *vtx    = kD3D11_UploadVertices2D(frame.vertices);
	ID3D11Buffer *idx    = kD3D11_UploadIndices2D(frame.indices);
	ID3D11Buffer *xform  = d3d11.resource.render2d.xform;
	HRESULT       hr     = S_OK;
	UINT          stride = sizeof(kVertex2D);
	UINT          offset = 0;
	DXGI_FORMAT   format = sizeof(kIndex2D) == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	if (!vtx || !idx) return;

	g_DeviceContext->IASetVertexBuffers(0, 1, &vtx, &stride, &offset);
	g_DeviceContext->IASetIndexBuffer(idx, format, 0);
	g_DeviceContext->VSSetConstantBuffers(0, 1, &xform);
	g_DeviceContext->VSSetShader(g_VertexShaders[kVertexShader_Quad], nullptr, 0);
	g_DeviceContext->PSSetShader(g_PixelShaders[kPixelShader_Quad], nullptr, 0);
	g_DeviceContext->IASetInputLayout(d3d11.resource.render2d.input);
	g_DeviceContext->RSSetState(g_RasterizerState);
	g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStates[kDepthTest_LessEquals], 0);
	g_DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	i32 vtx_offset = 0;
	u32 idx_offset = 0;

	for (const kRenderScene2D &scene : frame.scenes)
	{
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = scene.region.min.x;
		viewport.TopLeftY = scene.region.min.y;
		viewport.Width    = scene.region.max.x - scene.region.min.x;
		viewport.Height   = scene.region.max.y - scene.region.min.y;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		g_DeviceContext->RSSetViewports(1, &viewport);

		{
			kMat4 proj = kOrthographicLH(scene.camera.left, scene.camera.right, scene.camera.top, scene.camera.bottom,
			                             scene.camera.near, scene.camera.far);

			D3D11_MAPPED_SUBRESOURCE mapped;
			hr = g_DeviceContext->Map(xform, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			if (FAILED(hr))
			{
				kLogHresultError(hr, "DirectX11", "Failed to map constant buffer");
				continue;
			}
			memcpy(mapped.pData, proj.m, sizeof(kMat4));
			g_DeviceContext->Unmap(xform, 0);
		}

		for (u32 index = scene.commands.beg; index < scene.commands.end; ++index)
		{
			const kRenderCommand2D &cmd = frame.commands[index];

			if (cmd.flags & kRenderDirty_Blend)
			{
				g_DeviceContext->OMSetBlendState(g_BlendStates[cmd.blend], 0, 0xffffffff);
			}

			if (cmd.flags & kRenderDirty_TextureFilter)
			{
				g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerStates[cmd.filter]);
			}

			if (cmd.flags & kRenderDirty_Rect)
			{
				D3D11_RECT rect;
				kRect      krect = frame.rects[cmd.rect];
				rect.left        = (LONG)(krect.min.x);
				rect.top         = (LONG)(krect.min.y);
				rect.right       = (LONG)(krect.max.x);
				rect.bottom      = (LONG)(krect.max.y);
				g_DeviceContext->RSSetScissorRects(1, &rect);
			}

			if (cmd.flags & kRenderDirty_TextureColor)
			{
				u32             texoff = cmd.textures[kTextureType_Color];
				kD3D11_Texture *r      = (kD3D11_Texture *)frame.textures[kTextureType_Color][texoff];
				if (r->state != kResourceState_Ready) continue;
				g_DeviceContext->PSSetShaderResources(kTextureType_Color, 1, &r->srv);
			}

			if (cmd.flags & kRenderDirty_TextureMaskSDF)
			{
				u32             texoff = cmd.textures[kTextureType_MaskSDF];
				kD3D11_Texture *r      = (kD3D11_Texture *)frame.textures[kTextureType_MaskSDF][texoff];
				if (r->state != kResourceState_Ready) continue;
				g_DeviceContext->PSSetShaderResources(kTextureType_MaskSDF, 1, &r->srv);
			}

			g_DeviceContext->DrawIndexed(cmd.index, idx_offset, vtx_offset);
			vtx_offset += cmd.vertex;
			idx_offset += cmd.index;
		}

		g_DeviceContext->PSSetSamplers(0, 0, nullptr);
		g_DeviceContext->PSSetShaderResources(0, 0, nullptr);
	}
}

static void kD3D11_RenderPassBlit(const kD3D11_RenderPass &pass, const kRenderFrame &frame)
{
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX                     = 0;
	viewport.TopLeftY                     = 0;
	viewport.Width                        = (float)pass.rt->size.x;
	viewport.Height                       = (float)pass.rt->size.y;
	viewport.MinDepth                     = 0;
	viewport.MaxDepth                     = 1;

	kD3D11_RenderTarget      *src         = pass.params[0];
	ID3D11ShaderResourceView *resources[] = {src->srv};

	g_DeviceContext->RSSetViewports(1, &viewport);
	g_DeviceContext->VSSetShader(g_VertexShaders[kVertexShader_Blit], nullptr, 0);
	g_DeviceContext->PSSetShader(g_PixelShaders[kPixelShader_Blit], nullptr, 0);
	g_DeviceContext->PSSetShaderResources(0, kArrayCount(resources), resources);
	g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerStates[kTextureFilter_LinearWrap]);
	g_DeviceContext->OMSetBlendState(g_BlendStates[kBlendMode_Opaque], 0, 0xffffffff);
	g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStates[kDepthTest_Disabled], 0);
	g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_DeviceContext->Draw(6, 0);
}

static void kD3D11_RenderPassThreshold(const kD3D11_RenderPass &pass, const kRenderFrame &render)
{
	kD3D11_RenderTarget       *src     = pass.params[0];
	kD3D11_RenderTarget       *tonemap = pass.params[1];

	ID3D11ShaderResourceView  *srvs[]  = {src->srv};
	ID3D11UnorderedAccessView *uavs[]  = {tonemap->uav};

	D3D11_MAPPED_SUBRESOURCE   mapped;
	HRESULT hr = g_DeviceContext->Map(d3d11.swapchain.csbuffers[kComputeShader_Threshold], 0, D3D11_MAP_WRITE_DISCARD,
	                                  0, &mapped);
	if (FAILED(hr)) return;

	kVec4 threshold = kVec4(1);
	memcpy(mapped.pData, threshold.m, sizeof(threshold));

	g_DeviceContext->Unmap(d3d11.swapchain.csbuffers[kComputeShader_Threshold], 0);

	g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_Threshold], 0, 0);
	g_DeviceContext->CSSetConstantBuffers(0, 1, &d3d11.swapchain.csbuffers[kComputeShader_Threshold]);
	g_DeviceContext->CSSetShaderResources(0, kArrayCount(srvs), srvs);
	g_DeviceContext->CSSetUnorderedAccessViews(0, kArrayCount(uavs), uavs, 0);

	UINT x_thrd_group = (UINT)ceilf(tonemap->size.x / 32.0f);
	UINT y_thrd_group = (UINT)ceilf(tonemap->size.y / 16.0f);
	g_DeviceContext->Dispatch(x_thrd_group, y_thrd_group, 1);
}

static void kD3D11_RenderPassBloom(const kD3D11_RenderPass &pass, const kRenderFrame &render)
{
	kD3D11_RenderTarget       *src    = pass.params[0];
	kD3D11_RenderTarget       *dst    = pass.params[1];

	ID3D11ShaderResourceView  *srvs[] = {src->srv};
	ID3D11UnorderedAccessView *uavs[] = {dst->uav};

	D3D11_MAPPED_SUBRESOURCE   mapped;
	HRESULT                    hr =
		g_DeviceContext->Map(d3d11.swapchain.csbuffers[kComputeShader_Bloom], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr)) return;

	u32 value = 1;
	memcpy(mapped.pData, &value, sizeof(value));

	g_DeviceContext->Unmap(d3d11.swapchain.csbuffers[kComputeShader_Bloom], 0);

	g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_Bloom], 0, 0);
	g_DeviceContext->CSSetConstantBuffers(0, 1, &d3d11.swapchain.csbuffers[kComputeShader_Bloom]);
	g_DeviceContext->CSSetShaderResources(0, kArrayCount(srvs), srvs);
	g_DeviceContext->CSSetUnorderedAccessViews(0, kArrayCount(uavs), uavs, 0);

	// TODO:: USE CLAMP TO EDGE
	g_DeviceContext->CSSetSamplers(0, 1, &g_SamplerStates[kTextureFilter_LinearWrap]);

	UINT x_thrd_group = (UINT)ceilf((dst->size.x >> 1) / 32.0f);
	UINT y_thrd_group = (UINT)ceilf((dst->size.y >> 1) / 16.0f);
	g_DeviceContext->Dispatch(x_thrd_group, y_thrd_group, 1);
}

static void kD3D11_RenderPassTonemap(const kD3D11_RenderPass &pass, const kRenderFrame &render)
{
	kD3D11_RenderTarget       *src     = pass.params[0];
	kD3D11_RenderTarget       *tonemap = pass.params[1];

	ID3D11ShaderResourceView  *srvs[]  = {src->srv};
	ID3D11UnorderedAccessView *uavs[]  = {tonemap->uav};

	g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_Tonemap], 0, 0);
	g_DeviceContext->CSSetShaderResources(0, kArrayCount(srvs), srvs);
	g_DeviceContext->CSSetUnorderedAccessViews(0, kArrayCount(uavs), uavs, 0);

	UINT x_thrd_group = (UINT)ceilf(tonemap->size.x / 32.0f);
	UINT y_thrd_group = (UINT)ceilf(tonemap->size.y / 16.0f);
	g_DeviceContext->Dispatch(x_thrd_group, y_thrd_group, 1);
	g_DeviceContext->ClearState();
}

//
// SwapChain
//

static void kD3D11_DestroySwapChainBuffers(void)
{
	kDebugTraceEx("D3D11", "Destroying swap chain render targets.\n");
	memset(d3d11.swapchain.passes, 0, sizeof(d3d11.swapchain.passes));

	for (int target = 0; target < kD3D11_RenderTarget_Count; ++target)
	{
		kD3D11_UnprepareRenderTarget(&d3d11.swapchain.targets[target]);
		kRelease(&d3d11.swapchain.targets[target].texture2d);
	}

	for (int pass = 0; pass < kRenderPass_Count; ++pass)
	{
		d3d11.swapchain.passes[pass].Execute = kD3D11_RenderPassFallback;
	}
}

static void kD3D11_CreateSwapChainBuffers(void)
{
	kDebugTraceEx("D3D11", "Creating swap chain render targets.\n");

	kD3D11_RenderTarget &rt = d3d11.swapchain.targets[kD3D11_RenderTarget_Final];

	HRESULT              hr = d3d11.swapchain.native->GetBuffer(0, IID_PPV_ARGS(&rt.texture2d));

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create swapchain render target");
		return;
	}

	kD3D11_PrepareRenderTarget(&rt);

	const int samples = 4;

	{
		kD3D11_RenderTarget &target = d3d11.swapchain.targets[kD3D11_RenderTarget_Depth];

		D3D11_TEXTURE2D_DESC desc   = {};
		desc.Width                  = rt.size.x;
		desc.Height                 = rt.size.y;
		desc.MipLevels              = 1;
		desc.ArraySize              = 1;
		desc.Format                 = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count       = samples;
		desc.SampleDesc.Quality     = 0;
		desc.Usage                  = D3D11_USAGE_DEFAULT;
		desc.BindFlags              = D3D11_BIND_DEPTH_STENCIL;

		hr                          = g_Device->CreateTexture2D(&desc, 0, &target.texture2d);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create texture 2d");
			return;
		}

		kD3D11_PrepareRenderTarget(&target);
	}

	{
		kD3D11_RenderTarget &target = d3d11.swapchain.targets[kD3D11_RenderTarget_MASS];

		D3D11_TEXTURE2D_DESC desc   = {};
		desc.Width                  = rt.size.x;
		desc.Height                 = rt.size.y;
		desc.MipLevels              = 1;
		desc.ArraySize              = 1;
		desc.Format                 = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.SampleDesc.Count       = samples;
		desc.SampleDesc.Quality     = 0;
		desc.Usage                  = D3D11_USAGE_DEFAULT;
		desc.BindFlags              = D3D11_BIND_RENDER_TARGET;

		hr                          = g_Device->CreateTexture2D(&desc, 0, &target.texture2d);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create MSAA texture 2d");
			return;
		}

		kD3D11_PrepareRenderTarget(&target);
	}

	{
		kD3D11_RenderTarget &target = d3d11.swapchain.targets[kD3D11_RenderTarget_ResolveMASS];

		D3D11_TEXTURE2D_DESC desc   = {};
		desc.Width                  = rt.size.x;
		desc.Height                 = rt.size.y;
		desc.MipLevels              = 1;
		desc.ArraySize              = 1;
		desc.Format                 = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.SampleDesc.Count       = 1;
		desc.SampleDesc.Quality     = 0;
		desc.Usage                  = D3D11_USAGE_DEFAULT;
		desc.BindFlags              = D3D11_BIND_SHADER_RESOURCE;

		hr                          = g_Device->CreateTexture2D(&desc, 0, &target.texture2d);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create MSAA texture 2d");
			return;
		}

		kD3D11_PrepareRenderTarget(&target);
	}

	{
		kD3D11_RenderTarget &target = d3d11.swapchain.targets[kD3D11_RenderTarget_BloomDown];

		D3D11_TEXTURE2D_DESC desc   = {};
		desc.Width                  = rt.size.x;
		desc.Height                 = rt.size.y;
		desc.MipLevels              = 0;
		desc.ArraySize              = 1;
		desc.Format                 = DXGI_FORMAT_R11G11B10_FLOAT;
		desc.SampleDesc.Count       = 1;
		desc.SampleDesc.Quality     = 0;
		desc.Usage                  = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		hr             = g_Device->CreateTexture2D(&desc, 0, &target.texture2d);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create bloom texture 2d");
			return;
		}

		target.size = rt.size;

#if 0
		D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.Format                          = DXGI_FORMAT_R11G11B10_FLOAT;
		srv.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels             = 1;
		srv.Texture2D.MostDetailedMip       = 0;

		hr = g_Device->CreateShaderResourceView(target.texture2d, &srv, &target.srv);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create bloom shader resource view");
			return;
		}
#endif

#if 1
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav = {};
		uav.Format                           = DXGI_FORMAT_R11G11B10_FLOAT;
		uav.ViewDimension                    = D3D11_UAV_DIMENSION_TEXTURE2D;
		uav.Texture2D.MipSlice               = 0;

		hr                                   = g_Device->CreateUnorderedAccessView(target.texture2d, &uav, &target.uav);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create bloom unordered access view");
			return;
		}
#endif

		// kD3D11_PrepareTexture(&target);

		if (target.srv)
		{
			g_DeviceContext->GenerateMips(target.srv);
		}
	}

	//{
	//	kD3D11_Texture &     target = d3d11.swapchain.targets[kD3D11_RenderTarget_BloomUp];

	//	D3D11_TEXTURE2D_DESC desc   = {};
	//	desc.Width                  = rt.size.x;
	//	desc.Height                 = rt.size.y;
	//	desc.MipLevels              = 1;
	//	desc.ArraySize              = 1;
	//	desc.Format                 = DXGI_FORMAT_R11G11B10_FLOAT;
	//	desc.SampleDesc.Count       = 1;
	//	desc.SampleDesc.Quality     = 0;
	//	desc.Usage                  = D3D11_USAGE_DEFAULT;
	//	desc.BindFlags              = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	//	hr                          = g_Device->CreateTexture2D(&desc, 0, &target.texture2d);
	//	if (FAILED(hr))
	//	{
	//		kLogHresultError(hr, "DirectX11", "Failed to create MSAA texture 2d");
	//		return;
	//	}

	//	kD3D11_PrepareTexture(&target);
	//}

	{
		kD3D11_RenderTarget &target = d3d11.swapchain.targets[kD3D11_RenderTarget_Tonemap];

		D3D11_TEXTURE2D_DESC desc   = {};
		desc.Width                  = rt.size.x;
		desc.Height                 = rt.size.y;
		desc.MipLevels              = 1;
		desc.ArraySize              = 1;
		desc.Format                 = DXGI_FORMAT_R11G11B10_FLOAT;
		desc.SampleDesc.Count       = 1;
		desc.SampleDesc.Quality     = 0;
		desc.Usage                  = D3D11_USAGE_DEFAULT;
		desc.BindFlags              = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		hr                          = g_Device->CreateTexture2D(&desc, 0, &target.texture2d);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create MSAA texture 2d");
			return;
		}

		kD3D11_PrepareRenderTarget(&target);
	}

	{
		kD3D11_RenderPass &pass = d3d11.swapchain.passes[kRenderPass_Quad];
		pass.rt                 = &d3d11.swapchain.targets[kD3D11_RenderTarget_MASS];
		pass.ds                 = &d3d11.swapchain.targets[kD3D11_RenderTarget_Depth];
		pass.flags              = kD3D11_RenderPass_ClearColor | kD3D11_RenderPass_ClearDepth;
		pass.color              = kVec4(0, 0, 0, 1);
		pass.depth              = 1;
		pass.resolve            = &d3d11.swapchain.targets[kD3D11_RenderTarget_ResolveMASS];
		pass.format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pass.Execute            = kD3D11_RenderPassQuad2D;
	}

	{
		kD3D11_RenderPass &pass = d3d11.swapchain.passes[kRenderPass_Threshold];
		pass.flags              = kD3D11_RenderPass_Compute;
		pass.params[0]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_ResolveMASS];
		pass.params[1]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_BloomDown];
		pass.Execute            = kD3D11_RenderPassThreshold;
	}

	{
		kD3D11_RenderPass &pass = d3d11.swapchain.passes[kRenderPass_Bloom];
		pass.flags              = kD3D11_RenderPass_Compute | kD3D11_RenderPass_Disabled;
		pass.params[0]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_BloomDown];
		pass.params[1]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_BloomDown];
		pass.Execute            = kD3D11_RenderPassBloom;
	}

	{
		kD3D11_RenderPass &pass = d3d11.swapchain.passes[kRenderPass_Tonemap];
		pass.flags              = kD3D11_RenderPass_Compute;
		pass.params[0]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_ResolveMASS];
		pass.params[1]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_Tonemap];
		pass.Execute            = kD3D11_RenderPassTonemap;
	}

	{
		kD3D11_RenderPass &pass = d3d11.swapchain.passes[kRenderPass_Blit];
		pass.rt                 = &d3d11.swapchain.targets[kD3D11_RenderTarget_Final];
		pass.flags              = 0;
		pass.color              = kVec4(0, 0, 0, 1);
		pass.depth              = 1;
		pass.params[0]          = &d3d11.swapchain.targets[kD3D11_RenderTarget_Tonemap];
		pass.Execute            = kD3D11_RenderPassBlit;
	}
}

static void kD3D11_ResizeSwapChainBuffers(uint w, uint h)
{
	if (!w || !h) return;

	kD3D11_Flush();

	if (d3d11.swapchain.native)
	{
		kVec2u size = d3d11.swapchain.targets[0].size;
		if (size.x == w && size.y == h) return;

		kD3D11_DestroySwapChainBuffers();
		d3d11.swapchain.native->ResizeBuffers(K_BACK_BUFFER_COUNT, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		kD3D11_CreateSwapChainBuffers();
	}
}

static void kD3D11_DestroySwapChain(void)
{
	kLogInfoEx("D3D11", "Destroying swap chain.\n");

	kD3D11_Flush();

	if (d3d11.swapchain.native)
	{
		kD3D11_DestroySwapChainBuffers();
		kRelease(&d3d11.swapchain.native);
		for (int i = 0; i < kComputeShader_Count; ++i)
			kRelease(&d3d11.swapchain.csbuffers[i]);
	}
}

static void kD3D11_CreateSwapChain(void *_window)
{
	kLogInfoEx("D3D11", "Creating swap chain.\n");

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
	HRESULT          hr = g_Factory->CreateSwapChainForHwnd(g_Device, wnd, &swap_chain_desc, 0, 0, &swapchain1);
	kAssert(hr != DXGI_ERROR_INVALID_CALL);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create swap chain");
		return;
	}

	d3d11.swapchain.native = swapchain1;

	for (int i = 0; i < kComputeShader_Count; ++i)
	{
		if (!kD3D11_ComputeBufferSizes[i]) continue;
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth         = kAlignUp(kD3D11_ComputeBufferSizes[i], 16);
		desc.Usage             = D3D11_USAGE_DYNAMIC;
		desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr             = g_Device->CreateBuffer(&desc, 0, &d3d11.swapchain.csbuffers[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create constant buffer");
		}
	}

	kD3D11_CreateSwapChainBuffers();
}

static void kD3D11_Present(void) { d3d11.swapchain.native->Present(1, 0); }

static void kD3D11_ExecuteRenderPass(const kD3D11_RenderPass &pass, const kRenderFrame &frame)
{
	if (pass.flags & kD3D11_RenderPass_Disabled) return;

	b32 compute = (pass.flags & kD3D11_RenderPass_Compute);

	if (!compute)
	{
		if (pass.flags & kD3D11_RenderPass_ClearColor)
		{
			g_DeviceContext->ClearRenderTargetView(pass.rt->rtv, pass.color.m);
		}

		if (pass.flags & kD3D11_RenderPass_ClearDepth)
		{
			g_DeviceContext->ClearDepthStencilView(pass.ds->dsv, D3D11_CLEAR_DEPTH, pass.depth, 0);
		}

		g_DeviceContext->OMSetRenderTargets(1, &pass.rt->rtv, pass.ds ? pass.ds->dsv : nullptr);

		pass.Execute(pass, frame);

		if (pass.resolve)
		{
			g_DeviceContext->ResolveSubresource(pass.resolve->texture2d, 0, pass.rt->texture2d, 0, pass.format);
		}
	}
	else
	{
		pass.Execute(pass, frame);
	}

	g_DeviceContext->ClearState();
}

static void kD3D11_ExecuteFrame(const kRenderFrame &frame)
{
	for (int pass = 0; pass < kRenderPass_Count; ++pass)
	{
		kD3D11_ExecuteRenderPass(d3d11.swapchain.passes[pass], frame);
	}
}

//
// Resources
//

static void kD3D11_DestroyRenderResources2D(void)
{
	kRelease(&d3d11.resource.render2d.vertex);
	kRelease(&d3d11.resource.render2d.index);
	kRelease(&d3d11.resource.render2d.xform);
	kRelease(&d3d11.resource.render2d.input);
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

	HRESULT hr                = g_Device->CreateBuffer(&cb_desc, 0, &d3d11.resource.render2d.xform);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to allocate constant buffer");
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC attrs[] = {
		{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	hr = g_Device->CreateInputLayout(attrs, kArrayCount(attrs), vs.data, vs.count, &d3d11.resource.render2d.input);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create input layout");
		return false;
	}

	return true;
}

static void kD3D11_TextureDtor(kD3D11_Texture *r, void *data)
{
	kRelease(&r->srv);
	kRelease(&r->texture2d);
}

static void kD3D11_DestroyRenderResources(void)
{
	kLogInfoEx("D3D11", "Destroying resources.\n");

	kD3D11_DestroyRenderResources2D();
	d3d11.resource.textures.Destroy(kD3D11_TextureDtor, 0);
}

//
// Backend
//

static void kD3D11_DestroyRenderBackend(void)
{
	kD3D11_DestroyRenderResources();
	kD3D11_DestroyGraphicsDevice();
}

bool kD3D11_CreateRenderBackend(kRenderBackend *backend)
{
	kLogInfoEx("D3D11", "Creating graphics devices\n");
	if (!kD3D11_CreateGraphicsDevice())
	{
		return false;
	}

	kLogInfoEx("D3D11", "Creating render 2d resources.\n");
	if (!kD3D11_CreateRenderResources2D())
	{
		kD3D11_DestroyRenderBackend();
		return false;
	}

	kLogInfoEx("D3D11", "Registering render backend.\n");

	kDListInit(&d3d11.resource.textures.first);

	backend->CreateSwapChain  = kD3D11_CreateSwapChain;
	backend->DestroySwapChain = kD3D11_DestroySwapChain;
	backend->ResizeSwapChain  = kD3D11_ResizeSwapChainBuffers;
	backend->Present          = kD3D11_Present;

	backend->CreateTexture    = kD3D11_CreateTexture;
	backend->DestroyTexture   = kD3D11_DestroyTexture;
	backend->GetTextureSize   = kD3D11_GetTextureSize;
	backend->ResizeTexture    = kD3D11_ResizeTexture;

	backend->ExecuteFrame     = kD3D11_ExecuteFrame;
	backend->NextFrame        = kNextFrameFallback;
	backend->Flush            = kD3D11_Flush;

	backend->Destroy          = kD3D11_DestroyRenderBackend;

	return true;
}

#endif
