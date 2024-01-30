#pragma once
#include "kCommon.h"

constexpr int K_MAX_BONES = 256;

enum class kResourceState
{
	Error = -1,
	Unready,
	Ready,
};

struct kResourceID
{
	u16 Index;
	u16 GenID;
};

struct kTexture
{
	kResourceID ID;

	kTexture()
	{
		ID.Index = 0;
		ID.GenID = 0;
	}

	kTexture(kResourceID id) : ID(id)
	{}
};

struct kMesh
{
	kResourceID ID;

	kMesh()
	{
		ID.Index = 0;
		ID.GenID = 0;
	}

	kMesh(kResourceID id) : ID(id)
	{}
};

bool operator==(kTexture a, kTexture b);
bool operator!=(kTexture a, kTexture b);

enum class kFormat
{
	RGBA32_FLOAT,
	RGBA32_SINT,
	RGBA32_UINT,
	RGBA16_FLOAT,
	RGBA8_UNORM,
	RGBA8_UNORM_SRGB,
	RGB32_FLOAT,
	R11G11B10_FLOAT,
	RGB32_SINT,
	RGB32_UINT,
	RG32_FLOAT,
	RG32_SINT,
	RG32_UINT,
	RG8_UNORM,
	R32_FLOAT,
	R32_SINT,
	R32_UINT,
	R16_UINT,
	R8_UNORM,
	D32_FLOAT,
	D16_UNORM,
	D24_UNORM_S8_UINT,
	Count
};

typedef struct kVertex3D
{
	kVec3 Position;
	kVec3 Normal;
	kVec2 TexCoord;
	kVec4 Color;
} kVertex3D;

typedef struct kDynamicVertex3D
{
	kVec3 Position;
	kVec3 Normal;
	kVec2 TexCoord;
	kVec4 Color;
	float Weights[4];
	u32   Bones[4];
} kDynamicVertex3D;

enum class kMeshKind
{
	Static,
	Dynamic
};

struct kMeshSpec
{
	kMeshKind               Kind;
	kSpan<kDynamicVertex3D> DynamicVertices;
	kSpan<kVertex3D>        Vertices;
	kSpan<u32>              Indices;
	kString                 Name;
};

struct kTextureSpec
{
	kFormat Format;
	u32     Width;
	u32     Height;
	u32     Pitch;
	u8     *Pixels;
	kString Name;
};

enum class kRenderPass
{
	Default,
	HUD,
	Count
};

//
//
//

enum class kDepthTest : u8
{
	Disabled,
	LessEquals,
	Count
};

static const kString kDepthTestStrings[] = {"Disabled", "Enabled"};
static_assert(kArrayCount(kDepthTestStrings) == (int)kDepthTest::Count, "");

enum class kBlendMode : u8
{
	Opaque,
	Normal,
	Additive,
	Subtractive,
	Count,
};

static const kString kBlendModeStrings[] = {"Opaque", "Normal", "Additive", "Subtractive"};
static_assert(kArrayCount(kBlendModeStrings) == (int)kBlendMode::Count, "");

enum class kTextureFilter : u8
{
	LinearWrap,
	LinearClamp,
	PointWrap,
	PointClamp,
	Count
};

static const kString kTextureFilterStrings[] = {"LinearWrap", "LinearClamp", "PointWrap", "PointClamp"};
static_assert(kArrayCount(kTextureFilterStrings) == (int)kTextureFilter::Count, "");

enum class kMultiSamplingAntiAliasing
{
	Disabled = 1,
	x2       = 2,
	x4       = 4,
	x8       = 8,
};

enum class kBloom
{
	Disabled,
	Enabled,
};

enum class kHighDynamicRange
{
	Disabled,
	AES,
};

struct kRenderPipelineConfig
{
	kMultiSamplingAntiAliasing Msaa;
	kBloom                     Bloom;
	kHighDynamicRange          Hdr;
	kVec4                      Clear;
	float                      BloomFilterRadius;
	float                      BloomStrength;
	kVec3                      Intensity;
};
