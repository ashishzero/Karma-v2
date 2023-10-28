#include "kShader.h"
#include "kPlatform.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <d3d11_1.h>
#include <dxgi1_3.h>

#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude *)(UINT_PTR)1)

typedef HRESULT (*kHLSL_CompileProc)(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
                                     CONST D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint,
                                     LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode,
                                     ID3DBlob **ppErrorMsgs);

static kHLSL_CompileProc kHLSLCompileImpl;

static HRESULT           kHLSL_CompileFallback(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
                                               CONST D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint,
                                               LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode,
                                               ID3DBlob **ppErrorMsgs)
{
	return E_NOTIMPL;
}

static void kHLSL_LoadShaderCompiler()
{
	static kAtomic Lock;

	kAtomicLock(&Lock);

	if (kHLSLCompileImpl != nullptr)
	{
		kAtomicUnlock(&Lock);
		return;
	}

	const wchar_t *compilers[] = {L"D3DCompiler_47.dll", L"D3DCompiler_46.dll", L"D3DCompiler_45.dll",
	                              L"D3DCompiler_44.dll", L"D3DCompiler_43.dll"};

	HMODULE        handle      = 0;

	for (int i = 0; i < kArrayCount(compilers); ++i)
	{
		handle = LoadLibraryW(compilers[i]);
		if (handle) break;
	}

	if (handle)
	{
		kHLSLCompileImpl = (kHLSL_CompileProc)GetProcAddress(handle, "D3DCompile");
	}

	if (!kHLSLCompileImpl)
	{
		kHLSLCompileImpl = kHLSL_CompileFallback;
		kLogHresultError(GetLastError(), "HLSL", "Failed to load compiler");
	}

	kAtomicUnlock(&Lock);
}

static HRESULT kHLSL_Compile(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName, CONST D3D_SHADER_MACRO *pDefines,
                             ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2,
                             ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
{
	kHLSL_LoadShaderCompiler();
	return kHLSLCompileImpl(pSrcData, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, pTarget, Flags1,
	                        Flags2, ppCode, ppErrorMsgs);
}

bool kCompileShader(kString src, kString path, kString *compiled)
{
	ID3DBlob *  compiled_blob = nullptr;
	ID3DBlob *  err           = nullptr;
	bool        succeeded     = true;
	HRESULT     hr            = S_OK;

	const char *version       = "";

	if (kEndsWith(path, ".vs.hlsl"))
	{
		version = "vs_5_0";
	}
	else if (kEndsWith(path, ".ps.hlsl"))
	{
		version = "ps_5_0";
	}
	else if (kEndsWith(path, ".cs.hlsl"))
	{
		version = "cs_5_0";
	}
	else
	{
		kLogErrorEx("HLSL", "Failed to detect shader type: %.*s\n", kStrArg(path));
		return false;
	}

	hr = kHLSL_Compile(src.Items, src.Count, (char *)path.Items, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "Main", version, 0,
	                   0, &compiled_blob, &err);

	if (FAILED(hr))
	{
		if (err)
		{
			kLogErrorEx("HLSL", "Failed to compile shader: %.*s, Reason:\n%s\n", kStrArg(path),
			            err->GetBufferPointer());
			err->Release();
			err = nullptr;
		}
		else
		{
			kLogErrorEx("HLSL", "Failed to compile shader: %.*s, Reason: Unimplemented\n", kStrArg(path));
		}
		succeeded = false;
	}

	if (compiled_blob)
	{
		compiled->Count = (imem)compiled_blob->GetBufferSize();
		compiled->Items  = (u8 *)kAlloc(compiled->Count);
		memcpy(compiled->Items, compiled_blob->GetBufferPointer(), compiled->Count);
		compiled_blob->Release();
	}
	else
	{
		*compiled = {};
	}

	return succeeded;
}

#endif
