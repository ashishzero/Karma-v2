#include "kBuild.h"
#include "kMedia.h"
#include "kStrings.h"

#include <stdio.h>

struct kPair
{
	kString key;
	kString value;
};

struct kProject
{
	kArena		   *arena;
	kString			name;
	uint			flags;
	kBuildKind		kind;
	kBuildArch		arch;
	kString			dir;
	kString			outdir;
	kString			objdir;
	kString			gendir;
	kArray<kString> incdirs;
	kArray<kString> libdirs;
	kArray<kPair>	defines;
	kArray<kString> extras;
	kString			manifest;
	kArray<kString> sources;
	kArray<kString> resources;
	kArray<kString> objects;
	kArray<kString> libraries;
	uint			generation;
};

//
//
//

static kString kPushString(kProject *p, kString src)
{
	u8 *buff = (u8 *)kPushSize(p->arena, src.count + 1, 0);
	memcpy(buff, src.data, src.count);
	buff[src.count] = 0;
	return kString(buff, src.count);
}

static void kCreateBuildDir(kString path)
{
	uint attrs = kGetFileAttributes(path);
	if ((attrs & kFileAttribute_Directory) == 0)
	{
		kCreateDirectories(path);
	}
}

static void kPrepareForGeneration(kProject *p)
{
	kCreateBuildDir(p->gendir);
}

static void kPrepareForBuild(kProject *p)
{
	kCreateBuildDir(p->outdir);
	kCreateBuildDir(p->objdir);
}

static kString kGetNextGenFileName(kProject *p)
{
	uint gen = ++p->generation;

	char path[K_MAX_PATH];
	int	 len = snprintf(path, K_MAX_PATH, "%s/%u.cpp", p->gendir.data, gen);

	return kPushString(p, kString((u8 *)path, len));
}

//
//
//

//
//
//

kProject *kCreateProject(kString name)
{
	kArenaSpec spec	   = {.flags = 0, .alignment = sizeof(umem), .capacity = kMegaByte * 12};

	kContext  *context = kGetContext();
	kArena	  *arena   = kAllocArena(spec, &context->allocator);
	kProject  *project = kPushType(arena, kProject, kArena_Zero);

	project->arena	   = arena;
	project->name	   = kPushString(project, name);
	project->dir	   = ".";
	project->outdir	   = "build";
	project->objdir	   = "build/objs";
	project->gendir	   = "build/gens";

	return project;
}

void kConfigureProject(kProject *p, uint flags, kBuildKind kind, kBuildArch arch)
{
	p->flags = flags;
	p->kind	 = kind;
	p->arch	 = arch;
}

void kSetProjectDirectory(kProject *p, kString dir)
{
	p->dir = kPushString(p, dir);
}

void kSetBuildDirectory(kProject *p, kString out, kString obj)
{
	p->outdir = kPushString(p, out);
	p->objdir = kPushString(p, obj);
}

void kSetTemporaryDirectory(kProject *p, kString temp)
{
	p->gendir = kPushString(p, temp);
}

void kAddIncludeDirectory(kProject *p, kString path)
{
	path = kPushString(p, path);
	p->incdirs.Add(path);
}

void kAddLibraryDirectory(kProject *p, kString path)
{
	path = kPushString(p, path);
	p->libdirs.Add(path);
}

void kAddDefine(kProject *p, kString key, kString value)
{
	key	  = kPushString(p, key);
	value = kPushString(p, value);
	p->defines.Add({key, value});
}

void kAddLibrary(kProject *p, kString lib)
{
	lib = kPushString(p, lib);
	p->libraries.Add(lib);
}

void kAddManifestFile(kProject *p, kString path)
{
	p->manifest = kPushString(p, path);
}

void kAddSourceFile(kProject *p, kString path)
{
	kString source = kPushString(p, path);
	p->sources.Add(source);
}

void kAddResourceFile(kProject *p, kString path)
{
	kString resource = kPushString(p, path);
	p->resources.Add(resource);
}

void kAddObjectFile(kProject *p, kString path)
{
	kString object = kPushString(p, path);
	p->objects.Add(object);
}

void kAddFile(kProject *p, kString path)
{
	imem pos = kInvFindChar(path, '.', path.count);
	if (pos >= 0)
	{
		kString extension = kSubRight(path, pos);

		if (extension == ".cpp" || extension == ".CPP" || extension == ".c" || extension == ".C")
		{
			kAddSourceFile(p, path);
		}
		else if (extension == ".rc" || extension == ".RC")
		{
			kAddResourceFile(p, path);
		}
	}
}

