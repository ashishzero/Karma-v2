#include "kPrebuild.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <d3d11_1.h>
#include <dxgi1_3.h>

typedef HRESULT (*kHLSL_CompileProc)(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
									 CONST D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint,
									 LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode,
									 ID3DBlob **ppErrorMsgs);

static kHLSL_CompileProc kHLSLCompileImpl;

static HRESULT			 kHLSL_CompileFallback(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
											   CONST D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint,
											   LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode,
											   ID3DBlob **ppErrorMsgs)
{
	return ERROR_FILE_NOT_FOUND;
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

	HMODULE handle = LoadLibraryW(L"d3dcompiler_43.dll");
	if (handle)
	{
		kHLSLCompileImpl = (kHLSL_CompileProc)GetProcAddress(handle, "D3DCompile");
	}

	if (!kHLSLCompileImpl)
	{
		kHLSLCompileImpl = kHLSL_CompileFallback;
		kWinLogError(GetLastError(), "HLSL", "Failed to load compiler");
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

bool kCompileShader(kString vs, kString ps, const char *vs_path, const char *ps_path, const char *vs_main,
					const char *ps_main, kString *vs_out, kString *ps_out)
{
	ID3DBlob *vs_blob	= nullptr;
	ID3DBlob *ps_blob	= nullptr;
	ID3DBlob *err		= nullptr;
	bool	  succeeded = true;
	HRESULT	  hr		= S_OK;

	//
	// Vertex Shader compilation
	//

	hr = kHLSL_Compile(vs.data, vs.count, vs_path, 0, 0, vs_main, "vs_4_0", 0, 0, &vs_blob, &err);

	if (FAILED(hr))
	{
		kLogError("HLSL: Failed to compile vertex shader: %s, Reason: %.*s\n", vs_path, (int)err->GetBufferSize(),
				  err->GetBufferPointer());
		succeeded = false;
		err->Release();
		err = nullptr;
	}

	//
	// Pixel shader compilation
	//

	hr = kHLSL_Compile(ps.data, ps.count, ps_path, 0, 0, ps_main, "ps_4_0", 0, 0, &ps_blob, &err);
	if (FAILED(hr))
	{
		kLogError("HLSL: Failed to compile pixel shader: %s, Reason: %.*s\n", ps_path, (int)err->GetBufferSize(),
				  err->GetBufferPointer());
		succeeded = false;
		err->Release();
		err = nullptr;
	}

	if (vs_blob)
	{
		vs_out->count = (imem)vs_blob->GetBufferSize();
		vs_out->data  = (u8 *)kAlloc(vs_out->count);
		memcpy(vs_out->data, vs_blob->GetBufferPointer(), vs_out->count);
		vs_blob->Release();
	}
	else
	{
		*vs_out = "";
	}

	if (ps_blob)
	{
		ps_out->count = (imem)ps_blob->GetBufferSize();
		ps_out->data  = (u8 *)kAlloc(ps_out->count);
		memcpy(ps_out->data, ps_blob->GetBufferPointer(), ps_out->count);
		ps_blob->Release();
	}
	else
	{
		*ps_out = "";
	}

	return succeeded;
}

#else

bool kCompileShader(kString vs, kString ps, const char *vs_path, const char *ps_path, const char *vs_main,
					const char *ps_main, kString *vs_out, kString *ps_out)
{
	*vs_out = "";
	*ps_out = "";
	return false;
}

#endif

#include "kStrings.h"
#include "kPlatform.h"

static bool kGenerateString(const kString src, const kString path)
{
	return kWriteEntireFile(path, src.data, src.count);
}

static bool kGenerateBinaryData(kSlice<kString> buffers, kSlice<kString> names, kString path)
{
	kStringBuilder builder;

	builder.Write("#pragma once\n");
	builder.Write("#include \"kCommon.h\"\n\n");

	for (imem i = 0; i < buffers.count; ++i)
	{
		builder.Write("static const u8 ");
		builder.Write(names[i]);
		builder.Write("[] = {");

		for (imem j = 0; j < buffers[i].count; ++j)
		{
			if ((j) % 15 == 0)
				builder.Write('\n');
			builder.Write(buffers[i][j], "0x%02x");
			builder.Write(',');
		}

		builder.Write(kString("};\n\n"));
	}

	kString string = builder.ToString();

	bool	r	   = kGenerateString(string, path);

	kFreeStringBuilder(&builder);
	kFree(string.data, string.count);

	return r;
}

static bool kGenerateBinaryFile(kString inpath, kString name, kString path)
{
	umem count;
	u8	*buff = kReadEntireFile(inpath, &count);
	if (buff)
	{
		kString buffers[] = {kString(buff, count)};
		kString names[]	  = {name};
		bool	r		  = kGenerateBinaryData(buffers, names, path);
		kFree(buff, count);
		return r;
	}
	return false;
}

static bool kCompileShaderFile(const char *path, const char *vs_main, const char *ps_main, kString *vs, kString *ps)
{
	kLogTrace("Compiling shaders: %s...", path);

	umem size;
	u8	*content = kReadEntireFile(kString(path, strlen(path)), &size);
	if (!content)
	{
		return false;
	}

	kString shader = kString(content, size);

	return kCompileShader(shader, shader, path, path, vs_main, ps_main, vs, ps);
}

bool kExecutePrebuild(void)
{
	kString outdir	= "Code/Kr/Generated";
	kString outpath = "Code/Kr/Generated/kShaders.h";

	bool	ok		= true;

	if (kGetFileAttributes(outpath) == 0)
	{
		if (!kCreateDirectories(outdir))
			return false;

		kString vs, ps;
		ok = kCompileShaderFile("Code/Kr/Shaders/kRender2D.hlsl", "QuadVsMain", "QuadPsMain", &vs, &ps);

		if (ok)
		{
			kString buffers[] = {vs, ps};
			kString names[]	  = {"kQuadVS", "kQuadPS"};
			ok				  = kGenerateBinaryData(buffers, names, outpath);
		}

		kFree(vs.data, vs.count);
		kFree(ps.data, ps.count);
	}

	return ok;
}
