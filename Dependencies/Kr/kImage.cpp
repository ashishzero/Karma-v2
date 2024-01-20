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
#include "Image/stb_image.h"

#define STBIW_ASSERT(x)         kAssert(x)
#define STBIW_MALLOC(sz)        kAlloc(sz)
#define STBIW_REALLOC(p, newsz) kRealloc(p, 0, newsz)
#define STBIW_FREE(p)           kFree(p, 0)

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Image/stb_image_write.h"

u8  *kReadImage(kString buffer, int *w, int *h, int *channels, int req_channels)
{
	u8 *pixels = stbi_load_from_memory(buffer.Items, (int)buffer.Count, w, h, channels, req_channels);
	if (!pixels)
	{
		const char *err = stbi_failure_reason();
		kLogError("Failed to read image: %s", err);
		return 0;
	}
	return pixels;
}

bool kWriteImage(u8 *pixels, int w, int h, int channels, kImageFileFormat format, kImageDataWriterProc proc,
                 void *context)
{
	int res = 0;
	switch (format)
	{
		case kImageFileFormat::PNG:
			res = stbi_write_png_to_func(proc, context, w, h, channels, pixels, w * channels);
			break;
		case kImageFileFormat::BMP:
			res = stbi_write_bmp_to_func(proc, context, w, h, channels, pixels);
			break;
		case kImageFileFormat::TGA:
			res = stbi_write_tga_to_func(proc, context, w, h, channels, pixels);
			break;
		case kImageFileFormat::JPG:
			res = stbi_write_jpg_to_func(proc, context, w, h, channels, pixels, 0);
			break;
			kNoDefaultCase();
	}
	return res != 0;
}

void kFreeImage(u8 *pixels, int w, int h, int channels)
{
	kFree(pixels, w * h * channels);
}
