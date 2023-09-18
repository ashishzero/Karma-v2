#include "kFileSystem.h"
#include "kContext.h"

#if K_PLATFORM_WINDOWS == 1

#include "kWindowsCommon.h"

kFile kOpenFile(const char *mb_filepath, kFileAccess paccess, kFileShareMode pshare, kFileMethod method) {
	wchar_t filepath[MAX_PATH];
	kWinUTF8ToWide(filepath, MAX_PATH, mb_filepath);

	DWORD access = 0;
	if (paccess == kFileAccess_Read)           access = GENERIC_READ;
	else if (paccess == kFileAccess_Write)     access = GENERIC_WRITE;
	else if (paccess == kFileAccess_ReadWrite) access = GENERIC_READ | GENERIC_WRITE;

	DWORD share_mode = 0;
	if (pshare == kFileShareMode_Read)           share_mode = FILE_SHARE_READ;
	else if (pshare == kFileShareMode_Write)     share_mode = FILE_SHARE_WRITE;
	else if (pshare == kFileShareMode_ReadWrite) share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	DWORD disposition = 0;
	if (method == kFileMethod_CreateAlways)      disposition = CREATE_ALWAYS;
	else if (method == kFileMethod_CreateNew)    disposition = CREATE_NEW;
	else if (method == kFileMethod_OpenAlways)   disposition = OPEN_ALWAYS;
	else if (method == kFileMethod_OpenExisting) disposition = OPEN_EXISTING;

	HANDLE file = CreateFileW(filepath, access, share_mode, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		kContext *ctx = kGetContext();
		kWinLogError(&ctx->logger, GetLastError(), "Failed to open file: \"%s\"", mb_filepath);
		return (kFile) { .ptr = 0 };
	}

	return (kFile){ .ptr = file };
}

void kCloseFile(kFile handle) {
	CloseHandle((HANDLE)handle.ptr);
}

umem kReadFile(kFile handle, u8 *buffer, umem size) {
	// This is done because ReadFile can with blocks of DWORD and not LARGE_INTEGER
	DWORD read_size = 0;
	if (size > UINT32_MAX)
		read_size = UINT32_MAX;
	else
		read_size = (DWORD)size;

	umem total_bytes_read = 0;
	umem remaining_bytes_to_read = size;

	while (remaining_bytes_to_read) {
		DWORD read_bytes = 0;
		if (!ReadFile((HANDLE)handle.ptr, buffer + total_bytes_read, read_size, &read_bytes, NULL)) {
			kContext *ctx = kGetContext();
			kWinLogError(&ctx->logger, GetLastError(), "Windows", "Failed while reading file");
			break;
		}

		remaining_bytes_to_read -= read_bytes;
		total_bytes_read += read_bytes;

		if (read_size > remaining_bytes_to_read)
			read_size = (DWORD)remaining_bytes_to_read;
	}

	return total_bytes_read;
}

umem kWriteFile(kFile handle, u8 *buff, umem size) {
	// This is done because WriteFile can with blocks of DWORD and not LARGE_INTEGER
	DWORD write_size = 0;
	if (size > UINT32_MAX)
		write_size = UINT32_MAX;
	else
		write_size = (DWORD)size;

	umem total_bytes_written = 0;
	umem remaining_bytes_to_write = size;

	while (remaining_bytes_to_write) {
		DWORD written = 0;
		if (!WriteFile((HANDLE)handle.ptr, buff + total_bytes_written, write_size, &written, NULL)) {
			kContext *ctx = kGetContext();
			kWinLogError(&ctx->logger, GetLastError(), "Windows", "Failed while writing file");
			break;
		}

		remaining_bytes_to_write -= written;
		total_bytes_written += written;

		if (write_size > remaining_bytes_to_write)
			write_size = (DWORD)remaining_bytes_to_write;
	}

	return total_bytes_written;
}

umem kGetFileSize(kFile handle) {
	LARGE_INTEGER size = { 0 };
	GetFileSizeEx((HANDLE)handle.ptr, &size);
	return size.QuadPart;
}

u8 *kReadEntireFile(const char *filepath, umem *file_size) {
	*file_size    = 0;
	kFile handle  = kOpenFile(filepath, kFileAccess_Read, kFileShareMode_Read, kFileMethod_OpenExisting);
	if (handle.ptr) {
		umem size = kGetFileSize(handle);
		u8 *buff  = (u8 *)kAlloc(size);
		if (buff)
			*file_size = kReadFile(handle, buff, size);
		kCloseFile(handle);
		return buff;
	}
	return 0;
}

bool kWriteEntireFile(const char *filepath, u8 *buffer, umem size) {
	kFile handle = kOpenFile(filepath, kFileAccess_Write, kFileShareMode_Read, kFileMethod_CreateAlways);
	if (handle.ptr) {
		umem written = kWriteFile(handle, buffer, size);
		kCloseFile(handle);
		return written == size;
	}
	return false;
}

uint kGetFileAttributes(const char *mb_filepath) {
	wchar_t filepath[MAX_PATH];
	kWinUTF8ToWide(filepath, MAX_PATH, mb_filepath);

	DWORD attrs = GetFileAttributesW(filepath);
	uint translated_attrs = 0;
	if (attrs != INVALID_FILE_ATTRIBUTES) {
		if (attrs & FILE_ATTRIBUTE_ARCHIVE)    translated_attrs |= kFileAttribute_Archive;
		if (attrs & FILE_ATTRIBUTE_COMPRESSED) translated_attrs |= kFileAttribute_Compressed;
		if (attrs & FILE_ATTRIBUTE_DIRECTORY)  translated_attrs |= kFileAttribute_Directory;
		if (attrs & FILE_ATTRIBUTE_ENCRYPTED)  translated_attrs |= kFileAttribute_Encrypted;
		if (attrs & FILE_ATTRIBUTE_HIDDEN)     translated_attrs |= kFileAttribute_Hidden;
		if (attrs & FILE_ATTRIBUTE_NORMAL)     translated_attrs |= kFileAttribute_Normal;
		if (attrs & FILE_ATTRIBUTE_OFFLINE)    translated_attrs |= kFileAttribute_Offline;
		if (attrs & FILE_ATTRIBUTE_READONLY)   translated_attrs |= kFileAttribute_ReadOnly;
		if (attrs & FILE_ATTRIBUTE_SYSTEM)     translated_attrs |= kFileAttribute_System;
		if (attrs & FILE_ATTRIBUTE_TEMPORARY)  translated_attrs |= kFileAttribute_Temporary;
	}

	return translated_attrs;
}

#endif
