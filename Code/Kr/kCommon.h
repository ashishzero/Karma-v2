#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
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

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64) ||         \
	defined(_M_X64)
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
#elif ((!defined(__NACL__)) &&                                                                                         \
       ((defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))))
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

#if K_PLATFORM_WINDOWS == 1
#define K_EXPORT __declspec(dllexport)
#define K_IMPORT __declspec(dllimport)
#else
#define K_EXPORT
#define K_IMPORT
#endif

#if defined(K_EXPORT_SYMBOLS)
#define K_API K_EXPORT
#elif defined(K_IMPORT_SYMBOLS)
#define K_API K_IMPORT
#else
#define K_API
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
[[noreturn]] inproc __attribute__((always_inline)) void Unreachable()
{
	kDebugTriggerbreakpoint();
	__builtin_unreachable();
}
#elif defined(_MSC_VER)
[[noreturn]] __forceinline void kUnreachable()
{
	kDebugTriggerbreakpoint();
	__assume(false);
}
#else // ???
inproc void kUnreachable() { kTriggerBreakpoint(); }
#endif

#define kNoDefaultCase()                                                                                               \
	default: kUnreachable(); break

[[noreturn]] inproc void kUnimplemented()
{
	kTriggerBreakpoint();
	kUnreachable();
}

//
//
//

#define kConcatRaw(x, y) x##y
#define kConcatRaw2(x, y) kConcatRaw(x, y)

template <typename Item>
struct kExitScope
{
	Item lambda;
	kExitScope(Item lambda) : lambda(lambda) {}
	~kExitScope() { lambda(); }
};
struct kExitScope2
{
	template <typename Item>
	kExitScope<Item> operator+(Item t)
	{
		return t;
	}
};
#define kDefer const auto &kConcatRaw2(defer__, __LINE__) = kExitScope2() + [&]()

//
//
//

typedef uint32_t  uint;
typedef float     real;
typedef uint8_t   byte;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int32_t   b32;
typedef int64_t   i64;
typedef float     r32;
typedef double    r64;
typedef size_t    umem;
typedef ptrdiff_t imem;

#define REAL32_MIN FLT_MIN
#define REAL32_MAX FLT_MAX
#define REAL64_MIN DBL_MIN
#define REAL64_MAX DBL_MAX
#define REAL_MIN REAL32_MIN
#define REAL_MAX REAL32_MAX
#define REAL_EPSILON FLT_EPSILON

#define K_PI (3.1415926535f)
#define K_PI_INVERSE (1.0f / K_PI)
#define K_TAU (K_PI / 2)

#define kArrayCount(a) (sizeof(a) / sizeof((a)[0]))

template <typename Item>
constexpr Item kMin(Item a, Item b)
{
	return a < b ? a : b;
}
template <typename Item>
constexpr Item kMax(Item a, Item b)
{
	return a > b ? a : b;
}
template <typename Item>
constexpr Item kClamp(Item a, Item b, Item v)
{
	return kMin(b, kMax(a, v));
}
template <typename Item>
constexpr bool kIsInRange(Item a, Item b, Item v)
{
	return v >= a && v <= b;
}
template <typename Item>
void kSwap(Item *a, Item *b)
{
	Item t = *b;
	*b     = *a;
	*a     = t;
}
template <typename Item>
constexpr Item kIsPower2(Item value)
{
	return ((value != 0) && ((value) & ((value)-1)) == 0);
}
template <typename Item, typename U>
constexpr Item kAlignUp(Item x, U p)
{
	return (((x) + (p)-1) & ~((p)-1));
}
template <typename Item, typename U>
constexpr Item kAlignDown(Item x, U p)
{
	return ((x) & ~((p)-1));
}

template <typename Item, typename U>
constexpr Item kIPower(Item v, U p)
{
	Item r = v;
	for (Item i = 1; i < p; ++i)
		r *= v;
	return r;
}

