#pragma once
#include "kCommon.h"

typedef enum kThreadAttribute {
	kThreadAttribute_Audio,
	kThreadAttribute_Capture,
	kThreadAttribute_Distribution,
	kThreadAttribute_Games,
	kThreadAttribute_Playback,
	kThreadAttribute_ProAudio,
} kThreadAttribute;

typedef struct kSemaphore { void *_id; }              kSemaphore;
typedef struct kMutex     { u64   _id; u32 _mem[7]; } kMutex;

typedef struct kCondVar {
#if K_PLATFORM_WASM == 1 || K_PLATFORM_LINUX == 1 || PLATFORM_MACOS == 1
	u32 _mem[12];
#else
	void *_id;
#endif
} kCondVar;

typedef struct kThread { void *ptr; } kThread;
typedef int(*kThreadProc)(void *arg);

kThread  kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr);;
kThread  kGetCurrentThread(void);
void     kDetachThread(kThread thread);
void     kWaitThread(kThread thread);
void     kTerminateThread(kThread thread, uint code);
void     kSleep(u32 millisecs);
void     kYield(void);

void     kInitSemaphore(kSemaphore *sem, u32 value, u32 max);
void     kFreeSemaphore(kSemaphore *sem);
u32      kReleaseSemaphore(kSemaphore *sem, u32 count);
void     kWaitSemaphore(kSemaphore *sem);
int      kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs);
void     kInitMutex(kMutex *mutex);
void     kFreeMutex(kMutex *mutex);
void     kLockMutex(kMutex *mutex);
bool     kTryLockMutex(kMutex *mutex);
void     kUnlockMutex(kMutex *mutex);
void     kInitCondVar(kCondVar *cond);
void     kFreeCondVar(kCondVar *cond);
void     kSignalCondVar(kCondVar *cond);
void     kBroadcastCondVar(kCondVar *cond);
bool     kWaitCondVar(kCondVar *cond, kMutex *mutex);
int      kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs);
