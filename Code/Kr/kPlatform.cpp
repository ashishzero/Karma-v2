#include "kPlatform.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"
#include <UserEnv.h>
#include <avrt.h>

#pragma comment(lib, "Avrt.lib")     // AvSetMmThreadCharacteristicsW
#pragma comment(lib, "Userenv.lib")  // GetUserProfileDirectoryW
#pragma comment(lib, "Advapi32.lib") // OpenProcessToken

u64 kGetPerformanceFrequency(void)
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

u64 kGetPerformanceCounter(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

void kTerminate(uint code) { ExitProcess(code); }

//
//
//

kFile kOpenFile(kString mb_path, kFileAccess paccess, kFileShareMode pshare, kFileMethod method)
{
	wchar_t path[K_MAX_PATH];

	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)mb_path.data, (int)mb_path.count, path, kArrayCount(path) - 1);
	path[len] = 0;

	DWORD access = 0;
	if (paccess == kFileAccess_Read)
		access = GENERIC_READ;
	else if (paccess == kFileAccess_Write)
		access = GENERIC_WRITE;
	else if (paccess == kFileAccess_ReadWrite)
		access = GENERIC_READ | GENERIC_WRITE;

	DWORD share_mode = 0;
	if (pshare == kFileShareMode_Read)
		share_mode = FILE_SHARE_READ;
	else if (pshare == kFileShareMode_Write)
		share_mode = FILE_SHARE_WRITE;
	else if (pshare == kFileShareMode_ReadWrite)
		share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	DWORD disposition = 0;
	if (method == kFileMethod_CreateAlways)
		disposition = CREATE_ALWAYS;
	else if (method == kFileMethod_CreateNew)
		disposition = CREATE_NEW;
	else if (method == kFileMethod_OpenAlways)
		disposition = OPEN_ALWAYS;
	else if (method == kFileMethod_OpenExisting)
		disposition = OPEN_EXISTING;

	HANDLE file = CreateFileW(path, access, share_mode, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		kLogHresultError(GetLastError(), "Windows", "Failed to open file: \"%s\"", mb_path);
		return nullptr;
	}

	return file;
}

void kCloseFile(kFile handle) { CloseHandle((HANDLE)handle.resource); }

umem kReadFile(kFile handle, u8 *buffer, umem size)
{
	// This is done because ReadFile can with blocks of DWORD and not LARGE_INTEGER
	DWORD read_size = 0;
	if (size > UINT32_MAX)
		read_size = UINT32_MAX;
	else
		read_size = (DWORD)size;

	umem total_bytes_read        = 0;
	umem remaining_bytes_to_read = size;

	while (remaining_bytes_to_read)
	{
		DWORD read_bytes = 0;
		if (!ReadFile((HANDLE)handle.resource, buffer + total_bytes_read, read_size, &read_bytes, NULL))
		{
			kLogHresultError(GetLastError(), "Windows", "Failed while reading file");
			break;
		}

		remaining_bytes_to_read -= read_bytes;
		total_bytes_read += read_bytes;

		if (read_size > remaining_bytes_to_read) read_size = (DWORD)remaining_bytes_to_read;
	}

	return total_bytes_read;
}

umem kWriteFile(kFile handle, u8 *buff, umem size)
{
	// This is done because WriteFile can with blocks of DWORD and not LARGE_INTEGER
	DWORD write_size = 0;
	if (size > UINT32_MAX)
		write_size = UINT32_MAX;
	else
		write_size = (DWORD)size;

	umem total_bytes_written      = 0;
	umem remaining_bytes_to_write = size;

	while (remaining_bytes_to_write)
	{
		DWORD written = 0;
		if (!WriteFile((HANDLE)handle.resource, buff + total_bytes_written, write_size, &written, NULL))
		{
			kLogHresultError(GetLastError(), "Windows", "Failed while writing file");
			break;
		}

		remaining_bytes_to_write -= written;
		total_bytes_written += written;

		if (write_size > remaining_bytes_to_write) write_size = (DWORD)remaining_bytes_to_write;
	}

	return total_bytes_written;
}

