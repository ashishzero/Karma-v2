#pragma once
#include "kLinkedList.h"
#include "kContext.h"
#include <string.h>

constexpr int K_RESOUCE_PER_BLOCK = 64;

template <typename T>
struct kResouceType
{
	T             Resource;
	kResouceType *Next;
	kResouceType *Prev;
};

template <typename T>
struct kMemoryBlock
{
	kMemoryBlock   *Next;
	kResouceType<T> Data[K_RESOUCE_PER_BLOCK];
};

template <typename T>
using kMemoryPoolResourceFreeProc = void (*)(T *, void *);

template <typename T>
struct kMemoryPool
{
	kResouceType<T>  First;
	kResouceType<T> *FirstFree;
	kMemoryBlock<T> *Blocks;

	kMemoryPool()
	{
		kDListInit(&First);
		FirstFree   = nullptr;
		Blocks = nullptr;
	}

	void ResetPool(kMemoryPoolResourceFreeProc<T> dtor = nullptr, void *data = nullptr)
	{
		if (dtor)
		{
			while (!kDListIsEmpty(&First))
			{
				kResouceType *r = kDListPopFront(&First);
				dtor(&r->resource, data);

				memset(r, 0, sizeof(*r));
				r->next = FirstFree;
				FirstFree    = r;
			}
		}
		else
		{
			while (!kDListIsEmpty(&First))
			{
				kResouceType *r = kDListPopFront(&First);
				memset(r, 0, sizeof(*r));
				r->next = FirstFree;
				FirstFree    = r;
			}
		}
	}

	void Destroy(kMemoryPoolResourceFreeProc<T> dtor = nullptr, void *data = nullptr)
	{
		if (dtor)
		{
			while (!kDListIsEmpty(&First))
			{
				kResouceType<T> *r = kDListPopFront(&First);
				dtor(&r->Resource, data);
			}
		}

		kMemoryBlock<T> *prev  = nullptr;
		kMemoryBlock<T> *block = Blocks;

		while (block)
		{
			prev  = block;
			block = prev->Next;
			kFree(prev, sizeof(*prev));
		}

		*this = kMemoryPool<T>();
	}

	T *Alloc(void)
	{
		if (!FirstFree)
		{
			kMemoryBlock<T> *block = (kMemoryBlock<T> *)kAlloc(sizeof(kMemoryBlock<T>));

			if (!block) return nullptr;

			memset(block, 0, sizeof(*block));

			for (int i = 0; i < K_RESOUCE_PER_BLOCK - 1; ++i)
			{
				block->Data[i].Next = &block->Data[i + 1];
			}

			FirstFree        = block->Data;
			block->Next = Blocks;
			Blocks      = block;
		}

		kResouceType<T> *r = FirstFree;
		FirstFree               = r->Next;

		kDListPushBack(&First, r);

		return &r->Resource;
	}

	void Free(T *resource)
	{
		kResouceType<T> *r = (kResouceType<T> *)resource;
		kDListRemove(r);
		memset(r, 0, sizeof(*r));
		r->Next = FirstFree;
		FirstFree    = r;
	}
};