template <typename Item>
constexpr static Item kNextPowerOf2(Item n)
{
	if (kIsPower2(n)) return n;
	Item count = 0;
	while (n != 0)
	{
		n >>= 1;
		count += 1;
	}
	return (Item)1 << (Item)count;
}

inproc u16 constexpr kBSwap16(u16 a) { return ((((a)&0x00FF) << 8) | (((a)&0xFF00) >> 8)); }

inproc u32 constexpr kBSwap32(u32 a)
{
	return ((((a)&0x000000FF) << 24) | (((a)&0x0000FF00) << 8) | (((a)&0x00FF0000) >> 8) | (((a)&0xFF000000) >> 24));
}

inproc u64 constexpr kBSwap64(u64 a)
{
	return ((((a)&0x00000000000000FFULL) << 56) | (((a)&0x000000000000FF00ULL) << 40) |
	        (((a)&0x0000000000FF0000ULL) << 24) | (((a)&0x00000000FF000000ULL) << 8) |
	        (((a)&0x000000FF00000000ULL) >> 8) | (((a)&0x0000FF0000000000ULL) >> 24) |
	        (((a)&0x00FF000000000000ULL) >> 40) | (((a)&0xFF00000000000000ULL) >> 56));
}

inproc constexpr umem  kKiloByte  = (1024ULL);
inproc constexpr umem  kMegaByte  = (kKiloByte * 1024ULL);
inproc constexpr umem  kGegaByte  = (kMegaByte * 1024ULL);

inproc constexpr float kDegToRad  = ((K_PI / 180.0f));
inproc constexpr float kRadToDeg  = ((180.0f / K_PI));
inproc constexpr float kRadToTurn = (1.0f / (2.0f * K_PI));
inproc constexpr float kTurnToRad = (2.0f * K_PI);

#define kAlignNative alignas(sizeof(void *))

//
//
//

void kHandleAssertion(const char *file, int line, const char *proc, const char *desc);

#if defined(K_BUILD_DEBUG) || defined(K_BUILD_DEVELOPER) || defined(K_BUILD_TEST)

#define kAssert(x)                                                                                                     \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(x)) kHandleAssertion(__FILE__, __LINE__, __PROCEDURE__, #x);                                             \
	} while (0)
#else
#define kDebugTriggerbreakpoint()
#define kAssert(x)                                                                                                     \
	do                                                                                                                 \
	{                                                                                                                  \
		0;                                                                                                             \
	} while (0)
#endif

//
//
//

template <typename T>
struct kVec2T
{
	union {
		struct
		{
			T x, y;
		};
		T m[2];
	};

	constexpr kVec2T() : x(0), y(0) {}
	explicit constexpr kVec2T(T a) : x(a), y(a) {}
	explicit constexpr kVec2T(T a, T b) : x(a), y(b) {}
};

template <typename T>
struct kVec3T
{
	union {
		struct
		{
			T x, y, z;
		};
		T m[3];
		struct
		{
			kVec2T<T> xy;
			T         _end;
		};
		struct
		{
			T         _beg;
			kVec2T<T> yz;
		};
	};

	constexpr kVec3T() : x(0), y(0), z(0) {}
	explicit constexpr kVec3T(T a) : x(a), y(a), z(a) {}
	explicit constexpr kVec3T(T a, T b, T c) : x(a), y(b), z(c) {}
	explicit constexpr kVec3T(kVec2T<T> ab, T c) : x(ab.x), y(ab.y), z(c) {}
	explicit constexpr kVec3T(T a, kVec2T<T> cd) : x(a), y(cd.x), z(cd.y) {}
};

template <typename T>
struct kVec4T
{
	union {
		struct
		{
			T x, y, z, w;
		};
		T m[4];
		struct
		{
			kVec2T<T> xy;
			kVec2T<T> zw;
		};
		struct
		{
			kVec3T<T> xyz;
			T         _end;
		};
		struct
		{
			kVec3T<T> _beg;
			T         yzw;
		};
	};

