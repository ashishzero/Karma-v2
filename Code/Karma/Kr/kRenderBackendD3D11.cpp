#include "kRenderBackend.h"
#include "kLinkedList.h"
#include "kMath.h"
#include "kRenderResource.h"
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

constexpr uint K_BACK_BUFFER_COUNT            = 3;
constexpr uint K_MAX_BLOOM_BLUR_MIPS          = 16;
constexpr uint K_INITIAL_VERTEX_BUFFER2D_SIZE = 524288;
constexpr uint K_INITIAL_INDEX_BUFFER2D_SIZE  = 524288 * 6;
constexpr uint K_MAX_BLOOM_MIPS               = 6;
constexpr int  K_MAX_ACTIVE_TEXTURES          = 16384;

enum kD3D11_RasterizerState
{
	kD3D11_RasterizerState_ScissorDisabled,
	kD3D11_RasterizerState_ScissorEnabled,
	kD3D11_RasterizerState_Count,
};

enum kD3D11_Buffer
{
	kD3D11_Buffer_BloomParams,
	kD3D11_Buffer_TonemapParams,
	kD3D11_Buffer_Transform2D,
	kD3D11_Buffer_OutLineStyle2D,
	kD3D11_Buffer_Vertex2D,
	kD3D11_Buffer_Index2D,
	kD3D11_Buffer_Count
};

struct kD3D11_BloomTarget
{
	ID3D11UnorderedAccessView *MipsUAV[K_MAX_BLOOM_MIPS];
	ID3D11ShaderResourceView  *MipsSRV[K_MAX_BLOOM_MIPS];
	ID3D11Texture2D           *Buffer;
};

struct alignas(16) kD3D11_BloomParams
{
	kVec2 FilterRadius;
	float MixStrength;
};

struct alignas(16) kD3D11_TonemapParams
{
	kVec3 Intensity;
};

struct alignas(16) kD3D11_OutLineStyle
{
	kVec4 OutLineColorWidth;
};

struct kD3D11_RenderTargetConfig
{
	kVec4       Clear;
	kFormat     Format;
	u32         Multisamples;
	u32         Factor;
	const char *Name;
};

struct kD3D11_RenderTarget
{
	ID3D11RenderTargetView   *RTV;
	ID3D11ShaderResourceView *SRV;
	ID3D11DepthStencilView   *DSV;
	ID3D11Texture2D          *Resource;
	ID3D11Texture2D          *Resolved;
	ID3D11Texture2D          *DepthBuffer;
	kVec4                     Clear;
	DXGI_FORMAT               Format;
};

struct kD3D11_RenderPipeline
{
	kRenderPipelineConfig     Config;
	kVec2u                    BackBufferSize;
	ID3D11Texture2D          *BackBuffer;
	ID3D11RenderTargetView   *BackBufferRTV;
	kD3D11_RenderTarget       RenderTargets[kRenderPass_Count];
	int                       BloomMaxMipLevel;
	kVec2u                    BloomMipDispatchs[K_MAX_BLOOM_MIPS];
	kD3D11_BloomTarget        BloomTargets[2];
	ID3D11ShaderResourceView *PostProcessOutput;
};

struct kD3D11_TextureData
{
	ID3D11ShaderResourceView *SRV;
	ID3D11RenderTargetView   *RTV;
	kResourceState            State;
};

struct kD3D11_TexturePool
{
	ID3D11Texture2D   **Resources;
	kD3D11_TextureData *Data;
	kResourceIndex      Index;
};

//
//
//

static ID3D11Device1           *g_Device;
static ID3D11DeviceContext1    *g_DeviceContext;
static IDXGIFactory2           *g_Factory;

static ID3D11RasterizerState   *g_RasterizerStates[kD3D11_RasterizerState_Count];
static ID3D11DepthStencilState *g_DepthStencilStates[kDepthTest_Count];
static ID3D11SamplerState      *g_SamplerStates[kTextureFilter_Count];
static ID3D11BlendState        *g_BlendStates[kBlendMode_Count];

static ID3D11VertexShader      *g_VertexShaders[kVertexShader_Count];
static ID3D11PixelShader       *g_PixelShaders[kPixelShader_Count];
static ID3D11ComputeShader     *g_ComputeShaders[kComputeShader_Count];

static ID3D11InputLayout       *g_InputLayoutVertex2D;

static ID3D11Buffer            *g_Buffers[kD3D11_Buffer_Count];
static UINT                     g_VertexBufferSize2D;
static UINT                     g_IndexBufferSize2D;

static kD3D11_TexturePool       g_Textures;

static IDXGISwapChain1         *g_SwapChain;
static kD3D11_RenderPipeline    g_RenderPipeline;

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

static constexpr BOOL kD3D11_ScissorMap[]     = {FALSE, TRUE};
static const kString  kD3D11_ScissorMapName[] = {"Default", "Scissor"};
static_assert(kArrayCount(kD3D11_ScissorMap) == kD3D11_RasterizerState_Count, "");
static_assert(kArrayCount(kD3D11_ScissorMapName) == kD3D11_RasterizerState_Count, "");

static constexpr D3D11_FILTER kD3D11_FilterMap[] = {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                                                    D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_FILTER_MIN_MAG_MIP_POINT};
static_assert(kArrayCount(kD3D11_FilterMap) == kTextureFilter_Count, "");

static constexpr D3D11_TEXTURE_ADDRESS_MODE kD3D11_TextureAddressModeMap[] = {
	D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_CLAMP};
static_assert(kArrayCount(kD3D11_TextureAddressModeMap) == kTextureFilter_Count, "");

static constexpr UINT kD3D11_BufferSizeMap[] = {
	sizeof(kD3D11_BloomParams),  sizeof(kD3D11_TonemapParams),   sizeof(kMat4),
	sizeof(kD3D11_OutLineStyle), K_INITIAL_VERTEX_BUFFER2D_SIZE, K_INITIAL_INDEX_BUFFER2D_SIZE};
static_assert(kArrayCount(kD3D11_BufferSizeMap) == kD3D11_Buffer_Count, "");

static constexpr D3D11_USAGE kD3D11_BufferUsageMap[] = {D3D11_USAGE_DEFAULT, D3D11_USAGE_DEFAULT, D3D11_USAGE_DYNAMIC,
                                                        D3D11_USAGE_DYNAMIC, D3D11_USAGE_DYNAMIC, D3D11_USAGE_DYNAMIC};
static_assert(kArrayCount(kD3D11_BufferUsageMap) == kD3D11_Buffer_Count, "");

static constexpr UINT kD3D11_BufferBindMap[] = {D3D11_BIND_CONSTANT_BUFFER, D3D11_BIND_CONSTANT_BUFFER,
                                                D3D11_BIND_CONSTANT_BUFFER, D3D11_BIND_CONSTANT_BUFFER,
                                                D3D11_BIND_VERTEX_BUFFER,   D3D11_BIND_INDEX_BUFFER};
