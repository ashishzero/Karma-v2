#pragma once
#include "kMedia.h"

typedef void *(*kMediaBackendCreateWindow)(kWindowState *, const kWindowSpec &);
typedef void (*kMediaBackendDestroyWindow)(void);
typedef void (*kMediaBackendResizeWindow)(u32 w, u32 h);
typedef void (*kMediaBackendToggleWindowFullscreen)(void);
typedef void (*kMediaBackendReleaseCursor)(void);
typedef void (*kMediaBackendCaptureCursor)(void);
typedef int (*kMediaBackendGetWindowCaptionSize)(void);
typedef void (*kMediaBackendMaximizeWindow)(void);
typedef void (*kMediaBackendRestoreWindow)(void);
typedef void (*kMediaBackendMinimizeWindow)(void);
typedef void (*kMediaBackendCloseWindow)(void);

typedef void (*kMediaBackendLoadKeyboardState)(kKeyboardState *);
typedef void (*kMediaBackendLoadMouseState)(kMouseState *);

typedef int (*kMediaBackendEventLoop)(void);
typedef void (*kMediaBackendBreakLoop)(int);

typedef void (*kMediaBackendDestroy)(void);

typedef struct kMediaBackend
{
	kMediaBackendCreateWindow           CreateWindow;
	kMediaBackendDestroyWindow          DestroyWindow;
	kMediaBackendResizeWindow           ResizeWindow;
	kMediaBackendToggleWindowFullscreen ToggleWindowFullscreen;
	kMediaBackendReleaseCursor          ReleaseCursor;
	kMediaBackendCaptureCursor          CaptureCursor;
	kMediaBackendGetWindowCaptionSize   GetWindowCaptionSize;
	kMediaBackendMaximizeWindow         MaximizeWindow;
	kMediaBackendRestoreWindow          RestoreWindow;
	kMediaBackendMinimizeWindow         MinimizeWindow;
	kMediaBackendCloseWindow            CloseWindow;

	kMediaBackendLoadKeyboardState      LoadKeyboardState;
	kMediaBackendLoadMouseState         LoadMouseState;

	kMediaBackendEventLoop              EventLoop;
	kMediaBackendBreakLoop              BreakLoop;

	kMediaBackendDestroy                Destroy;
} kMediaBackend;

void kCreateMediaBackend(kMediaBackend *backend);
