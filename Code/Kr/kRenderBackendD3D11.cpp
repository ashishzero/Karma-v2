#include "kRenderBackend.h"
#include "kLinkedList.h"
#include "kMath.h"
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

constexpr uint K_BACK_BUFFER_COUNT     = 3;
constexpr uint K_TEXTURE_MAX_PER_BLOCK = 32;

typedef struct kD3D11_Texture : kTexture
{
	kVec2i                     size;
	ID3D11ShaderResourceView  *srv;
	ID3D11UnorderedAccessView *uav;
	ID3D11Texture2D           *texture2d;
	kD3D11_Texture            *prev;
	kD3D11_Texture            *next;
} kD3D11_Texture;

typedef struct kD3D11_SwapChain
{
	IDXGISwapChain1        *native;
	ID3D11RenderTargetView *rtv;
	ID3D11DepthStencilView *dsv;
	ID3D11Texture2D        *rtv_texture2d;
	ID3D11Texture2D        *dsv_texture2d;
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

typedef struct kD3D11_Resource
{
	kD3D11_TexturePool       textures;
	kD3D11_Render2D          render2d;
	ID3D11DepthStencilState *depth;
	ID3D11SamplerState      *samplers[kTextureFilter_Count];
	ID3D11BlendState        *blends[kBlendMode_Count];
} kD3D11_Resource;

typedef void (*kD3D11_RenderPassProc)(const kRenderFrame2D &);

enum kRenderPassFlags
{
	kRenderPass_ClearColor = 0x1,
	kRenderPass_ClearDepth = 0x2,
};

typedef struct kD3D11_RenderPass
{
	ID3D11RenderTargetView *rtv;
	ID3D11DepthStencilView *dsv;
	UINT                    flags;
	kVec4                   color;
	float                   depth;
	DXGI_FORMAT             format;
	ID3D11Resource         *target;
	ID3D11Resource         *resolve;
	kD3D11_RenderPassProc   Execute;
} kD3D11_RenderPass;

typedef enum kRenderPass
{
	kRenderPass_Render2D,
	kRenderPass_Count,
} kRenderPass;

typedef struct kD3D11_Backend
{
	ID3D11Device1        *device;
	ID3D11DeviceContext1 *device_context;
	IDXGIFactory2        *factory;
	kD3D11_SwapChain      swapchain;
	kD3D11_RenderPass     passes[kRenderPass_Count];
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

static void kD3D11_RenderPassFallback(const kRenderFrame2D &) {}

//
// Mappings
//

#include "Shaders/Generated/kRender2D.hlsl.h"
#include "Shaders/Generated/kPostProcessVS.hlsl.h"
#include "Shaders/Generated/kToneMapping.hlsl.h"
#include "Shaders/Generated/kToneMappingCS.hlsl.h"

// static constexpr UINT kD3D11_SampleCountMap[] = {1, 2, 4, 8};
// static_assert(kArrayCount(kD3D11_SampleCountMap) == kAntiAliasingMethod_Count, "");

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
// Render State Objects
//

static bool kD3D11_CreateDepthStencilState(void)
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

		HRESULT hr                 = d3d11.device->CreateSamplerState(&sampler, &d3d11.resource.samplers[filter]);
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
		HRESULT hr = d3d11.device->CreateBlendState(BlendModeDesc[mode], &d3d11.resource.blends[mode]);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create alpha blend state");
			return false;
		}
	}

	return true;
}

// static ID3D11VertexShader *kD3D11_GetPostProcessVertexShader(void)
//{
//	if (d3d11.resource.deferred.postprocess.vs)
//	{
//		return d3d11.resource.deferred.postprocess.vs;
//	}
//
//	kString vs = kString(kPostProcessVS);
//
//	kLogInfoEx("D3D11", "Compiling post process vertex shader.\n");
//
//	HRESULT hr = d3d11.device->CreateVertexShader(vs.data, vs.count, 0, &d3d11.resource.deferred.postprocess.vs);
//	if (FAILED(hr))
//	{
//		kLogHresultError(hr, "DirectX11", "Failed to create post processing vertex shader");
//	}
//
//	return d3d11.resource.deferred.postprocess.vs;
// }

