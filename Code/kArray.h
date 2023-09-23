#include "kCommon.h"

typedef bool(*kArrayFindFn)(void *, imem, void *);

imem  kPrivate_ArrayCount(void **parr);
imem  kPrivate_ArrayCapacity(void **parr);
void *kPrivate_ArrayGet(void **parr, imem index, imem elemsz);

void  kPrivate_ArrayPop(void **parr);
void  kPrivate_kArrayReset(void **parr);
void  kPrivate_kArrayRemove(void **parr, imem index, imem elemsz);
void  kPrivate_kArrayRemoveUnordered(void **parr, imem index, imem elemsz);
bool  kPrivate_ArrayReserve(void **parr, imem req_cap, imem elemsz);
bool  kPrivate_ArrayResize(void **parr, imem count, imem elemsz);
bool  kPrivate_ArrayResizeValue(void **parr, imem count, void *src, imem elemsz);
void *kPrivate_ArrayExtend(void **parr, imem count, imem elemsz);
void *kPrivate_ArrayAddEx(void **parr, imem elemsz);
void  kPrivate_ArrayAdd(void **parr, void *src, imem elemsz);
bool  kPrivate_ArrayCopyBuffer(void **dst_parr, void *src, imem count, imem elemsz);
bool  kPrivate_ArrayCopyArray(void **dst_parr, void **src_parr, imem elemsz);
bool  kPrivate_ArrayInsert(void **parr, imem index, void *src, imem elemsz);
void  kPrivate_ArrayPack(void **parr, imem elemsz);
void *kPrivate_ArrayClone(void **src_parr, imem elemsz);
void  kPrivate_ArrayFree(void **parr, imem elemsz);

imem  kPrivate_ArrayFindEx(void *arr, imem count, kArrayFindFn fn, void *arg, imem elemsz);
imem  kPrivate_ArrayFind(void *arr, imem count, void *src, imem elemsz);

//
//
//

#define kArrayCount(parr)                         kPrivate_ArrayCount(parr)
#define kArrayCapacity(parr)                      kPrivate_ArrayCapacity(parr)
#define kArrayGet(parr, index)                    kPrivate_ArrayGet(parr, index, sizeof(**(parr)))
#define kArrayFirst(parr)                         (*(parr))[0]
#define kArrayLast(parr)                          (*(parr))[kPrivate_ArrayCount(parr)-1]

#define kArrayPop(parr)                           kPrivate_ArrayPop(parr)
#define kArrayReset(parr)                         kPrivate_kArrayReset(parr)
#define kArrayRemove(parr, index)                 kPrivate_kArrayRemove(parr, index, sizeof(**(parr)))
#define kArrayRemoveUnordered(parr, index)        kPrivate_kArrayRemoveUnordered(parr, index, sizeof(**(parr)))
#define ArrayReserve(parr, req_cap)               kPrivate_ArrayReserve(parr, req_cap, sizeof(**(parr)))
#define kArrayResize(parr, count)                 kPrivate_ArrayResize(parr, count, sizeof(**(parr)))
#define kArrayResizeValue(parr, count, src)       kPrivate_ArrayResizeValue(parr, count, src, sizeof(**(parr)))
#define kArrayExtend(parr, count)                 kPrivate_ArrayExtend(parr, count, sizeof(**(parr)))
#define kArrayAddEx(parr)                         kPrivate_ArrayAddEx(parr, sizeof(**(parr)))
#define kArrayAdd(parr, src)                      kPrivate_ArrayAdd(parr, &(src), sizeof(**(parr)))
#define kArrayCopyBuffer(dst_parr, src, count)    kPrivate_ArrayCopyBuffer(dst_parr, src, count, sizeof(**(parr)))
#define kArrayCopyArray(dst_parr, src_parr)       kPrivate_ArrayCopyArray(dst_parr, src_parr, sizeof(**(parr)))
#define kArrayInsert(parr, index, src)            kPrivate_ArrayInsert(parr, index, src, sizeof(**(parr)))
#define kArrayPack(parr)                          kPrivate_ArrayPack(parr, sizeof(**(parr)))
#define kArrayClone(parr)                         kPrivate_ArrayClone(src_parr, sizeof(**(parr)))
#define kArrayFree(parr)                          kPrivate_ArrayFree(parr, sizeof(**(parr)))
#define krrayFindEx(parr, count, fn, arg)         kPrivate_ArrayFindEx(arr, count, fn, arg, sizeof(**(parr)))
#define krrayFind(parr, count, src)               kPrivate_ArrayFind(arr, count, src, sizeof(**(parr)))
