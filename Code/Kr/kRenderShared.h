#pragma once
#include "kRenderApi.h"

typedef struct kGlyph
{
	kRect  rect;
	kVec2i bearing;
	kVec2i size;
	kVec2i advance;
} kGlyph;

typedef struct kFont
{
	kTexture texture;
	u16     *map;
	kGlyph  *glyphs;
	kGlyph  *fallback;
	u32      height;
	u32      mincp;
	u32      maxcp;
	u32      count;
} kFont;

//
//
//

typedef struct kRenderSpec
{
	float thickness;
	u32   vertices;
	u32   indices;
	u32   params;
	u32   commands;
	u32   passes;
	u32   rects;
	u32   transforms;
	u32   textures;
	u32   builder;
} kRenderSpec;

enum kRenderFeatureFlagBit
{
	kRenderFeature_MSAA = 0x1
};

typedef struct kRenderFeatureMSAA
{
	u32 samples;
} kRenderFeatureMSAA;

typedef struct kRenderFeatures
{
	uint               flags;
	kRenderFeatureMSAA msaa;
} kRenderFeatures;

//
//
//

static constexpr kRenderFeatures kDefaultRenderFeatures = {.flags = kRenderFeature_MSAA, .msaa = {.samples = 8}};

static constexpr kRenderSpec     kDefaultRenderSpec     = {.thickness  = 1,
                                                           .vertices   = 1048576,
                                                           .indices    = 1048576 * 6,
                                                           .params     = 1024,
                                                           .commands   = 512,
                                                           .passes     = 64,
                                                           .rects      = 256,
                                                           .transforms = 256,
                                                           .textures   = 256,
                                                           .builder    = 16384};

//
//
//
