#pragma once

#include "kContext.h"
#include <string.h>

template <typename T>
struct kArray
{
	imem Count;
	imem Capacity;
	T   *Items;

	kArray() : Count(0), Capacity(0), Items(nullptr)
	{}

	operator kSpan<T>()
	{
		return kSpan<T>(Items, Count);
	}

	operator const kSpan<T>() const
	{
		return kSpan<T>(Items, Count);
	}

	T &operator[](ptrdiff_t i)
	{
		kAssert(i >= 0 && i < Count);
		return Items[i];
	}

	const T &operator[](ptrdiff_t i) const
	{
		kAssert(i >= 0 && i < Count);
		return Items[i];
	}

	T *begin()
	{
		return Items;
	}

	T *end()
	{
		return Items + Count;
	}

	const T *begin() const
	{
		return Items;
	}

	const T *end() const
	{
		return Items + Count;
	}

	imem NextCapacity(imem count)
	{
		imem new_cap = Capacity ? (Capacity << 1) : 8;
		return new_cap > count ? new_cap : count;
	}

	T *TryGet(imem index)
	{
		if (index < Count)
		{
			return &Items[index];
		}
		return 0;
	}

	T &First(void)
	{
		kAssert(Count != 0);
		return Items[0];
	}

	T &Last(void)
	{
		kAssert(Count != 0);
		return Items[Count - 1];
	}

	const T &First(void) const
	{
		kAssert(Count != 0);
		return Items[0];
	}

	const T &Last(void) const
	{
		kAssert(Count != 0);
		return Items[Count - 1];
	}

	void Pop(void)
	{
		kAssert(Count > 0);
		Count -= 1;
	}

	void Reset(void)
	{
		Count = 0;
	}

	void Remove(imem index)
	{
		kAssert(index < Count);
		memmove(Items + index, Items + (index + 1), (Count - index - 1) * sizeof(T));
		Count -= 1;
	}

	void RemoveUnordered(imem index)
	{
		kAssert(index < Count);
		Items[index] = Items[Count - 1];
		Count -= 1;
	}

	bool Reserve(imem req_cap)
	{
		if (req_cap <= Capacity)
			return true;

		void *mem = kRealloc(Items, Capacity * sizeof(T), req_cap * sizeof(T));
		if (mem)
		{
			Items    = (T *)mem;
			Capacity = req_cap;
			return true;
		}

		return false;
	}

	bool Resize(imem n)
	{
		if (Reserve(n))
		{
			Count = n;
			return true;
		}
		return false;
	}

	bool Resize(imem n, const T &src)
	{
		if (Reserve(n))
		{
			for (imem idx = Count; idx < n; ++idx)
			{
				Items[idx] = src;
			}
			Count = n;
			return true;
		}
		return false;
	}

	T *Extend(imem n)
	{
		imem req_count = Count + n;
		imem cur_count = Count;

		if (req_count < Capacity)
		{
			Count = req_count;
			return &Items[cur_count];
		}

		imem new_cap = NextCapacity(req_count);

		if (Reserve(new_cap))
		{
			Count = req_count;
			return &Items[cur_count];
		}

		return 0;
	}

	T *Add(void)
	{
		return Extend(1);
	}

	void Add(const T &src)
	{
		T *dst = Add();
		if (dst)
			*dst = src;
	}

	bool CopyBuffer(T *src, imem src_count)
	{
		if (Count + src_count > Capacity)
		{
			imem new_cap = NextCapacity(Capacity + src_count);
			if (!Reserve(new_cap))
				return false;
		}

		memcpy(Items + Count, src, src_count * sizeof(T));
		Count += src_count;

		return true;
	}

	bool CopyArray(kSpan<T> src)
	{
		if (src.Count)
		{
			return CopyBuffer(src.Items, src.Count);
		}
		return false;
	}

	bool Insert(imem index, const T &src)
	{
		kAssert(index < Count);

		if (Add())
		{
			memmove(Items + (index + 1), Items + index, (Count - index));
			Items[index] = src;
			return true;
		}

		return false;
	}

	void Pack(void)
	{
		if (Count != Capacity)
		{
			void *mem = kRealloc(Items, Capacity * sizeof(T), Count * sizeof(T));
			if (mem)
			{
				Items    = (T *)mem;
				Capacity = Count;
			}
		}
	}

	kArray<T> Clone(void)
	{
		kArray<T> dst = {};
		dst.CopyBuffer(Items, Count);
		return dst;
	}
};

template <typename T>
void kFree(kArray<T> *arr)
{
	kFree(arr->Items, arr->Capacity * sizeof(T));
}

template <typename T>
void kFree(kSpan<T> *arr)
{
	kFree(arr->Items, arr->Count * sizeof(T));
}

//
//
//

template <typename T, typename SearchFunc, typename... Args>
imem Find(kSpan<T> arr, SearchFunc func, const Args &...args)
{
	for (imem index = 0; index < arr.Count; ++index)
	{
		if (func(arr[index], args...))
		{
			return index;
		}
	}
	return -1;
}

template <typename T>
imem Find(kSpan<T> arr, const T &v)
{
	for (imem index = 0; index < arr.Count; ++index)
	{
		auto elem = arr.Items + index;
		if (*elem == v)
		{
			return index;
		}
	}
	return -1;
}