static_assert(kArrayCount(kD3D11_BufferBindMap) == kD3D11_Buffer_Count, "");

static constexpr UINT kD3D11_BufferCPUAccessMap[] = {
	0, 0, D3D11_CPU_ACCESS_WRITE, D3D11_CPU_ACCESS_WRITE, D3D11_CPU_ACCESS_WRITE, D3D11_CPU_ACCESS_WRITE};
static_assert(kArrayCount(kD3D11_BufferCPUAccessMap) == kD3D11_Buffer_Count, "");

static const kString kD3D11_BufferStrings[] = {"BloomParams",    "TonemapParams", "Transform2D",
                                               "OutLineStyle2D", "Vertex2D",      "Index2D"};
static_assert(kArrayCount(kD3D11_BufferStrings) == kD3D11_Buffer_Count, "");

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

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
#pragma comment(lib, "dxguid.lib")
static void kD3D11_Name(ID3D11DeviceChild *dev, kString name)
{
	dev->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.Count, name.Items);
}
static void kD3D11_NameFmt(ID3D11DeviceChild *dev, const char *fmt, ...)
{
	char    buff[512];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buff, kArrayCount(buff), fmt, args);
	va_end(args);
	kD3D11_Name(dev, kString(buff, len));
}
#else
#define kD3D11_Name(...)
#define kD3D11_NameFmt(...)
#endif

//
// Texture Pool
//

static void kD3D11_DestroyTexturePool(void)
{
	if (g_Textures.Resources && g_Textures.Data)
	{
		for (int i = 0; i < K_MAX_ACTIVE_TEXTURES; ++i)
		{
			kRelease(&g_Textures.Data[i].SRV);
			kRelease(&g_Textures.Resources[i]);
		}
	}

	umem res_size  = sizeof(ID3D11Texture2D *) * K_MAX_ACTIVE_TEXTURES;
	umem data_size = sizeof(kD3D11_TextureData) * K_MAX_ACTIVE_TEXTURES;

	if (g_Textures.Data)
	{
		kFree(g_Textures.Data, data_size);
		g_Textures.Data = nullptr;
	}

	if (g_Textures.Resources)
	{
		kFree(g_Textures.Resources, res_size);
		g_Textures.Resources = nullptr;
	}

	kDestroyResourceIndex(&g_Textures.Index);
}

static bool kD3D11_CreateTexturePool(void)
{
	if (!kCreateResourceIndex(&g_Textures.Index, K_MAX_ACTIVE_TEXTURES))
	{
		return false;
	}

	umem res_size        = sizeof(ID3D11Texture2D *) * K_MAX_ACTIVE_TEXTURES;
	umem data_size       = sizeof(kD3D11_TextureData) * K_MAX_ACTIVE_TEXTURES;

	g_Textures.Resources = (ID3D11Texture2D **)kAlloc(res_size);
	g_Textures.Data      = (kD3D11_TextureData *)kAlloc(data_size);

	if (!g_Textures.Resources || !g_Textures.Data)
	{
		kD3D11_DestroyTexturePool();
		return false;
	}

	memset(g_Textures.Resources, 0, res_size);
	memset(g_Textures.Data, 0, data_size);

	//
	//
	//

	D3D11_TEXTURE2D_DESC desc      = {};
	desc.Width                     = 1;
	desc.Height                    = 1;
	desc.MipLevels                 = 1;
	desc.ArraySize                 = 1;
	desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count          = 1;
	desc.SampleDesc.Quality        = 0;
	desc.Usage                     = D3D11_USAGE_DEFAULT;
	desc.BindFlags                 = D3D11_BIND_SHADER_RESOURCE;

	u8                     black[] = {0x00, 0x00, 0x00, 0xff};

	D3D11_SUBRESOURCE_DATA data    = {};
	data.pSysMem                   = black;
	data.SysMemPitch               = 4;

	HRESULT hr                     = g_Device->CreateTexture2D(&desc, &data, &g_Textures.Resources[0]);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create fallback texture");
		kD3D11_DestroyTexturePool();
		return false;
	}

	hr = g_Device->CreateShaderResourceView(g_Textures.Resources[0], 0, &g_Textures.Data[0].SRV);
	if (FAILED(hr))
	{
		g_Textures.Resources[0]->Release();
		kLogHresultError(hr, "D3D11", "Failed to create fallback texture view");
		kD3D11_DestroyTexturePool();
		return false;
	}

	g_Textures.Data[0].State = kResourceState_Ready;

	return true;
}

//
// Device and State
//

static constexpr D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};

static IDXGIAdapter1              *kD3D11_FindAdapter(IDXGIFactory2 *factory, UINT flags)
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
	kD3D11_DestroyTexturePool();

	kLogInfoEx("D3D11", "Destroying graphics devices.\n");

	for (ID3D11RasterizerState *&rs : g_RasterizerStates)
		kRelease(&rs);
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
	for (ID3D11Buffer *&buff : g_Buffers)
		kRelease(&buff);

	kRelease(&g_InputLayoutVertex2D);

	g_VertexBufferSize2D = g_IndexBufferSize2D = 0;

	kRelease(&g_Device);
	kRelease(&g_DeviceContext);
	kRelease(&g_Factory);
}

static bool kD3D11_CreateRasterizerState(void)
{
	for (int i = 0; i < kD3D11_RasterizerState_Count; ++i)
	{
		D3D11_RASTERIZER_DESC ras_desc = {};
		ras_desc.FillMode              = D3D11_FILL_SOLID;
		ras_desc.CullMode              = D3D11_CULL_NONE;
		ras_desc.ScissorEnable         = kD3D11_ScissorMap[i];
		ras_desc.MultisampleEnable     = TRUE;
		ras_desc.AntialiasedLineEnable = TRUE;

		HRESULT hr                     = g_Device->CreateRasterizerState(&ras_desc, &g_RasterizerStates[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create rasterizer");
			return false;
		}

		kD3D11_Name(g_RasterizerStates[i], kD3D11_ScissorMapName[i]);
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
			kLogHresultError(hr, "D3D11", "Failed to create depth stencil state");
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
			kLogHresultError(hr, "D3D11", "Failed to create depth stencil state");
			return false;
		}
	}

	for (int i = 0; i < kDepthTest_Count; ++i)
	{
		kD3D11_Name(g_DepthStencilStates[i], kDepthTestStrings[i]);
	}

	return true;
}