	constexpr kVec4T() : x(0), y(0), z(0), w(0) {}
	explicit constexpr kVec4T(T a) : x(a), y(a), z(a), w(a) {}
	explicit constexpr kVec4T(T a, T b, T c, T d) : x(a), y(b), z(c), w(d) {}
	explicit constexpr kVec4T(kVec2T<T> ab, kVec2T<T> cd) : x(ab.x), y(ab.y), z(cd.x), w(cd.y) {}
	explicit constexpr kVec4T(kVec2T<T> ab, T c, T d) : x(ab.x), y(ab.y), z(c), w(d) {}
	explicit constexpr kVec4T(kVec3T<T> abc, T d) : x(abc.x), y(abc.y), z(abc.z), w(d) {}
	explicit constexpr kVec4T(T a, kVec3T<T> bcd) : x(a), y(bcd.x), z(bcd.y), w(bcd.z) {}
};

using kVec2  = kVec2T<float>;
using kVec3  = kVec3T<float>;
using kVec4  = kVec4T<float>;
using kVec2i = kVec2T<int>;
using kVec3i = kVec3T<int>;
using kVec4i = kVec4T<int>;

typedef union kMat2 {
	kVec2 rows[2];
	float m[4];
	float m2[2][2];

	inline kMat2() {}
	explicit inline kMat2(kVec2 a, kVec2 b) : rows{a, b} {}
	explicit inline kMat2(float a)
	{
		rows[0] = kVec2(a, 0);
		rows[1] = kVec2(0, a);
	}
	explicit inline kMat2(float _00, float _01, float _10, float _11)
	{
		m2[0][0] = _00;
		m2[0][1] = _01;
		m2[1][0] = _10;
		m2[1][1] = _11;
	}
} kMat2;

typedef union kMat3 {
	kVec3 rows[3];
	float m[9];
	float m2[3][3];

	inline kMat3() {}
	explicit inline kMat3(kVec3 a, kVec3 b, kVec3 c) : rows{a, b, c} {}
	explicit inline kMat3(float a)
	{
		rows[0] = kVec3(a, 0, 0);
		rows[1] = kVec3(0, a, 0);
		rows[2] = kVec3(0, 0, a);
	}
} kMat3;

typedef union kMat4 {
	kVec4 rows[4];
	float m[16];
	float m2[4][4];

	inline kMat4() {}
	explicit inline kMat4(kVec4 a, kVec4 b, kVec4 c, kVec4 d) : rows{a, b, c, d} {}
	explicit inline kMat4(float a)
	{
		rows[0] = kVec4(a, 0, 0, 0);
		rows[1] = kVec4(0, a, 0, 0);
		rows[2] = kVec4(0, 0, a, 0);
		rows[3] = kVec4(0, 0, 0, a);
	}
} kMat4;

typedef union kQuat {
	struct
	{
		float x, y, z, w;
	};
	kVec4 vector;

	kQuat() : x(0), y(0), z(0), w(0) {}
	kQuat(kVec4 v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
	}
	explicit kQuat(float b, float c, float d, float a)
	{
		x = b;
		y = c;
		z = d;
		w = a;
	}
} kQuat;

typedef struct kRect
{
	kVec2 min;
	kVec2 max;

	kRect() {}

	kRect(float minx, float miny, float maxx, float maxy)
	{
		min = kVec2(minx, miny);
		max = kVec2(maxx, maxy);
	}

	kRect(kVec2 rmin, kVec2 rmax)
	{
		min = rmin;
		max = rmax;
	}
} kRect;

//
//
//

template <typename Item>
struct kSpan
{
	imem  count;
	Item *data;

	inline kSpan() : count(0), data(nullptr) {}
	inline kSpan(const Item *p, imem n) : count(n), data((Item *)p) {}
	template <imem _Count>
	constexpr kSpan(const Item (&a)[_Count]) : count(_Count), data((Item *)a)
	{}
	inline Item &operator[](imem index) const
	{
		kAssert(index < count);
		return data[index];
	}
	inline Item       *begin() { return data; }
	inline Item       *end() { return data + count; }
	inline const Item *begin() const { return data; }
	inline const Item *end() const { return data + count; }

