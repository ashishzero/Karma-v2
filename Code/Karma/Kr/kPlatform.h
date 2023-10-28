#pragma once
#include "kCommon.h"

constexpr int K_MAX_PATH = 512;

struct kFile
{
	void *Resource;
};

typedef enum kFileAccess
{
	kFileAccess_Read,
	kFileAccess_Write,
	kFileAccess_ReadWrite
} kFileAccess;

typedef enum kFileShareMode
{
	kFileShareMode_Exclusive,
	kFileShareMode_Read,
	kFileShareMode_Write,
	kFileShareMode_ReadWrite
} kFileShareMode;

typedef enum kFileMethod
{
	kFileMethod_CreateAlways,
	kFileMethod_CreateNew,
	kFileMethod_OpenAlways,
	kFileMethod_OpenExisting
} kFileMethod;

typedef enum kFileAttribute
{
	kFileAttribute_Archive    = 0x1,
	kFileAttribute_Compressed = 0x2,
	kFileAttribute_Directory  = 0x4,
	kFileAttribute_Encrypted  = 0x8,
	kFileAttribute_Hidden     = 0x10,
	kFileAttribute_Normal     = 0x20,
	kFileAttribute_Offline    = 0x40,
	kFileAttribute_ReadOnly   = 0x80,
	kFileAttribute_System     = 0x100,
	kFileAttribute_Temporary  = 0x200,
} kFileAttribute;

typedef enum kDirectoryVisit
{
	kDirectoryVisit_Next,
	kDirectoryVisit_Break,
	kDirectoryVisit_Recurse
} kDirectoryVisit;

typedef struct kDirectoryItem
{
	uint    attributes;
	u64     size;
	u64     created;
	u64     modified;
	u64     accessed;
	kString path;
	kString name;
} kDirectoryItem;

typedef kDirectoryVisit (*kDirectoryVisitorProc)(const kDirectoryItem &, void *);

//
//
//

typedef enum kThreadAttribute
{
	kThreadAttribute_Audio,
	kThreadAttribute_Capture,
	kThreadAttribute_Distribution,
	kThreadAttribute_Games,
	kThreadAttribute_Playback,
	kThreadAttribute_ProAudio,
} kThreadAttribute;

typedef struct kSemaphore
{
	void *_id;
} kSemaphore;
typedef struct kMutex
{
	u64 _id;
	u32 _mem[7];
} kMutex;

typedef struct kCondVar
{
#if K_PLATFORM_WASM == 1 || K_PLATFORM_LINUX == 1 || PLATFORM_MACOS == 1
	u32 _mem[12];
#else
	void *_id;
#endif
} kCondVar;

struct kThread
{
	void *Resource;
};

typedef int (*kThreadProc)(void *arg);

struct kModule
{
	void *Resource;
};

typedef void (*kProcedure)(void);

//
//
//

u64  kGetPerformanceFrequency(void);
u64  kGetPerformanceCounter(void);
void kTerminate(uint code);

//
//
//

kFile   kOpenFile(kString mb_filepath, kFileAccess paccess, kFileShareMode pshare, kFileMethod method);
void    kCloseFile(kFile handle);
umem    kReadFile(kFile handle, u8 *buffer, umem size);
umem    kWriteFile(kFile handle, u8 *buff, umem size);
umem    kGetFileSize(kFile handle);
kString kReadEntireFile(kString filepath);
bool    kWriteEntireFile(kString filepath, u8 *buffer, umem size);
kString kGetFileName(kString path);
kString kGetDirectoryPath(kString path);
uint    kGetFileAttributes(kString mb_filepath);
u64     kGetFileLastModifiedTime(kString mb_filepath);
bool    kSetWorkingDirectory(kString mb_path);
int     kGetWorkingDirectory(u8 *mb_path, int len);
bool    kSearchPath(kString exe);
bool    kCreateDirectories(kString mb_path);
int     kGetUserPath(u8 *mb_path, int len);
bool    kVisitDirectories(kString mb_path, kDirectoryVisitorProc visitor, void *data);

//
//
//

int        kExecuteProcess(kString cmdline);
kThread    kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr);

kThread    kGetCurrentThread(void);
void       kDetachThread(kThread thread);
void       kWaitThread(kThread thread);
void       kTerminateThread(kThread thread, uint code);
void       kSleep(u32 millisecs);
void       kYield(void);

void       kInitSemaphore(kSemaphore *sem, u32 value, u32 max);
void       kFreeSemaphore(kSemaphore *sem);
u32        kReleaseSemaphore(kSemaphore *sem, u32 count);
void       kWaitSemaphore(kSemaphore *sem);
int        kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs);
void       kInitMutex(kMutex *mutex);
void       kFreeMutex(kMutex *mutex);
void       kLockMutex(kMutex *mutex);
bool       kTryLockMutex(kMutex *mutex);
void       kUnlockMutex(kMutex *mutex);
void       kInitCondVar(kCondVar *cond);
void       kFreeCondVar(kCondVar *cond);
void       kSignalCondVar(kCondVar *cond);
void       kBroadcastCondVar(kCondVar *cond);
bool       kWaitCondVar(kCondVar *cond, kMutex *mutex);
int        kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs);

kModule    kLoadModule(kString path);
void       kFreeModule(kModule module);
kProcedure kGetProcAddress(kModule module, const char *name);
