#include "kImage.h"
#include "kContext.h"

#define STBI_NO_HDR
#define STBI_NO_GIF
#define STBI_ASSERT(x)         kAssert(x)
#define STBI_MALLOC(sz)        kAlloc(sz)
#define STBI_REALLOC(p, newsz) kRealloc(p, 0, newsz)
#define STBI_FREE(p)           kFree(p, 0)

#define STBI_NO_STDIO
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBIW_ASSERT(x)         kAssert(x)
#define STBIW_MALLOC(sz)        kAlloc(sz)
#define STBIW_REALLOC(p, newsz) kRealloc(p, 0, newsz)
#define STBIW_FREE(p)           kFree(p, 0)

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool kReadImage(kString buffer, kImage *image, int req_channels)
{
	image->Pixels = stbi_load_from_memory(buffer.Items, (int)buffer.Count, &image->Width, &image->Height,
	                                      &image->Channels, req_channels);
	if (!image->Pixels)
	{
		const char *err = stbi_failure_reason();
		kLogError("Failed to read image: %s\n", err);
		return false;
	}
	return true;
}

bool kWriteImage(const kImage &image, kImageFormat format, kImageDataWriterProc proc, void *context)
{
	int res = 0;
	switch (format)
	{
		case kImageFormat_PNG:
			res = stbi_write_png_to_func(proc, context, image.Width, image.Height, image.Channels, image.Pixels,
			                             image.Width * image.Channels);
			break;
		case kImageFormat_BMP:
			res = stbi_write_bmp_to_func(proc, context, image.Width, image.Height, image.Channels, image.Pixels);
			break;
		case kImageFormat_TGA:
			res = stbi_write_tga_to_func(proc, context, image.Width, image.Height, image.Channels, image.Pixels);
			break;
		case kImageFormat_JPG:
			res = stbi_write_jpg_to_func(proc, context, image.Width, image.Height, image.Channels, image.Pixels, 0);
			break;
			kNoDefaultCase();
	}
	return res != 0;
}

void kFreeImage(const kImage &image)
{
	kFree(image.Pixels, image.Width * image.Height * image.Channels);
}