static kDirectoryVisit kVisitProjectFile(const kDirectoryItem &item, void *data)
{
	kProject *p = (kProject *)data;
	if ((item.attributes & kFileAttribute_Directory) == 0)
	{
		kAddFile(p, item.path);
	}
	if (p->flags & kBuild_Recurse)
		return kDirectoryVisit_Recurse;
	return kDirectoryVisit_Next;
}

void kAddFilesFromDirectory(kProject *p, kString path, bool recurse)
{
	kString mb_path = kPushString(p, path);
	kVisitDirectories(mb_path, kVisitProjectFile, p);
}

bool kAddString(kProject *p, const kString src)
{
	kPrepareForGeneration(p);
	kString path = kGetNextGenFileName(p);
	if (kWriteEntireFile(path, src.data, src.count))
	{
		kAddFile(p, path);
		return true;
	}
	return false;
}

bool kEmbedBinaryData(kProject *p, const kSlice<u8> buff, kString name)
{
	kStringBuilder builder;

	builder.Write("#include <stdint.h>\n\n");
	builder.Write("extern const uint8_t ");
	builder.Write(name);
	builder.Write("[] = {");

	for (imem i = 0; i < buff.count; ++i)
	{
		if ((i) % 15 == 0)
		{
			builder.Write('\n');
		}
		builder.Write(buff[i]);
		builder.Write(',');
	}

	builder.Write(kString("};\n"));

	kString string = builder.ToString();

	bool	r	   = kAddString(p, string);

	kFreeStringBuilder(&builder);
	kFree(string.data, string.count);

	return r;
}

bool kEmbedFile(kProject *p, kString path, kString name)
{
	umem count;
	u8	*buff = kReadEntireFile(path, &count);
	if (buff)
	{
		return kEmbedBinaryData(p, kSlice<u8>(buff, count), name);
		kFree(buff, count);
	}
	return false;
}

void kDiscardProject(kProject *p)
{
	kContext *context = kGetContext();

	kFree(&p->incdirs);
	kFree(&p->libdirs);
	kFree(&p->defines);
	kFree(&p->extras);
	kFree(&p->sources);
	kFree(&p->resources);
	kFree(&p->objects);
	kFree(&p->libraries);

	kFreeArena(p->arena, &context->allocator);
}

//
//
//

static kString kOutputSourceFile(kProject *p, kString src)
{
	imem pos = src.count;

	for (; pos > 0; --pos)
	{
		if (src[pos - 1] == '\\' || src[pos - 1] == '/')
			break;
	}

	char	path[K_MAX_PATH];

	int		len = snprintf(path, kArrayCount(path), "%s/%s.obj", p->objdir.data, src.data + pos);

	kString obj = kPushString(p, kString(path, len));
	p->objects.Add(obj);

	return obj;
}

static kString kOutputResourceFile(kProject *p, kString src)
{
	imem pos = src.count;

	for (; pos > 0; --pos)
	{
		if (src[pos - 1] == '\\' || src[pos - 1] == '/')
			break;
	}

	char	path[K_MAX_PATH];

	int		len = snprintf(path, kArrayCount(path), "%s/%s.aps", p->objdir.data, src.data + pos);

	kString obj = kPushString(p, kString(path, len));
	p->objects.Add(obj);

	return obj;
}

