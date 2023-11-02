#pragma once
#include "kCommon.h"
#include "kRenderShared.h"

struct kResourceIndex
{
	u16 *GenID;
	u16 *FreeIndices;
	u32  FreeCount;
	int  Cap;
};

void        kDestroyResourceIndex(kResourceIndex *index);
bool        kCreateResourceIndex(kResourceIndex *index, int cap);
kResourceID kAllocResource(kResourceIndex *index);
void        kFreeResource(kResourceIndex *index, kResourceID id);
bool        kIsResourceValid(kResourceIndex *index, kResourceID id);