static bool kD3D11_CreateSamplerState(void)
{
	for (int filter = 0; filter < kTextureFilter_Count; ++filter)
	{
		D3D11_SAMPLER_DESC sampler = {};
		sampler.Filter             = kD3D11_FilterMap[filter];
		sampler.AddressU           = kD3D11_TextureAddressModeMap[filter];
		sampler.AddressV           = kD3D11_TextureAddressModeMap[filter];
		sampler.AddressW           = kD3D11_TextureAddressModeMap[filter];
		sampler.MipLODBias         = 0.0f;
		sampler.MaxAnisotropy      = 1;
		sampler.ComparisonFunc     = D3D11_COMPARISON_NEVER;
		sampler.MinLOD             = -FLT_MAX;
		sampler.MaxLOD             = FLT_MAX;

		HRESULT hr                 = g_Device->CreateSamplerState(&sampler, &g_SamplerStates[filter]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create sampler");
			return false;
		}

		kD3D11_Name(g_SamplerStates[filter], kTextureFilterStrings[filter]);
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
			kLogHresultError(hr, "D3D11", "Failed to create alpha blend state");
			return false;
		}
		kD3D11_Name(g_BlendStates[mode], kBlendModeStrings[mode]);
	}

	return true;
}

static bool kD3D11_CreateVertexShaders(void)
{
	kLogInfoEx("D3D11", "Compiling vertex shaders.\n");

	for (int i = 0; i < kVertexShader_Count; ++i)
	{
		kString vs = kVertexShadersMap[i];
		HRESULT hr = g_Device->CreateVertexShader(vs.Items, vs.Count, 0, &g_VertexShaders[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create vertex shader (%d)", i);
			return false;
		}

		kD3D11_Name(g_VertexShaders[i], kVertexShaderStrings[i]);
	}

	D3D11_INPUT_ELEMENT_DESC attrs[] = {
		{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	kString vs = kVertexShadersMap[kVertexShader_Quad];

	HRESULT hr = g_Device->CreateInputLayout(attrs, kArrayCount(attrs), vs.Items, vs.Count, &g_InputLayoutVertex2D);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create input layout");
		return false;
	}

	kD3D11_Name(g_InputLayoutVertex2D, "InputLayoutVertex2D");

	return true;
}

static bool kD3D11_CreatePixelShaders(void)
{
	kLogInfoEx("D3D11", "Compiling pixel shaders.\n");

	for (int i = 0; i < kPixelShader_Count; ++i)
	{
		kString ps = kPixelShadersMap[i];
		HRESULT hr = g_Device->CreatePixelShader(ps.Items, ps.Count, 0, &g_PixelShaders[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create pixel shader (%d)", i);
			return false;
		}
		kD3D11_Name(g_PixelShaders[i], kPixelShaderStrings[i]);
	}

	return true;
}

static bool kD3D11_CreateComputeShaders(void)
{
	kLogInfoEx("D3D11", "Compiling compute shaders.\n");

	for (int i = 0; i < kComputeShader_Count; ++i)
	{
		kString cs = kComputeShadersMap[i];
		HRESULT hr = g_Device->CreateComputeShader(cs.Items, cs.Count, 0, &g_ComputeShaders[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create compute shader (%d)", i);
			return false;
		}
		kD3D11_Name(g_ComputeShaders[i], kComputeShaderStrings[i]);
	}

	return true;
}

static bool kD3D11_CreateBuffers(void)
{
	for (int i = 0; i < kD3D11_Buffer_Count; ++i)
	{
		D3D11_BUFFER_DESC buffer = {};
		buffer.ByteWidth         = kD3D11_BufferSizeMap[i];
		buffer.Usage             = kD3D11_BufferUsageMap[i];
		buffer.BindFlags         = kD3D11_BufferBindMap[i];
		buffer.CPUAccessFlags    = kD3D11_BufferCPUAccessMap[i];

		HRESULT hr               = g_Device->CreateBuffer(&buffer, 0, &g_Buffers[i]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create dynamic buffer (%d)", i);
			return false;
		}

		kD3D11_Name(g_Buffers[i], kD3D11_BufferStrings[i]);
	}

	g_VertexBufferSize2D = kD3D11_BufferSizeMap[kD3D11_Buffer_Vertex2D];
	g_IndexBufferSize2D  = kD3D11_BufferSizeMap[kD3D11_Buffer_Index2D];

	return true;
}

static bool kD3D11_TryCreateGraphicsDevice(void)
{
	IDXGIAdapter1 *adapter      = nullptr;
	UINT           device_flags = 0;
	UINT           flags        = 0;

	for (int i = 0; i < 2; ++i)
	{
		if (kEnableDebugLayer)
			flags |= DXGI_CREATE_FACTORY_DEBUG;

		if (!g_Factory)
		{
			HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&g_Factory));
			if (FAILED(hr))
			{
				hr = CreateDXGIFactory1(IID_PPV_ARGS(&g_Factory));
				if (FAILED(hr))
				{
					kLogHresultError(hr, "D3D11", "Failed to create factory");
					return false;
				}
			}
		}

		device_flags = 0;
		if (kEnableDebugLayer)
			device_flags |= D3D11_CREATE_DEVICE_DEBUG;

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
		if (adapter)
			adapter->Release();
	};

	{
		ID3D11Device *device = nullptr;
		HRESULT       hr     = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, device_flags, FeatureLevels,
		                                         kArrayCount(FeatureLevels), D3D11_SDK_VERSION, &device, nullptr, nullptr);
		kAssert(hr != E_INVALIDARG);

		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create device");
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

	if (!kD3D11_CreateRasterizerState())
		return false;
	if (!kD3D11_CreateDepthStencilState())
		return false;
	if (!kD3D11_CreateBlendState())
		return false;
	if (!kD3D11_CreateSamplerState())
		return false;
	if (!kD3D11_CreateVertexShaders())
		return false;
	if (!kD3D11_CreatePixelShaders())
		return false;
	if (!kD3D11_CreateComputeShaders())
		return false;
	if (!kD3D11_CreateBuffers())
		return false;

	if (!kD3D11_CreateTexturePool())
		return false;

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
// Buffer Uploads
//

static bool kD3D11_UploadVertices2D(kSpan<kVertex2D> vertices)
{
	uint vb_size = (uint)vertices.Size();

	if (vb_size > g_VertexBufferSize2D)
	{
		kRelease(&g_Buffers[kD3D11_Buffer_Vertex2D]);
		g_VertexBufferSize2D      = 0;

		D3D11_BUFFER_DESC vb_desc = {};
		vb_desc.ByteWidth         = vb_size;
		vb_desc.Usage             = D3D11_USAGE_DYNAMIC;
		vb_desc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
		vb_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr                = g_Device->CreateBuffer(&vb_desc, 0, &g_Buffers[kD3D11_Buffer_Vertex2D]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to allocate vertex buffer");
			return false;
		}

		g_VertexBufferSize2D = vb_size;
	}

	ID3D11Buffer            *buffer = g_Buffers[kD3D11_Buffer_Vertex2D];

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT                  hr = g_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to map vertex2d buffer");
		return false;
	}
	memcpy(mapped.pData, vertices.Items, vb_size);
	g_DeviceContext->Unmap(buffer, 0);
	return true;
}

static bool kD3D11_UploadIndices2D(kSpan<kIndex2D> indices)
{
	uint ib_size = (uint)indices.Size();

	if (ib_size > g_IndexBufferSize2D)
	{
		kRelease(&g_Buffers[kD3D11_Buffer_Index2D]);
		g_IndexBufferSize2D       = 0;

		D3D11_BUFFER_DESC ib_desc = {};
		ib_desc.ByteWidth         = ib_size;
		ib_desc.Usage             = D3D11_USAGE_DYNAMIC;
		ib_desc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
		ib_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr                = g_Device->CreateBuffer(&ib_desc, 0, &g_Buffers[kD3D11_Buffer_Index2D]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to allocate index buffer");
			return false;
		}

		g_IndexBufferSize2D = ib_size;
	}

	ID3D11Buffer            *buffer = g_Buffers[kD3D11_Buffer_Index2D];

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT                  hr = g_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to map index2d buffer");
		return false;
	}
	memcpy(mapped.pData, indices.Items, ib_size);
	g_DeviceContext->Unmap(buffer, 0);

	return true;
}

static bool kD3D11_UploadTransform2D(const kMat4 &transform)
{
	ID3D11Buffer            *xform = g_Buffers[kD3D11_Buffer_Transform2D];
	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT                  hr = g_DeviceContext->Map(xform, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to map constant buffer");
		return false;
	}
	memcpy(mapped.pData, transform.m, sizeof(transform));
	g_DeviceContext->Unmap(xform, 0);
	return true;
}

static bool kD3D11_UploadOutLineStyle2D(const kVec4 &ow)
{
	ID3D11Buffer            *buffer = g_Buffers[kD3D11_Buffer_OutLineStyle2D];
	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT                  hr = g_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to map constant buffer");
		return false;
	}

	kD3D11_OutLineStyle params;
	params.OutLineColorWidth = ow;

	memcpy(mapped.pData, &params, sizeof(params));
	g_DeviceContext->Unmap(buffer, 0);
	return true;
}

