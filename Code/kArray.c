#pragma once
#include "kArray.h"
#include "kContext.h"

#include <string.h>

typedef struct kArrayHeader {
	imem count;
	imem allocated;
	imem data[0];
} kArrayHeader;

static kArrayHeader *kPrivate_ArrayGetHeader(void **parr) {
	return *parr ? (kArrayHeader *)((u8 *)*parr - sizeof(kArrayHeader)) : 0;
}

static kArrayHeader *kPrivate_ArraySetHeader(void **parr, void *arr) {
	*parr = (void *)((u8 *)arr + sizeof(kArrayHeader));
	return (kArrayHeader *)arr;
}

static inline imem kPrivate_ArrayNextCapacity(imem allocated, imem count) {
	imem new_cap = allocated ? (allocated << 1) : 8;
	return new_cap > count ? new_cap : count;
}

imem kPrivate_ArrayCount(void **parr) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	return arr ? arr->count : 0;
}

imem kPrivate_ArrayCapacity(void **parr) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	return arr ? arr->allocated : 0;
}

void *kPrivate_ArrayGet(void **parr, imem index, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	if (arr && index < arr->count) {
		return (u8 *)arr->data + index * elemsz;
	}
	return 0;
}

void kPrivate_ArrayPop(void **parr) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	kAssert(arr->count > 0);
	arr->count -= 1;
}

void kPrivate_kArrayReset(void **parr) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	arr->count = 0;
}

void kPrivate_kArrayRemove(void **parr, imem index, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	kAssert(index < arr->count);
	memmove((u8 *)arr->data + index * elemsz, arr->data + (index + 1) * elemsz, (arr->count - index - 1) * elemsz);
	arr->count -= 1;
}

void kPrivate_kArrayRemoveUnordered(void **parr, imem index, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	kAssert(index < arr->count);
	memcpy((u8 *)arr->data + index * elemsz, arr->data + (arr->count - 1) * elemsz, elemsz);
	arr->count -= 1;
}

bool kPrivate_ArrayReserve(void **parr, imem req_cap, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	if (arr && req_cap <= arr->allocated)
		return true;

	void *mem = kRealloc(arr, arr ? arr->allocated * elemsz + sizeof(kArrayHeader) : 0, req_cap * elemsz + sizeof(kArrayHeader));
	if (mem) {
		arr = kPrivate_ArraySetHeader(parr, mem);
		arr->allocated = req_cap;
		return true;
	}

	return false;
}

bool kPrivate_ArrayResize(void **parr, imem count, imem elemsz) {
	if (kPrivate_ArrayReserve(parr, count, elemsz)) {
		kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
		arr->count = count;
		return true;
	}
	return false;
}

bool kPrivate_ArrayResizeValue(void **parr, imem count, void *src, imem elemsz) {
	if (kPrivate_ArrayReserve(parr, count, elemsz)) {
		kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
		for (imem idx = 0; idx < arr->count; ++idx) {
			memcpy((u8 *)arr->data + idx * elemsz, src, elemsz);
		}
		return true;
	}
	return false;
}

void *kPrivate_ArrayExtend(void **parr, imem count, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	imem req_count    = arr->count + count;
	imem cur_count    = arr->count;

	if (arr) {
		if (req_count < arr->allocated) {
			arr->count = req_count;
			return (u8 *)arr->data + cur_count * elemsz;
		}
	}

	imem new_cap = kPrivate_ArrayNextCapacity(arr ? arr->allocated : 0, req_count);

	if (kPrivate_ArrayReserve(parr, new_cap, elemsz)) {
		arr = kPrivate_ArrayGetHeader(parr);
		arr->count = req_count;
		return (u8 *)arr->data + cur_count * elemsz;
	}

	return 0;
}

