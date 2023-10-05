#pragma once
#include "kCommon.h"
#include "kContext.h"
#include <string.h>
#include <stdio.h>

constexpr int K_STRING_BUILDER_DEFAULT_BUCKET_SIZE = 16 * 1024;

template <int N> struct kStringsBucket
{
	kStringsBucket *next	= nullptr;
	imem			written = 0;
	u8				buffer[N];
};

template <int N = K_STRING_BUILDER_DEFAULT_BUCKET_SIZE> struct kStringBuilder
{
	kStringsBucket<N>  head;
	kStringsBucket<N> *tail	   = &head;
	umem			   written = 0;
	kStringsBucket<N> *free	   = nullptr;

	//
	//
	//

	imem Write(u8 *buffer, imem size)
	{
		u8	*data = buffer;
		imem rc	  = 0;

		while (size > 0)
		{
			if (tail->written == N)
			{
				kStringsBucket<N> *new_buk = (kStringsBucket<N> *)kAlloc(sizeof(kStringsBucket<N>));
				if (!new_buk)
					break;

				new_buk->next	 = 0;
				new_buk->written = 0;
				tail->next		 = new_buk;
				tail			 = new_buk;
			}

			imem write_size = kMin(size, N - tail->written);

			memcpy(tail->buffer + tail->written, data, write_size);

			tail->written += write_size;
			size -= write_size;
			data += write_size;

			rc += write_size;
		}

		written += rc;
		return rc;
	}

	imem Write(kString string)
	{
		return Write(string.data, string.count);
	}

	imem Write(char value)
	{
		return Write((u8 *)&value, 1);
	}

	imem Write(u8 value)
	{
		u8	buffer[32];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%u", (u32)value);
		return Write(buffer, len);
	}

	imem Write(i32 value)
	{
		u8	buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%d", value);
		return Write(buffer, len);
	}

	imem Write(u32 value)
	{
		u8	buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%u", value);
		return Write(buffer, len);
	}

	imem Write(i64 value)
	{
		u8	buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%lld", (long long)value);
		return Write(buffer, len);
	}

	imem Write(u64 value)
	{
		u8	buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%llu", (unsigned long long)value);
		return Write(buffer, len);
	}

	imem Write(float value)
	{
		u8	buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%f", value);
		return Write(buffer, len);
	}

	imem Write(double value)
	{
		u8	buffer[128];
		int len = snprintf((char *)buffer, kArrayCount(buffer), "%lf", value);
		return Write(buffer, len);
	}

	kString ToString(kAllocator *allocator = nullptr)
	{
		if (written == 0)
			return kString("");

		kString string;
		string.data	 = allocator ? (uint8_t *)kAlloc(allocator, written + 1) : (uint8_t *)kAlloc(written + 1);
		string.count = 0;

		if (!string.data)
			return string;

		for (kStringsBucket<N> *bucket = &head; bucket; bucket = bucket->next)
		{
			imem copy_size = bucket->written;
			memcpy(string.data + string.count, bucket->buffer, copy_size);
			string.count += copy_size;
		}

		string.data[string.count] = 0;
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
		if (tail != &head)
		{
			kAssert(tail->next == nullptr && head.next);
			tail->next = free;
			free	   = head.next;
		}
		head = {};
		tail = &head;
	}
};

template <int N> void kFreeStringBuilder(kStringBuilder<N> *builder)
{
	builder->Reset();
	kStringsBucket<N> *bucket = builder->free;
	while (bucket)
	{
		kStringsBucket<N> *next = bucket->next;
		kFree(bucket, sizeof(*bucket));
		bucket = next;
	}
	builder->free = nullptr;
}

inproc kString kFormatStringV(const char *fmt, va_list list)
{
	va_list args;
	va_copy(args, list);
	int	  len = 1 + vsnprintf(NULL, 0, fmt, args);
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
