#include "kPrebuild.h"

enum kShaderKind
{
	kShader_Vertex,
	kShader_Pixel,
	kShader_Compute
};

struct kShaderSource
{
	const char *name;
	const char *proc;
	kShaderKind kind;
};

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <d3d11_1.h>
#include <dxgi1_3.h>

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
		if (handle)
			break;
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

bool kCompileShader(const kShaderSource &shader, kString src, kString path, kString *compiled)
{
	ID3DBlob   *compiled_blob = nullptr;
	ID3DBlob   *err           = nullptr;
	bool        succeeded     = true;
	HRESULT     hr            = S_OK;

	const char *version       = "";

	if (shader.kind == kShader_Vertex)
	{
		version = "vs_4_0";
	}
	else if (shader.kind == kShader_Pixel)
	{
		version = "ps_4_0";
	}
	else if (shader.kind == kShader_Compute)
	{
		version = "cs_4_0";
	}

	hr = kHLSL_Compile(src.data, src.count, (char *)path.data, 0, 0, shader.proc, version, 0, 0, &compiled_blob, &err);

	if (FAILED(hr))
	{
		if (err)
		{
			kLogError("HLSL: Failed to compile shader: %s, Reason:\n%s\n", shader.name, err->GetBufferPointer());
			err->Release();
			err = nullptr;
		}
		else
		{
			kLogError("HLSL: Failed to compile shader: %s, Reason: Unimplemented\n", shader.name);
		}
		succeeded = false;
	}

	if (compiled_blob)
	{
		compiled->count = (imem)compiled_blob->GetBufferSize();
		compiled->data  = (u8 *)kAlloc(compiled->count);
		memcpy(compiled->data, compiled_blob->GetBufferPointer(), compiled->count);
		compiled_blob->Release();
	}
	else
	{
		*compiled = {};
	}

	return succeeded;
}

#endif

#include "kStrings.h"
#include "kPlatform.h"

static void kWriteBuffer(kStringBuilder<> *builder, kString buffer, kString name)
{
	builder->Write("static const u8 ");
	builder->Write(name);
	builder->Write("[] = {");

	for (imem j = 0; j < buffer.count; ++j)
	{
		if ((j) % 15 == 0)
			builder->Write('\n');
		builder->Write(buffer[j], "0x%02x");
		builder->Write(',');
	}

	builder->Write(kString("};\n\n"));
}

static bool kWriteShaderFile(kStringBuilder<> *builder, kString path, kSpan<kShaderSource> shaders)
{
	kString source = kReadEntireFile(path);
	if (!source.count)
	{
		return false;
	}

	imem count = 0;

	for (kShaderSource &src : shaders)
	{
		kLogTrace("Compiling shader: %.*s:%s...\n", kStrArg(path), src.name);

		kString compiled;
		if (kCompileShader(src, source, path, &compiled))
		{
			kWriteBuffer(builder, compiled, kString(src.name, strlen(src.name)));
			kFree(compiled.data, compiled.count);
			count += 1;
		}
	}

	kFree(source.data, source.count + 1);

	return count == shaders.count;
}

static bool kFlushBuilderToFile(kString path, kStringBuilder<> *builder)
{
	kString buffer = builder->ToString();
	bool    rc     = kWriteEntireFile(path, buffer.data, buffer.count);
	kFree(buffer.data, buffer.count + 1);
	builder->Reset();
	return rc;
}

bool kExecutePrebuild(void)
{
	if (!kCreateDirectories("Code/Kr/Generated"))
		return false;

	bool ok = true;

	if (ok)
	{
		kString outpath = "Code/Kr/Generated/kShaders.h";
		kString inpath  = "Code/Kr/Shaders/kRender2D.hlsl";

		bool    rebuild = true;

		if (kGetFileAttributes(outpath))
		{
			u64 outtm = kGetFileLastModifiedTime(outpath);
			u64 intm  = kGetFileLastModifiedTime(inpath);
			rebuild   = (intm > outtm);
		}

		if (rebuild)
		{
			kLogTrace("Generating shaders...\n");

			kShaderSource shaders[] = {
				{"kQuadVS", "QuadVsMain", kShader_Vertex},
				{"kQuadPS", "QuadPsMain", kShader_Pixel},
			};

			kStringBuilder builder;
			builder.Write("#pragma once\n");
			builder.Write("#include \"kCommon.h\"\n\n");

			ok = kWriteShaderFile(&builder, inpath, shaders);

			if (ok)
			{
				ok = kFlushBuilderToFile(outpath, &builder);
			}

			kFreeStringBuilder(&builder);
		}
		else
		{
			kLogTrace("Shaders are upto date...\n");
		}
	}

	return ok;
}
