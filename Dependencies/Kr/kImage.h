#pragma once
#include "kCommon.h"

typedef struct kImage
{
	int Width;
	int Height;
	int Channels;
	u8 *Pixels;
} kImage;

enum class kImageFileFormat
{
	PNG,
	BMP,
	TGA,
	JPG,
};

typedef void (*kImageDataWriterProc)(void *ctx, void *data, int);

kImage kAllocImage(int width, int height, int channels);
bool kReadImage(kString buffer, kImage *image, int req_channels);
bool kWriteImage(const kImage &image, kImageFileFormat format, kImageDataWriterProc proc, void *context);
void kFreeImage(const kImage &image);