//
// Render Passes
//

static ID3D11ShaderResourceView *kD3D11_GetTextureSRV(kTexture texture)
{
	kAssert(texture.ID.Index < K_MAX_ACTIVE_TEXTURES);
	kAssert(kIsResourceValid(&g_Textures.Index, texture.ID));
	kD3D11_TextureData &r = g_Textures.Data[texture.ID.Index];
	if (r.State == kResourceState_Ready)
		return r.SRV;
	return g_Textures.Data[0].SRV;
}

static void kD3D11_BeginRenderPass(kD3D11_RenderTarget &rt)
{
	g_DeviceContext->ClearRenderTargetView(rt.RTV, rt.Clear.m);
	if (rt.DSV)
	{
		g_DeviceContext->ClearDepthStencilView(rt.DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
}

static void kD3D11_EndRenderPass(kD3D11_RenderTarget &rt)
{
	if (rt.Resolved)
	{
		g_DeviceContext->ResolveSubresource(rt.Resolved, 0, rt.Resource, 0, rt.Format);
	}
}

static void kD3D11_RenderFrame2D(const kRenderFrame2D &render)
{
	ID3D11Buffer *xform  = g_Buffers[kD3D11_Buffer_Transform2D];
	ID3D11Buffer *vtx    = g_Buffers[kD3D11_Buffer_Vertex2D];
	ID3D11Buffer *idx    = g_Buffers[kD3D11_Buffer_Index2D];
	HRESULT       hr     = S_OK;
	UINT          stride = sizeof(kVertex2D);
	UINT          offset = 0;
	DXGI_FORMAT   format = sizeof(kIndex2D) == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	if (!vtx || !idx)
		return;

	g_DeviceContext->IASetVertexBuffers(0, 1, &vtx, &stride, &offset);
	g_DeviceContext->IASetIndexBuffer(idx, format, 0);
	g_DeviceContext->VSSetConstantBuffers(0, 1, &xform);
	g_DeviceContext->VSSetShader(g_VertexShaders[kVertexShader_Quad], nullptr, 0);
	g_DeviceContext->PSSetShader(g_PixelShaders[kPixelShader_Quad], nullptr, 0);
	g_DeviceContext->PSSetConstantBuffers(0, 1, &g_Buffers[kD3D11_Buffer_OutLineStyle2D]);
	g_DeviceContext->IASetInputLayout(g_InputLayoutVertex2D);
	g_DeviceContext->RSSetState(g_RasterizerStates[kD3D11_RasterizerState_ScissorEnabled]);
	g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStates[kDepthTest_LessEquals], 0);
	g_DeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	i32 vtx_offset = 0;
	u32 idx_offset = 0;

	for (int pass = 0; pass < kRenderPass_Count; ++pass)
	{
		kD3D11_RenderTarget &rt = g_RenderPipeline.RenderTargets[pass];
		kD3D11_BeginRenderPass(rt);

		for (const kRenderScene &scene : render.Scenes[pass])
		{
			g_DeviceContext->OMSetRenderTargets(1, &rt.RTV, rt.DSV);

			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = (float)scene.Viewport.x;
			viewport.TopLeftY = (float)scene.Viewport.y;
			viewport.Width    = (float)scene.Viewport.w;
			viewport.Height   = (float)scene.Viewport.h;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			g_DeviceContext->RSSetViewports(1, &viewport);

			kMat4 proj;

			if (scene.CameraView.Type == kCameraView_Orthographic)
			{
				auto &view = scene.CameraView.Orthographic;
				proj       = kOrthographicLH(view.Left, view.Right, view.Top, view.Bottom, view.Near, view.Far);
			}
			else
			{
				auto &view = scene.CameraView.Perspective;
				proj       = kPerspectiveLH(view.FieldOfView, view.AspectRatio, view.Near, view.Far);
			}

			if (!kD3D11_UploadTransform2D(proj))
				return;

			for (u32 index = scene.Commands.Beg; index < scene.Commands.End; ++index)
			{
				const kRenderCommand2D &cmd = render.Commands[index];

				if (cmd.Flags & kRenderDirty_OutLineStyle)
				{
					kVec4 param = render.OutLineStyles[cmd.OutLineStyle];
					if (!kD3D11_UploadOutLineStyle2D(param))
						continue;
				}

				if (cmd.Flags & kRenderDirty_Blend)
				{
					g_DeviceContext->OMSetBlendState(g_BlendStates[cmd.BlendMode], 0, 0xffffffff);
				}

				if (cmd.Flags & kRenderDirty_TextureFilter)
				{
					g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerStates[cmd.TextureFilter]);
				}

				if (cmd.Flags & kRenderDirty_Rect)
				{
					D3D11_RECT rect;
					kRect      krect = render.Rects[cmd.Rect];
					rect.left        = (LONG)(krect.min.x);
					rect.top         = (LONG)(krect.min.y);
					rect.right       = (LONG)(krect.max.x);
					rect.bottom      = (LONG)(krect.max.y);
					g_DeviceContext->RSSetScissorRects(1, &rect);
				}

				if (cmd.Flags & kRenderDirty_TextureColor)
				{
					ID3D11ShaderResourceView *r = kD3D11_GetTextureSRV(cmd.Textures[kTextureType_Color]);
					g_DeviceContext->PSSetShaderResources(kTextureType_Color, 1, &r);
				}

				if (cmd.Flags & kRenderDirty_TextureMaskSDF)
				{
					ID3D11ShaderResourceView *r = kD3D11_GetTextureSRV(cmd.Textures[kTextureType_MaskSDF]);
					g_DeviceContext->PSSetShaderResources(kTextureType_MaskSDF, 1, &r);
				}

				g_DeviceContext->DrawIndexed(cmd.IndexCount, idx_offset, vtx_offset);
				vtx_offset += cmd.VertexCount;
				idx_offset += cmd.IndexCount;
			}
		}

		kD3D11_EndRenderPass(rt);
	}
}

