#pragma once
#include "kCommon.h"

typedef struct kFile { void *ptr; } kFile;

typedef enum kFileAccess {
	kFileAccess_Read,
	kFileAccess_Write,
	kFileAccess_ReadWrite
} kFileAccess;

typedef enum kFileShareMode {
	kFileShareMode_Exclusive,
	kFileShareMode_Read,
	kFileShareMode_Write,
	kFileShareMode_ReadWrite
} kFileShareMode;

typedef enum kFileMethod {
	kFileMethod_CreateAlways,
	kFileMethod_CreateNew,
	kFileMethod_OpenAlways,
	kFileMethod_OpenExisting
} kFileMethod;

typedef enum kFileAttribute {
	kFileAttribute_Archive    = 0x1,
	kFileAttribute_Compressed = 0x2,
	kFileAttribute_Directory  = 0x4,
	kFileAttribute_Encrypted  = 0x8,
	kFileAttribute_Hidden     = 0x10,
	kFileAttribute_Normal     = 0x20,
	kFileAttribute_Offline    = 0x40,
	kFileAttribute_ReadOnly   = 0x80,
	kFileAttribute_System     = 0x100,
	kFileAttribute_Temporary  = 0x200,
} kFileAttribute;

kFile    kOpenFile(const char *mb_filepath, kFileAccess paccess, kFileShareMode pshare, kFileMethod method);
void     kCloseFile(kFile handle);
umem     kReadFile(kFile handle, u8 *buffer, umem size);
umem     kWriteFile(kFile handle, u8 *buff, umem size);
umem     kGetFileSize(kFile handle);
u8 *     kReadEntireFile(const char *filepath, umem *file_size);
bool     kWriteEntireFile(const char *filepath, u8 *buffer, umem size);
uint     kGetFileAttributes(const char *mb_filepath);
