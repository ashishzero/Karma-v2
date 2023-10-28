#include "kShader.h"
#include "kPlatform.h"
#include "kStrings.h"

static void kWriteBuffer(kStringBuilder<> *builder, kString buffer, kString name, kString prefix, kString postfix)
{
	builder->Write("static const u8 ");
	builder->Write(prefix);
	builder->Write(name);
	builder->Write(postfix);
	builder->Write("[] = {");

	for (imem j = 0; j < buffer.Count; ++j)
	{
		if ((j) % 15 == 0) builder->Write('\n');
		builder->Write(buffer[j], "0x%02x");
		builder->Write(',');
	}

	builder->Write(kString("};\n\n"));
}

static kString kPostfixFromPath(kString name)
{
	if (kEndsWith(name, ".vs.hlsl")) return "VS";
	if (kEndsWith(name, ".ps.hlsl")) return "PS";
	if (kEndsWith(name, ".cs.hlsl")) return "CS";
	return "";
}

static bool kWriteShaderFile(kStringBuilder<> *builder, kString path)
{
	kString source = kReadEntireFile(path);
	if (!source.Count)
	{
		return false;
	}

	bool ok = false;

	kLogInfoEx("HLSL", "Compiling shader: %.*s...\n", kStrArg(path));

	kString compiled;
	if (kCompileShader(source, path, &compiled))
	{
		kString extension = ".xx.hlsl";
		kString shader    = kRemoveSuffix(kGetFileName(path), extension.Count);
		kWriteBuffer(builder, compiled, shader, "", kPostfixFromPath(path));
		kFree(compiled.Items, compiled.Count);
		ok = true;
	}

	kFree(source.Items, source.Count + 1);

	return ok;
}

static bool kFlushBuilderToFile(kString path, kStringBuilder<> *builder)
{
	kString dir = kGetDirectoryPath(path);
	kCreateDirectories(dir);

	kString buffer = builder->ToString();
	bool    rc     = kWriteEntireFile(path, buffer.Items, buffer.Count);
	kFree(buffer.Items, buffer.Count + 1);
	builder->Reset();
	return rc;
}

static bool kBuildShader(kString shader)
{
	kStringBuilder builder;
	builder.Write("#pragma once\n");
	builder.Write("#include \"kCommon.h\"\n\n");

	bool    ok     = false;

	kString name   = kGetFileName(shader);
	kString path   = kGetDirectoryPath(shader);
	kString output = kFormatString(kStrFmt "/Generated/" kStrFmt ".h", kStrArg(path), kStrArg(name));

	kDefer { kFree(output.Items, output.Count + 1); };

	if (kWriteShaderFile(&builder, shader))
	{
		ok = kFlushBuilderToFile(output, &builder);
		if (ok)
		{
			kLogInfoEx("HLSL", "Wrote %.*s.\n", kStrArg(output));
		}
	}

	kFreeStringBuilder(&builder);

	return ok;

	return true;
}

void Main(int argc, const char **argv)
{
	if (argc == 1)
	{
		kFatalError("Missing input files.");
	}
	for (int i = 1; i < argc; ++i)
	{
		kString input = kString(argv[i], strlen(argv[i]));
		if (!kBuildShader(input))
		{
			kTerminate(1);
		}
	}
}

#include "kCommon.cpp"
#include "kContext.cpp"
#include "kShader.cpp"
#include "kPlatform.cpp"
#include "kMain.cpp"
