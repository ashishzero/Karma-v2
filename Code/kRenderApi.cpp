#include "kRenderApi.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")

#include <d3d11_1.h>
#include <dxgi1_3.h>

constexpr uint PL_BACK_BUFFER_COUNT = 3;

typedef struct kPlatformRenderBackend
{
	ID3D11Device1		 *device;
	ID3D11DeviceContext1 *device_context;
	IDXGIFactory2		 *factory;
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
//
//

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
#ifdef K_BUILD_DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	UINT device_flags = 0;
	if (flags & DXGI_CREATE_FACTORY_DEBUG)
		device_flags |= D3D11_CREATE_DEVICE_DEBUG;

	if (!d3d11.factory)
	{
		HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&d3d11.factory));
		if (FAILED(hr))
		{
			kWinLogError(hr, "DirectX11", "Failed to create factory");
			return false;
		}
	}

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

	return true;
}

bool kD3D11_CreateRenderBackend(kRenderBackend *backend, kSwapChainBackend *swap_chain)
{
	if (!kD3D11_CreateGraphicsDevice())
		return false;

	backend->destroy = kD3D11_DestroyGraphicsDevice;
}

#endif

void kCreateRenderBackend(kRenderBackend *backend, kSwapChainBackend *swap_chain)
{}
