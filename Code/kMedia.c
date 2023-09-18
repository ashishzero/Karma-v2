#pragma once

u64 kGetPerformanceFrequency(void) {
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

u64 kGetPerformanceCounter(void) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

void kTerminate(uint code) {
	ExitProcess(code);
}