// static ID3D11ComputeShader *kD3D11_GetPostProcessToneMappingComputeShader(void)
//{
//	if (d3d11.resource.deferred.postprocess.tonemapping_cs)
//	{
//		return d3d11.resource.deferred.postprocess.tonemapping_cs;
//	}
//
//	kString ps = kString(kToneMapAcesCS);
//
//	kLogInfoEx("D3D11", "Compiling tone mapping compute shader.\n");
//
//	HRESULT hr =
//		d3d11.device->CreateComputeShader(ps.data, ps.count, 0, &d3d11.resource.deferred.postprocess.tonemapping_cs);
//	if (FAILED(hr))
//	{
//		kLogHresultError(hr, "DirectX11", "Failed to create tone mapping compute shader");
//	}
//
//	return d3d11.resource.deferred.postprocess.tonemapping_cs;
// }

// static ID3D11PixelShader *kD3D11_GetPostProcessToneMappingShader(kToneMappingMethod method)
//{
//	if (d3d11.resource.deferred.postprocess.tonemapping[method])
//	{
//		return d3d11.resource.deferred.postprocess.tonemapping[method];
//	}
//
//	kString ps = kD3D11_ToneMappingPS[method];
//
//	kLogInfoEx("D3D11", "Compiling %s pixel shader.\n", kD3D11_ToneMappingPSNames[method]);
//
//	HRESULT hr =
//		d3d11.device->CreatePixelShader(ps.data, ps.count, 0, &d3d11.resource.deferred.postprocess.tonemapping[method]);
//	if (FAILED(hr))
//	{
//		kLogHresultError(hr, "DirectX11", "Failed to create post processing pixel shader");
//	}
//
//	return d3d11.resource.deferred.postprocess.tonemapping[method];
// }

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

	if (bind_flags & D3D11_BIND_UNORDERED_ACCESS)
	{
		hr = d3d11.device->CreateUnorderedAccessView(texture->texture2d, nullptr, &texture->uav);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to create unordered access view");
		}
	}

	D3D11_TEXTURE2D_DESC desc;
	texture->texture2d->GetDesc(&desc);

	texture->size.x = (int)desc.Width;
	texture->size.y = (int)desc.Height;

	texture->state  = kResourceState_Ready;
}

static kTexture *kD3D11_CreateTexture(const kTextureSpec &spec)
{
	kD3D11_Texture *r = kD3D11_AllocTexture();

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

	desc.BindFlags            = 0;

	if (spec.flags & kResource_AllowRenderTarget) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	if (spec.flags & kResource_AllowDepthStencil) desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	if (spec.flags & kResource_AllowUnorderedAccess) desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	if ((spec.flags & kResource_DenyShaderResource) == 0) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

	desc.Usage              = D3D11_USAGE_DEFAULT;
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
		kRelease(&r->srv);
		kRelease(&r->uav);
		r->size  = kVec2i(0);
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

		HRESULT hr                        = d3d11.device->CreateBuffer(&vb_desc, 0, &d3d11.resource.render2d.vertex);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to allocate vertex buffer");
			return nullptr;
		}

		d3d11.resource.render2d.vertex_sz = vb_size;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = d3d11.device_context->Map(d3d11.resource.render2d.vertex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to map vertex2d buffer");
		return nullptr;
	}
	memcpy(mapped.pData, vertices.data, vb_size);
	d3d11.device_context->Unmap(d3d11.resource.render2d.vertex, 0);

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

		HRESULT hr                       = d3d11.device->CreateBuffer(&ib_desc, 0, &d3d11.resource.render2d.index);
		if (FAILED(hr))
		{
			kLogHresultError(hr, "DirectX11", "Failed to allocate index buffer");
			return nullptr;
		}

		d3d11.resource.render2d.index_sz = ib_size;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = d3d11.device_context->Map(d3d11.resource.render2d.index, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to map index2d buffer");
		return nullptr;
	}
	memcpy(mapped.pData, indices.data, ib_size);
	d3d11.device_context->Unmap(d3d11.resource.render2d.index, 0);

	return d3d11.resource.render2d.index;
}

