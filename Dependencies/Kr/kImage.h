#pragma once
#include "kCommon.h"

enum class kImageFileFormat
{
	PNG,
	BMP,
	TGA,
	JPG,
};

typedef void (*kImageDataWriterProc)(void *ctx, void *data, int);

u8  *kReadImage(kString buffer, int *w, int *h, int *channels, int req_channels);
bool kWriteImage(u8 *pixels, int w, int h, int channels, kImageFileFormat format, kImageDataWriterProc proc, void *context);
void kFreeImage(u8 *pixels, int w, int h, int channels);
