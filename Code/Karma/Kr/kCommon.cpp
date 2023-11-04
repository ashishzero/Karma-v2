#include "kCommon.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

static void *kArenaAllocatorProc(kAllocatorMode mode, void *ptr, umem prev, umem size, void *context)
{
	kArena *arena = (kArena *)context;
	if (mode == kAllocatorMode_Alloc)
	{
		return kPushSizeAligned(arena, size, sizeof(void *), 0);
	}
	else if (mode == kAllocatorMode_Realloc)
	{
		u8 *mem = arena->Ptr;

		if (prev >= size)
		{
			if (mem + arena->Pos == ((u8 *)ptr + prev))
				kPopSize(arena, prev - size, 0);
			return ptr;
		}

		if (mem + arena->Pos == ((u8 *)ptr + prev))
		{
			if (kPushSize(arena, size - prev, 0))
				return ptr;
			return 0;
		}

		void *new_ptr = kPushSizeAligned(arena, size, sizeof(void *), 0);
		if (new_ptr)
		{
			memmove(new_ptr, ptr, prev);
			return new_ptr;
		}
		return 0;
	}
	else
	{
		u8 *current = arena->Ptr + arena->Pos;
		u8 *end_ptr = (u8 *)ptr + prev;

		if (current == end_ptr)
		{
			arena->Pos -= prev;
		}
		return 0;
	}
}

//
//
//

u8 *kAlignPointer(u8 *location, umem alignment)
{
	return (u8 *)((umem)(location + (alignment - 1)) & ~(alignment - 1));
}

void *kAlloc(kAllocator *allocator, umem size)
{
	return allocator->Proc(kAllocatorMode_Alloc, 0, 0, size, allocator->Data);
}

void *kRealloc(kAllocator *allocator, void *ptr, umem prev, umem size)
{
	return allocator->Proc(kAllocatorMode_Realloc, ptr, prev, size, allocator->Data);
}

void kFree(kAllocator *allocator, void *ptr, umem size)
{
	allocator->Proc(kAllocatorMode_Free, ptr, size, 0, allocator->Data);
}

void kArenaAllocator(kArena *arena, kAllocator *allocator)
{
	allocator->Data = arena;
	allocator->Proc = kArenaAllocatorProc;
}

kArena *kAllocArena(const kArenaSpec &spec, kAllocator *allocator)
{
	umem max_size = kAlignUp(spec.Capacity, 4 * 1024);
	u8  *mem      = (u8 *)kAlloc(allocator, max_size);

	if (!mem)
		return (kArena *)&kFallbackArena;

	kArena  stub  = {.Ptr = mem, .Pos = 0, .Cap = max_size, .Alignment = spec.Alignment, .Flags = spec.Flags};

	kArena *arena = (kArena *)kPushSize(&stub, sizeof(kArena), kArena_Zero);

	if (!arena)
	{
		return (kArena *)&kFallbackArena;
	}

	*arena     = stub;
	arena->Pos = sizeof(kArena);

	return arena;
}

void kFreeArena(kArena *arena, kAllocator *allocator)
{
	kFree(allocator, arena->Ptr, arena->Cap);
}

void kResetArena(kArena *arena)
{
	arena->Pos = sizeof(kArena);
}

void kLockArena(kArena *arena)
{
	kAtomicLock(&arena->Lock);
}

void kUnlockArena(kArena *arena)
{
	kAtomicUnlock(&arena->Lock);
}

static void LockSyncFlaggedArena(kArena *arena, uint flags)
{
	if ((arena->Flags | flags) & kArena_Sync)
		kAtomicLock(&arena->Lock);
}

static void UnlockSyncFlaggedArena(kArena *arena, uint flags)
{
	if ((arena->Flags | flags) & kArena_Sync)
		kAtomicUnlock(&arena->Lock);
}

bool kSetPosition(kArena *arena, umem pos, uint flags)
{
	bool ok = false;
	LockSyncFlaggedArena(arena, flags);
	if (pos <= arena->Cap)
	{
		arena->Pos = pos;
		ok         = true;
	}
	UnlockSyncFlaggedArena(arena, flags);
	return ok;
}

bool kAlignPosition(kArena *arena, umem alignment, uint flags)
{
	LockSyncFlaggedArena(arena, flags);
	u8  *mem     = arena->Ptr + arena->Pos;
	u8  *aligned = kAlignPointer(mem, alignment);
	umem pos     = arena->Pos + (aligned - mem);
	if (kSetPosition(arena, pos, flags))
	{
		UnlockSyncFlaggedArena(arena, flags);
		return true;
	}
	UnlockSyncFlaggedArena(arena, flags);
	return false;
}