static void kD3D11_ExecuteGeometryPass(const kRenderFrame &frame)
{
	bool vtx_uploaded = false;
	bool idx_uploaded = false;

	if (frame.Frame2D.Vertices.Count != 0)
	{
		vtx_uploaded = kD3D11_UploadVertices2D(frame.Frame2D.Vertices);
	}

	if (frame.Frame2D.Indices.Count != 0)
	{
		idx_uploaded = kD3D11_UploadIndices2D(frame.Frame2D.Indices);
	}

	kD3D11_RenderFrame2D(frame.Frame2D);
	g_DeviceContext->ClearState();
}

static void kD3D11_ExecuteTonemapPass(void)
{
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width    = (float)g_RenderPipeline.BackBufferSize.x;
	viewport.Height   = (float)g_RenderPipeline.BackBufferSize.y;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	g_DeviceContext->OMSetRenderTargets(1, &g_RenderPipeline.BackBufferRTV, 0);
	g_DeviceContext->RSSetViewports(1, &viewport);
	g_DeviceContext->VSSetShader(g_VertexShaders[kVertexShader_Blit], 0, 0);
	g_DeviceContext->PSSetShader(g_PixelShaders[kPixelShader_Tonemap], 0, 0);
	g_DeviceContext->PSSetShaderResources(0, 1, &g_RenderPipeline.PostProcessOutput);
	g_DeviceContext->PSSetShaderResources(1, 1, &g_RenderPipeline.RenderTargets[kRenderPass_HUD].SRV);
	g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerStates[kTextureFilter_LinearWrap]);
	g_DeviceContext->PSSetConstantBuffers(0, 1, &g_Buffers[kD3D11_Buffer_TonemapParams]);
	g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStates[kDepthTest_Disabled], 0);
	g_DeviceContext->RSSetState(g_RasterizerStates[kD3D11_RasterizerState_ScissorDisabled]);
	g_DeviceContext->OMSetBlendState(g_BlendStates[kBlendMode_Opaque], 0, 0xffffffff);
	g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_DeviceContext->Draw(4, 0);
	g_DeviceContext->ClearState();
}

static void kD3D11_ExecuteMixPass(void)
{
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width    = (float)g_RenderPipeline.BackBufferSize.x;
	viewport.Height   = (float)g_RenderPipeline.BackBufferSize.y;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	g_DeviceContext->RSSetViewports(1, &viewport);
	g_DeviceContext->OMSetRenderTargets(1, &g_RenderPipeline.BackBufferRTV, 0);
	g_DeviceContext->VSSetShader(g_VertexShaders[kVertexShader_Blit], nullptr, 0);
	g_DeviceContext->PSSetShader(g_PixelShaders[kPixelShader_Mix], nullptr, 0);
	g_DeviceContext->PSSetShaderResources(0, 1, &g_RenderPipeline.PostProcessOutput);
	g_DeviceContext->PSSetShaderResources(1, 1, &g_RenderPipeline.RenderTargets[kRenderPass_HUD].SRV);
	g_DeviceContext->PSSetSamplers(0, 1, &g_SamplerStates[kTextureFilter_LinearWrap]);
	g_DeviceContext->OMSetBlendState(g_BlendStates[kBlendMode_Opaque], 0, 0xffffffff);
	g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStates[kDepthTest_Disabled], 0);
	g_DeviceContext->RSSetState(g_RasterizerStates[kD3D11_RasterizerState_ScissorDisabled]);
	g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_DeviceContext->Draw(4, 0);
	g_DeviceContext->ClearState();
}

static void kD3D11_ExecuteBloomPass(void)
{
	kD3D11_BloomParams params;
	params.FilterRadius.x       = g_RenderPipeline.Config.BloomFilterRadius;
	params.FilterRadius.y       = g_RenderPipeline.Config.BloomFilterRadius;
	params.MixStrength          = g_RenderPipeline.Config.BloomStrength;

	kVec2u             *runs    = g_RenderPipeline.BloomMipDispatchs;
	kD3D11_BloomTarget &down    = g_RenderPipeline.BloomTargets[0];
	kD3D11_BloomTarget &up      = g_RenderPipeline.BloomTargets[1];
	int                 max_mip = g_RenderPipeline.BloomMaxMipLevel;

	g_DeviceContext->CSSetSamplers(0, 1, &g_SamplerStates[kTextureFilter_LinearClamp]);

	{
		g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_Blit], 0, 0);
		g_DeviceContext->CSSetUnorderedAccessViews(0, 1, &down.MipsUAV[0], 0);
		g_DeviceContext->CSSetShaderResources(0, 1, &g_RenderPipeline.RenderTargets[kRenderPass_Default].SRV);
		g_DeviceContext->Dispatch(runs[0].x, runs[0].y, 1);
	}

	{
		g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_BloomDownSampleKarisAvg], 0, 0);
		g_DeviceContext->CSSetUnorderedAccessViews(0, 1, &down.MipsUAV[1], 0);
		g_DeviceContext->CSSetShaderResources(0, 1, &down.MipsSRV[0]);
		g_DeviceContext->Dispatch(runs[0].x, runs[0].y, 1);
	}

	{
		g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_BloomDownSample], 0, 0);

		for (int iter = 2; iter < max_mip; ++iter)
		{
			g_DeviceContext->CSSetUnorderedAccessViews(0, 1, &down.MipsUAV[iter], 0);
			g_DeviceContext->CSSetShaderResources(0, 1, &down.MipsSRV[iter - 1]);
			g_DeviceContext->Dispatch(runs[iter].x, runs[iter].y, 1);
		}
	}

	g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_BloomBlurUpSample], 0, 0);
	g_DeviceContext->CSSetConstantBuffers(0, 1, &g_Buffers[kD3D11_Buffer_BloomParams]);

	if (max_mip - 2 >= 0)
	{
		int mip = max_mip - 2;
		g_DeviceContext->CSSetUnorderedAccessViews(0, 1, &up.MipsUAV[mip], 0);
		g_DeviceContext->CSSetShaderResources(0, 2, &down.MipsSRV[mip]);
		g_DeviceContext->Dispatch(runs[mip].x, runs[mip].y, 1);
	}

	for (int mip = max_mip - 3; mip >= 1; --mip)
	{
		ID3D11ShaderResourceView *resources[] = {down.MipsSRV[mip], up.MipsSRV[mip + 1]};
		g_DeviceContext->CSSetUnorderedAccessViews(0, 1, &up.MipsUAV[mip], 0);
		g_DeviceContext->CSSetShaderResources(0, 2, resources);
		g_DeviceContext->Dispatch(runs[mip].x, runs[mip].y, 1);
	}

	{
		ID3D11ShaderResourceView *resources[] = {down.MipsSRV[0], up.MipsSRV[1]};
		g_DeviceContext->CSSetShader(g_ComputeShaders[kComputeShader_BloomUpSampleMix], 0, 0);
		g_DeviceContext->CSSetUnorderedAccessViews(0, 1, &up.MipsUAV[0], 0);
		g_DeviceContext->CSSetShaderResources(0, 2, resources);
		g_DeviceContext->Dispatch(runs[0].x, runs[0].y, 1);
	}

	g_DeviceContext->ClearState();
}

