#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <float.h>
#include <assert.h>

#if defined(__clang__) || defined(__ibmxl__)
#define K_COMPILER_CLANG 1
#elif defined(_MSC_VER)
#define K_COMPILER_MSVC 1
#elif defined(__GNUC__)
#define K_COMPILER_GCC 1
#elif defined(__MINGW32__) || defined(__MINGW64__)
#define K_COMPILER_MINGW 1
#elif defined(__INTEL_COMPILER)
#define K_COMPILER_INTEL 1
#else
#error Missing Compiler detection
#endif

#if !defined(K_COMPILER_CLANG)
#define K_COMPILER_CLANG 0
#endif
#if !defined(K_COMPILER_MSVC)
#define K_COMPILER_MSVC 0
#endif
#if !defined(K_COMPILER_GCC)
#define K_COMPILER_GCC 0
#endif
#if !defined(K_COMPILER_INTEL)
#define K_COMPILER_INTEL 0
#endif

#if defined(__ANDROID__) || defined(__ANDROID_API__)
#define K_PLATFORM_ANDROID 1
#define __PLATFORM__ "Andriod"
#elif defined(__gnu_linux__) || defined(__linux__) || defined(linux) || defined(__linux)
#define K_PLATFORM_LINUX 1
#define __PLATFORM__ "linux"
#elif defined(macintosh) || defined(Macintosh)
#define K_PLATFORM_MAC 1
#define __PLATFORM__ "Mac"
#elif defined(__APPLE__) && defined(__MACH__)
#define K_PLATFORM_MAC 1
#define __PLATFORM__ "Mac"
#elif defined(__APPLE__)
#define K_PLATFORM_IOS 1
#define __PLATFORM__ "iOS"
#elif defined(_WIN64) || defined(_WIN32)
#define K_PLATFORM_WINDOWS 1
#define __PLATFORM__ "Windows"
#elif defined(K_PLATFORM_WASM)
#define __PLATFORM__ "WebAssembly"
#else
#error Missing Operating System Detection
#endif

#if !defined(K_PLATFORM_ANDROID)
#define K_PLATFORM_ANDROID 0
#endif
#if !defined(K_PLATFORM_LINUX)
#define K_PLATFORM_LINUX 0
#endif
#if !defined(K_PLATFORM_MAC)
#define K_PLATFORM_MAC 0
#endif
#if !defined(K_PLATFORM_IOS)
#define K_PLATFORM_IOS 0
#endif
#if !defined(K_PLATFORM_WINDOWS)
#define K_PLATFORM_WINDOWS 0
#endif
#if !defined(K_PLATFORM_WASM)
#define K_PLATFORM_WASM 0
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64) || defined(_M_X64)
#define K_ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(_X86_)
#define K_ARCH_X86 1
#elif defined(__arm__) || defined(__thumb__) || defined(_ARM) || defined(_M_ARM) || defined(_M_ARMT)
#define K_ARCH_ARM 1
#elif defined(__aarch64__)
#define K_ARCH_ARM64 1
#elif defined(K_PLATFORM_WASM)
#define K_ARCH_WASM32 1
#else
#error Missing Architecture Identification
#endif

#if !defined(K_ARCH_X64)
#define K_ARCH_X64 0
#endif
#if !defined(K_ARCH_X86)
#define K_ARCH_X86 0
#endif
#if !defined(K_ARCH_ARM)
#define K_ARCH_ARM 0
#endif
#if !defined(K_ARCH_ARM64)
#define K_ARCH_ARM64 0
#endif
#if !defined(K_ARCH_WASM32)
#define K_ARCH_WASM32 0
#endif

#if defined(__GNUC__)
#define __PROCEDURE__ __FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define __PROCEDURE__ __PRETTY_PROCEDURE__
#elif defined(__FUNCSIG__)
#define __PROCEDURE__ __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define __PROCEDURE__ __PROCEDURE__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define __PROCEDURE__ __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define __PROCEDURE__ __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define __PROCEDURE__ __func__
#elif defined(_MSC_VER)
#define __PROCEDURE__ __FUNCSIG__
#else
#define __PROCEDURE__ "_unknown_"
#endif

#if defined(HAVE_SIGNAL_H) && !defined(__WATCOMC__)
#include <signal.h> // raise()
#endif