void SetArenaAlignment(kArena *arena, u32 alignment, uint flags)
{
	LockSyncFlaggedArena(arena, flags);
	arena->Alignment = alignment;
	UnlockSyncFlaggedArena(arena, flags);
}

void *kPushSize(kArena *arena, umem size, uint flags)
{
	LockSyncFlaggedArena(arena, flags);
	if (kAlignPosition(arena, arena->Alignment, flags))
	{
		u8  *mem = arena->Ptr + arena->Pos;
		umem pos = arena->Pos + size;
		if (kSetPosition(arena, pos, flags))
		{
			if ((arena->Flags | flags) & kArena_Zero)
				memset(mem, 0, size);
			UnlockSyncFlaggedArena(arena, flags);
			return mem;
		}
	}
	UnlockSyncFlaggedArena(arena, flags);
	return 0;
}

void *kPushSizeAligned(kArena *arena, umem size, u32 alignment, uint flags)
{
	u32 saved = arena->Alignment;
	LockSyncFlaggedArena(arena, flags);
	arena->Alignment = alignment;
	void *ptr        = kPushSize(arena, size, 0);
	arena->Alignment = saved;
	UnlockSyncFlaggedArena(arena, flags);
	return ptr;
}

kTempBlock kBeginTemporaryMemory(kArena *arena, uint flags)
{
	LockSyncFlaggedArena(arena, flags);

	kTempBlock temp = {.Arena = arena, .Checkpoint = arena->Pos};

	return temp;
}

void kPopSize(kArena *arena, umem size, uint flags)
{
	LockSyncFlaggedArena(arena, flags);
	umem pos = arena->Pos - size;
	kAssert(pos >= sizeof(kArena) && pos <= arena->Cap);
	kSetPosition(arena, pos, flags);
	UnlockSyncFlaggedArena(arena, flags);
}

void kEndTemporaryMemory(kTempBlock *temp, uint flags)
{
	temp->Arena->Pos = temp->Checkpoint;
	UnlockSyncFlaggedArena(temp->Arena, flags);
}

//
//
//

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http://www.pcg-random.org
 */

u32 kRandom(kRandomSource *random)
{
	u64 oldstate   = random->State;
	random->State  = oldstate * 6364136223846793005ULL + random->Inc;
	u32 xorshifted = (u32)(((oldstate >> 18u) ^ oldstate) >> 27u);
	u32 rot        = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((rot & (~rot + 1u)) & 31));
}

u32 kRandomBound(kRandomSource *random, u32 bound)
{
	u32 threshold = (bound & (~bound + 1u)) % bound;
	for (;;)
	{
		u32 r = kRandom(random);
		if (r >= threshold)
			return r % bound;
	}
}

u32 kRandomRange(kRandomSource *random, u32 min, u32 max)
{
	return min + kRandomBound(random, max - min);
}

float kRandomFloat01(kRandomSource *random)
{
	return (float)ldexpf((float)kRandom(random), -32);
}

float kRandomFloatBound(kRandomSource *random, float bound)
{
	return kRandomFloat01(random) * bound;
}

float kRandomFloatRange(kRandomSource *random, float min, float max)
{
	return min + kRandomFloat01(random) * (max - min);
}

float kRandomFloat(kRandomSource *random)
{
	return kRandomFloatRange(random, -1, 1);
}

void kRandomSourceSeed(kRandomSource *random, u64 state, u64 seq)
{
	random->State = 0U;
	random->Inc   = (seq << 1u) | 1u;
	kRandom(random);
	random->State += state;
	kRandom(random);
}

//
//
//

int kCodepointToUTF8(u32 codepoint, u8 buffer[4])
{
	int bytes = 0;

	if (codepoint < 0x0080)
	{
		buffer[0] = codepoint & 0x007f;
		bytes     = 1;
	}
	else if (codepoint < 0x0800)
	{
		buffer[0] = ((codepoint >> 6) & 0x1f) | 0xc0;
		buffer[1] = (codepoint & 0x3f) | 0x80;
		bytes     = 2;
	}
	else if (codepoint < 0x10000)
	{
		buffer[0] = ((codepoint >> 12) & 0x0f) | 0xe0;
		buffer[1] = ((codepoint >> 6) & 0x3f) | 0x80;
		buffer[2] = (codepoint & 0x3f) | 0x80;
		bytes     = 3;
	}
	else
	{
		buffer[0] = ((codepoint >> 18) & 0xf7);
		buffer[1] = ((codepoint >> 12) & 0x3f) | 0x80;
		buffer[2] = ((codepoint >> 6) & 0x3f) | 0x80;
		buffer[3] = (codepoint & 0x3f) | 0x80;
		bytes     = 4;
	}

	return bytes;
}

