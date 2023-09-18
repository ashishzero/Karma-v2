#include "kThread.h"

#if K_PLATFORM_WINDOWS == 1

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <avrt.h>

#pragma comment(lib, "Avrt.lib")

typedef struct ThreadUserData {
	kThreadProc      proc;
	void *          data;
	kThreadAttribute attr;
	HANDLE          event;
} ThreadUserData;

static void kSetThreadAttribute(kThreadAttribute attr) {
	const wchar_t *Name[] = {
		L"Audio", L"Capture", L"Distribution", L"Games", L"Playback", L"Pro Audio"
	};
	DWORD task_index = 0;
	HANDLE task_handle = AvSetMmThreadCharacteristicsW(Name[attr], &task_index);
	(void)task_handle;
}

static DWORD WINAPI kThreadStartRoutine(LPVOID thread_param) {
	ThreadUserData thrd = *(ThreadUserData *)thread_param;
	kSetThreadAttribute(thrd.attr);
	SetEvent(thrd.event);
	return thrd.proc(thrd.data);
}

kThread kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr) {
	ThreadUserData data;
	data.proc       = proc;
	data.data       = arg;
	data.attr       = attr;
	data.event      = CreateEventW(0, 0, 0, 0);
	HANDLE handle   = CreateThread(NULL, 0, kThreadStartRoutine, &data, 0, NULL);
	if (handle)
		WaitForSingleObject(data.event, INFINITE);
	CloseHandle(data.event);
	return (kThread) { .ptr = handle };
}

kThread kGetCurrentThread(void) {
	HANDLE handle = GetCurrentThread();
	return (kThread ) { .ptr = handle };
}

void kDetachThread(kThread thread) {
	CloseHandle((HANDLE)thread.ptr);
}

void kWaitThread(kThread thread) {
	WaitForSingleObject((HANDLE)thread.ptr, INFINITE);
	CloseHandle((HANDLE)thread.ptr);
}

void kTerminateThread(kThread thread, uint code) {
	TerminateThread((HANDLE)thread.ptr, code);
	CloseHandle((HANDLE)thread.ptr);
}

void kSleep(u32 millisecs) {
	Sleep(millisecs);
}

void kYield(void) {
	SwitchToThread();
}

void kInitSemaphore(kSemaphore *sem, u32 value, u32 max_value) {
	static_assert(sizeof(HANDLE) >= sizeof(kSemaphore), "");

	kAssert(max_value > 0);
	sem->_id = CreateSemaphoreW(NULL, value, max_value, NULL);
}

void kFreeSemaphore(kSemaphore *sem) {
	HANDLE handle = sem->_id;
	CloseHandle(handle);
}

u32 kReleaseSemaphore(kSemaphore *sem, u32 count) {
	LONG out;
	HANDLE handle = sem->_id;
	ReleaseSemaphore(handle, count, &out);
	return (u32)out;
}

void kWaitSemaphore(kSemaphore *sem) {
	HANDLE handle = sem->_id;
	WaitForSingleObject(handle, INFINITE);
}

int kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs) {
	HANDLE handle = sem->_id;
	DWORD res = WaitForSingleObject(handle, millisecs);
	if (res == WAIT_OBJECT_0)
		return 1;
	if (res == WAIT_TIMEOUT)
		return -1;
	return 0;
}

void kInitMutex(kMutex *mutex) {
	static_assert(sizeof(CRITICAL_SECTION) <= sizeof(kMutex), "");
	InitializeCriticalSection((CRITICAL_SECTION *)mutex);
}

void kFreeMutex(kMutex *mutex) {
	DeleteCriticalSection((CRITICAL_SECTION *)mutex);
	memset(mutex, 0, sizeof(*mutex));
}

void kLockMutex(kMutex *mutex) {
	EnterCriticalSection((CRITICAL_SECTION *)mutex);
}

bool kTryLockMutex(kMutex *mutex) {
	return TryEnterCriticalSection((CRITICAL_SECTION *)mutex);
}

void kUnlockMutex(kMutex *mutex) {
	LeaveCriticalSection((CRITICAL_SECTION *)mutex);
}

void kInitCondVar(kCondVar *cond) {
	static_assert(sizeof(CONDITION_VARIABLE) <= sizeof(kCondVar), "");
	InitializeConditionVariable((CONDITION_VARIABLE *)cond);
}

void kFreeCondVar(kCondVar *cond) {
}

void kSignalCondVar(kCondVar *cond) {
	WakeConditionVariable((CONDITION_VARIABLE *)cond);
}

void kBroadcastCondVar(kCondVar *cond) {
	WakeAllConditionVariable((CONDITION_VARIABLE *)cond);
}

bool kWaitCondVar(kCondVar *cond, kMutex *mutex) {
	return SleepConditionVariableCS((CONDITION_VARIABLE *)cond, (CRITICAL_SECTION *)mutex, INFINITE);
}

int kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs) {
	if (SleepConditionVariableCS((CONDITION_VARIABLE *)cond, (CRITICAL_SECTION *)mutex, millisecs))
		return 1;
	if (GetLastError() == ERROR_TIMEOUT)
		return -1;
	return 0;
}

#endif