	Item              &First(void)
	{
		kAssert(count);
		return data[0];
	}
	Item &Last(void)
	{
		kAssert(count);
		return data[count - 1];
	}
	const Item &First(void) const
	{
		kAssert(count);
		return data[0];
	}
	const Item &Last(void) const
	{
		kAssert(count);
		return data[count - 1];
	}

	umem Size(void) const
	{
		return count * sizeof(Item);
	}
};

#define kStrFmt "%.*s"
#define kStrArg(x) (int)((x).count), ((x).data)
#define kArrFmt "{ %zd, %p }"
#define kArrArg(x) ((x).count), ((x).data)

//
//
//

template <typename T>
struct kHandle
{
	T *resource;

	kHandle() : resource(0) {}

	kHandle(void *p) { resource = (T *)p; }

	kHandle(nullptr_t) { resource = 0; }

	operator bool() { return resource != 0; }
};

template <typename T>
bool operator==(kHandle<T> a, kHandle<T> b)
{
	return a.resource == b.resource;
}

template <typename T>
bool operator!=(kHandle<T> a, kHandle<T> b)
{
	return a.resource != b.resource;
}

//
//
//

struct kString
{
	imem count;
	u8  *data;

	kString() : count(0), data(0) {}
	kString(kSpan<u8> av) : count(av.count), data(av.data) {}
	kString(kSpan<char> av) : count(av.count), data((u8 *)av.data) {}
	template <imem _Length>
	constexpr kString(const char (&a)[_Length]) : count(_Length - 1), data((u8 *)a)
	{}
	kString(const u8 *_Data, imem _Length) : count(_Length), data((u8 *)_Data) {}
	kString(const char *_Data, imem _Length) : count(_Length), data((u8 *)_Data) {}
	const u8 &operator[](const imem index) const
	{
		kAssert(index < count);
		return data[index];
	}
	u8 &operator[](const imem index)
	{
		kAssert(index < count);
		return data[index];
	}
	inline u8       *begin() { return data; }
	inline u8       *end() { return data + count; }
	inline const u8 *begin() const { return data; }
	inline const u8 *end() const { return data + count; }
	operator kSpan<u8>() { return kSpan<u8>(data, count); }
};

//
//
//

typedef struct kRandomSource
{
	u64 state;
	u64 inc;
} kRandomSource;

//
//
//

typedef enum kAllocatorMode
{
	kAllocatorMode_Alloc,
	kAllocatorMode_Realloc,
	kAllocatorMode_Free,
} kAllocatorMode;

typedef void *(*kAllocatorProc)(kAllocatorMode, void *, umem, umem, void *);

typedef struct kAllocator
{
	kAllocatorProc proc;
	void          *data;
} kAllocator;

enum kArenaFlags
{
	kArena_Sync = 0x1,
	kArena_Zero = 0x2
};

//
//
//

typedef struct kAtomic
{
	i32 volatile value;
} kAtomic;

typedef struct kArena
{
	u8     *mem;
	umem    pos;
	umem    cap;
	u32     alignment;
	u32     flags;
	kAtomic lock;
} kArena;

typedef struct kTempBlock
{
	kArena *arena;
	umem    checkpoint;
} kTempBlock;

typedef struct kArenaSpec
{
	u32  flags;
	u32  alignment;
	umem capacity;
} kArenaSpec;

//
//
//

typedef enum kLogLevel
{
	kLogLevel_Verbose = 0,
	kLogLevel_Warning,
	kLogLevel_Error
} kLogLevel;
typedef void (*kLogProc)(void *, kLogLevel, const u8 *, imem);

typedef void (*kFatalErrorProc)(const char *message);
typedef void (*kHandleAssertionProc)(const char *file, int line, const char *proc, const char *string);

//
//
//

typedef struct kLogger
{
	kLogProc  proc;
	kLogLevel level;
	void     *data;
} kLogger;

//
//
//

inproc void *kFallbackAllocatorProc(kAllocatorMode mode, void *ptr, umem prev, umem nsize, void *data) { return ptr; }