int kUTF8ToCodepoint(const u8 *start, u8 *end, u32 *codepoint)
{
	u32 first = *start;

	if (first <= 0x7f)
	{
		*codepoint = first;
		return 1;
	}

	if ((first & 0xe0) == 0xc0)
	{
		if (start + 1 < end)
		{
			*codepoint = ((int)(start[0] & 0x1f) << 6);
			*codepoint |= (int)(start[1] & 0x3f);
			return 2;
		}
		else
		{
			*codepoint = 0xfffd;
			return (int)(end - start);
		}
	}

	if ((first & 0xf0) == 0xe0)
	{
		if (start + 2 <= end)
		{
			*codepoint = ((int)(start[0] & 0x0f) << 12);
			*codepoint |= ((int)(start[1] & 0x3f) << 6);
			*codepoint |= (int)(start[2] & 0x3f);
			return 3;
		}
		else
		{
			*codepoint = 0xfffd;
			return (int)(end - start);
		}
	}

	if ((first & 0xf8) == 0xf0)
	{
		if (start + 3 < end)
		{
			*codepoint = ((int)(start[0] & 0x07) << 18);
			*codepoint |= ((int)(start[1] & 0x3f) << 12);
			*codepoint |= ((int)(start[2] & 0x3f) << 6);
			*codepoint |= (int)(start[3] & 0x3f);
			return 4;
		}
		else
		{
			*codepoint = 0xfffd;
			return (int)(end - start);
		}
	}

	*codepoint = 0xfffd;

	return 1;
}

kString kCopyString(kString string, kAllocator *allocator)
{
	u8 *data = (u8 *)kAlloc(allocator, string.Count + 1);
	memmove(data, string.Items, string.Count);
	data[string.Count] = 0;
	kString result     = kString(data, string.Count);
	return result;
}

char *kStringToCstr(kString string, kAllocator *allocator)
{
	u8 *data = (u8 *)kAlloc(allocator, string.Count + 1);
	memmove(data, string.Items, string.Count);
	data[string.Count] = 0;
	return (char *)data;
}

bool kIsWhitespace(u32 ch)
{
	return ch == ' ' || ch == '\f' || ch == '\n' || ch == '\r' || ch == '\t' || ch == '\v';
}

kString kTrimString(kString str)
{
	imem trim = 0;
	for (imem index = 0; index < str.Count; ++index)
	{
		if (kIsWhitespace(str.Items[index]))
			trim += 1;
		else
			break;
	}
	str.Items += trim;
	str.Count -= trim;
	for (imem index = str.Count - 1; index >= 0; --index)
	{
		if (kIsWhitespace(str.Items[index]))
			str.Count -= 1;
		else
			break;
	}
	return str;
}

kString kSubString(const kString str, imem index, imem count)
{
	kAssert(index <= str.Count);
	count       = (imem)kMin(str.Count - index, count);
	kString sub = kString(str.Items + index, count);
	return sub;
}

kString kSubLeft(const kString str, imem count)
{
	return kSubString(str, 0, count);
}

kString kSubRight(const kString str, imem index)
{
	kAssert(index <= str.Count);
	imem    count = str.Count - index;
	kString sub   = kString(str.Items + index, count);
	return sub;
}

bool kStringEquals(kString a, kString b)
{
	if (a.Count != b.Count)
		return false;
	return memcmp(a.Items, b.Items, a.Count) == 0;
}

bool kStartsWith(kString str, kString sub)
{
	if (str.Count < sub.Count)
		return false;
	kString left = kString(str.Items, sub.Count);
	return kStringEquals(left, sub);
}

bool kEndsWith(kString str, kString sub)
{
	if (str.Count < sub.Count)
		return false;
	kString left = kString(str.Items + str.Count - sub.Count, sub.Count);
	return kStringEquals(left, sub);
}

kString kRemovePrefix(kString str, imem count)
{
	kAssert(str.Count >= count);
	str.Items += count;
	str.Count -= count;
	return str;
}

kString kRemoveSuffix(kString str, imem count)
{
	kAssert(str.Count >= count);
	str.Count -= count;
	return str;
}

