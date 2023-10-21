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

typedef struct kViewport
{
	float x, y;
	float w, h;
	float n, f;
} kViewport;

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
	kTextureFilter_Linear,
	kTextureFilter_Point,
	kTextureFilter_Count
} kTextureFilter;

enum kRenderPassFlags
{
	kRenderPass_ClearColor   = 0x1,
	kRenderPass_ClearDepth   = 0x2,
	kRenderPass_ClearStencil = 0x4,
};

typedef struct kRenderClear2D
{
	kVec4 color;
	float depth;
	u8    stencil;
} kRenderClear2D;

enum kRenderDirtyFlags2D
{
	kRenderDirty_TextureColor   = 0x1,
	kRenderDirty_TextureMaskSDF = 0x2,
	kRenderDirty_Transform      = 0x4,
	kRenderDirty_Rect           = 0x8,
	kRenderDirty_Blend          = 0x10,
	kRenderDirty_TextureFilter  = 0x20,
	kRenderDirty_Everything     = kRenderDirty_TextureColor | kRenderDirty_TextureMaskSDF | kRenderDirty_Transform |
	                          kRenderDirty_Rect | kRenderDirty_Blend | kRenderDirty_TextureFilter
};

typedef struct kRenderCommand2D
{
	u32            flags;
	u32            textures[kTextureType_Count];
	u32            transform;
	kBlendMode     blend;
	kTextureFilter filter;
	u16            rect;
	u32            vertex;
	u32            index;
} kRenderCommand2D;

typedef struct kRenderPass2D
{
	kTexture      *rt;
	kTexture      *ds;
	kViewport      viewport;
	u32            flags;
	kRenderClear2D clear;
	kRange<u32>    commands;
} kRenderPass2D;

typedef struct kRenderFrame2D
{
	kSpan<kRenderPass2D>    passes;
	kSpan<kRenderCommand2D> commands;
	kSpan<kTexture *>       textures[kTextureType_Count];
	kSpan<kMat4>            transforms;
	kSpan<kRect>            rects;
	kSpan<kVertex2D>        vertices;
	kSpan<kIndex2D>         indices;
} kRenderFrame2D;