#ifndef kTriggerBreakpoint
#if defined(_MSC_VER)
#define kTriggerBreakpoint() __debugbreak()
#elif ((!defined(__NACL__)) && ((defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))))
#define kTriggerBreakpoint() __asm__ __volatile__("int $3\n\t")
#elif defined(__386__) && defined(__WATCOMC__)
#define kTriggerBreakpoint() _asm { int 0x03}
#elif defined(HAVE_SIGNAL_H) && !defined(__WATCOMC__)
#define kTriggerBreakpoint() raise(SIGTRAP)
#elif defined(__clang__)
#define kTriggerBreakpoint() __builtin_trap()
#else
#define kTriggerBreakpoint() ((int *)0) = 0
#endif
#endif

#if defined(K_COMPILER_GCC)
#define inproc static inline
#else
#define inproc inline
#endif

#if !defined(K_BUILD_DEBUG) && !defined(K_BUILD_DEVELOPER) && !defined(K_BUILD_RELEASE) && !defined(K_BUILD_TEST)
#if defined(_DEBUG) || defined(DEBUG)
#define K_BUILD_DEBUG
#elif defined(NDEBUG)
#define K_BUILD_RELEASE
#else
#define K_BUILD_DEBUG
#endif
#endif

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER) || defined(K_BUILD_TEST)
#define kDebugTriggerbreakpoint kTriggerBreakpoint
#else
#define kDebugTriggerbreakpoint()
#endif

#ifdef __GNUC__
noreturn inproc __attribute__((always_inline)) void Unreachable() { kDebugTriggerbreakpoint(); __builtin_unreachable(); }
#elif defined(_MSC_VER)
noreturn __forceinline void kUnreachable() { kDebugTriggerbreakpoint(); __assume(false); }
#else // ???
inproc void kUnreachable() { kTriggerBreakpoint(); }
#endif

#define kNoDefaultCase() default: kUnreachable(); break

noreturn inproc void kUnimplemented() { kTriggerBreakpoint(); kUnreachable(); }

//
//
//

typedef uint32_t              uint;
typedef float                 real;
typedef uint8_t               byte;
typedef uint8_t               u8;
typedef uint16_t              u16;
typedef uint32_t              u32;
typedef uint64_t              u64;
typedef int8_t                i8;
typedef int16_t               i16;
typedef int32_t               i32;
typedef int32_t               b32;
typedef int64_t               i64;
typedef float                 r32;
typedef double                r64;
typedef size_t                umem;
typedef ptrdiff_t             imem;

#define REAL32_MIN            FLT_MIN
#define REAL32_MAX            FLT_MAX
#define REAL64_MIN            DBL_MIN
#define REAL64_MAX            DBL_MAX
#define REAL_MIN              REAL32_MIN
#define REAL_MAX              REAL32_MAX
#define REAL_EPSILON          FLT_EPSILON;

#define K_PI                  (3.1415926535f)
#define K_PI_INVERSE          (1.0f / K_PI)
#define K_TAU                 (K_PI / 2)

#define kArrayCount(a)         (sizeof(a) / sizeof((a)[0]))

#define kMin(a, b)             ((a) < (b) ? (a) : (b))
#define kMax(a, b)             ((a) > (b) ? (a) : (b))
#define kClamp(a, b, v)        kMin(b, kMax(a, v))
#define kIsInRange(a, b, v)    (((v) >= (a)) && ((v) <= (b)))
#define kSwap(a, b, Type)      do { Type temp = b; b + a; a = temp; } while (0)
#define kIsPower2(value)       ((value != 0) && ((value) & ((value)-1)) == 0)
#define kAlignUp(x, p)         (((x) + (p)-1) & ~((p)-1))
#define kAlignDown(x, p)       ((x) & ~((p)-1))
#define kBSwap16(a)            ((((a)&0x00FF) << 8) | (((a)&0xFF00) >> 8))
#define kBSwap32(a)            ((((a)&0x000000FF) << 24) | (((a)&0x0000FF00) << 8) | (((a)&0x00FF0000) >> 8) | (((a)&0xFF000000) >> 24))
#define kBSwap64(a)            ((((a)&0x00000000000000FFULL) << 56) | (((a)&0x000000000000FF00ULL) << 40) | (((a)&0x0000000000FF0000ULL) << 24) | \
                               (((a)&0x00000000FF000000ULL) << 8) | (((a)&0x000000FF00000000ULL) >> 8) | (((a)&0x0000FF0000000000ULL) >> 24) | \
                               (((a)&0x00FF000000000000ULL) >> 40) | (((a)&0xFF00000000000000ULL) >> 56))

inproc imem kIPower(imem v, imem p) {
	imem r = v;
	for (imem i = 1; i < p; ++i)
		r *= v;
	return r;
}

