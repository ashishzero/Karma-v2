#pragma once
#include "kRenderShared.h"

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
	kRenderDirty_OutLineStyle   = 0x20,
	kRenderDirty_Everything     = kRenderDirty_TextureColor | kRenderDirty_TextureMaskSDF | kRenderDirty_Rect |
	                          kRenderDirty_Blend | kRenderDirty_TextureFilter | kRenderDirty_OutLineStyle
};

typedef struct kRenderCommand2D
{
	u32            Flags;
	kTexture       Textures[kTextureType_Count];
	kBlendMode     BlendMode;
	kTextureFilter TextureFilter;
	u16            Rect;
	u16            OutLineStyle;
	u32            VertexCount;
	u32            IndexCount;
} kRenderCommand2D;

struct kOrthographicViewData
{
	float Left;
	float Right;
	float Top;
	float Bottom;
	float Near;
	float Far;
};

struct kPerspectiveViewData
{
	float FieldOfView;
	float AspectRatio;
	float Near;
	float Far;
};

typedef enum kCameraViewType
{
	kCameraView_Orthographic,
	kCameraView_Perspective,
} kCameraLensType;

typedef struct kCameraView
{
	kCameraViewType Type;
	union
	{
		kOrthographicViewData Orthographic;
		kPerspectiveViewData  Perspective;
	};
} kCameraView;

typedef struct kViewport
{
	int x, y;
	int w, h;
} kViewport;

typedef struct kRenderScene
{
	kCameraView CameraView;
	kViewport   Viewport;
	kRangeT<u32> Commands;
} kRenderScene;

typedef struct kRenderFrame2D
{
	kSpan<kRenderScene>     Scenes[kRenderPass_Count];
	kSpan<kRenderCommand2D> Commands;
	kSpan<kRect>            Rects;
	kSpan<kVec4>            OutLineStyles;
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