umem kGetFileSize(kFile handle)
{
	LARGE_INTEGER size = {};
	GetFileSizeEx((HANDLE)handle.resource, &size);
	return size.QuadPart;
}

kString kReadEntireFile(kString path)
{
	kFile handle = kOpenFile(path, kFileAccess_Read, kFileShareMode_Read, kFileMethod_OpenExisting);
	if (handle.resource)
	{
		umem size = kGetFileSize(handle);
		u8  *buff = (u8 *)kAlloc(size);
		if (buff) size = kReadFile(handle, buff, size);
		kCloseFile(handle);
		return kString(buff, size);
	}
	return "";
}

bool kWriteEntireFile(kString path, u8 *buffer, umem size)
{
	kFile handle = kOpenFile(path, kFileAccess_Write, kFileShareMode_Read, kFileMethod_CreateAlways);
	if (handle.resource)
	{
		umem written = kWriteFile(handle, buffer, size);
		kCloseFile(handle);
		return written == size;
	}
	return false;
}

static uint kTranslateAttributes(DWORD attrs)
{
	uint translated_attrs = 0;
	if (attrs != INVALID_FILE_ATTRIBUTES)
	{
		if (attrs & FILE_ATTRIBUTE_ARCHIVE) translated_attrs |= kFileAttribute_Archive;
		if (attrs & FILE_ATTRIBUTE_COMPRESSED) translated_attrs |= kFileAttribute_Compressed;
		if (attrs & FILE_ATTRIBUTE_DIRECTORY) translated_attrs |= kFileAttribute_Directory;
		if (attrs & FILE_ATTRIBUTE_ENCRYPTED) translated_attrs |= kFileAttribute_Encrypted;
		if (attrs & FILE_ATTRIBUTE_HIDDEN) translated_attrs |= kFileAttribute_Hidden;
		if (attrs & FILE_ATTRIBUTE_NORMAL) translated_attrs |= kFileAttribute_Normal;
		if (attrs & FILE_ATTRIBUTE_OFFLINE) translated_attrs |= kFileAttribute_Offline;
		if (attrs & FILE_ATTRIBUTE_READONLY) translated_attrs |= kFileAttribute_ReadOnly;
		if (attrs & FILE_ATTRIBUTE_SYSTEM) translated_attrs |= kFileAttribute_System;
		if (attrs & FILE_ATTRIBUTE_TEMPORARY) translated_attrs |= kFileAttribute_Temporary;
	}
	return translated_attrs;
}

uint kGetFileAttributes(kString mb_path)
{
	wchar_t path[K_MAX_PATH];
	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)mb_path.data, (int)mb_path.count, path, kArrayCount(path) - 1);
	path[len] = 0;
	DWORD attrs = GetFileAttributesW(path);
	return kTranslateAttributes(attrs);
}

u64 kGetFileLastModifiedTime(kString mb_filepath)
{
	kFile file = kOpenFile(mb_filepath, kFileAccess_Read, kFileShareMode_ReadWrite, kFileMethod_OpenExisting);
	if (file)
	{
		HANDLE   handle = file.resource;
		FILETIME tm;
		if (GetFileTime(handle, 0, 0, &tm))
		{
			ULARGE_INTEGER u;
			u.HighPart = tm.dwHighDateTime;
			u.LowPart  = tm.dwLowDateTime;
			kCloseFile(file);
			return u.QuadPart;
		}
		kCloseFile(file);
	}
	return 0;
}

bool kSetWorkingDirectory(kString mb_path)
{
	wchar_t path[K_MAX_PATH];
	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)mb_path.data, (int)mb_path.count, path, kArrayCount(path) - 1);
	path[len] = 0;
	return SetCurrentDirectoryW(path);
}

int kGetWorkingDirectory(u8 *mb_path, int len)
{
	wchar_t path[K_MAX_PATH] = {};
	if (GetCurrentDirectoryW(kArrayCount(path), path))
	{
		return WideCharToMultiByte(CP_UTF8, 0, path, -1, (char *)mb_path, len, 0, 0) - 1;
	}
	return 0;
}

