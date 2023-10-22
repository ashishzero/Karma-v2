#include "kPrebuild.h"
#include "kArray.h"
#include "kPlatform.h"

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

bool kCompileShader(const char *proc, kString src, kString path, kString *compiled)
{
	ID3DBlob *  compiled_blob = nullptr;
	ID3DBlob *  err           = nullptr;
	bool        succeeded     = true;
	HRESULT     hr            = S_OK;

	const char *version       = "";

	kString     name          = kString(proc, strlen(proc));

	if (kEndsWith(name, "VS"))
	{
		version = "vs_5_0";
	}
	else if (kEndsWith(name, "PS"))
	{
		version = "ps_5_0";
	}
	else if (kEndsWith(name, "CS"))
	{
		version = "cs_5_0";
	}
	else
	{
		kLogErrorEx("HLSL", "Failed to detect shader type: %s\n", proc);
		return false;
	}

	hr = kHLSL_Compile(src.data, src.count, (char *)path.data, 0, 0, proc, version, 0, 0, &compiled_blob, &err);

	if (FAILED(hr))
	{
		if (err)
		{
			kLogErrorEx("HLSL", "Failed to compile shader: %s, Reason:\n%s\n", proc, err->GetBufferPointer());
			err->Release();
			err = nullptr;
		}
		else
		{
			kLogErrorEx("HLSL", "Failed to compile shader: %s, Reason: Unimplemented\n", proc);
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
		if ((j) % 15 == 0) builder->Write('\n');
		builder->Write(buffer[j], "0x%02x");
		builder->Write(',');
	}

	builder->Write(kString("};\n\n"));
}

static bool kWriteShaderFile(kStringBuilder<> *builder, kString path, kSpan<const char *> shaders)
{
	kString source = kReadEntireFile(path);
	if (!source.count)
	{
		return false;
	}

	imem count = 0;

	for (const char *src : shaders)
	{
		kLogInfoEx("HLSL", "Compiling shader: %.*s:%s...\n", kStrArg(path), src);

		kString compiled;
		if (kCompileShader(src, source, path, &compiled))
		{
			kWriteBuffer(builder, compiled, kString(src, strlen(src)));
			kFree(compiled.data, compiled.count);
			count += 1;
		}
	}

	kFree(source.data, source.count + 1);

	return count == shaders.count;
}

static bool kFlushBuilderToFile(kString path, kStringBuilder<> *builder)
{
	kString dir = kGetDirectoryPath(path);
	kCreateDirectories(dir);

	kString buffer = builder->ToString();
	bool    rc     = kWriteEntireFile(path, buffer.data, buffer.count);
	kFree(buffer.data, buffer.count + 1);
	builder->Reset();
	return rc;
}

static bool kRebuildShader(kString input, kSpan<const char *> shaders)
{
	kString path   = kGetDirectoryPath(input);
	kString name   = kGetFileName(input);
	kString output = kFormatString(kStrFmt "/Generated/" kStrFmt ".h", kStrArg(path), kStrArg(name));

	kDefer { kFree(output.data, output.count + 1); };

	bool rebuild = true;

	if (kGetFileAttributes(output))
	{
		u64 outtm = kGetFileLastModifiedTime(output);
		u64 intm  = kGetFileLastModifiedTime(input);
		rebuild   = (intm > outtm);
	}

	if (!rebuild)
	{
		kLogInfoEx("Prebuild", "Shaders are upto date : %.*s...\n", kStrArg(input));
		return true;
	}

	kLogInfoEx("Prebuild", "Generating shaders :%.*s...\n", kStrArg(input));

	kStringBuilder builder;
	builder.Write("#pragma once\n");
	builder.Write("#include \"kCommon.h\"\n\n");

	bool ok = false;

	if (kWriteShaderFile(&builder, input, shaders))
	{
		ok = kFlushBuilderToFile(output, &builder);
	}

	kFreeStringBuilder(&builder);

	return ok;
}

struct kShaderSource
{
	kString             path;
	kSpan<const char *> shaders;
};

bool kExecutePrebuild(void)
{
	kArray<kShaderSource> sources;

	const char *          render2d[] = {"kQuadVS", "kQuadPS"};
	sources.Add({"Code/Kr/Shaders/kQuad.hlsl", render2d});

	const char *ppvs[] = {"kFullscreenQuadVS"};
	sources.Add({"Code/Kr/Shaders/kFullscreenQuadVS.hlsl", ppvs});

	const char *blitps[] = {"kBlitPS"};
	sources.Add({"Code/Kr/Shaders/kBlitPS.hlsl", blitps});

	const char *thresholdcs[] = { "kToneMapAcesCS" };
	sources.Add({"Code/Kr/Shaders/kToneMapCS.hlsl", thresholdcs});

	const char *tmcs[] = { "kThresholdCS" };
	sources.Add({"Code/Kr/Shaders/kThresholdCS.hlsl", tmcs});

	for (kShaderSource &src : sources)
	{
		if (!kRebuildShader(src.path, src.shaders))
		{
			return false;
		}
	}

	kFree(&sources);

	return true;
}