imem kFindString(kString str, const kString key, imem pos)
{
	str = kSubRight(str, pos);
	while (str.Count >= key.Count)
	{
		kString sub = kString(str.Items, key.Count);
		if (kStringEquals(sub, key))
		{
			return pos;
		}
		pos += 1;
		str = kRemovePrefix(str, 1);
	}
	return -1;
}

imem kFindChar(kString str, u32 key, imem pos)
{
	u8  buffer[4];
	int len = kCodepointToUTF8(key, buffer);
	return kFindString(str, kString(buffer, len), pos);
}

imem kInvFindString(kString str, kString key, imem pos)
{
	imem index = kClamp((imem)0, str.Count - key.Count, pos);
	while (index >= 0)
	{
		kString sub = kString(str.Items + index, key.Count);
		if (kStringEquals(sub, key))
			return index;
		index -= 1;
	}
	return -1;
}

imem kInvFindChar(kString str, u32 key, imem pos)
{
	u8  buffer[4];
	int len = kCodepointToUTF8(key, buffer);
	return kInvFindString(str, kString(buffer, len), pos);
}

bool kSplitString(kString str, kString substr, kString *left, kString *right)
{
	imem pos = kFindString(str, substr, 0);
	if (pos >= 0)
	{
		*left  = kSubString(str, 0, pos);
		*right = kSubRight(str, pos + substr.Count);
		return true;
	}
	return false;
}

bool operator==(const kString a, const kString b)
{
	return kStringEquals(a, b);
}

bool operator!=(const kString a, const kString b)
{
	return !kStringEquals(a, b);
}

//
//
//

#if K_COMPILER_MSVC == 1

#include <intrin.h>

static_assert(sizeof(int) == sizeof(long), "");

int kAtomicLoad(kAtomic *atomic)
{
	return _InterlockedOr((volatile long *)&atomic->Value, 0);
}

void kAtomicStore(kAtomic *atomic, int value)
{
	_InterlockedExchange((volatile long *)&atomic->Value, value);
}

int kAtomicInc(kAtomic *dst)
{
	return _InterlockedIncrement((volatile long *)&dst->Value);
}

int kAtomicDec(kAtomic *dst)
{
	return _InterlockedDecrement((volatile long *)&dst->Value);
}

int kAtomicAdd(kAtomic *dst, int val)
{
	return _interlockedadd((volatile long *)&dst->Value, val);
}

int kAtomicCmpExg(kAtomic *dst, int exchange, int compare)
{
	return _InterlockedCompareExchange((volatile long *)&dst->Value, exchange, compare);
}

void *kAtomicCmpExgPtr(void *volatile *dst, void *exchange, void *compare)
{
	return _InterlockedCompareExchangePointer(dst, exchange, compare);
}

int kAtomicExg(kAtomic *dst, int val)
{
	return _InterlockedExchange((volatile long *)&dst->Value, val);
}
#endif

#if K_COMPILER_CLANG == 1 || K_COMPILER_GCC == 1
int kAtomicLoad(kAtomic *atomic)
{
	return __atomic_load_n(&atomic->value, __ATOMIC_SEQ_CST);
}

void kAtomicStore(kAtomic *atomic, int value)
{
	__atomic_store_n(&atomic->value, value, __ATOMIC_SEQ_CST);
}

int kAtomicInc(kAtomic *dst)
{
	return __atomic_add_fetch(&dst->value, 1, __ATOMIC_SEQ_CST);
}

int kAtomicDec(kAtomic *dst)
{
	return __atomic_add_fetch(&dst->value, -1, __ATOMIC_SEQ_CST);
}

int kAtomicAdd(kAtomic *dst, int val)
{
	return __atomic_fetch_add(&dst->value, val, __ATOMIC_SEQ_CST);
}

int kAtomicCmpExg(kAtomic *dst, int exchange, int compare)
{
	__atomic_compare_exchange_n(&dst->value, &compare, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return compare;
}

void *kAtomicCmpExgPtr(void *volatile *dst, void *exchange, void *compare)
{
	__atomic_compare_exchange_n(dst, &compare, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return compare;
}

int kAtomicExg(kAtomic *dst, int val)
{
	return __atomic_exchange_n(&dst->value, val, __ATOMIC_SEQ_CST);
}
#endif

void kAtomicLock(kAtomic *lock)
{
	while (kAtomicCmpExg(lock, 1, 0) == 1)
	{
	}
}

void kAtomicUnlock(kAtomic *lock)
{
	kAtomicExg(lock, 0);
}

//
//
//
