#pragma once
#include "kContext.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")

static void kLogHresultError(DWORD error, const char *source, const char *fmt, ...)
{
	if (error)
	{
		LPWSTR message = 0;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		               NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&message, 0, NULL);
		char    buff[4096];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buff, sizeof(buff), fmt, args);
		va_end(args);
		kLogErrorEx(source, "%s. Reason: %S.", buff, message);
		LocalFree(message);
	}
	else
	{
		char    buff[4096];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buff, sizeof(buff), fmt, args);
		va_end(args);
		kLogErrorEx(source, "%s: Reason: Unknown.", buff);
	}
}
