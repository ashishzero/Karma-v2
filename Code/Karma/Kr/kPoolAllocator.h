#pragma once
#include "kLinkedList.h"
#include "kContext.h"
#include <string.h>

constexpr int K_RESOUCE_PER_BLOCK = 64;

template <typename T>
struct kResouceType
{
	T             resource;
	kResouceType *next;
	kResouceType *prev;
};

template <typename T>
struct kMemoryBlock
{
	kMemoryBlock   *next;
	kResouceType<T> data[K_RESOUCE_PER_BLOCK];
};

template <typename T>
using kMemoryPoolResourceFreeProc = void (*)(T *, void *);

template <typename T>
struct kMemoryPool
{
	kResouceType<T>  first;
	kResouceType<T> *free;
	kMemoryBlock<T> *blocks;

	kMemoryPool()
	{
		kDListInit(&first);
		free   = nullptr;
		blocks = nullptr;
	}

	void ResetPool(kMemoryPoolResourceFreeProc<T> dtor = nullptr, void *data = nullptr)
	{
		if (dtor)
		{
			while (!kDListIsEmpty(&first))
			{
				kResouceType *r = kDListPopFront(&first);
				dtor(&r->resource, data);

				memset(r, 0, sizeof(*r));
				r->next = free;
				free    = r;
			}
		}
		else
		{
			while (!kDListIsEmpty(&first))
			{
				kResouceType *r = kDListPopFront(&first);
				memset(r, 0, sizeof(*r));
				r->next = free;
				free    = r;
			}
		}
	}

	void Destroy(kMemoryPoolResourceFreeProc<T> dtor = nullptr, void *data = nullptr)
	{
		if (dtor)
		{
			while (!kDListIsEmpty(&first))
			{
				kResouceType<T> *r = kDListPopFront(&first);
				dtor(&r->resource, data);
			}
		}

		kMemoryBlock<T> *prev  = nullptr;
		kMemoryBlock<T> *block = blocks;

		while (block)
		{
			prev  = block;
			block = prev->next;
			kFree(prev, sizeof(*prev));
		}

		*this = kMemoryPool<T>();
	}

	T *Alloc(void)
	{
		if (!free)
		{
			kMemoryBlock<T> *block = (kMemoryBlock<T> *)kAlloc(sizeof(kMemoryBlock<T>));

			if (!block) return nullptr;

			memset(block, 0, sizeof(*block));

			for (int i = 0; i < K_RESOUCE_PER_BLOCK - 1; ++i)
			{
				block->data[i].next = &block->data[i + 1];
			}

			free        = block->data;
			block->next = blocks;
			blocks      = block;
		}

		kResouceType<T> *r = free;
		free               = r->next;

		kDListPushBack(&first, r);

		return &r->resource;
	}

	void Free(T *resource)
	{
		kResouceType<T> *r = (kResouceType<T> *)resource;
		kDListRemove(r);
		memset(r, 0, sizeof(*r));
		r->next = free;
		free    = r;
	}
};