static void kD3D11_RenderPass2D(const kRenderFrame2D &frame)
{
	if (frame.vertices.count == 0 || frame.indices.count == 0) return;

	ID3D11DeviceContext1 *ctx    = d3d11.device_context;
	ID3D11Buffer         *vtx    = kD3D11_UploadVertices2D(frame.vertices);
	ID3D11Buffer         *idx    = kD3D11_UploadIndices2D(frame.indices);
	ID3D11Buffer         *xform  = d3d11.resource.render2d.constant;
	HRESULT               hr     = S_OK;
	UINT                  stride = sizeof(kVertex2D);
	UINT                  offset = 0;
	DXGI_FORMAT           format = sizeof(kIndex2D) == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	if (!vtx || !idx || !ctx) return;

	ctx->IASetVertexBuffers(0, 1, &vtx, &stride, &offset);
	ctx->IASetIndexBuffer(idx, format, 0);
	ctx->VSSetConstantBuffers(0, 1, &xform);
	ctx->VSSetShader(d3d11.resource.render2d.vs, nullptr, 0);
	ctx->PSSetShader(d3d11.resource.render2d.ps, nullptr, 0);
	ctx->IASetInputLayout(d3d11.resource.render2d.input);
	ctx->RSSetState(d3d11.resource.render2d.rasterizer);
	ctx->OMSetDepthStencilState(d3d11.resource.depth, 0);
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

		ctx->RSSetViewports(1, &viewport);

		{
			kMat4 proj = kOrthographicLH(scene.camera.left, scene.camera.right, scene.camera.top, scene.camera.bottom,
			                             scene.camera.near, scene.camera.far);

			D3D11_MAPPED_SUBRESOURCE mapped;
			hr = ctx->Map(xform, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			if (FAILED(hr))
			{
				kLogHresultError(hr, "DirectX11", "Failed to map constant buffer");
				continue;
			}
			memcpy(mapped.pData, proj.m, sizeof(kMat4));
			ctx->Unmap(xform, 0);
		}

		for (u32 index = scene.commands.beg; index < scene.commands.end; ++index)
		{
			const kRenderCommand2D &cmd = frame.commands[index];

			if (cmd.flags & kRenderDirty_Blend)
			{
				ctx->OMSetBlendState(d3d11.resource.blends[cmd.blend], 0, 0xffffffff);
			}

			if (cmd.flags & kRenderDirty_TextureFilter)
			{
				ctx->PSSetSamplers(0, 1, &d3d11.resource.samplers[cmd.filter]);
			}

			if (cmd.flags & kRenderDirty_Rect)
			{
				D3D11_RECT rect;
				kRect      krect = frame.rects[cmd.rect];
				rect.left        = (LONG)(krect.min.x);
				rect.top         = (LONG)(krect.min.y);
				rect.right       = (LONG)(krect.max.x);
				rect.bottom      = (LONG)(krect.max.y);
				ctx->RSSetScissorRects(1, &rect);
			}

			if (cmd.flags & kRenderDirty_TextureColor)
			{
				u32             texoff = cmd.textures[kTextureType_Color];
				kD3D11_Texture *r      = (kD3D11_Texture *)frame.textures[kTextureType_Color][texoff];
				if (r->state != kResourceState_Ready) continue;
				ctx->PSSetShaderResources(kTextureType_Color, 1, &r->srv);
			}

			if (cmd.flags & kRenderDirty_TextureMaskSDF)
			{
				u32             texoff = cmd.textures[kTextureType_MaskSDF];
				kD3D11_Texture *r      = (kD3D11_Texture *)frame.textures[kTextureType_MaskSDF][texoff];
				if (r->state != kResourceState_Ready) continue;
				ctx->PSSetShaderResources(kTextureType_MaskSDF, 1, &r->srv);
			}

			ctx->DrawIndexed(cmd.index, idx_offset, vtx_offset);
			vtx_offset += cmd.vertex;
			idx_offset += cmd.index;
		}

		ctx->PSSetSamplers(0, 0, nullptr);
		ctx->PSSetShaderResources(0, 0, nullptr);
	}
}