bool kSearchPath(kString exe)
{
	wchar_t path[K_MAX_PATH];
	int     len = MultiByteToWideChar(CP_UTF8, 0, (char *)exe.data, (int)exe.count, path, kArrayCount(path) - 1);
	path[len]   = 0;
	return SearchPathW(0, path, L".exe", 0, 0, 0);
}

bool kCreateDirectories(kString mb_path)
{
	wchar_t path[K_MAX_PATH];
	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)mb_path.data, (int)mb_path.count, path, kArrayCount(path) - 1);
	path[len] = 0;
	int count = 0;

	for (int i = 0; i < len + 1; i++)
	{
		if (path[i] == '/' || path[i] == 0)
		{
			path[i] = 0;
			if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) break;
			path[i] = '/';
			count += 1;
		}
	}

	return count;
}

int kGetUserPath(u8 *mb_path, int len)
{
	HANDLE token = INVALID_HANDLE_VALUE;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token))
	{
		wchar_t path[K_MAX_PATH] = {};

		DWORD   wlen             = kArrayCount(path);
		if (GetUserProfileDirectoryW(token, path, &wlen))
		{
			return WideCharToMultiByte(CP_UTF8, 0, path, -1, (char *)mb_path, len, 0, 0) - 1;
		}
	}
	return snprintf((char *)mb_path, len, "%s", "C:\\");
}

static void kTranslateDirectoryItem(kDirectoryItem *dst, WIN32_FIND_DATAW *src, wchar_t *root, imem root_len,
                                    char *buffer, imem buff_len)
{
	ULARGE_INTEGER converter;

	converter.HighPart = src->ftCreationTime.dwHighDateTime;
	converter.LowPart  = src->ftCreationTime.dwLowDateTime;
	dst->created       = converter.QuadPart;

	converter.HighPart = src->ftLastAccessTime.dwHighDateTime;
	converter.LowPart  = src->ftLastAccessTime.dwLowDateTime;
	dst->accessed      = converter.QuadPart;

	converter.HighPart = src->ftLastWriteTime.dwHighDateTime;
	converter.LowPart  = src->ftLastWriteTime.dwLowDateTime;
	dst->modified      = converter.QuadPart;

	converter.HighPart = src->nFileSizeHigh;
	converter.LowPart  = src->nFileSizeLow;
	dst->size          = converter.QuadPart;

	DWORD attr         = src->dwFileAttributes;
	dst->attributes    = kTranslateAttributes(attr);

	int count          = snprintf(buffer, buff_len, "%S%S", root, src->cFileName);

	for (int i = 0; i < count; ++i)
	{
		if (buffer[i] == '\\') buffer[i] = '/';
	}

	dst->path = kString(buffer, count);
	dst->name = kSubRight(dst->path, root_len);
}

static bool kVisitDirectories(wchar_t *path, int len, kDirectoryVisitorProc visitor, void *data)
{
	WIN32_FIND_DATAW find;
	HANDLE           handle = FindFirstFileW(path, &find);
	if (handle == INVALID_HANDLE_VALUE)
	{
		kLogHresultError(GetLastError(), "Windows", "Failed to visit directory: \"%S\"", path);
		return false;
	}

	// Removing "*"
	len -= 1;
	path[len] = 0;

	while (true)
	{
		if (wcscmp(find.cFileName, L".") != 0 && wcscmp(find.cFileName, L"..") != 0)
		{
			char           buffer[K_MAX_PATH];

			kDirectoryItem item;
			kTranslateDirectoryItem(&item, &find, path, len, buffer, kArrayCount(buffer));

			kDirectoryVisit r = visitor(item, data);

			if ((item.attributes & kFileAttribute_Directory) && r == kDirectoryVisit_Recurse)
			{
				wchar_t append[] = L"\\*";
				int     flen     = lstrlenW(find.cFileName);
				int     sublen   = len + flen + kArrayCount(append) - 1;

				if (sublen + 1 >= K_MAX_PATH) return false;

				wchar_t subpath[K_MAX_PATH];
				memcpy(subpath, path, len * sizeof(wchar_t));
				memcpy(subpath + len, find.cFileName, flen * sizeof(wchar_t));
				memcpy(subpath + len + flen, append, kArrayCount(append) * sizeof(wchar_t));

				if (!kVisitDirectories(subpath, sublen, visitor, data)) return false;
			}
			else if (r == kDirectoryVisit_Break)
			{
				break;
			}
		}

		if (FindNextFileW(handle, &find) == 0) break;
	}

	FindClose(handle);

	return true;
}

