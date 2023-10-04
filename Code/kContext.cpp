#include "kContext.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#if defined(K_EXPORT_SYMBOLS)
K_EXPORT kContext context = {.allocator = {.proc = kDefaultHeapAllocator},
							 .random	= {.state = 0x853c49e6748fea9bULL, .inc = 0xda3e39cb94b95bdbULL},
							 .assertion = kDefaultHandleAssertion,
							 .logger	= {.proc = kDefaultHandleLog},
							 .fatal		= kDefaultFatalError};
#elif defined(K_IMPORT_SYMBOLS)
K_IMPORT kContext context;
#else
static kContext context = {.allocator = {.proc = kDefaultHeapAllocator},
						   .random	  = {.state = 0x853c49e6748fea9bULL, .inc = 0xda3e39cb94b95bdbULL},
						   .assertion = kDefaultHandleAssertion,
						   .logger	  = {.proc = kDefaultHandleLog},
						   .fatal	  = kDefaultFatalError};
#endif

void kHandleAssertion(const char *file, int line, const char *proc, const char *desc)
{
	context.assertion(file, line, proc, desc);
	kTriggerBreakpoint();
}

//
//
//

kContext *kGetContext(void)
{
	return &context;
}

void *kAlloc(umem size)
{
	return kAlloc(&context.allocator, size);
}

void *kRealloc(void *ptr, umem prev, umem size)
{
	return kRealloc(&context.allocator, ptr, prev, size);
}

void kFree(void *ptr, umem size)
{
	kFree(&context.allocator, ptr, size);
}

u32 kRandom(void)
{
	return kRandom(&context.random);
}

u32 kRandomBound(u32 bound)
{
	return kRandomBound(&context.random, bound);
}

u32 kRandomRange(u32 min, u32 max)
{
	return kRandomRange(&context.random, min, max);
}

float kRandomFloat01(void)
{
	return kRandomFloat01(&context.random);
}

float kRandomFloatBound(float bound)
{
	return kRandomFloatBound(&context.random, bound);
}

float kRandomFloatRange(float min, float max)
{
	return kRandomFloatRange(&context.random, min, max);
}

float kRandomFloat(void)
{
	return kRandomFloat(&context.random);
}

void kRandomSourceSeed(u64 state, u64 seq)
{
	kRandomSourceSeed(&context.random, state, seq);
}

void kLogPrintV(kLogLevel level, const char *fmt, va_list list)
{
	if (level >= context.logger.level)
	{
		char buff[4096];
		int	 len = vsnprintf(buff, kArrayCount(buff), fmt, list);
		context.logger.proc(context.logger.data, context.logger.level, (u8 *)buff, len);
	}
}

void kLogTraceV(const char *fmt, va_list list)
{
	kLogPrintV(kLogLevel_Verbose, fmt, list);
}

void kLogWarningV(const char *fmt, va_list list)
{
	kLogPrintV(kLogLevel_Warning, fmt, list);
}

void kLogErrorV(const char *fmt, va_list list)
{
	kLogPrintV(kLogLevel_Error, fmt, list);
}

void kLogPrint(kLogLevel level, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	kLogPrintV(level, fmt, args);
	va_end(args);
}

void kLogTrace(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	kLogTraceV(fmt, args);
	va_end(args);
}

void kLogWarning(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	kLogWarningV(fmt, args);
	va_end(args);
}

void kLogError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	kLogErrorV(fmt, args);
	va_end(args);
}

void kFatalError(const char *msg)
{
	context.fatal(msg);
}

//
//
//

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"

void kDefaultHandleAssertion(const char *file, int line, const char *proc, const char *string)
{
	kLogError("Assertion Failed: %(%): % ; Procedure: %", file, line, string, proc);
	kTriggerBreakpoint();
}

void kDefaultFatalError(const char *message)
{
	int		 wlen = MultiByteToWideChar(CP_UTF8, 0, message, (int)strlen(message), NULL, 0);
	wchar_t *msg  = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, ((imem)wlen + 1) * sizeof(wchar_t));
	if (msg)
	{
		MultiByteToWideChar(CP_UTF8, 0, message, (int)strlen(message), msg, wlen + 1);
		msg[wlen] = 0;
		FatalAppExitW(0, msg);
	}
	FatalAppExitW(0, L"out of memory");
}

void kDefaultHandleLog(void *data, kLogLevel level, const u8 *msg, imem msg_len)
{
	static kAtomic	  Guard		  = {0};

	static const WORD ColorsMap[] = {FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
									 FOREGROUND_RED | FOREGROUND_GREEN, FOREGROUND_RED};
	static_assert(kArrayCount(ColorsMap) == kLogLevel_Error + 1, "");

	wchar_t buff[4096];

	int		len = MultiByteToWideChar(CP_UTF8, 0, (char *)msg, (int)msg_len, buff, kArrayCount(buff) - 1);
	buff[len]	= 0;

	kAtomicLock(&Guard);

	HANDLE handle = (level == kLogLevel_Error) ? GetStdHandle(STD_ERROR_HANDLE) : GetStdHandle(STD_OUTPUT_HANDLE);

	if (handle != INVALID_HANDLE_VALUE)
	{
		CONSOLE_SCREEN_BUFFER_INFO buffer_info;
		GetConsoleScreenBufferInfo(handle, &buffer_info);
		SetConsoleTextAttribute(handle, ColorsMap[level]);
		DWORD written = 0;
		WriteConsoleW(handle, buff, len, &written, 0);
		SetConsoleTextAttribute(handle, buffer_info.wAttributes);
	}

	if (IsDebuggerPresent())
		OutputDebugStringW(buff);

	kAtomicUnlock(&Guard);
}

void *kDefaultHeapAllocator(kAllocatorMode mode, void *ptr, umem prev, umem size, void *ctx)
{
	HANDLE heap = GetProcessHeap();
	if (mode == kAllocatorMode_Alloc)
		return HeapAlloc(heap, 0, size);
	else if (mode == kAllocatorMode_Realloc)
		return ptr ? HeapReAlloc(heap, 0, ptr, size) : HeapAlloc(heap, 0, size);
	else
		HeapFree(heap, 0, ptr);
	return 0;
}

#endif
