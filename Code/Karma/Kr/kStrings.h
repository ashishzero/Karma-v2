#pragma once
#include "kCommon.h"
#include "kContext.h"
#include <string.h>
#include <stdio.h>

constexpr int K_STRING_BUILDER_DEFAULT_BUCKET_SIZE = 4 * 1024;

template <int N>
struct kStringsBucket
{
	kStringsBucket *Next    = nullptr;
	imem            Written = 0;
	u8              Buffer[N];
};

template <int N = K_STRING_BUILDER_DEFAULT_BUCKET_SIZE>
struct kStringBuilder
{
	kStringsBucket<N>  Head;
	kStringsBucket<N> *Tail      = &Head;
	umem               Written   = 0;
	kStringsBucket<N> *FirstFree = nullptr;

	//
	//
	//

	imem Write(u8 *buffer, imem size)
	{
		u8  *data = buffer;
		imem rc   = 0;

		while (size > 0)
		{
			if (Tail->Written == N)
			{
				kStringsBucket<N> *new_buk = (kStringsBucket<N> *)kAlloc(sizeof(kStringsBucket<N>));
				if (!new_buk) break;

				new_buk->Next    = 0;
				new_buk->Written = 0;
				Tail->Next       = new_buk;
				Tail             = new_buk;
			}

			imem write_size = kMin(size, N - Tail->Written);

			memcpy(Tail->Buffer + Tail->Written, data, write_size);

			Tail->Written += write_size;
			size -= write_size;
			data += write_size;

			rc += write_size;
		}

		Written += rc;
		return rc;
	}

	imem Write(kString string) { return Write(string.Items, string.Count); }

	imem Write(char value) { return Write((u8 *)&value, 1); }

	imem Write(u8 value, const char *fmt = "%u")
	{
		u8  buffer[32];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, (u32)value);
		return Write(buffer, len);
	}

	imem Write(i32 value, const char *fmt = "%d")
	{
		u8  buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, value);
		return Write(buffer, len);
	}

	imem Write(u32 value, const char *fmt = "%u")
	{
		u8  buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, value);
		return Write(buffer, len);
	}

	imem Write(i64 value, const char *fmt = "%lld")
	{
		u8  buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, (long long)value);
		return Write(buffer, len);
	}

	imem Write(u64 value, const char *fmt = "%llu")
	{
		u8  buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, (unsigned long long)value);
		return Write(buffer, len);
	}

	imem Write(float value, const char *fmt = "%f")
	{
		u8  buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, value);
		return Write(buffer, len);
	}

	imem Write(double value, const char *fmt = "%lf")
	{
		u8  buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), fmt, value);
		return Write(buffer, len);
	}

	kString ToString(kAllocator *allocator = nullptr)
	{
		if (Written == 0) return kString("");

		kString string;
		string.Items = allocator ? (uint8_t *)kAlloc(allocator, Written + 1) : (uint8_t *)kAlloc(Written + 1);
		string.Count = 0;

		if (!string.Items) return string;

		for (kStringsBucket<N> *bucket = &Head; bucket; bucket = bucket->Next)
		{
			imem copy_size = bucket->Written;
			memcpy(string.Items + string.Count, bucket->Buffer, copy_size);
			string.Count += copy_size;
		}

		string.Items[string.Count] = 0;
		return string;
	}

	kString ToString(kArena *arena)
	{
		kAllocator allocator;
		kArenaAllocator(arena, &allocator);
		return ToString(&allocator);
	}

	void Reset()
	{
		if (Tail != &Head)
		{
			kAssert(Tail->Next == nullptr && Head.Next);
			Tail->Next = FirstFree;
			FirstFree  = Head.Next;
		}
		Head = {};
		Tail = &Head;
	}
};

template <int N>
void kFreeStringBuilder(kStringBuilder<N> *builder)
{
	builder->Reset();
	kStringsBucket<N> *bucket = builder->FirstFree;
	while (bucket)
	{
		kStringsBucket<N> *next = bucket->Next;
		kFree(bucket, sizeof(*bucket));
		bucket = next;
	}
	builder->FirstFree = nullptr;
}

inproc kString kFormatStringV(const char *fmt, va_list list)
{
	va_list args;
	va_copy(args, list);
	int   len = 1 + vsnprintf(NULL, 0, fmt, args);
	char *buf = (char *)kAlloc(len);
	vsnprintf(buf, len, fmt, list);
	va_end(args);
	return kString(buf, len - 1);
}

inproc kString kFormatString(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	kString string = kFormatStringV(fmt, args);
	va_end(args);
	return string;
}

inproc kString kFormatStringV(kArena *arena, const char *fmt, va_list list)
{
	va_list args;
	va_copy(args, list);
	int   len = 1 + vsnprintf(NULL, 0, fmt, args);
	char *buf = (char *)kPushSize(arena, len, 0);
	vsnprintf(buf, len, fmt, list);
	va_end(args);
	return kString(buf, len - 1);
}

inproc kString kFormatString(kArena *arena, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	kString string = kFormatStringV(arena, fmt, args);
	va_end(args);
	return string;
}
