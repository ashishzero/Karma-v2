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
	kVec3 Position;
	kVec2 TexCoord;
	kVec4 Color;
} kVertex2D;

typedef u32 kIndex2D;

typedef enum kDepthTest : u8
{
	kDepthTest_Disabled,
	kDepthTest_LessEquals,
	kDepthTest_Count
} kDepthTest;

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
	kTextureFilter_LinearClamp,
	kTextureFilter_PointWrap,
	kTextureFilter_PointClamp,
	kTextureFilter_Count
} kTextureFilter;

enum kMultiSamplingAntiAliasing
{
	kMultiSamplingAntiAliasing_Disabled = 1,
	kMultiSamplingAntiAliasing_2        = 2,
	kMultiSamplingAntiAliasing_4        = 4,
	kMultiSamplingAntiAliasing_8        = 8,
};

enum kBloom
{
	kBloom_Disabled,
	kBloom_Enabled,
};

enum kHighDynamicRange
{
	kHighDynamicRange_Disabled,
	kHighDynamicRange_AES,
};

struct kRenderPipelineConfig
{
	kMultiSamplingAntiAliasing Msaa;
	kBloom                     Bloom;
	kHighDynamicRange          Hdr;
	kVec4                      Clear;
	float                      Threshold;
	kVec3                      Intensity;
};

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
	u32            Flags;
	u32            Textures[kTextureType_Count];
	kBlendMode     BlendMode;
	kTextureFilter TextureFilter;
	u16            Rect;
	u32            VertexOffset;
	u32            IndexOffset;
} kRenderCommand2D;

typedef struct kCamera2D
{
	float Left;
	float Right;
	float Top;
	float Bottom;
	float Near;
	float Far;
} kCamera2D;

typedef struct kRenderScene2D
{
	kCamera2D   Camera;
	kRect       Region;
	kRange<u32> Commands;
} kRenderScene2D;

typedef struct kRenderFrame2D
{
	kSpan<kRenderScene2D>   Scenes;
	kSpan<kRenderCommand2D> Commands;
	kSpan<kTexture *>       Textures[kTextureType_Count];
	kSpan<kRect>            Rects;
	kSpan<kVertex2D>        Vertices;
	kSpan<kIndex2D>         Indices;
} kRenderFrame2D;

//
//
//

typedef struct kRenderFrame
{
	kRenderFrame2D Frame2D;
} kRenderFrame;
