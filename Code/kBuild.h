#pragma once
#include "kCommon.h"

enum kBuildKind
{
	kBuildKind_EXE,
	kBuildKind_DLL,
	kBuildKind_LIB
};

enum kBuildArch
{
	kBuildArch_x86,
	kBuildArch_x64,
};

enum kBuildFlags
{
	kBuild_Optimization	 = 0x1,
	kBuild_DebugSymbols	 = 0x2,
	kBuild_WindowsSystem = 0x4,
	kBuild_Recurse		 = 0x100 // internal
};

struct kProject;
typedef void (*kProjectBuildProc)(int, const char **);

#define K_BUILD_PROC K_EXPORT extern "C"

kProject *kCreateProject(kString name);
void	  kConfigureProject(kProject *p, uint flags, kBuildKind kind, kBuildArch arch);
void	  kSetProjectDirectory(kProject *p, kString dir);
void	  kSetBuildDirectory(kProject *p, kString out, kString obj = "");
void	  kSetTemporaryDirectory(kProject *p, kString temp);
void	  kAddIncludeDirectory(kProject *p, kString path);
void	  kAddLibraryDirectory(kProject *p, kString path);
void	  kAddDefine(kProject *p, kString key, kString value = "");
void	  kAddLibrary(kProject *p, kString lib);
void	  kAddManifestFile(kProject *p, kString path);
void	  kAddSourceFile(kProject *p, kString path);
void	  kAddResourceFile(kProject *p, kString path);
void	  kAddObjectFile(kProject *p, kString path);
void	  kAddFile(kProject *p, kString path);
void	  kAddFilesFromDirectory(kProject *p, kString path, bool recurse = false);
bool	  kAddString(kProject *p, const kString src);
bool	  kEmbedBinaryData(kProject *p, const kSlice<u8> buff, kString name);
bool	  kEmbedFile(kProject *p, kString path, kString name);
bool	  kAddUnityFiles(kProject *p, kSlice<kString> files);
void	  kDiscardProject(kProject *p);
int		  kBuildProject(kProject *p);
