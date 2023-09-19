#include "kMedia.h"
#include "kMath.h"

void Update(float dt) {
	uint count;
	kEvent *events = kGetEvents(&count);

	if (kKeyPressed(kKey_Escape)) {
		kBreakEventLoop(0);
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
}

void Main(int argc, const char **argv) {
	kMediaUser user  = {
		.proc.update = Update
	};
	kEventLoop(user, 0);
}
