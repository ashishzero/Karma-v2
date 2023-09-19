#pragma once
#include "kCommon.h"

typedef struct kContext {
	kAllocator           allocator;
	kRandomSource        random;
	kHandleAssertionProc assertion;
	kLogger              logger;
	kFatalErrorProc      fatal;
} kContext;

//
//
//

kContext *kGetContext(void);

void     *kAlloc(umem size);
void     *kRealloc(void *ptr, umem prev, umem size);
void      kFree(void *ptr, umem size);

u32       kRandom(void);
u32       kRandomBound(u32 bound);
u32       kRandomRange(u32 min, u32 max);
float     kRandomFloat01(void);
float     kRandomFloatBound(float bound);
float     kRandomFloatRange(float min, float max);
float     kRandomFloat(void);
void      kRandomSourceSeed(u64 state, u64 seq);

void      kLogPrintV(kLogLevel level, const char *fmt, va_list list);
void      kLogTraceV(const char *fmt, va_list list);
void      kLogWarningV(const char *fmt, va_list list);
void      kLogErrorV(const char *fmt, va_list list);

void      kLogPrint(kLogLevel level, const char *fmt, ...);
void      kLogTrace(const char *fmt, ...);
void      kLogWarning(const char *fmt, ...);
void      kLogError(const char *fmt, ...);

void      kFatalError(const char *msg);

void      kDefaultHandleAssertion(const char *file, int line, const char *proc, const char *string);
void      kDefaultFatalError(const char *message);
void      kDefaultHandleLog(void *data, kLogLevel level, const u8 *msg, imem len);
void     *kDefaultHeapAllocator(kAllocatorMode mode, void *ptr, umem prev, umem size, void *context);