bool kVisitDirectories(kString mb_path, kDirectoryVisitorProc visitor, void *data)
{
	if (!visitor) return false;

	wchar_t path[K_MAX_PATH];
	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)mb_path.data, (int)mb_path.count, path, kArrayCount(path) - 1);
	path[len] = 0;

	wchar_t append[] = L"\\*";

	if (len + kArrayCount(append) >= K_MAX_PATH) return false;

	memcpy(path + len, append, sizeof(append));
	len += kArrayCount(append) - 1;

	path[len] = 0;

	for (int i = 0; i < len; ++i)
	{
		if (path[i] == '/') path[i] = '\\';
	}

	return kVisitDirectories(path, len, visitor, data);
}

//
//
//

typedef struct ThreadUserData
{
	kThreadProc      proc;
	void            *data;
	kThreadAttribute attr;
	HANDLE           event;
} ThreadUserData;

static void kSetThreadAttribute(kThreadAttribute attr)
{
	const wchar_t *Name[]      = {L"Audio", L"Capture", L"Distribution", L"Games", L"Playback", L"Pro Audio"};
	DWORD          task_index  = 0;
	HANDLE         task_handle = AvSetMmThreadCharacteristicsW(Name[attr], &task_index);
	(void)task_handle;
}

static DWORD WINAPI kThreadStartRoutine(LPVOID thread_param)
{
	ThreadUserData thrd = *(ThreadUserData *)thread_param;
	kSetThreadAttribute(thrd.attr);
	SetEvent(thrd.event);
	return thrd.proc(thrd.data);
}

int kExecuteProcess(kString cmdline)
{
	int                 len      = MultiByteToWideChar(CP_UTF8, 0, (char *)cmdline.data, (int)cmdline.count, 0, 0) + 1;
	wchar_t            *cmd      = (wchar_t *)kAlloc(sizeof(wchar_t) * len);
	STARTUPINFOW        start_up = {sizeof(start_up)};
	PROCESS_INFORMATION process  = {};

	if (!cmd)
	{
		kLogError("Windows: Failed to execute process. Reason: Out of memory");
		return 1;
	}

	int wlen  = MultiByteToWideChar(CP_UTF8, 0, (char *)cmdline.data, (int)cmdline.count, cmd, len - 1);
	cmd[wlen] = 0;

	DWORD rc  = 1;
	if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &start_up, &process))
	{
		WaitForSingleObject(process.hProcess, INFINITE);

		GetExitCodeProcess(process.hProcess, &rc);

		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
	}
	else
	{
		kLogHresultError(GetLastError(), "Windows", "Failed to execute process");
	}

	kFree(cmd, sizeof(wchar_t) * (len + 1));

	return rc;
}

kThread kLaunchThread(kThreadProc proc, void *arg, kThreadAttribute attr)
{
	ThreadUserData data;
	data.proc     = proc;
	data.data     = arg;
	data.attr     = attr;
	data.event    = CreateEventW(0, 0, 0, 0);
	HANDLE handle = CreateThread(NULL, 0, kThreadStartRoutine, &data, 0, NULL);
	if (handle) WaitForSingleObject(data.event, INFINITE);
	CloseHandle(data.event);
	return handle;
}

kThread kGetCurrentThread(void)
{
	HANDLE handle = GetCurrentThread();
	return handle;
}

void kDetachThread(kThread thread) { CloseHandle((HANDLE)thread.resource); }

void kWaitThread(kThread thread)
{
	WaitForSingleObject((HANDLE)thread.resource, INFINITE);
	CloseHandle((HANDLE)thread.resource);
}

