#pragma once
#include "kCommon.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")

static void kWinLogError(kLogger *logger, DWORD error, const char *source, const char *fmt, ...) {
	if (error) {
		LPWSTR message = 0;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&message, 0, NULL);
		char buff[4096];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buff, sizeof(buff), fmt, args);
		va_end(args);
		kLogErrorEx(logger, "%s: %s: %S\n", source, buff, message);
		LocalFree(message);
	} else {
		char buff[4096];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buff, sizeof(buff), fmt, args);
		va_end(args);
		kLogErrorEx(logger, "%s: %s: Unknown error\n", source, buff);
	}
}

static int kWinWideToUTF8(char *utf8_buff, int utf8_buff_len, const wchar_t *utf16_string) {
	int length = WideCharToMultiByte(CP_UTF8, 0, utf16_string, -1, utf8_buff, utf8_buff_len, 0, 0);
	return length;
}

static int kWinUTF8ToWide(wchar_t *utf16_buff, int utf16_buff_len, const char *utf8_string) {
	int length = MultiByteToWideChar(CP_UTF8, 0, utf8_string, -1, utf16_buff, utf16_buff_len);
	return length;
}