static const kAllocator kFallbackAllocator = {kFallbackAllocatorProc};
static const kArena     kFallbackArena     = {};

//
//
//

u8     *kAlignPointer(u8 *location, umem alignment);

void   *kAlloc(kAllocator *allocator, umem size);
void   *kRealloc(kAllocator *allocator, void *ptr, umem prev, umem size);
void    kFree(kAllocator *allocator, void *ptr, umem size);

void    kArenaAllocator(kArena *arena, kAllocator *allocator);
kArena *kAllocArena(const kArenaSpec &spec, kAllocator *allocator);
void    kFreeArena(kArena *arena, kAllocator *allocator);
void    kResetArena(kArena *arena);

void    kLockArena(kArena *arena);
void    kUnlockArena(kArena *arena);

bool    kSetPosition(kArena *arena, umem pos, uint flags);
bool    kAlignPosition(kArena *arena, umem alignment, uint flags);

void   *kPushSize(kArena *arena, umem size, uint flags);
void   *kPushSizeAligned(kArena *arena, umem size, u32 alignment, uint flags);

#define kPushType(arena, type, flags) (type *)kPushSizeAligned(arena, sizeof(type), alignof(type), flags)
#define kPushArray(arena, type, count, flags)                                                                          \
	(type *)kPushSizeAligned(arena, sizeof(type) * (count), alignof(type), flags)

void       kPopSize(kArena *arena, umem size, uint flags);

kTempBlock kBeginTemporaryMemory(kArena *arena, uint flags);
void       kEndTemporaryMemory(kTempBlock *temp, uint flags);

//
//
//

u32   kRandom(kRandomSource *random);
u32   kRandomBound(kRandomSource *random, u32 bound);
u32   kRandomRange(kRandomSource *random, u32 min, u32 max);
float kRandomFloat01(kRandomSource *random);
float kRandomFloatBound(kRandomSource *random, float bound);
float kRandomFloatRange(kRandomSource *random, float min, float max);
float kRandomFloat(kRandomSource *random);
void  kRandomSourceSeed(kRandomSource *random, u64 state, u64 seq);

//
//
//

int     kCodepointToUTF8(u32 codepoint, u8 buffer[4]);
int     kUTF8ToCodepoint(const u8 *start, u8 *end, u32 *codepoint);

kString kCopyString(kString string, kAllocator *allocator);
char   *kStringToCstr(kString string, kAllocator *allocator);

bool    kIsWhitespace(u32 ch);
kString kTrimString(kString str);
kString kSubString(const kString str, imem index, imem count);
kString kSubLeft(const kString str, imem count);
kString kSubRight(const kString str, imem index);

bool    kStringEquals(kString a, kString b);
bool    kStartsWith(kString str, kString sub);
bool    kEndsWith(kString str, kString sub);

kString kRemovePrefix(kString str, imem count);
kString kRemoveSuffix(kString str, imem count);

imem    kFindString(kString str, const kString key, imem pos);
imem    kFindChar(kString str, u32 key, imem pos);
imem    kInvFindString(kString str, kString key, imem pos);
imem    kInvFindChar(kString str, u32 key, imem pos);

bool    kSplitString(kString str, kString substr, kString *left, kString *right);

bool    operator==(const kString a, const kString b);
bool    operator!=(const kString a, const kString b);

//
//
//

int   kAtomicLoad(kAtomic *atomic);
void  kAtomicStore(kAtomic *atomic, int value);
int   kAtomicInc(kAtomic *dst);
int   kAtomicDec(kAtomic *dst);
int   kAtomicAdd(kAtomic *dst, int val);
int   kAtomicCmpExg(kAtomic *dst, int exchange, int compare);
void *kAtomicCmpExgPtr(void *volatile *dst, void *exchange, void *compare);
int   kAtomicExg(kAtomic *dst, int val);
void  kAtomicLock(kAtomic *lock);
void  kAtomicUnlock(kAtomic *lock);