inproc imem kNextPower2(imem n) {
	if (kIsPower2(n))
		return n;
	imem count = 0;
	while (n != 0) {
		n >>= 1;
		count += 1;
	}
	return 1LL << count;
}

#define kKiloByte  (1024ULL)
#define kMegaByte  (kKiloByte * 1024ULL)
#define kGegaByte  (kMegaByte * 1024ULL)

#define kDegToRad  ((K_PI / 180.0f))
#define kRadToDeg  ((180.0f / K_PI))
#define kRadToTurn (1.0f / (2.0f * K_PI))
#define kTurnToRad (2.0f * K_PI)

#define kAlignNative alignas(sizeof(void*))

//
//
//

void kHandleAssertion(const char *file, int line, const char *proc, const char *desc);

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER) || defined(K_BUILD_TEST)

#define kAssert(x)                                                   \
	do                                                               \
	{                                                                \
		if (!(x))                                                    \
			kHandleAssertion(__FILE__, __LINE__, __PROCEDURE__, #x); \
	} while (0)
#else
#define kDebugTriggerbreakpoint()
#define kAssert(x) \
	do             \
	{              \
		0;         \
	} while (0)
#endif

//
//
//

typedef struct kVec2 {
	union {
		struct { float x, y; };
		float  m[2];
	};
} kVec2;

typedef struct kVec2i {
	union {
		struct { int x, y; };
		int    m[2];
	};
} kVec2i;

typedef struct kVec3 {
	union {
		struct { float x, y, z; };
		float  m[3];
		struct { kVec2 xy; float _end; };
		struct { float _beg; kVec2 yz; };
	};
} kVec3;

typedef struct kVec3i {
	union {
		struct { int x, y, z; };
		int    m[3];
		struct { kVec2i xy; int _end; };
		struct { int _beg; kVec2i yz; };
	};
} kVec3i;

typedef struct kVec4 {
	union {
		struct { float x, y, z, w; };
		float  m[4];
		struct { kVec2 xy; kVec2 zw; };
		struct { kVec3 xyz; float _end; };
		struct { kVec3 _beg; kVec3 yzw; };
	};
} kVec4;

typedef struct kVec4i {
	union {
		struct { int x, y, z, w; };
		int    m[4];
		struct { kVec2i xy; kVec2i zw; };
		struct { kVec3i xyz; int _end; };
		struct { kVec3i _beg; kVec3i yzw; };
	};
} kVec4i;

typedef union kMat2 {
	kVec2 rows[2];
	float m[4];
	float m2[2][2];
} kMat2;

typedef union kMat3 {
	kVec3 rows[3];
	float m[9];
	float m2[3][3];
} kMat3;

typedef union kMat4 {
	kVec4 rows[4];
	float m[16];
	float m2[4][4];
} kMat4;

typedef union kQuat {
	struct { float x, y, z, w; };
	kVec4  vector;
} kQuat;

//
//
//

typedef struct kString {
	imem count;
	u8 * data;
} kString;

#define kFormatString      "%.*s"
#define kExpandString(str) (int)((str).count), (str).data
#define kStringView(cstr)  (kString) { .count = sizeof(cstr)-1, .data = cstr }

//
//
//

typedef struct kRandomSource {
	u64 state;
	u64 inc;
} kRandomSource;

//
//
//

typedef enum kAllocatorMode {
	kAllocatorMode_Alloc,
	kAllocatorMode_Realloc,
	kAllocatorMode_Free,
} kAllocatorMode;

typedef void *(*kAllocatorProc)(kAllocatorMode, void *, umem, umem , void *);

typedef struct kAllocator {
	kAllocatorProc proc;
	void *        data;
} kAllocator;

enum kArenaFlags {
	kArena_Sync = 0x1,
	kArena_Zero = 0x2
};

//
//
//

typedef struct kAtomic { i32 volatile value; } kAtomic;

typedef struct kArena {
	u8 *    mem;
	umem    pos;
	umem    cap;
	u32     alignment;
	u32     flags;
	kAtomic lock;
} kArena;

typedef struct kTempBlock {
	kArena *arena;
	umem    checkpoint;
} kTempBlock;

typedef struct kArenaSpec {
	u32  flags;
	u32  alignment;
	umem capacity;
} kArenaSpec;

//
//
//

typedef enum  kLogLevel { kLogLevel_Verbose = 0, kLogLevel_Warning, kLogLevel_Error } kLogLevel;
typedef void(*kLogProc)(void *, kLogLevel, const u8 *, imem);

typedef void (*kFatalErrorProc)(const char *message);
typedef void (*kHandleAssertionProc)(const char *file, int line, const char *proc, const char *string);

//
//
//

typedef struct kLogger {
	kLogProc  proc;
	kLogLevel level;
	void *    data;
} kLogger;

//
//
//

inproc void *kFallbackAllocatorProc(kAllocatorMode mode, void *ptr, umem prev, umem nsize, void* data) { return ptr; }

static const kAllocator   kFallbackAllocator = { kFallbackAllocatorProc };
static const kArena       kFallbackArena     = { 0, 0, 0, 0, 0, 0 };

//
//
//

u8   *     kAlignPointer(u8 *location, umem alignment);

void *     kAllocEx(kAllocator *allocator, umem size);
void *     kReallocEx(kAllocator *allocator, void *ptr, umem prev, umem size);
void       kFreeEx(kAllocator *allocator, void *ptr, umem size);

void       kArenaAllocator(kArena *arena, kAllocator *allocator);
kArena *   kAllocArena(kArenaSpec *spec, kAllocator *allocator);
void       kFreeArena(kArena *arena, kAllocator *allocator);
void       kResetArena(kArena *arena);

void       kLockArena(kArena *arena);
void       kUnlockArena(kArena *arena);

bool       kSetPosition(kArena *arena, umem pos, uint flags);
bool       kAlignPosition(kArena *arena, umem alignment, uint flags);

void *     kPushSize(kArena *arena, umem size, uint flags);
void *     kPushSizeAligned(kArena *arena, umem size, u32 alignment, uint flags);

#define    kPushType(arena, type, flags)         (type *)kPushSizeAligned(arena, sizeof(type), alignof(type), flags)
#define    kPushArray(arena, type, count, flags) (type *)kPushSizeAligned(arena, sizeof(type) * (count), alignof(type), flags)

void       kPopSize(kArena *arena, umem size, uint flags);

kTempBlock kBeginTemporaryMemory(kArena *arena, uint flags);
void       kEndTemporaryMemory(kTempBlock *temp, uint flags);

//
//
//

u32        kRandomEx(kRandomSource *random);
u32        kRandomBoundEx(kRandomSource *random, u32 bound);
u32        kRandomRangeEx(kRandomSource *random, u32 min, u32 max);
float      kRandomFloat01Ex(kRandomSource *random);
float      kRandomFloatBoundEx(kRandomSource *random, float bound);
float      kRandomFloatRangeEx(kRandomSource *random, float min, float max);
float      kRandomFloatEx(kRandomSource *random);
void       kRandomSourceSeedEx(kRandomSource *random, u64 state, u64 seq);

//
//
//

int        kCodepointToUTF8(u32 codepoint, u8 buffer[4]);
int        kUTF8ToCodepoint(const u8 *start, imem count, u32 *codepoint);

kString    kCopyString(kString string, kAllocator *allocator);
char *     kStringToCstr(kString string, kAllocator *allocator);

bool       kIsWhitespace(u32 ch);
kString    kTrimString(kString str);
kString    kSubString(const kString str, imem index, imem count);
kString    kSubLeft(const kString str, imem count);
kString    kSubRight(const kString str, imem index);

bool       kStringEquals(kString a, kString b);
bool       kStringStartsWith(kString str, kString sub);
bool       kStringEndsWith(kString str, kString sub);

kString    kRemovePrefix(kString str, imem count);
kString    kRemoveSuffix(kString str, imem count);

imem       kFindString(kString str, const kString key, imem pos);
imem       kFindChar(kString str, u32 key, imem pos);
imem       kInvFindString(kString str, kString key, imem pos);
imem       kInvFindChar(kString str, u32 key, imem pos);

bool       kSplitString(kString str, kString substr, kString *left, kString *right);

//
//
//

int        kAtomicLoad(kAtomic *atomic);
void       kAtomicStore(kAtomic *atomic, int value);
int        kAtomicInc(kAtomic *dst);
int        kAtomicDec(kAtomic *dst);
int        kAtomicAdd(kAtomic *dst, int val);
int        kAtomicCmpExg(kAtomic *dst, int exchange, int compare);
void *     kAtomicCmpExgPtr(void *volatile *dst, void *exchange, void *compare);
int        kAtomicExg(kAtomic *dst, int val);
void       kAtomicLock(kAtomic *lock);
void       kAtomicUnlock(kAtomic *lock);