void kTerminateThread(kThread thread, uint code)
{
	TerminateThread((HANDLE)thread.resource, code);
	CloseHandle((HANDLE)thread.resource);
}

void kSleep(u32 millisecs) { Sleep(millisecs); }

void kYield(void) { SwitchToThread(); }

void kInitSemaphore(kSemaphore *sem, u32 value, u32 max_value)
{
	static_assert(sizeof(HANDLE) >= sizeof(kSemaphore), "");

	kAssert(max_value > 0);
	sem->_id = CreateSemaphoreW(NULL, value, max_value, NULL);
}

void kFreeSemaphore(kSemaphore *sem)
{
	HANDLE handle = sem->_id;
	CloseHandle(handle);
}

u32 kReleaseSemaphore(kSemaphore *sem, u32 count)
{
	LONG   out;
	HANDLE handle = sem->_id;
	ReleaseSemaphore(handle, count, &out);
	return (u32)out;
}

void kWaitSemaphore(kSemaphore *sem)
{
	HANDLE handle = sem->_id;
	WaitForSingleObject(handle, INFINITE);
}

int kWaitSemaphoreTimed(kSemaphore *sem, u32 millisecs)
{
	HANDLE handle = sem->_id;
	DWORD  res    = WaitForSingleObject(handle, millisecs);
	if (res == WAIT_OBJECT_0) return 1;
	if (res == WAIT_TIMEOUT) return -1;
	return 0;
}

void kInitMutex(kMutex *mutex)
{
	static_assert(sizeof(CRITICAL_SECTION) <= sizeof(kMutex), "");
	InitializeCriticalSection((CRITICAL_SECTION *)mutex);
}

void kFreeMutex(kMutex *mutex)
{
	DeleteCriticalSection((CRITICAL_SECTION *)mutex);
	memset(mutex, 0, sizeof(*mutex));
}

void kLockMutex(kMutex *mutex) { EnterCriticalSection((CRITICAL_SECTION *)mutex); }

bool kTryLockMutex(kMutex *mutex) { return TryEnterCriticalSection((CRITICAL_SECTION *)mutex); }

void kUnlockMutex(kMutex *mutex) { LeaveCriticalSection((CRITICAL_SECTION *)mutex); }

void kInitCondVar(kCondVar *cond)
{
	static_assert(sizeof(CONDITION_VARIABLE) <= sizeof(kCondVar), "");
	InitializeConditionVariable((CONDITION_VARIABLE *)cond);
}

void kFreeCondVar(kCondVar *cond) {}

void kSignalCondVar(kCondVar *cond) { WakeConditionVariable((CONDITION_VARIABLE *)cond); }

void kBroadcastCondVar(kCondVar *cond) { WakeAllConditionVariable((CONDITION_VARIABLE *)cond); }

bool kWaitCondVar(kCondVar *cond, kMutex *mutex)
{
	return SleepConditionVariableCS((CONDITION_VARIABLE *)cond, (CRITICAL_SECTION *)mutex, INFINITE);
}

int kWaitCondVarTimed(kCondVar *cond, kMutex *mutex, u32 millisecs)
{
	if (SleepConditionVariableCS((CONDITION_VARIABLE *)cond, (CRITICAL_SECTION *)mutex, millisecs)) return 1;
	if (GetLastError() == ERROR_TIMEOUT) return -1;
	return 0;
}

kModule kLoadModule(kString mb_path)
{
	wchar_t path[K_MAX_PATH];
	int len   = MultiByteToWideChar(CP_UTF8, 0, (char *)mb_path.data, (int)mb_path.count, path, kArrayCount(path) - 1);
	path[len] = 0;
	HMODULE mod = LoadLibraryW(path);
	kModule res = {(kPlatformModule *)mod};
	return res;
}

void       kFreeModule(kModule module) { FreeLibrary((HMODULE)module.resource); }

kProcedure kGetProcAddress(kModule module, const char *name)
{
	HMODULE    mod  = (HMODULE)module.resource;
	kProcedure proc = (kProcedure)GetProcAddress(mod, name);
	return proc;
}

#endif
