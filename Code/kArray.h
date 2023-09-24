#include "kCommon.h"

typedef struct kArrayHeader {
	imem  count;
	imem  allocated;
	void *data;
} kArrayHeader;

typedef bool(*kArrayFindFn)(void *, imem, void *);

#define      kDefineArray(type) typedef union type##_Array {    \
									struct {                    \
										imem count;             \
										imem allocated;         \
										type *data;             \
									};                          \
									kArrayHeader header;        \
								} type##_Array

#define      kArray(type)       type##_Array 

void *       kPrivate_ArrayGet(kArrayHeader *arr, imem index, imem elemsz);
void         kPrivate_ArrayPop(kArrayHeader *arr);
void         kPrivate_kArrayReset(kArrayHeader *arr);
void         kPrivate_kArrayRemove(kArrayHeader *arr, imem index, imem elemsz);
void         kPrivate_kArrayRemoveUnordered(kArrayHeader *arr, imem index, imem elemsz);
bool         kPrivate_ArrayReserve(kArrayHeader *arr, imem req_cap, imem elemsz);
bool         kPrivate_ArrayResize(kArrayHeader *arr, imem count, imem elemsz);
bool         kPrivate_ArrayResizeValue(kArrayHeader *arr, imem count, void *src, imem elemsz);
void *       kPrivate_ArrayExtend(kArrayHeader *arr, imem count, imem elemsz);
void *       kPrivate_ArrayAddEx(kArrayHeader *arr, imem elemsz);
void         kPrivate_ArrayAdd(kArrayHeader *arr, void *src, imem elemsz);
bool         kPrivate_ArrayCopyBuffer(kArrayHeader *dst, void *src, imem count, imem elemsz);
bool         kPrivate_ArrayCopyArray(kArrayHeader *dst, kArrayHeader *src, imem elemsz);
bool         kPrivate_ArrayInsert(kArrayHeader *arr, imem index, void *src, imem elemsz);
void         kPrivate_ArrayPack(kArrayHeader *arr, imem elemsz);
kArrayHeader kPrivate_ArrayClone(kArrayHeader *src, imem elemsz);
void         kPrivate_ArrayFree(kArrayHeader *arr, imem elemsz);

imem         kPrivate_ArrayFindEx(void *arr, imem count, kArrayFindFn fn, void *arg, imem elemsz);
imem         kPrivate_ArrayFind(void *arr, imem count, void *src, imem elemsz);

//
//
//

#define kArrayGet(arr, index)                    kPrivate_ArrayGet(arr, index, sizeof(((arr)->data)[0]))
#define kArrayFirst(arr)                         ((arr).data)[0]
#define kArrayLast(arr)                          ((arr).data)[(arr).count - 1]

#define kArrayPop(arr)                           kPrivate_ArrayPop(&(arr)->header)
#define kArrayReset(arr)                         kPrivate_kArrayReset(&(arr)->header)
#define kArrayRemove(arr, index)                 kPrivate_kArrayRemove(&(arr)->header, index, sizeof(((arr)->data)[0]))
#define kArrayRemoveUnordered(arr, index)        kPrivate_kArrayRemoveUnordered(&(arr)->header, index, sizeof(((arr)->data)[0]))
#define ArrayReserve(arr, req_cap)               kPrivate_ArrayReserve(&(arr)->header, req_cap, sizeof(((arr)->data)[0]))
#define kArrayResize(arr, count)                 kPrivate_ArrayResize(&(arr)->header, count, sizeof(((arr)->data)[0]))
#define kArrayResizeValue(arr, count, src)       kPrivate_ArrayResizeValue(&(arr)->header, count, src, sizeof(((arr)->data)[0]))
#define kArrayExtend(arr, count)                 kPrivate_ArrayExtend(&(arr)->header, count, sizeof(((arr)->data)[0]))
#define kArrayAddEx(arr)                         kPrivate_ArrayAddEx(&(arr)->header, sizeof(((arr)->data)[0]))
#define kArrayAdd(arr, src)                      kPrivate_ArrayAdd(&(arr)->header, &(src), sizeof(((arr)->data)[0]))
#define kArrayCopyBuffer(dst, src, count)        kPrivate_ArrayCopyBuffer(&(dst)->header, src, count, sizeof(((arr)->data)[0]))
#define kArrayCopyArray(dst, src)                kPrivate_ArrayCopyArray(&(dst)->header, &(src)->header, sizeof(((arr)->data)[0]))
#define kArrayInsert(arr, index, src)            kPrivate_ArrayInsert(&(arr)->header, index, src, sizeof(((arr)->data)[0]))
#define kArrayPack(arr)                          kPrivate_ArrayPack(&(arr)->header, sizeof(((arr)->data)[0]))
#define kArrayClone(arr)                         kPrivate_ArrayClone(&(arr)->header, sizeof(((arr)->data)[0]))
#define kArrayFree(arr)                          kPrivate_ArrayFree(&(arr)->header, sizeof(((arr)->data)[0]))
#define krrayFindEx(arr, count, fn, arg)         kPrivate_ArrayFindEx(&(arr)->header, count, fn, arg, sizeof(((arr)->data)[0]))
#define krrayFind(arr, count, src)               kPrivate_ArrayFind(&(arr)->header, count, src, sizeof(((arr)->data)[0]))