//
// Textures
//

static kTexture kD3D11_CreateTexture(const kTextureSpec &spec)
{
	kResourceID ID = kAllocResource(&g_Textures.Index);

	if (ID.Index == 0)
		return ID;

	kD3D11_TextureData  *r    = &g_Textures.Data[ID.Index];

	D3D11_TEXTURE2D_DESC desc = {};

	desc.Width                = spec.Width;
	desc.Height               = spec.Height;
	desc.Format               = kD3D11_FormatMap[spec.Format];
	desc.ArraySize            = 1;
	desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage                = D3D11_USAGE_DEFAULT;
	desc.SampleDesc.Count     = 1;
	desc.SampleDesc.Quality   = 0;
	desc.CPUAccessFlags       = 0;
	desc.MiscFlags            = 0;
	desc.MipLevels            = 1;

	D3D11_SUBRESOURCE_DATA source;
	source.pSysMem          = spec.Pixels;
	source.SysMemPitch      = spec.Pitch;
	source.SysMemSlicePitch = 0;

	HRESULT hr = g_Device->CreateTexture2D(&desc, spec.Pixels ? &source : nullptr, &g_Textures.Resources[ID.Index]);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create texture");
		r->State = kResourceState_Error;
		return ID;
	}

	kD3D11_Name(g_Textures.Resources[ID.Index], spec.Name);

	hr = g_Device->CreateShaderResourceView(g_Textures.Resources[ID.Index], 0, &r->SRV);
	if (FAILED(hr))
	{
		kRelease(&g_Textures.Resources[ID.Index]);
		kLogHresultError(hr, "D3D11", "Failed to create texture view");
		r->State = kResourceState_Error;
		return ID;
	}

	kD3D11_Name(r->SRV, spec.Name);

	r->State = kResourceState_Ready;

	return ID;
}

static void kD3D11_DestroyTexture(kTexture texture)
{
	if (!kIsResourceValid(&g_Textures.Index, texture.ID))
	{
		kLogErrorEx("D3D11", "Referencing deleted texture");
		kDebugTriggerbreakpoint();
		return;
	}
	g_Textures.Data[texture.ID.Index].State = kResourceState_Unready;
	kRelease(&g_Textures.Data[texture.ID.Index].SRV);
	kRelease(&g_Textures.Resources[texture.ID.Index]);
	kFreeResource(&g_Textures.Index, texture.ID);
}

static kVec2u kD3D11_GetTextureSize(kTexture texture)
{
	if (!kIsResourceValid(&g_Textures.Index, texture.ID))
	{
		kLogErrorEx("D3D11", "Referencing deleted texture");
		kDebugTriggerbreakpoint();
		return kVec2u(0);
	}
	D3D11_TEXTURE2D_DESC desc;
	g_Textures.Resources[texture.ID.Index]->GetDesc(&desc);
	return kVec2u(desc.Width, desc.Height);
}

//
// Render Target
//

static void kD3D11_DestroyRenderTarget(kD3D11_RenderTarget *rt)
{
	kRelease(&rt->RTV);
	kRelease(&rt->SRV);
	kRelease(&rt->DSV);
	kRelease(&rt->Resource);
	kRelease(&rt->Resolved);
	kRelease(&rt->DepthBuffer);
}

static bool kD3D11_TryCreateRenderTarget(kD3D11_RenderTarget *rt, const kD3D11_RenderTargetConfig &config)
{
	kVec2u size               = g_RenderPipeline.BackBufferSize / config.Factor;
	size                      = kMax(size, kVec2u(1));

	rt->Clear                 = config.Clear;
	rt->Format                = kD3D11_FormatMap[config.Format];

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width                = size.x;
	desc.Height               = size.y;
	desc.MipLevels            = 1;
	desc.ArraySize            = 1;
	desc.Format               = rt->Format;
	desc.SampleDesc.Quality   = 0;
	desc.SampleDesc.Count     = kMax(config.Multisamples, 1u);
	desc.Usage                = D3D11_USAGE_DEFAULT;
	desc.BindFlags            = D3D11_BIND_RENDER_TARGET;

	if (config.Multisamples <= 1)
	{
		desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	}

	HRESULT hr = g_Device->CreateTexture2D(&desc, 0, &rt->Resource);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create %s render target texture", config.Name);
		return false;
	}

	kD3D11_NameFmt(rt->Resource, "%s.Resource", config.Name);

	hr = g_Device->CreateRenderTargetView(rt->Resource, 0, &rt->RTV);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create %s render target view", config.Name);
		return false;
	}

	kD3D11_NameFmt(rt->RTV, "%s.RTV", config.Name);

	D3D11_TEXTURE2D_DESC ds = desc;
	ds.Format               = DXGI_FORMAT_D32_FLOAT;
	ds.BindFlags            = D3D11_BIND_DEPTH_STENCIL;

	hr                      = g_Device->CreateTexture2D(&ds, 0, &rt->DepthBuffer);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create %s depth texture", config.Name);
		return false;
	}

	kD3D11_NameFmt(rt->DepthBuffer, "%s.DepthBuffer", config.Name);

	hr = g_Device->CreateDepthStencilView(rt->DepthBuffer, 0, &rt->DSV);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create %s depth view", config.Name);
		return false;
	}

	kD3D11_NameFmt(rt->DSV, "%s.DSV", config.Name);

	if (config.Multisamples != 1)
	{
		D3D11_TEXTURE2D_DESC resolved = desc;
		resolved.SampleDesc.Count     = 1;
		resolved.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

		hr                            = g_Device->CreateTexture2D(&resolved, 0, &rt->Resolved);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "D3D11", "Failed to create %s resolve texture", config.Name);
			return false;
		}

		kD3D11_NameFmt(rt->Resolved, "%s.Resolved", config.Name);
	}

	hr = g_Device->CreateShaderResourceView(rt->Resolved ? rt->Resolved : rt->Resource, 0, &rt->SRV);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create %s resolve shader resource view", config.Name);
		return false;
	}

	kD3D11_NameFmt(rt->SRV, "%s.SRV", config.Name);

	return true;
}

