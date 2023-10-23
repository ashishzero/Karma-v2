#pragma once
#include "kCommon.h"

struct kTexture;

typedef enum kTextureType
{
	kTextureType_Color,
	kTextureType_MaskSDF,
	kTextureType_Count
} kTextureType;

typedef struct kVertex2D
{
	kVec3 pos;
	kVec2 tex;
	kVec4 col;
} kVertex2D;

typedef u32 kIndex2D;

typedef enum kBlendMode : u8
{
	kBlendMode_Opaque,
	kBlendMode_Normal,
	kBlendMode_Additive,
	kBlendMode_Subtractive,
	kBlendMode_Count,
} kBlendMode;

typedef enum kTextureFilter : u8
{
	kTextureFilter_LinearWrap,
	kTextureFilter_PointWrap,
	kTextureFilter_Count
} kTextureFilter;

typedef enum kRenderPass
{
	kRenderPass_Quad2D,
	kRenderPass_Threshold,
	kRenderPass_Bloom,
	kRenderPass_Tonemap,
	kRenderPass_Blit,
	kRenderPass_Count,
} kRenderPass;

//
//
//

enum kRenderDirtyFlags2D
{
	kRenderDirty_TextureColor   = 0x1,
	kRenderDirty_TextureMaskSDF = 0x2,
	kRenderDirty_Rect           = 0x4,
	kRenderDirty_Blend          = 0x8,
	kRenderDirty_TextureFilter  = 0x10,
	kRenderDirty_Everything     = kRenderDirty_TextureColor | kRenderDirty_TextureMaskSDF | kRenderDirty_Rect |
	                          kRenderDirty_Blend | kRenderDirty_TextureFilter
};

typedef struct kRenderCommand2D
{
	u32            flags;
	u32            textures[kTextureType_Count];
	kBlendMode     blend;
	kTextureFilter filter;
	u16            rect;
	u32            vertex;
	u32            index;
} kRenderCommand2D;

typedef struct kCamera2D
{
	float left;
	float right;
	float top;
	float bottom;
	float near;
	float far;
} kCamera2D;

typedef struct kRenderScene2D
{
	kCamera2D   camera;
	kRect       region;
	kRange<u32> commands;
} kRenderScene2D;

typedef struct kRenderFrame2D
{
	kSpan<kRenderScene2D>   scenes;
	kSpan<kRenderCommand2D> commands;
	kSpan<kTexture *>       textures[kTextureType_Count];
	kSpan<kRect>            rects;
	kSpan<kVertex2D>        vertices;
	kSpan<kIndex2D>         indices;
} kRenderFrame2D;

//
//
//

typedef struct kRenderFrame
{
	kRenderFrame2D render2d;
} kRenderFrame;