void *kPrivate_ArrayAddEx(void **parr, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	if (!arr || arr->count == arr->allocated) {
		imem cap = kPrivate_ArrayNextCapacity(arr ? arr->allocated : 0, arr ? arr->allocated + 1 : 1);
		if (!kPrivate_ArrayReserve(parr, cap, elemsz))
			return 0;
	}
	arr = kPrivate_ArrayGetHeader(parr);
	return (u8 *)arr->data + (arr->count - 1) * elemsz;
}

void kPrivate_ArrayAdd(void **parr, void *src, imem elemsz) {
	void *dst = kPrivate_ArrayAddEx(parr, elemsz);
	if (dst) {
		memcpy(dst, src, elemsz);
	}
}

bool kPrivate_ArrayCopyBuffer(void **dst_parr, void *src, imem count, imem elemsz) {
	kArrayHeader *dst = kPrivate_ArrayGetHeader(dst_parr);

	if (!dst || dst->count + count >= dst->allocated) {
		imem allocated = dst ? dst->allocated : 0;
		imem new_cap   = kPrivate_ArrayNextCapacity(allocated, allocated + count + 1);
		if (!kPrivate_ArrayReserve(dst_parr, new_cap, elemsz))
			return false;
	}

	dst = kPrivate_ArrayGetHeader(dst_parr);
	memcpy((u8 *)dst->data + dst->count  * elemsz, src, count * elemsz);

	return true;
}

bool kPrivate_ArrayCopyArray(void **dst_parr, void **src_parr, imem elemsz) {
	kArrayHeader *src = kPrivate_ArrayGetHeader(src_parr);
	if (src && src->count) {
		return kPrivate_ArrayCopyBuffer(dst_parr, (u8 *)src->data, src->count, elemsz);
	}
	return true;
}

bool kPrivate_ArrayInsert(void **parr, imem index, void *src, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	kAssert(index < (arr ? arr->count : 0) + 1);

	if (kPrivate_ArrayAddEx(parr, elemsz)) {
		arr = kPrivate_ArrayGetHeader(parr);
		memmove((u8 *)arr->data + (index + 1) * elemsz, (u8 *)arr->data + index * elemsz, (arr->count - index) * elemsz);
		memcpy((u8 *)arr->data + index * elemsz, src, elemsz);
		return true;
	}

	return false;
}

void kPrivate_ArrayPack(void **parr, imem elemsz) {
	kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
	if (arr && arr->count != arr->allocated) {
		void *mem = kRealloc(arr, arr->allocated * elemsz + sizeof(kArrayHeader), arr->count * elemsz + sizeof(kArrayHeader));
		if (mem) {
			arr = kPrivate_ArraySetHeader(parr, mem);
			arr->allocated = arr->count;
		}
	}
}

void *kPrivate_ArrayClone(void **src_parr, imem elemsz) {
	kArrayHeader *src = kPrivate_ArrayGetHeader(src_parr);
	void *dst_parr = 0;
	if (kPrivate_ArrayResize(&dst_parr, src->count, elemsz)) {
		kArrayHeader *dst = kPrivate_ArrayGetHeader(&dst_parr);
		memcpy((u8 *)dst->data, (u8 *)src->data, src->count * elemsz);
	}
	return dst_parr;
}

void kPrivate_ArrayFree(void **parr, imem elemsz) {
	if (parr) {
		kArrayHeader *arr = kPrivate_ArrayGetHeader(parr);
		kFree(arr, arr->count * elemsz + sizeof(kArrayHeader));
		*parr = 0;
	}
}

imem kPrivate_ArrayFindEx(void *arr, imem count, kArrayFindFn fn, void *arg, imem elemsz) {
	for (imem index = 0; index < count; ++index) {
		if (fn((u8 *)arr + index * elemsz, elemsz, arg)) {
			return index;
		}
	}
	return -1;
}

imem kPrivate_ArrayFind(void *arr, imem count, void *src, imem elemsz) {
	for (imem index = 0; index < count; ++index) {
		if (memcmp((u8 *)arr + index * elemsz, src, elemsz) == 0)
			return index;
	}
	return -1;
}