int kBuildProject(kProject *p)
{
	kArenaSpec spec	   = {.flags = 0, .alignment = sizeof(umem), .capacity = kMegaByte * 6};

	kContext  *context = kGetContext();

	kArena	  *temp	   = kAllocArena(spec, &context->allocator);

	kPrepareForBuild(p);

	kAssert(p->kind == kBuildKind_EXE);

	kStringBuilder builder;
	builder.Write("clang++ -Wall -march=native -std=c++20 -DUNICODE -Wno-unused-function ");

	if (p->flags & kBuild_DebugSymbols)
	{
		builder.Write("-g -gcodeview ");
	}

	if (p->flags & kBuild_Optimization)
	{
		builder.Write("-O3 -funroll-loops -fprefetch-loop-arrays ");
	}
	else
	{
		builder.Write("-fstandalone-debug -fno-limit-debug-info ");
	}

	for (kString flag : p->extras)
	{
		builder.Write(flag);
		builder.Write(" ");
	}

	for (kString inc : p->incdirs)
	{
		builder.Write("-I ");
		builder.Write(inc);
		builder.Write(" ");
	}

	for (kPair define : p->defines)
	{
		builder.Write("-D");
		builder.Write(define.key);

		if (define.value.count)
		{
			builder.Write("=");
			builder.Write(define.value);
		}

		builder.Write(" ");
	}

	builder.Write("-c -o ");

	kString compile = builder.ToString(temp);

	builder.Reset();

	for (kString src : p->sources)
	{
		kString	   obj	  = kOutputSourceFile(p, src);

		kTempBlock tblock = kBeginTemporaryMemory(temp, 0);

		builder.Write(compile);
		builder.Write('"');
		builder.Write(obj);
		builder.Write('"');
		builder.Write(' ');
		builder.Write('"');
		builder.Write(src);
		builder.Write('"');

		kString cmd = builder.ToString(temp);
		builder.Reset();

		kLogTrace("CMD: %s\n", cmd.data);

		int rc = kExecuteProcess(cmd);

		if (rc)
		{
			kDiscardProject(p);
			return rc;
		}

		kEndTemporaryMemory(&tblock, 0);
	}

	for (kString res : p->resources)
	{
		kString	   obj	  = kOutputResourceFile(p, res);
		kTempBlock tblock = kBeginTemporaryMemory(temp, 0);

		builder.Write("llvm-rc -Fo ");
		builder.Write('"');
		builder.Write(obj);
		builder.Write('"');
		builder.Write(' ');
		builder.Write('"');
		builder.Write(res);
		builder.Write('"');

		kString cmd = builder.ToString(temp);
		builder.Reset();

		kLogTrace("CMD: %s\n", cmd.data);

		int rc = kExecuteProcess(cmd);

		if (rc)
		{
			kDiscardProject(p);
			return rc;
		}

		kEndTemporaryMemory(&tblock, 0);
	}

	builder.Write("lld-link /OUT:");
	builder.Write('"');
	builder.Write(p->outdir);
	builder.Write('/');
	builder.Write(p->name);
	builder.Write(".exe");
	builder.Write('"');

	builder.Write(" /NXCOMPAT ");
	builder.Write("/PDB:");
	builder.Write('"');
	builder.Write(p->outdir);
	builder.Write('/');
	builder.Write(p->name);
	builder.Write(".pdb");
	builder.Write('"');

	builder.Write(" /DYNAMICBASE ");

	for (kString dir : p->libdirs)
	{
		builder.Write("/LIBPATH:");
		builder.Write('"');
		builder.Write(dir);
		builder.Write('"');
		builder.Write(' ');
	}

	if (p->flags & kBuild_Optimization)
	{
		p->libraries.Add("libucrt.lib");
		p->libraries.Add("libvcruntime.lib");
		p->libraries.Add("libcmt.lib");
	}
	else
	{
		p->libraries.Add("libucrtd.lib");
		p->libraries.Add("libvcruntimed.lib");
		p->libraries.Add("libcmtd.lib");
	}

	for (kString lib : p->libraries)
	{
		builder.Write('"');
		builder.Write(lib);
		builder.Write('"');
		builder.Write(' ');
	}

	builder.Write("/DEBUG ");

	if (p->arch == kBuildArch_x64)
	{
		builder.Write("/MACHINE:X64 ");
	}
	else
	{
		builder.Write("/MACHINE:X86 ");
	}

	if (p->manifest.count)
	{
		builder.Write("/ManifestFile:");
		builder.Write('"');
		builder.Write(p->manifest);
		builder.Write('"');
		builder.Write(' ');
	}

	builder.Write("/NOLOGO /ERRORREPORT:PROMPT ");

	if (p->flags & kBuild_WindowsSystem)
	{
		builder.Write("/SUBSYSTEM:WINDOWS ");
	}
	else
	{
		builder.Write("/SUBSYSTEM:CONSOLE ");
	}

	for (kString obj : p->objects)
	{
		builder.Write('"');
		builder.Write(obj);
		builder.Write('"');
		builder.Write(' ');
	}

	kString cmd = builder.ToString(temp);

	kLogTrace("CMD: %s\n", cmd.data);

	int rc = kExecuteProcess(cmd);

	if (rc)
	{
		kDiscardProject(p);
		return rc;
	}

	kDiscardProject(p);

	return 0;
}
