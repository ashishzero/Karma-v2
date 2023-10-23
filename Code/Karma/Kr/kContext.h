#pragma once
#include "kCommon.h"

typedef struct kContext
{
	kAllocator           allocator;
	kRandomSource        random;
	kHandleAssertionProc assertion;
	kLogger              logger;
	kFatalErrorProc      fatal;
} kContext;

//
//
//

kContext   *kGetContext(void);
kAllocator *kGetContextAllocator(void);

void       *kAlloc(umem size);
void       *kRealloc(void *ptr, umem prev, umem size);
void        kFree(void *ptr, umem size);

u32         kRandom(void);
u32         kRandomBound(u32 bound);
u32         kRandomRange(u32 min, u32 max);
float       kRandomFloat01(void);
float       kRandomFloatBound(float bound);
float       kRandomFloatRange(float min, float max);
float       kRandomFloat(void);
void        kRandomSourceSeed(u64 state, u64 seq);

void        kSetLogLevel(kLogLevel level);

void        kLogPrintV(kLogLevel level, const char *src, const char *fmt, va_list list);
void        kLogTraceExV(const char *src, const char *fmt, va_list list);
void        kLogInfoExV(const char *src, const char *fmt, va_list list);
void        kLogWarningExV(const char *src, const char *fmt, va_list list);
void        kLogErrorExV(const char *src, const char *fmt, va_list list);

void        kLogTraceV(const char *fmt, va_list list);
void        kLogInfoV(const char *fmt, va_list list);
void        kLogWarningV(const char *fmt, va_list list);
void        kLogErrorV(const char *fmt, va_list list);

void        kLogPrint(kLogLevel level, const char *src, const char *fmt, ...);
void        kLogTraceEx(const char *src, const char *fmt, ...);
void        kLogInfoEx(const char *src, const char *fmt, ...);
void        kLogWarningEx(const char *src, const char *fmt, ...);
void        kLogErrorEx(const char *src, const char *fmt, ...);

void        kLogTrace(const char *fmt, ...);
void        kLogInfo(const char *fmt, ...);
void        kLogWarning(const char *fmt, ...);
void        kLogError(const char *fmt, ...);

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER)
#define kDebugTraceVEx(src, fmt, list) kLogTraceExV(src, fmt, list)
#define kDebugTraceEx(src, fmt, ...)   kLogTraceEx(src, fmt, __VA_ARGS__)
#endif

void        kFatalError(const char *msg);

void        kDefaultHandleAssertion(const char *file, int line, const char *proc, const char *string);
void        kDefaultFatalError(const char *message);
void        kDefaultHandleLog(void *data, kLogLevel level, const char *src, const u8 *msg, imem len);
void       *kDefaultHeapAllocator(kAllocatorMode mode, void *ptr, umem prev, umem size, void *context);