//
// SwapChain
//

// static void kD3D11_UnprepareSwapChainFeatureTextures(kD3D11_SwapChain *r)
//{
//	for (int i = 1; i < kD3D11_RenderTarget_Count; ++i)
//	{
//		kD3D11_UnprepareTexture(&r->targets[i]);
//		kRelease(&r->targets[i].texture2d);
//		memset(&r->targets[i], 0, sizeof(r->targets[i]));
//	}
//	r->rt = kD3D11_RenderTarget_Final;
// }

static void kD3D11_DestroySwapChainBuffers(void)
{
	kDebugTraceEx("D3D11", "Destroying swap chain render targets.\n");
	memset(d3d11.passes, 0, sizeof(d3d11.passes));
	kRelease(&d3d11.swapchain.rtv);
	kRelease(&d3d11.swapchain.dsv);
	kRelease(&d3d11.swapchain.rtv_texture2d);
	kRelease(&d3d11.swapchain.dsv_texture2d);

	for (int pass = 0; pass < kRenderPass_Count; ++pass)
	{
		d3d11.passes[pass].Execute = kD3D11_RenderPassFallback;
	}
}

static void kD3D11_CreateSwapChainBuffers(void)
{
	kDebugTraceEx("D3D11", "Creating swap chain render targets.\n");

	HRESULT hr = d3d11.swapchain.native->GetBuffer(0, IID_PPV_ARGS(&d3d11.swapchain.rtv_texture2d));

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create swapchain render target");
		return;
	}

	hr = d3d11.device->CreateRenderTargetView(d3d11.swapchain.rtv_texture2d, 0, &d3d11.swapchain.rtv);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create swapchain render target view");
		return;
	}

	D3D11_TEXTURE2D_DESC rt;
	d3d11.swapchain.rtv_texture2d->GetDesc(&rt);

	D3D11_TEXTURE2D_DESC dt = {};
	dt.Width                = rt.Width;
	dt.Height               = rt.Height;
	dt.MipLevels            = 1;
	dt.ArraySize            = 1;
	dt.Format               = DXGI_FORMAT_D32_FLOAT;
	dt.SampleDesc.Count     = 1;
	dt.SampleDesc.Quality   = 0;
	dt.Usage                = D3D11_USAGE_DEFAULT;
	dt.BindFlags            = D3D11_BIND_DEPTH_STENCIL;

	hr                      = d3d11.device->CreateTexture2D(&dt, 0, &d3d11.swapchain.dsv_texture2d);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create texture 2d");
		return;
	}

	hr = d3d11.device->CreateDepthStencilView(d3d11.swapchain.dsv_texture2d, 0, &d3d11.swapchain.dsv);
	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create depth stencil view");
		return;
	}

	kD3D11_RenderPass *pass = &d3d11.passes[kRenderPass_Render2D];
	pass->rtv               = d3d11.swapchain.rtv;
	pass->dsv               = d3d11.swapchain.dsv;
	pass->flags             = kRenderPass_ClearColor | kRenderPass_ClearDepth;
	pass->color             = kVec4(0, 0, 0, 1);
	pass->depth             = 1;
	pass->target            = d3d11.swapchain.rtv_texture2d;
	pass->Execute           = kD3D11_RenderPass2D;
}