static bool kD3D11_CreateRenderTarget(kD3D11_RenderTarget *rt, const kD3D11_RenderTargetConfig &config)
{
	if (!kD3D11_TryCreateRenderTarget(rt, config))
	{
		kD3D11_DestroyRenderTarget(rt);
		return false;
	}
	return true;
}

//
// SwapChain
//

static void kD3D11_DestroySwapChainBuffers(void)
{
	kDebugTraceEx("D3D11", "Destroying swap chain render targets.\n");

	g_RenderPipeline.BackBufferSize   = kVec2u(0);
	g_RenderPipeline.BloomMaxMipLevel = 0;

	for (int iter = 0; iter < kArrayCount(g_RenderPipeline.BloomTargets); ++iter)
	{
		for (int mip = 0; mip < K_MAX_BLOOM_MIPS; ++mip)
		{
			kRelease(&g_RenderPipeline.BloomTargets[iter].MipsUAV[mip]);
			kRelease(&g_RenderPipeline.BloomTargets[iter].MipsSRV[mip]);
		}
		kRelease(&g_RenderPipeline.BloomTargets[iter].Buffer);
	}
	for (int mip = 0; mip < K_MAX_BLOOM_MIPS; ++mip)
	{
		g_RenderPipeline.BloomMipDispatchs[mip] = kVec2u(0);
	}

	for (int iter = 0; iter < kRenderPass_Count; ++iter)
	{
		kD3D11_DestroyRenderTarget(&g_RenderPipeline.RenderTargets[iter]);
	}

	kRelease(&g_RenderPipeline.PostProcessOutput);
	kRelease(&g_RenderPipeline.BackBufferRTV);
	kRelease(&g_RenderPipeline.BackBuffer);
}

static void kD3D11_ReCalculateBloomMaxLevels(void)
{
	g_RenderPipeline.BloomMaxMipLevel = 0;
	kVec2u size                       = g_RenderPipeline.BackBufferSize;
	for (uint mip = 0; mip < K_MAX_BLOOM_MIPS; ++mip)
	{
		if (size.x == 0 || size.y == 0)
		{
			break;
		}

		g_RenderPipeline.BloomMipDispatchs[mip].x = (u32)ceilf((float)size.x / 32.0f);
		g_RenderPipeline.BloomMipDispatchs[mip].y = (u32)ceilf((float)size.y / 16.0f);

		size.x /= 2;
		size.y /= 2;
		g_RenderPipeline.BloomMaxMipLevel += 1;
	}
}

static void kD3D11_UpdateRenderPipelineParameters(void)
{
	kD3D11_BloomParams bloom     = {};
	bloom.FilterRadius           = kVec2(g_RenderPipeline.Config.BloomFilterRadius);
	bloom.MixStrength            = g_RenderPipeline.Config.BloomStrength;

	kD3D11_TonemapParams tonemap = {};
	tonemap.Intensity            = g_RenderPipeline.Config.Intensity;

	g_DeviceContext->UpdateSubresource(g_Buffers[kD3D11_Buffer_BloomParams], 0, 0, &bloom, sizeof(bloom), 1);
	g_DeviceContext->UpdateSubresource(g_Buffers[kD3D11_Buffer_TonemapParams], 0, 0, &tonemap, sizeof(tonemap), 1);
}

static void kD3D11_CreateSwapChainBuffers(void)
{
	HRESULT hr = S_OK;

	//
	// Back Buffer
	//

	hr = g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&g_RenderPipeline.BackBuffer));
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to retrive back buffer texture");
		return;
	}

	D3D11_TEXTURE2D_DESC backbuffer_desc;
	g_RenderPipeline.BackBuffer->GetDesc(&backbuffer_desc);

	g_RenderPipeline.BackBufferSize.x = backbuffer_desc.Width;
	g_RenderPipeline.BackBufferSize.y = backbuffer_desc.Height;

	hr = g_Device->CreateRenderTargetView(g_RenderPipeline.BackBuffer, 0, &g_RenderPipeline.BackBufferRTV);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create back buffer render target view");
		return;
	}

	kD3D11_Name(g_RenderPipeline.BackBuffer, "BackBuffer");
	kD3D11_Name(g_RenderPipeline.BackBufferRTV, "BackBufferRTV");

	//
	//
	//

	kFormat format =
		g_RenderPipeline.Config.Hdr == kHighDynamicRange_Disabled ? kFormat_RGBA8_UNORM : kFormat_RGBA16_FLOAT;

	kD3D11_RenderTargetConfig rt_config[kRenderPass_Count];
	rt_config[0].Clear        = g_RenderPipeline.Config.Clear;
	rt_config[0].Factor       = 1;
	rt_config[0].Format       = format;
	rt_config[0].Multisamples = g_RenderPipeline.Config.Msaa;
	rt_config[0].Name         = "RenderTarget.Default";

	rt_config[1].Clear        = kVec4(0);
	rt_config[1].Factor       = 1;
	rt_config[1].Format       = kFormat_RGBA8_UNORM;
	rt_config[1].Multisamples = g_RenderPipeline.Config.Msaa;
	rt_config[1].Name         = "RenderTarget.HUD";

	for (int iter = 0; iter < kRenderPass_Count; ++iter)
	{
		if (!kD3D11_CreateRenderTarget(&g_RenderPipeline.RenderTargets[iter], rt_config[iter]))
		{
			return;
		}
	}

	ID3D11Texture2D *output = nullptr;

	if (g_RenderPipeline.Config.Bloom != kBloom_Disabled)
	{
		kD3D11_ReCalculateBloomMaxLevels();

		kVec2u               size            = g_RenderPipeline.BackBufferSize;

		D3D11_TEXTURE2D_DESC bloom           = {};
		bloom.Width                          = size.x;
		bloom.Height                         = size.y;
		bloom.MipLevels                      = g_RenderPipeline.BloomMaxMipLevel;
		bloom.ArraySize                      = 1;
		bloom.Format                         = DXGI_FORMAT_R11G11B10_FLOAT;
		bloom.SampleDesc.Quality             = 0;
		bloom.SampleDesc.Count               = 1;
		bloom.Usage                          = D3D11_USAGE_DEFAULT;
		bloom.BindFlags                      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		D3D11_SHADER_RESOURCE_VIEW_DESC srv  = {};
		srv.Format                           = bloom.Format;
		srv.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels              = 1;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uav = {};
		uav.Format                           = bloom.Format;
		uav.ViewDimension                    = D3D11_UAV_DIMENSION_TEXTURE2D;

		for (int iter = 0; iter < kArrayCount(g_RenderPipeline.BloomTargets); ++iter)
		{
			kD3D11_BloomTarget &target = g_RenderPipeline.BloomTargets[iter];

			bloom.MipLevels            = g_RenderPipeline.BloomMaxMipLevel - iter;
			hr                         = g_Device->CreateTexture2D(&bloom, 0, &target.Buffer);
			if (FAILED(hr))
			{
				kLogHresultError(hr, "D3D11", "Failed to create bloom texture");
				return;
			}
			kD3D11_NameFmt(target.Buffer, "Bloom[%d].Buffer", iter);

			for (int mip = 0; mip < (int)bloom.MipLevels; ++mip)
			{
				srv.Texture2D.MostDetailedMip = mip;
				hr = g_Device->CreateShaderResourceView(target.Buffer, &srv, &target.MipsSRV[mip]);
				if (FAILED(hr))
				{
					kLogHresultError(hr, "D3D11", "Failed to create bloom shader resource view (%u)", mip);
					return;
				}
				kD3D11_NameFmt(target.MipsSRV[mip], "Bloom[%d].MipsSRV[%d]", iter, mip);

				uav.Texture2D.MipSlice = mip;
				hr                     = g_Device->CreateUnorderedAccessView(target.Buffer, &uav, &target.MipsUAV[mip]);
				if (FAILED(hr))
				{
					kLogHresultError(hr, "D3D11", "Failed to create bloom unordered access view (%d)", mip);
					return;
				}
				kD3D11_NameFmt(target.MipsUAV[mip], "Bloom[%d].MipsUAV[%d]", iter, mip);
			}
		}

		output = g_RenderPipeline.BloomTargets[1].Buffer;
	}
	else
	{
		kD3D11_RenderTarget &rt = g_RenderPipeline.RenderTargets[kRenderPass_Default];
		output                  = rt.Resolved ? rt.Resolved : rt.Resolved;
	}

	D3D11_TEXTURE2D_DESC output_desc;
	output->GetDesc(&output_desc);

	D3D11_SHADER_RESOURCE_VIEW_DESC pp_srv = {};
	pp_srv.Format                          = output_desc.Format;
	pp_srv.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
	pp_srv.Texture2D.MipLevels             = 1;
	pp_srv.Texture2D.MostDetailedMip       = 0;

	hr = g_Device->CreateShaderResourceView(output, &pp_srv, &g_RenderPipeline.PostProcessOutput);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create post process shader resource view");
		return;
	}

	kD3D11_Name(g_RenderPipeline.PostProcessOutput, "PostProcessOutput");
}

