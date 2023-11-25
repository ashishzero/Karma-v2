#pragma once
#include "kRenderResource.h"
#include "kContext.h"
#include <string.h>

void kDestroyResourceIndex(kResourceIndex *index)
{
	umem size = sizeof(u16) * index->Cap;
	if (index->GenID)
	{
		kFree(index->GenID, size);
	}
	if (index->FreeIndices)
	{
		kFree(index->FreeIndices, size);
	}
	memset(index, 0, sizeof(*index));
}

bool kCreateResourceIndex(kResourceIndex *index, int cap)
{
	index->Cap         = cap;
	umem size          = sizeof(u16) * index->Cap;

	index->GenID       = (u16 *)kAlloc(size);
	index->FreeIndices = (u16 *)kAlloc(size);

	if (!index->GenID || !index->FreeIndices)
	{
		kDestroyResourceIndex(index);
		return false;
	}

	index->FreeCount = cap - 1;
	for (int iter = 0; iter < cap - 1; ++iter)
	{
		index->FreeIndices[iter] = cap - 1 - iter;
	}
	index->FreeIndices[index->FreeCount] = 0;

	memset(index->GenID, 0, size);

	return true;
}

kResourceID kAllocResource(kResourceIndex *index)
{
	kResourceID id = {.Index = 0, .GenID = 0};

	if (index->FreeCount == 0)
		return id;

	index->FreeCount -= 1;

	int off = index->FreeIndices[index->FreeCount];
	index->GenID[off] += 1;

	id.Index = off;
	id.GenID = index->GenID[off];

	return id;
}

void kFreeResource(kResourceIndex *index, kResourceID id)
{
	kAssert(index->GenID[id.Index] == id.GenID);
	index->FreeIndices[index->FreeCount++] = id.Index;
}

bool kIsResourceValid(kResourceIndex *index, kResourceID id)
{
	return index->GenID[id.Index] == id.GenID;
}