static void kD3D11_ResizeSwapChainBuffers(uint w, uint h)
{
	if (!w || !h) return;

	kD3D11_Flush();

	if (d3d11.swapchain.native)
	{
		D3D11_TEXTURE2D_DESC desc;
		d3d11.swapchain.rtv_texture2d->GetDesc(&desc);
		if (desc.Width == w && desc.Height == h) return;

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
	HRESULT          hr = d3d11.factory->CreateSwapChainForHwnd(d3d11.device, wnd, &swap_chain_desc, 0, 0, &swapchain1);
	kAssert(hr != DXGI_ERROR_INVALID_CALL);

	if (FAILED(hr))
	{
		kLogHresultError(hr, "DirectX11", "Failed to create swap chain");
		return;
	}

	d3d11.swapchain.native = swapchain1;
	kD3D11_CreateSwapChainBuffers();
}

static void kD3D11_Present(void)
{
	// kD3D11_SwapChain *r = (kD3D11_SwapChain *)swap_chain;
	// if (r->state != kResourceState_Ready) return;

	// kD3D11_RenderTarget rt = r->rt;

	// if (rt == kD3D11_RenderTarget_MSAA)
	//{
	//	kD3D11_Texture *src = &r->targets[kD3D11_RenderTarget_MSAA];
	//	kD3D11_Texture *dst = &r->targets[kD3D11_RenderTarget_Resolved];
	//	d3d11.device_context->ResolveSubresource(dst->texture2d, 0, src->texture2d, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	//	rt = kD3D11_RenderTarget_Resolved;
	// }

	// if (rt == kD3D11_RenderTarget_Resolved && r->rt_config.tonemapping != kToneMappingMethod_SDR)
	//{
	//	kD3D11_Texture            *resolved    = &r->targets[kD3D11_RenderTarget_Resolved];
	//	kD3D11_Texture            *tonemapping = &r->targets[kD3D11_RenderTarget_ToneMapping];

	//	ID3D11ComputeShader       *cs          = kD3D11_GetPostProcessToneMappingComputeShader();

	//	ID3D11ShaderResourceView  *srvs[]      = {resolved->srv};
	//	ID3D11UnorderedAccessView *uavs[]      = {tonemapping->uav};

	//	d3d11.device_context->CSSetShader(cs, 0, 0);
	//	d3d11.device_context->CSSetShaderResources(0, 1, srvs);
	//	d3d11.device_context->CSSetUnorderedAccessViews(0, 1, uavs, 0);

	//	UINT x_thrd_group = (UINT)ceilf(tonemapping->size.x / 32.0f);
	//	UINT y_thrd_group = (UINT)ceilf(tonemapping->size.y / 16.0f);
	//	d3d11.device_context->Dispatch(x_thrd_group, y_thrd_group, 1);
	//	d3d11.device_context->ClearState();

	//	rt = kD3D11_RenderTarget_ToneMapping;
	//}

	// if (rt != kD3D11_RenderTarget_Final)
	//{
	//	kD3D11_Texture *src    = &r->targets[rt];
	//	kD3D11_Texture *target = &r->targets[kD3D11_RenderTarget_Final];

	//	D3D11_VIEWPORT  viewport;
	//	viewport.TopLeftX = 0;
	//	viewport.TopLeftY = 0;
	//	viewport.Width    = (float)target->size.x;
	//	viewport.Height   = (float)target->size.y;
	//	viewport.MinDepth = 0;
	//	viewport.MaxDepth = 1;

	//	float clear[]     = {0.0f, 0.0f, 0.0f, 0.0f};
	//	d3d11.device_context->ClearRenderTargetView(target->rtv, clear);

	//	ID3D11VertexShader       *vs          = kD3D11_GetPostProcessVertexShader();
	//	ID3D11PixelShader        *ps          = kD3D11_GetPostProcessToneMappingShader(kToneMappingMethod_SDR);
	//	ID3D11SamplerState       *sampler     = kD3D11_GetSampleState(kTextureFilter_Linear);
	//	ID3D11BlendState         *blend       = kD3D11_GetBlendState(kBlendMode_Opaque);
	//	ID3D11ShaderResourceView *resources[] = {src->srv};

	//	d3d11.device_context->OMSetRenderTargets(1, &target->rtv, 0);
	//	d3d11.device_context->RSSetViewports(1, &viewport);
	//	d3d11.device_context->VSSetShader(vs, 0, 0);
	//	d3d11.device_context->PSSetShader(ps, 0, 0);
	//	d3d11.device_context->PSSetShaderResources(0, 1, resources);
	//	d3d11.device_context->PSSetSamplers(0, 1, &sampler);
	//	d3d11.device_context->OMSetBlendState(blend, 0, 0xffffffff);
	//	d3d11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	d3d11.device_context->Draw(6, 0);

	//	d3d11.device_context->ClearState();
	//}

	d3d11.swapchain.native->Present(1, 0);
}

static void kD3D11_ExecuteRenderPass(const kD3D11_RenderPass &pass, const kRenderFrame2D &frame)
{
	ID3D11DeviceContext1 *ctx = d3d11.device_context;

	if (pass.flags & kRenderPass_ClearColor)
	{
		ctx->ClearRenderTargetView(pass.rtv, pass.color.m);
	}

	if (pass.flags & kRenderPass_ClearDepth)
	{
		ctx->ClearDepthStencilView(pass.dsv, D3D11_CLEAR_DEPTH, pass.depth, 0);
	}

	ctx->OMSetRenderTargets(1, &pass.rtv, pass.dsv);
	pass.Execute(frame);

	if (pass.resolve)
	{
		ctx->ResolveSubresource(pass.resolve, 0, pass.target, 0, pass.format);
	}

	ctx->ClearState();
}

static void kD3D11_ExecuteFrame(const kRenderFrame2D &frame)
{
	for (int pass = 0; pass < kRenderPass_Count; ++pass)
	{
		kD3D11_ExecuteRenderPass(d3d11.passes[pass], frame);
	}
}

//
// Resources
//

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

static void kD3D11_DestroyRenderResources(void)
{
	kLogInfoEx("D3D11", "Destroying render 2d resources.\n");
	kD3D11_DestroyRenderResources2D();

	{
		kLogInfoEx("D3D11", "Destroying textures.\n");

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

	kLogInfoEx("D3D11", "Destroying deferred resources.\n");

	kRelease(&d3d11.resource.depth);

	for (int i = 0; i < kTextureFilter_Count; ++i)
	{
		kRelease(&d3d11.resource.samplers[i]);
	}

	for (int i = 0; i < kBlendMode_Count; ++i)
	{
		kRelease(&d3d11.resource.blends[i]);
	}
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
	kD3D11_Flush();
	kD3D11_DestroySwapChain();
	kD3D11_DestroyRenderResources();

	kLogInfoEx("D3D11", "Destroying graphics devices.\n");

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
		if (kEnableDebugLayer) flags |= DXGI_CREATE_FACTORY_DEBUG;

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
		if (kEnableDebugLayer) device_flags |= D3D11_CREATE_DEVICE_DEBUG;

		adapter = kD3D11_FindAdapter(d3d11.factory, device_flags);

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
				kLogTraceEx("D3D11", "ID3D11Debug enabled.");
				info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
				info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
				info_queue->Release();
			}
		}
		debug->Release();
	}

	d3d11.device->GetImmediateContext1(&d3d11.device_context);

	if (!kD3D11_CreateDepthStencilState()) return false;
	if (!kD3D11_CreateBlendState()) return false;
	if (!kD3D11_CreateSamplerState()) return false;

	for (int pass = 0; pass < kRenderPass_Count; ++pass)
	{
		d3d11.passes[pass].Execute = kD3D11_RenderPassFallback;
	}

	return true;
}

//
// Backend
//

bool kD3D11_CreateRenderBackend(kRenderBackend *backend)
{
	kLogInfoEx("D3D11", "Creating graphics devices\n");
	if (!kD3D11_CreateGraphicsDevice())
	{
		kD3D11_DestroyGraphicsDevice();
		return false;
	}

	kLogInfoEx("D3D11", "Creating render 2d resources.\n");
	if (!kD3D11_CreateRenderResources2D())
	{
		kD3D11_DestroyGraphicsDevice();
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

	backend->Destroy          = kD3D11_DestroyGraphicsDevice;

	return true;
}

#endif