static void kD3D11_ResizeSwapChainBuffers(uint w, uint h)
{
	if (!w || !h)
		return;

	kD3D11_Flush();

	if (g_SwapChain)
	{
		if (g_RenderPipeline.BackBufferSize.x == w && g_RenderPipeline.BackBufferSize.y == h)
		{
			return;
		}
		kD3D11_DestroySwapChainBuffers();
		g_SwapChain->ResizeBuffers(K_BACK_BUFFER_COUNT, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		kD3D11_CreateSwapChainBuffers();
	}
}

static void kD3D11_DestroySwapChain(void)
{
	kLogInfoEx("D3D11", "Destroying swap chain.\n");

	kD3D11_Flush();
	kD3D11_DestroySwapChainBuffers();
	kRelease(&g_SwapChain);
}

static void kD3D11_CreateSwapChain(void *_window, const kRenderPipelineConfig &config)
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

	HRESULT hr = g_Factory->CreateSwapChainForHwnd(g_Device, wnd, &swap_chain_desc, 0, 0, &g_SwapChain);
	kAssert(hr != DXGI_ERROR_INVALID_CALL);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "D3D11", "Failed to create swap chain");
		return;
	}

	g_RenderPipeline.Config = config;

	kD3D11_UpdateRenderPipelineParameters();
	kD3D11_CreateSwapChainBuffers();
}

static void kD3D11_Present(void)
{
	g_SwapChain->Present(1, 0);
}

static void kD3D11_GetRenderPipelineConfig(kRenderPipelineConfig *config)
{
	*config = g_RenderPipeline.Config;
}

static void kD3D11_ApplyRenderPipelineConfig(const kRenderPipelineConfig &config)
{
	bool resize = false;
	if (config.Msaa != g_RenderPipeline.Config.Msaa || config.Bloom != g_RenderPipeline.Config.Bloom ||
	    config.Hdr != g_RenderPipeline.Config.Hdr)
	{
		resize = true;
	}

	g_RenderPipeline.Config = config;
	if (resize)
	{
		kD3D11_ResizeSwapChainBuffers(0, 0);
	}
	kD3D11_UpdateRenderPipelineParameters();
}

static void kD3D11_ExecuteFrame(const kRenderFrame &frame)
{
	kD3D11_ExecuteGeometryPass(frame);

	if (g_RenderPipeline.Config.Bloom != kBloom_Disabled)
	{
		kD3D11_ExecuteBloomPass();
	}

	if (g_RenderPipeline.Config.Hdr != kHighDynamicRange_Disabled)
	{
		kD3D11_ExecuteTonemapPass();
	}
	else
	{
		kD3D11_ExecuteMixPass();
	}
}

//
// Backend
//

static void kD3D11_DestroyRenderBackend(void)
{
	kD3D11_Flush();
	kD3D11_DestroySwapChain();
	kD3D11_DestroyGraphicsDevice();
}

bool kD3D11_CreateRenderBackend(kRenderBackend *backend)
{
	kLogInfoEx("D3D11", "Creating graphics devices\n");
	if (!kD3D11_CreateGraphicsDevice())
	{
		return false;
	}

	kLogInfoEx("D3D11", "Registering render backend.\n");

	backend->CreateSwapChain           = kD3D11_CreateSwapChain;
	backend->DestroySwapChain          = kD3D11_DestroySwapChain;
	backend->ResizeSwapChain           = kD3D11_ResizeSwapChainBuffers;
	backend->Present                   = kD3D11_Present;

	backend->GetRenderPipelineConfig   = kD3D11_GetRenderPipelineConfig;
	backend->ApplyRenderPipelineConfig = kD3D11_ApplyRenderPipelineConfig;

	backend->CreateTexture             = kD3D11_CreateTexture;
	backend->DestroyTexture            = kD3D11_DestroyTexture;
	backend->GetTextureSize            = kD3D11_GetTextureSize;

	backend->ExecuteFrame              = kD3D11_ExecuteFrame;
	backend->Flush                     = kD3D11_Flush;

	backend->Destroy                   = kD3D11_DestroyRenderBackend;

	return true;
}

#endif
