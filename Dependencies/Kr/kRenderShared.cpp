#include "kRenderShared.h"

bool operator==(kTexture a, kTexture b)
{
	return a.ID.Index == b.ID.Index;
}

bool operator!=(kTexture a, kTexture b)
{
	return a.ID.Index != b.ID.Index;
}
