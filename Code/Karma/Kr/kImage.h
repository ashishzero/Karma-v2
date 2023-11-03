#pragma once
#include "kCommon.h"

typedef struct kImage
{
	int Width;
	int Height;
	int Channels;
	u8 *Pixels;
} kImage;

typedef enum kImageFormat
{
	kImageFormat_PNG,
	kImageFormat_BMP,
	kImageFormat_TGA,
	kImageFormat_JPG,
} kImageFormat;

typedef void (*kImageDataWriterProc)(void *ctx, void *data, int);
