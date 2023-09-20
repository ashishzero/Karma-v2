#include "kMedia.h"

void Update(float dt) {
	uint count;
	kEvent *events = kGetEvents(&count);

	if (kKeyPressed(kKey_Escape)) {
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey_F11)) {
		kToggleWindowFullscreen();
	}

	if (kKeyPressed(kKey_H)) {
		if (kIsCursorEnabled()) {
			kDisableCursor();
		} else {
			kEnableCursor();
		}
	}

	if (kKeyPressed(kKey_M)) {
		if (kIsWindowMaximized()) {
			kRestoreWindow();
		} else {
			kMaximizeWindow();
		}
	}
}

void Main(int argc, const char **argv) {
	kMediaUserEvents user = {
		.update = Update
	};

	kEventLoop(&kDefaultSpec, user);
}
