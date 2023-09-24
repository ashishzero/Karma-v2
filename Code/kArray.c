#pragma once
#include "kArray.h"
#include "kContext.h"

#include <string.h>

static inline imem kPrivate_ArrayNextCapacity(imem allocated, imem count) {
	imem new_cap = allocated ? (allocated << 1) : 8;
	return new_cap > count ? new_cap : count;
}

void *kPrivate_ArrayGet(kArrayHeader *arr, imem index, imem elemsz) {
	if (index < arr->count) {
		return (u8 *)arr->data + index * elemsz;
	}
	return 0;
}

void kPrivate_ArrayPop(kArrayHeader *arr) {
	kAssert(arr->count > 0);
	arr->count -= 1;
}

void kPrivate_kArrayReset(kArrayHeader *arr) {
	arr->count = 0;
}

void kPrivate_kArrayRemove(kArrayHeader *arr, imem index, imem elemsz) {
	kAssert(index < arr->count);
	memmove((u8 *)arr->data + index * elemsz, (u8 *)arr->data + (index + 1) * elemsz, (arr->count - index - 1) * elemsz);
	arr->count -= 1;
}

void kPrivate_kArrayRemoveUnordered(kArrayHeader *arr, imem index, imem elemsz) {
	kAssert(index < arr->count);
	memcpy((u8 *)arr->data + index * elemsz, (u8 *)arr->data + (arr->count - 1) * elemsz, elemsz);
	arr->count -= 1;
}

bool kPrivate_ArrayReserve(kArrayHeader *arr, imem req_cap, imem elemsz) {
	if (req_cap <= arr->allocated)
		return true;

	void *mem = kRealloc(arr->data, arr->allocated * elemsz, req_cap * elemsz);
	if (mem) {
		arr->data      = mem;
		arr->allocated = req_cap;
		return true;
	}

	return false;
}

bool kPrivate_ArrayResize(kArrayHeader *arr, imem count, imem elemsz) {
	if (kPrivate_ArrayReserve(arr, count, elemsz)) {
		arr->count = count;
		return true;
	}
	return false;
}

bool kPrivate_ArrayResizeValue(kArrayHeader *arr, imem count, void *src, imem elemsz) {
	if (kPrivate_ArrayReserve(arr, count, elemsz)) {
		for (imem idx = 0; idx < arr->count; ++idx) {
			memcpy((u8 *)arr->data + idx * elemsz, src, elemsz);
		}
		return true;
	}
	return false;
}

void *kPrivate_ArrayExtend(kArrayHeader *arr, imem count, imem elemsz) {
	imem req_count = arr->count + count;
	imem cur_count = arr->count;

	if (req_count < arr->allocated) {
		arr->count = req_count;
		return (u8 *)arr->data + cur_count * elemsz;
	}

	imem new_cap = kPrivate_ArrayNextCapacity(arr->allocated, req_count);

	if (kPrivate_ArrayReserve(arr, new_cap, elemsz)) {
		arr->count = req_count;
		return (u8 *)arr->data + cur_count * elemsz;
	}

	return 0;
}

void *kPrivate_ArrayAddEx(kArrayHeader *arr, imem elemsz) {
	if (arr->count == arr->allocated) {
		imem cap = kPrivate_ArrayNextCapacity(arr->allocated, arr->allocated + 1);
		if (!kPrivate_ArrayReserve(arr, cap, elemsz))
			return 0;
	}
	return (u8 *)arr->data + (arr->count - 1) * elemsz;
}

void kPrivate_ArrayAdd(kArrayHeader *arr, void *src, imem elemsz) {
	void *dst = kPrivate_ArrayAddEx(arr, elemsz);
	if (dst) {
		memcpy(dst, src, elemsz);
	}
}

bool kPrivate_ArrayCopyBuffer(kArrayHeader *dst, void *src, imem count, imem elemsz) {
	if (dst->count + count >= dst->allocated) {
		imem new_cap   = kPrivate_ArrayNextCapacity(dst->allocated, dst->allocated + count + 1);
		if (!kPrivate_ArrayReserve(dst, new_cap, elemsz))
			return false;
	}

	memcpy((u8 *)dst->data + dst->count  * elemsz, src, count * elemsz);

	return true;
}

bool kPrivate_ArrayCopyArray(kArrayHeader *dst, kArrayHeader *src, imem elemsz) {
	if (src->count) {
		return kPrivate_ArrayCopyBuffer(dst, src->data, src->count, elemsz);
	}
	return true;
}

bool kPrivate_ArrayInsert(kArrayHeader *arr, imem index, void *src, imem elemsz) {
	kAssert(index < arr->count);

	if (kPrivate_ArrayAddEx(arr, elemsz)) {
		memmove((u8 *)arr->data + (index + 1) * elemsz, (u8 *)arr->data + index * elemsz, (arr->count - index) * elemsz);
		memcpy((u8 *)arr->data + index * elemsz, src, elemsz);
		return true;
	}

	return false;
}

void kPrivate_ArrayPack(kArrayHeader *arr, imem elemsz) {
	if (arr->count != arr->allocated) {
		void *mem = kRealloc(arr, arr->allocated * elemsz, arr->count * elemsz);
		if (mem) {
			arr->data      = mem;
			arr->allocated = arr->count;
		}
	}
}

kArrayHeader kPrivate_ArrayClone(kArrayHeader *src, imem elemsz) {
	kArrayHeader dst = { 0 };
	kPrivate_ArrayCopyBuffer(&dst, src->data, src->count, elemsz);
	return dst;
}

void kPrivate_ArrayFree(kArrayHeader *arr, imem elemsz) {
	if (arr) {
		kFree(arr, arr->count * elemsz + sizeof(kArrayHeader));
		*arr = (kArrayHeader){ 0 };
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
