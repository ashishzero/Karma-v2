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

typedef struct kRenderCommand2D
{
	u32            material;
	u32            transform;
	kBlendMode     blend;
	kTextureFilter filter;
	u16            rect;
	i32            vertex;
	u32            index;
	u32            count;
} kRenderCommand2D;

typedef struct kRenderPass2D
{
	kTexture *     rt;
	kTexture *     ds;
	kViewport      viewport;
	u32            flags;
	kRenderClear2D clear;
	kRange<u32>    commands;
} kRenderPass2D;

typedef struct kMaterial2D
{
	kTexture *textures[kTextureType_Count];
} kMaterial2D;

typedef struct kRenderFrame2D
{
	kSpan<kRenderPass2D>    passes;
	kSpan<kRenderCommand2D> commands;
	kSpan<kMaterial2D>      materials;
	kSpan<kMat4>            transforms;
	kSpan<kRect>            rects;
	kSpan<kVertex2D>        vertices;
	kSpan<kIndex2D>         indices;
} kRenderFrame2D;
