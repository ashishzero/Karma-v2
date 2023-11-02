#include "kMath.h"
#include "kArray.h"
#include "kRender.h"

#include <string.h>

#define K_MAX_CIRCLE_SEGMENTS 512

//
//
//

typedef struct kRenderStack2D
{
	kArray<kTexture>       Textures[kTextureType_Count];
	kArray<kMat3>          Transforms;
	kArray<kRect>          Rects;
	kArray<kBlendMode>     BlendModes;
	kArray<kTextureFilter> TextureFilters;
	kArray<kVec4>          OutLineStyles;
	kArray<kVec2>          Paths;
	kArray<kVec2>          Normals;
	float                  Thickness;
} kRenderStack2D;

typedef struct kRenderContext2D
{
	kArray<kVertex2D>        Vertices;
	kArray<kIndex2D>         Indices;
	kArray<kVec4>            OutLineStyles;
	kArray<kRect>            Rects;
	kArray<kRenderScene2D>   Scenes;
	kArray<kRenderCommand2D> Commands;
	kRenderStack2D           Stack;
} kRenderContext2D;

typedef struct kRenderContextBuiltin
{
	kTexture Textures[kTextureType_Count];
	kFont *  Font;
} kRenderContextBuiltin;

struct kRenderMemoryStats
{
	kRenderMemory Used;
	kRenderMemory Caps;
};

kRenderContext2D          g_Render2D;
kRenderContextBuiltin     g_Builtin;
static kRenderMemoryStats g_Memory;

static float              Sines[K_MAX_CIRCLE_SEGMENTS];
static float              Cosines[K_MAX_CIRCLE_SEGMENTS];

//
//
//

static float kWrapTurns(float turns)
{
	while (turns < 0.0f)
		turns += 1.0f;
	while (turns > 1.0f)
		turns -= 1.0f;
	return turns;
}

static float kSinLookup(int index, int segments)
{
	float lookup = ((float)index / (float)segments) * (K_MAX_CIRCLE_SEGMENTS - 1);
	int   a      = (int)lookup;
	int   b      = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(Sines[a], Sines[b], lookup - a);
}

static float kCosLookup(int index, int segments)
{
	float lookup = ((float)index / (float)segments) * (K_MAX_CIRCLE_SEGMENTS - 1);
	int   a      = (int)lookup;
	int   b      = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(Cosines[a], Cosines[b], lookup - a);
}

static float kSinLookup(float turns)
{
	float lookup = turns * (K_MAX_CIRCLE_SEGMENTS - 1);
	int   a      = (int)lookup;
	int   b      = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(Sines[a], Sines[b], lookup - a);
}

static float kCosLookup(float turns)
{
	float lookup = turns * (K_MAX_CIRCLE_SEGMENTS - 1);
	int   a      = (int)lookup;
	int   b      = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(Cosines[a], Cosines[b], lookup - a);
}

//
//
//

static void kPushRenderCommand(u32 dirty_flags)
{
	kRenderCommand2D *cmd = g_Render2D.Commands.Add();
	cmd->Flags            = dirty_flags;
	for (int i = 0; i < kTextureType_Count; ++i)
		cmd->Textures[i] = g_Render2D.Stack.Textures[i].Last();
	cmd->OutLineStyle  = (u32)g_Render2D.OutLineStyles.Count - 1;
	cmd->Rect          = (u32)g_Render2D.Rects.Count - 1;
	cmd->BlendMode     = g_Render2D.Stack.BlendModes.Last();
	cmd->TextureFilter = g_Render2D.Stack.TextureFilters.Last();
	cmd->VertexCount   = 0;
	cmd->IndexCount    = 0;
}

static void kHandleBlendModeChange(void)
{
	kBlendMode        mode = g_Render2D.Stack.BlendModes.Last();
	kRenderCommand2D &cmd  = g_Render2D.Commands.Last();

	if (cmd.BlendMode == mode)
		return;

	if (cmd.IndexCount)
	{
		kPushRenderCommand(kRenderDirty_Blend);
		return;
	}

	cmd.BlendMode = mode;

	if (g_Render2D.Commands.Count <= 1)
		return;

	kRenderCommand2D &prev = g_Render2D.Commands[g_Render2D.Commands.Count - 2];
	if (prev.BlendMode != mode)
	{
		cmd.Flags |= kRenderDirty_Blend;
		return;
	}

	cmd.Flags &= ~kRenderDirty_Blend;
	if (!cmd.Flags)
	{
		g_Render2D.Commands.Pop();
	}
}

static void kHandleTextureFilterChange(void)
{
	kTextureFilter    filter = g_Render2D.Stack.TextureFilters.Last();
	kRenderCommand2D &cmd    = g_Render2D.Commands.Last();

	if (cmd.TextureFilter == filter)
		return;

	if (cmd.IndexCount)
	{
		kPushRenderCommand(kRenderDirty_TextureFilter);
		return;
	}

	cmd.TextureFilter = filter;

	if (g_Render2D.Commands.Count <= 1)
		return;

	kRenderCommand2D &prev = g_Render2D.Commands[g_Render2D.Commands.Count - 2];
	if (prev.TextureFilter != filter)
	{
		cmd.Flags |= kRenderDirty_TextureFilter;
		return;
	}

	cmd.Flags &= ~kRenderDirty_TextureFilter;
	if (!cmd.Flags)
	{
		g_Render2D.Commands.Pop();
	}
}

static void kHandleRectChange(void)
{
	kRect &           rect = g_Render2D.Stack.Rects.Last();
	kRenderCommand2D &cmd  = g_Render2D.Commands.Last();

	if (memcmp(&g_Render2D.Rects[cmd.Rect], &rect, sizeof(kRect)) == 0)
		return;

	if (cmd.IndexCount)
	{
		g_Render2D.Rects.Add(rect);
		kPushRenderCommand(kRenderDirty_Rect);
		return;
	}

	if (g_Render2D.Commands.Count <= 1)
	{
		g_Render2D.Rects[0] = rect;
		return;
	}

	if (cmd.Flags & kRenderDirty_Rect)
	{
		g_Render2D.Rects.Pop();
	}

	kRenderCommand2D &prev = g_Render2D.Commands[g_Render2D.Commands.Count - 2];
	if (memcmp(&g_Render2D.Rects[prev.Rect], &rect, sizeof(kRect)) != 0)
	{
		g_Render2D.Rects.Add(rect);
		cmd.Flags |= kRenderDirty_Rect;
		cmd.Rect = (u32)g_Render2D.Rects.Count - 1;
		return;
	}

	cmd.Flags &= ~kRenderDirty_Rect;
	cmd.Rect = (u32)g_Render2D.Rects.Count - 1;
	if (!cmd.Flags)
	{
		g_Render2D.Commands.Pop();
	}
}

static void kHandleOutLineStyleChange(void)
{
	kVec4 &           style = g_Render2D.Stack.OutLineStyles.Last();
	kRenderCommand2D &cmd   = g_Render2D.Commands.Last();

	if (memcmp(&g_Render2D.OutLineStyles[cmd.OutLineStyle], &style, sizeof(kVec4)) == 0)
		return;

	if (cmd.IndexCount)
	{
		g_Render2D.OutLineStyles.Add(style);
		kPushRenderCommand(kRenderDirty_OutLineStyle);
		return;
	}

	if (g_Render2D.Commands.Count <= 1)
	{
		g_Render2D.OutLineStyles[0] = style;
		return;
	}

	if (cmd.Flags & kRenderDirty_OutLineStyle)
	{
		g_Render2D.OutLineStyles.Pop();
	}

	kRenderCommand2D &prev = g_Render2D.Commands[g_Render2D.Commands.Count - 2];
	if (memcmp(&g_Render2D.OutLineStyles[prev.OutLineStyle], &style, sizeof(kVec4)) != 0)
	{
		g_Render2D.OutLineStyles.Add(style);
		cmd.Flags |= kRenderDirty_OutLineStyle;
		cmd.OutLineStyle = (u32)g_Render2D.OutLineStyles.Count - 1;
		return;
	}

	cmd.Flags &= ~kRenderDirty_OutLineStyle;
	cmd.OutLineStyle = (u32)g_Render2D.OutLineStyles.Count - 1;
	if (!cmd.Flags)
	{
		g_Render2D.Commands.Pop();
	}
}

static void kHandleTextureChange(kTextureType type)
{
	kTexture          texture = g_Render2D.Stack.Textures[type].Last();
	kRenderCommand2D &cmd     = g_Render2D.Commands.Last();
	u32               bit     = type == kTextureType_Color ? kRenderDirty_TextureColor : kRenderDirty_TextureMaskSDF;

	if (cmd.Textures[type] == texture)
		return;

	if (cmd.IndexCount)
	{
		kPushRenderCommand(bit);
		return;
	}

	cmd.Textures[type] = texture;

	if (g_Render2D.Commands.Count <= 1)
		return;

	kRenderCommand2D &prev = g_Render2D.Commands[g_Render2D.Commands.Count - 2];
	if (prev.Textures[type] != texture)
	{
		cmd.Flags |= bit;
		return;
	}

	cmd.Flags &= ~bit;
	if (!cmd.Flags)
	{
		g_Render2D.Commands.Pop();
	}
}

//
//
//

void kBeginScene(const kCamera2D &camera, kRect region)
{
	kRenderScene2D *scene = g_Render2D.Scenes.Add();
	scene->Camera         = camera;
	scene->Region         = region;
	scene->Commands       = kRange<u32>((u32)g_Render2D.Commands.Count);

	g_Render2D.Rects.Add(g_Render2D.Stack.Rects.Last());
	g_Render2D.OutLineStyles.Add(g_Render2D.Stack.OutLineStyles.Last());

	kPushRenderCommand(kRenderDirty_Everything);
	kPushRect(region);
}

void kBeginScene(float left, float right, float top, float bottom, float near, float far, kRect region)
{
	kCamera2D camera;
	camera.Left   = left;
	camera.Right  = right;
	camera.Top    = top;
	camera.Bottom = bottom;
	camera.Near   = near;
	camera.Far    = far;
	kBeginScene(camera, region);
}

void kBeginScene(float ar, float height, kRect region)
{
	float halfx = ar * 0.5f * kAbsolute(height);
	float halfy = 0.5f * height;
	kBeginScene(-halfx, halfx, -halfy, halfy, -1.0f, 1.0f, region);
}

void kEndScene(void)
{
	kPopRect();

	kRenderCommand2D &cmd = g_Render2D.Commands.Last();
	if (!cmd.IndexCount)
	{
		u32 flags = cmd.Flags;
		if (flags & kRenderDirty_Rect)
			g_Render2D.Rects.Pop();
		if (flags & kRenderDirty_OutLineStyle)
			g_Render2D.OutLineStyles.Pop();
		g_Render2D.Commands.Pop();
	}

	kRenderScene2D *pass = &g_Render2D.Scenes.Last();
	pass->Commands.End   = (u32)g_Render2D.Commands.Count;

	if (!pass->Commands.Length())
	{
		g_Render2D.Scenes.Pop();
	}
}

void kLineThickness(float thickness)
{
	g_Render2D.Stack.Thickness = thickness;
}

void kSetBlendMode(kBlendMode mode)
{
	g_Render2D.Stack.BlendModes.Last() = mode;
	kHandleBlendModeChange();
}

void kBeginBlendMode(kBlendMode mode)
{
	kBlendMode last = g_Render2D.Stack.BlendModes.Last();
	g_Render2D.Stack.BlendModes.Add(last);
	kSetBlendMode(mode);
}

void kEndBlendMode(void)
{
	imem count = g_Render2D.Stack.BlendModes.Count;
	kSetBlendMode(g_Render2D.Stack.BlendModes[count - 2]);
	g_Render2D.Stack.BlendModes.Pop();
}

void kSetTextureFilter(kTextureFilter filter)
{
	g_Render2D.Stack.TextureFilters.Last() = filter;
	kHandleTextureFilterChange();
}

void kSetTexture(kTexture texture, kTextureType idx)
{
	g_Render2D.Stack.Textures[idx].Last() = texture;
	kHandleTextureChange(idx);
}

void kPushTexture(kTexture texture, kTextureType idx)
{
	kTexture last = g_Render2D.Stack.Textures[idx].Last();
	g_Render2D.Stack.Textures[idx].Add(last);
	kSetTexture(texture, idx);
}

void kPopTexture(kTextureType idx)
{
	imem count = g_Render2D.Stack.Textures[idx].Count;
	kSetTexture(g_Render2D.Stack.Textures[idx][count - 2], idx);
	g_Render2D.Stack.Textures[idx].Pop();
}

void kSetOutLineStyle(kVec3 color, float width)
{
	g_Render2D.Stack.OutLineStyles.Last() = kVec4(color, width);
	kHandleOutLineStyleChange();
}

void kPushOutLineStyle(kVec3 color, float width)
{
	kVec4 last = g_Render2D.Stack.OutLineStyles.Last();
	g_Render2D.Stack.OutLineStyles.Add(last);
	kSetOutLineStyle(color, width);
}

void kPopOutLineStyle(void)
{
	imem  count = g_Render2D.Stack.OutLineStyles.Count;
	kVec4 style = g_Render2D.Stack.OutLineStyles[count - 2];
	kSetOutLineStyle(style.xyz, style.w);
	g_Render2D.Stack.OutLineStyles.Pop();
}

void kSetRect(kRect rect)
{
	g_Render2D.Stack.Rects.Last() = rect;
	kHandleRectChange();
}

void kSetRectEx(float x, float y, float w, float h)
{
	kRect rect;
	rect.min.x = x;
	rect.min.y = y;
	rect.max.x = x + w;
	rect.max.y = y + h;
	kSetRect(rect);
}

void kPushRect(kRect rect)
{
	kRect last = g_Render2D.Stack.Rects.Last();
	g_Render2D.Stack.Rects.Add(last);
	kSetRect(rect);
}

void kPushRectEx(float x, float y, float w, float h)
{
	kRect rect;
	rect.min.x = x;
	rect.min.y = y;
	rect.max.x = x + w;
	rect.max.y = y + h;
	kPushRect(rect);
}

void kPopRect(void)
{
	imem count = g_Render2D.Stack.Rects.Count;
	kSetRect(g_Render2D.Stack.Rects[count - 2]);
	g_Render2D.Stack.Rects.Pop();
}

void kSetTransform(const kMat3 &transform)
{
	g_Render2D.Stack.Transforms.Last() = transform;
}

void kPushTransform(const kMat3 &transform)
{
	kMat3 last = g_Render2D.Stack.Transforms.Last();
	g_Render2D.Stack.Transforms.Add(last);
	kMat3 t = last * transform;
	kSetTransform(t);
}

void kPushTransform(kVec2 pos, float angle)
{
	kVec2 arm = kArm(angle);
	kMat3 mat;
	mat.rows[0] = kVec3(arm.x, -arm.y, pos.x);
	mat.rows[1] = kVec3(arm.y, arm.x, pos.y);
	mat.rows[2] = kVec3(0.0f, 1.0f, 0.0f);
	kPushTransform(mat);
}

void kPopTransform(void)
{
	imem count = g_Render2D.Stack.Transforms.Count;
	kSetTransform(g_Render2D.Stack.Transforms[count - 2]);
	g_Render2D.Stack.Transforms.Pop();
}

//
//
//

static void kCommitPrimitive(u32 vertex, u32 index)
{
	kRenderCommand2D &cmd = g_Render2D.Commands.Last();
	cmd.VertexCount += vertex;
	cmd.IndexCount += index;
}

//
//
//

void kDrawTriangle(kVec3 va, kVec3 vb, kVec3 vc, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 ca, kVec4 cb, kVec4 cc)
{
	kVertex2D *vtx  = g_Render2D.Vertices.Extend(3);
	vtx[0].Position = va;
	vtx[0].TexCoord = ta;
	vtx[0].Color    = ca;
	vtx[1].Position = vb;
	vtx[1].TexCoord = tb;
	vtx[1].Color    = cb;
	vtx[2].Position = vc;
	vtx[2].TexCoord = tc;
	vtx[2].Color    = cc;

	if (g_Render2D.Stack.Transforms.Count > 1)
	{
		kMat3 &transform = g_Render2D.Stack.Transforms.Last();
		vtx[0].Position  = transform * vtx[0].Position;
		vtx[1].Position  = transform * vtx[1].Position;
		vtx[2].Position  = transform * vtx[2].Position;
	}

	u32       next = g_Render2D.Commands.Last().VertexCount;

	kIndex2D *idx  = g_Render2D.Indices.Extend(3);
	idx[0]         = next + 0;
	idx[1]         = next + 1;
	idx[2]         = next + 2;

	kCommitPrimitive(3, 3);
}

void kDrawTriangle(kVec3 a, kVec3 b, kVec3 c, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 col)
{
	kDrawTriangle(a, b, c, ta, tb, tc, col, col, col);
}

void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 col)
{
	kDrawTriangle(kVec3(a, 0), kVec3(b, 0), kVec3(c, 0), ta, tb, tc, col, col, col);
}

void kDrawTriangle(kVec3 a, kVec3 b, kVec3 c, kVec4 color)
{
	kDrawTriangle(a, b, c, kVec2(0), kVec2(0), kVec2(0), color, color, color);
}

void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec4 color)
{
	kDrawTriangle(kVec3(a, 0), kVec3(b, 0), kVec3(c, 0), kVec2(0), kVec2(0), kVec2(0), color, color, color);
}

void kDrawQuad(kVec3 va, kVec3 vb, kVec3 vc, kVec3 vd, kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color)
{
	kVertex2D *vtx  = g_Render2D.Vertices.Extend(4);
	vtx[0].Position = va;
	vtx[0].TexCoord = ta;
	vtx[0].Color    = color;
	vtx[1].Position = vb;
	vtx[1].TexCoord = tb;
	vtx[1].Color    = color;
	vtx[2].Position = vc;
	vtx[2].TexCoord = tc;
	vtx[2].Color    = color;
	vtx[3].Position = vd;
	vtx[3].TexCoord = td;
	vtx[3].Color    = color;

	if (g_Render2D.Stack.Transforms.Count > 1)
	{
		kMat3 &transform = g_Render2D.Stack.Transforms.Last();
		vtx[0].Position  = transform * vtx[0].Position;
		vtx[1].Position  = transform * vtx[1].Position;
		vtx[2].Position  = transform * vtx[2].Position;
		vtx[3].Position  = transform * vtx[3].Position;
	}

	u32       next = g_Render2D.Commands.Last().VertexCount;

	kIndex2D *idx  = g_Render2D.Indices.Extend(6);
	idx[0]         = next + 0;
	idx[1]         = next + 1;
	idx[2]         = next + 2;
	idx[3]         = next + 0;
	idx[4]         = next + 2;
	idx[5]         = next + 3;

	kCommitPrimitive(4, 6);
}

void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kDrawQuad(kVec3(a, 0), kVec3(b, 0), kVec3(c, 0), kVec3(d, 0), uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawQuad(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec4 color)
{
	kDrawQuad(a, b, c, d, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color)
{
	kDrawQuad(kVec3(a, 0), kVec3(b, 0), kVec3(c, 0), kVec3(d, 0), color);
}

void kDrawQuad(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kRect rect, kVec4 color)
{
	kVec2 uv_a = rect.min;
	kVec2 uv_b = kVec2(rect.min.x, rect.max.y);
	kVec2 uv_c = rect.max;
	kVec2 uv_d = kVec2(rect.max.x, rect.min.y);
	kDrawQuad(a, b, c, d, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kRect rect, kVec4 color)
{
	kDrawQuad(kVec3(a, 0), kVec3(b, 0), kVec3(c, 0), kVec3(d, 0), rect, color);
}

void kDrawRect(kVec3 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kVec3 a = pos;
	kVec3 b = kVec3(pos.x, pos.y + dim.y, pos.z);
	kVec3 c = kVec3(pos.xy + dim, pos.z);
	kVec3 d = kVec3(pos.x + dim.x, pos.y, pos.z);
	kDrawQuad(a, b, c, d, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRect(kVec2 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kDrawRect(kVec3(pos, 0), dim, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRect(kVec3 pos, kVec2 dim, kVec4 color)
{
	kDrawRect(pos, dim, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawRect(kVec2 pos, kVec2 dim, kVec4 color)
{
	kDrawRect(kVec3(pos, 0), dim, color);
}

void kDrawRect(kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kVec2 uv_a = rect.min;
	kVec2 uv_b = kVec2(rect.min.x, rect.max.y);
	kVec2 uv_c = rect.max;
	kVec2 uv_d = kVec2(rect.max.x, rect.min.y);
	kDrawRect(pos, dim, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRect(kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kDrawRect(kVec3(pos, 0), dim, rect, color);
}

void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kVec2 center = 0.5f * (2.0f * pos.xy + dim);

	kVec2 a      = pos.xy;
	kVec2 b      = kVec2(pos.x, pos.y + dim.y);
	kVec2 c      = pos.xy + dim;
	kVec2 d      = kVec2(pos.x + dim.x, pos.y);

	auto  t0     = a - center;
	auto  t1     = b - center;
	auto  t2     = c - center;
	auto  t3     = d - center;

	float cv     = kCos(angle);
	float sv     = kSin(angle);

	a.x          = t0.x * cv - t0.y * sv;
	a.y          = t0.x * sv + t0.y * cv;
	b.x          = t1.x * cv - t1.y * sv;
	b.y          = t1.x * sv + t1.y * cv;
	c.x          = t2.x * cv - t2.y * sv;
	c.y          = t2.x * sv + t2.y * cv;
	d.x          = t3.x * cv - t3.y * sv;
	d.y          = t3.x * sv + t3.y * cv;

	a += center;
	b += center;
	c += center;
	d += center;

	kDrawQuad(kVec3(a, pos.z), kVec3(b, pos.z), kVec3(c, pos.z), kVec3(d, pos.z), uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kDrawRectRotated(kVec3(pos, 0), dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kVec4 color)
{
	kDrawRectRotated(pos, dim, angle, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color)
{
	kDrawRectRotated(kVec3(pos, 0), dim, angle, color);
}

void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kRect rect, kVec4 color)
{
	kVec2 uv_a = rect.min;
	kVec2 uv_b = kVec2(rect.min.x, rect.max.y);
	kVec2 uv_c = rect.max;
	kVec2 uv_d = kVec2(rect.max.x, rect.min.y);
	kDrawRectRotated(pos, dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kRect rect, kVec4 color)
{
	kDrawRectRotated(kVec3(pos, 0), dim, angle, rect, color);
}

void kDrawRectCentered(kVec3 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kVec2 half_dim = 0.5f * dim;

	kVec3 a, b, c, d;
	a.xy = pos.xy - half_dim;
	b.xy = kVec2(pos.x - half_dim.x, pos.y + half_dim.y);
	c.xy = pos.xy + half_dim;
	d.xy = kVec2(pos.x + half_dim.x, pos.y - half_dim.y);

	a.z  = pos.z;
	b.z  = pos.z;
	c.z  = pos.z;
	d.z  = pos.z;

	kDrawQuad(a, b, c, d, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color)
{
	kDrawRectCentered(kVec3(pos, 0), dim, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectCentered(kVec3 pos, kVec2 dim, kVec4 color)
{
	kDrawRectCentered(pos, dim, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec4 color)
{
	kDrawRectCentered(kVec3(pos, 0), dim, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawRectCentered(kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kVec2 uv_a = rect.min;
	kVec2 uv_b = kVec2(rect.min.x, rect.max.y);
	kVec2 uv_c = rect.max;
	kVec2 uv_d = kVec2(rect.max.x, rect.min.y);
	kDrawRectCentered(pos, dim, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectCentered(kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kDrawRectCentered(kVec3(pos, 0), dim, rect, color);
}

void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
                              kVec4 color)
{
	kVec2 center   = pos.xy;

	kVec2 half_dim = 0.5f * dim;
	kVec2 a, b, c, d;
	a        = pos.xy - half_dim;
	b        = kVec2(pos.x - half_dim.x, pos.y + half_dim.y);
	c        = pos.xy + half_dim;
	d        = kVec2(pos.x + half_dim.x, pos.y - half_dim.y);

	auto  t0 = a - center;
	auto  t1 = b - center;
	auto  t2 = c - center;
	auto  t3 = d - center;

	float cv = kCos(angle);
	float sv = kSin(angle);

	a.x      = t0.x * cv - t0.y * sv;
	a.y      = t0.x * sv + t0.y * cv;
	b.x      = t1.x * cv - t1.y * sv;
	b.y      = t1.x * sv + t1.y * cv;
	c.x      = t2.x * cv - t2.y * sv;
	c.y      = t2.x * sv + t2.y * cv;
	d.x      = t3.x * cv - t3.y * sv;
	d.y      = t3.x * sv + t3.y * cv;

	a += center;
	b += center;
	c += center;
	d += center;

	kDrawQuad(kVec3(a, pos.z), kVec3(b, pos.z), kVec3(c, pos.z), kVec3(d, pos.z), uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
                              kVec4 color)
{
	kDrawRectCenteredRotated(kVec3(pos, 0), dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kVec4 color)
{
	kDrawRectCenteredRotated(pos, dim, angle, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color)
{
	kDrawRectCenteredRotated(kVec3(pos, 0), dim, angle, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kRect rect, kVec4 color)
{
	kVec2 uv_a = rect.min;
	kVec2 uv_b = kVec2(rect.min.x, rect.max.y);
	kVec2 uv_c = rect.max;
	kVec2 uv_d = kVec2(rect.max.x, rect.min.y);
	kDrawRectCenteredRotated(pos, dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kRect rect, kVec4 color)
{
	kDrawRectCenteredRotated(kVec3(pos, 0), dim, angle, rect, color);
}

void kDrawEllipse(kVec3 pos, float radius_a, float radius_b, kVec4 color)
{
	int   segments = (int)kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));

	float px       = Cosines[0] * radius_a;
	float py       = Sines[0] * radius_b;

	float npx, npy;
	for (int index = 1; index <= segments; ++index)
	{
		npx = kCosLookup(index, segments) * radius_a;
		npy = kSinLookup(index, segments) * radius_b;

		kDrawTriangle(pos, pos + kVec3(npx, npy, 0), pos + kVec3(px, py, 0), color);

		px = npx;
		py = npy;
	}
}

void kDrawEllipse(kVec2 pos, float radius_a, float radius_b, kVec4 color)
{
	kDrawEllipse(kVec3(pos, 0), radius_a, radius_b, color);
}

void kDrawCircle(kVec3 pos, float radius, kVec4 color)
{
	kDrawEllipse(pos, radius, radius, color);
}

void kDrawCircle(kVec2 pos, float radius, kVec4 color)
{
	kDrawEllipse(kVec3(pos, 0), radius, radius, color);
}

void kDrawPie(kVec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color)
{
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a)
	{
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments =
		kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float px = kCosLookup(theta_a) * radius_a;
	float py = kSinLookup(theta_a) * radius_b;

	float npx, npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt)
	{
		npx = kCosLookup(theta_a) * radius_a;
		npy = kSinLookup(theta_a) * radius_b;

		kDrawTriangle(pos, pos + kVec3(npx, npy, 0), pos + kVec3(px, py, 0), color);

		px = npx;
		py = npy;
	}
}

void kDrawPie(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color)
{
	kDrawPie(kVec3(pos, 0), radius_a, radius_b, theta_a, theta_b, color);
}

void kDrawPie(kVec3 pos, float radius, float theta_a, float theta_b, kVec4 color)
{
	kDrawPie(pos, radius, radius, theta_a, theta_b, color);
}

void kDrawPie(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color)
{
	kDrawPie(kVec3(pos, 0), radius, radius, theta_a, theta_b, color);
}

void kDrawPiePart(kVec3 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max,
                  float theta_a, float theta_b, kVec4 color)
{
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a)
	{
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a_max * radius_a_max + radius_b_max * radius_b_max)) /
	                       (theta_b - theta_a));
	float dt       = 1.0f / segments;

	float min_px   = kCosLookup(theta_a) * radius_a_min;
	float min_py   = kSinLookup(theta_a) * radius_b_min;
	float max_px   = kCosLookup(theta_a) * radius_a_max;
	float max_py   = kSinLookup(theta_a) * radius_b_max;

	float min_npx, min_npy;
	float max_npx, max_npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt)
	{
		min_npx = kCosLookup(theta_a) * radius_a_min;
		min_npy = kSinLookup(theta_a) * radius_b_min;
		max_npx = kCosLookup(theta_a) * radius_a_max;
		max_npy = kSinLookup(theta_a) * radius_b_max;

		kDrawQuad(pos + kVec3(min_npx, min_npy, 0), pos + kVec3(max_npx, max_npy, 0), pos + kVec3(max_px, max_py, 0),
		          pos + kVec3(min_px, min_py, 0), color);

		min_px = min_npx;
		min_py = min_npy;
		max_px = max_npx;
		max_py = max_npy;
	}
}

void kDrawPiePart(kVec2 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max,
                  float theta_a, float theta_b, kVec4 color)
{
	kDrawPiePart(kVec3(pos, 0), radius_a_min, radius_b_min, radius_a_max, radius_b_max, theta_a, theta_b, color);
}

void kDrawPiePart(kVec3 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color)
{
	kDrawPiePart(pos, radius_min, radius_min, radius_max, radius_max, theta_a, theta_b, color);
}

void kDrawPiePart(kVec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color)
{
	kDrawPiePart(kVec3(pos, 0), radius_min, radius_min, radius_max, radius_max, theta_a, theta_b, color);
}

void kDrawLine(kVec3 a, kVec3 b, kVec4 color)
{
	if (kIsNull(b - a))
		return;

	float thickness = g_Render2D.Stack.Thickness * 0.5f;
	float dx        = b.x - a.x;
	float dy        = b.y - a.y;
	float ilen      = 1.0f / kSquareRoot(dx * dx + dy * dy);
	dx *= (thickness * ilen);
	dy *= (thickness * ilen);

	kVec3 c0 = kVec3(a.x - dy, a.y + dx, a.z);
	kVec3 c1 = kVec3(b.x - dy, b.y + dx, b.z);
	kVec3 c2 = kVec3(b.x + dy, b.y - dx, b.z);
	kVec3 c3 = kVec3(a.x + dy, a.y - dx, a.z);

	kDrawQuad(c0, c1, c2, c3, kVec2(0, 0), kVec2(0, 1), kVec2(1, 1), kVec2(1, 0), color);
}

void kDrawLine(kVec2 a, kVec2 b, kVec4 color)
{
	kDrawLine(kVec3(a, 0), kVec3(b, 0), color);
}

void kPathTo(kVec2 a)
{
	if (g_Render2D.Stack.Paths.Count)
	{
		if (kIsNull(g_Render2D.Stack.Paths.Last() - a))
		{
			return;
		}
	}
	g_Render2D.Stack.Paths.Add(a);
}

void kArcTo(kVec2 position, float radius_a, float radius_b, float theta_a, float theta_b)
{
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a)
	{
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments =
		kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float npx, npy;
	for (; theta_a <= theta_b; theta_a += dt)
	{
		npx = kCosLookup(theta_a) * radius_a;
		npy = kSinLookup(theta_a) * radius_b;
		kPathTo(position + kVec2(npx, npy));
	}
}

void kBezierQuadraticTo(kVec2 a, kVec2 b, kVec2 c)
{
	imem index    = g_Render2D.Stack.Paths.Count;
	int  segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	if (g_Render2D.Stack.Paths.Resize(g_Render2D.Stack.Paths.Count + segments + 1))
		kBuildBezierQuadratic(a, b, c, &g_Render2D.Stack.Paths[index], segments);
}

void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d)
{
	imem index    = g_Render2D.Stack.Paths.Count;
	int  segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	if (g_Render2D.Stack.Paths.Resize(g_Render2D.Stack.Paths.Count + segments + 1))
		kBuildBezierCubic(a, b, c, d, &g_Render2D.Stack.Paths[index], segments);
}

static inline kVec2 kLineLineIntersect(kVec2 p1, kVec2 q1, kVec2 p2, kVec2 q2)
{
	kVec2 d1 = p1 - q1;
	kVec2 d2 = p2 - q2;

	float d  = d1.x * d2.y - d1.y * d2.x;
	float n2 = -d1.x * (p1.y - p2.y) + d1.y * (p1.x - p2.x);

	if (d != 0)
	{
		float u = n2 / d;
		return p2 - u * d2;
	}

	return p1;
}

void kDrawPathStroked(kVec4 color, bool closed, float z)
{
	kArray<kVec2> &path = g_Render2D.Stack.Paths;

	if (path.Count < 2)
	{
		path.Reset();
		return;
	}

	if (path.Count == 2 && closed)
	{
		path.Reset();
		return;
	}

	if (closed)
	{
		kPathTo(path[0]);
	}

	g_Render2D.Stack.Normals.Resize(path.Count - 1);

	for (imem index = 0; index < g_Render2D.Stack.Normals.Count; ++index)
	{
		kVec2 v                         = kNormalize(path[index + 1] - path[index]);
		g_Render2D.Stack.Normals[index] = v;
	}

	float thickness = 0.5f * g_Render2D.Stack.Thickness;

	kVec2 p, q;
	kVec2 start_p, start_q;
	kVec2 prev_p, prev_q;
	kVec2 a, b, c;
	kVec2 v1, v2, n1;
	kVec2 n2, t1, t2;
	kVec2 p1, q1, p2, q2;

	if (closed)
	{
		a       = path.Last();
		b       = path[0];
		c       = path[1];

		v1      = g_Render2D.Stack.Normals.Last();
		v2      = g_Render2D.Stack.Normals[0];

		n1      = kVec2(-v1.y, v1.x);
		n2      = kVec2(-v2.y, v2.x);

		t1      = n1 * thickness;
		t2      = n2 * thickness;

		p1      = b + t1;
		q1      = p1 + v1;
		p2      = b + t2;
		q2      = p2 + v2;
		start_p = prev_p = kLineLineIntersect(p1, q1, p2, q2);

		p1               = b - t1;
		q1               = p1 + v1;
		p2               = b - t2;
		q2               = p2 + v2;
		start_q = prev_q = kLineLineIntersect(p1, q1, p2, q2);
	}
	else
	{
		kVec2 v, t, n;

		v       = g_Render2D.Stack.Normals[0];
		n       = kVec2(-v.y, v.x);
		t       = thickness * n;
		prev_p  = path[0] + t;
		prev_q  = path[0] - t;

		v       = g_Render2D.Stack.Normals.Last();
		n       = kVec2(-v.y, v.x);
		t       = thickness * n;
		start_p = path.Last() + t;
		start_q = path.Last() - t;
	}

	for (imem index = 0; index + 1 < g_Render2D.Stack.Normals.Count; ++index)
	{
		a  = path[index + 0];
		b  = path[index + 1];
		c  = path[index + 2];

		v1 = g_Render2D.Stack.Normals[index + 0];
		v2 = g_Render2D.Stack.Normals[index + 1];

		n1 = kVec2(-v1.y, v1.x);
		n2 = kVec2(-v2.y, v2.x);

		t1 = n1 * thickness;
		t2 = n2 * thickness;

		{
			p1 = b + t1;
			q1 = p1 + v1;
			p2 = b + t2;
			q2 = p2 + v2;
			p  = kLineLineIntersect(p1, q1, p2, q2);
		}

		{
			p1 = b - t1;
			q1 = p1 + v1;
			p2 = b - t2;
			q2 = p2 + v2;
			q  = kLineLineIntersect(p1, q1, p2, q2);
		}

		kDrawQuad(prev_q, prev_p, p, q, color);

		prev_p = p;
		prev_q = q;
	}

	kDrawQuad(prev_q, prev_p, start_p, start_q, color);

	path.Reset();
}

void kDrawPathFilled(kVec4 color, float z)
{
	kArray<kVec2> &path = g_Render2D.Stack.Paths;

	if (path.Count < 3)
	{
		path.Reset();
		return;
	}

	int triangle_count = (int)path.Count - 2;
	for (int ti = 0; ti < triangle_count; ++ti)
	{
		kDrawTriangle(kVec3(path[0], z), kVec3(path[ti + 1], z), kVec3(path[ti + 2], z), color);
	}

	path.Reset();
}

void kDrawBezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec4 color, float z)
{
	kBezierQuadraticTo(a, b, c);
	kDrawPathStroked(color, false, z);
}

void kDrawBezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color, float z)
{
	kBezierCubicTo(a, b, c, d);
	kDrawPathStroked(color, false, z);
}

void kDrawPolygon(const kVec2 *vertices, u32 count, float z, kVec4 color)
{
	kAssert(count >= 3);
	u32 triangle_count = count - 2;
	for (u32 triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
	{
		kDrawTriangle(kVec3(vertices[0], z), kVec3(vertices[triangle_index + 1], z),
		              kVec3(vertices[triangle_index + 2], z), color);
	}
}

void kDrawPolygon(const kVec2 *vertices, u32 count, kVec4 color)
{
	kDrawPolygon(vertices, count, 0, color);
}

void kDrawTriangleOutline(kVec3 a, kVec3 b, kVec3 c, kVec4 color)
{
	kPathTo(a.xy);
	kPathTo(b.xy);
	kPathTo(c.xy);
	kDrawPathStroked(color, true, a.z);
}

void kDrawTriangleOutline(kVec2 a, kVec2 b, kVec2 c, kVec4 color)
{
	kPathTo(a);
	kPathTo(b);
	kPathTo(c);
	kDrawPathStroked(color, true, 0.0f);
}

void kDrawQuadOutline(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec4 color)
{
	kPathTo(a.xy);
	kPathTo(b.xy);
	kPathTo(c.xy);
	kPathTo(d.xy);
	kDrawPathStroked(color, true, a.z);
}

void kDrawQuadOutline(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color)
{
	kPathTo(a);
	kPathTo(b);
	kPathTo(c);
	kPathTo(d);
	kDrawPathStroked(color, true, 0.0f);
}

void kDrawRectOutline(kVec3 pos, kVec2 dim, kVec4 color)
{
	kVec3 a = pos;
	kVec3 b = pos + kVec3(0, dim.y, 0);
	kVec3 c = pos + kVec3(dim, 0);
	kVec3 d = pos + kVec3(dim.x, 0, 0);
	kDrawQuadOutline(a, b, c, d, color);
}

void kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color)
{
	kDrawRectOutline(kVec3(pos, 0), dim, color);
}

void kDrawRectCenteredOutline(kVec3 pos, kVec2 dim, kVec4 color)
{
	kVec2 half_dim = 0.5f * dim;

	kVec3 a, b, c, d;
	a.xy = pos.xy - half_dim;
	b.xy = kVec2(pos.x - half_dim.x, pos.y + half_dim.y);
	c.xy = pos.xy + half_dim;
	d.xy = kVec2(pos.x + half_dim.x, pos.y - half_dim.y);

	a.z  = pos.z;
	b.z  = pos.z;
	c.z  = pos.z;
	d.z  = pos.z;
	kDrawQuadOutline(a, b, c, d, color);
}

void kDrawRectCenteredOutline(kVec2 pos, kVec2 dim, kVec4 color)
{
	kDrawRectCenteredOutline(kVec3(pos, 0), dim, color);
}

void kDrawEllipseOutline(kVec3 pos, float radius_a, float radius_b, kVec4 color)
{
	int   segments = (int)kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));
	float npx, npy;
	for (int index = 0; index <= segments; ++index)
	{
		npx = kCosLookup(index, segments) * radius_a;
		npy = kSinLookup(index, segments) * radius_b;
		kPathTo(pos.xy + kVec2(npx, npy));
	}
	kDrawPathStroked(color, true, pos.z);
}

void kDrawEllipseOutline(kVec2 pos, float radius_a, float radius_b, kVec4 color)
{
	kDrawEllipseOutline(kVec3(pos, 0), radius_a, radius_b, color);
}

void kDrawCircleOutline(kVec3 pos, float radius, kVec4 color)
{
	kDrawEllipseOutline(pos, radius, radius, color);
}

void kDrawCircleOutline(kVec2 pos, float radius, kVec4 color)
{
	kDrawEllipseOutline(kVec3(pos, 0), radius, radius, color);
}

void kDrawArcOutline(kVec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed)
{
	kArcTo(pos.xy, radius_a, radius_b, theta_a, theta_b);
	if (closed)
	{
		kPathTo(pos.xy);
	}
	kDrawPathStroked(color, closed, pos.z);
}

void kDrawArcOutline(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed)
{
	kDrawArcOutline(kVec3(pos, 0), radius_a, radius_b, theta_a, theta_b, color, closed);
}

void kDrawArcOutline(kVec3 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed)
{
	kDrawArcOutline(pos, radius, radius, theta_a, theta_b, color, closed);
}

void kDrawArcOutline(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed)
{
	kDrawArcOutline(kVec3(pos, 0), radius, radius, theta_a, theta_b, color, closed);
}

void kDrawPolygonOutline(const kVec2 *vertices, u32 count, float z, kVec4 color)
{
	for (u32 Indices = 0; Indices < count; ++Indices)
	{
		kPathTo(vertices[Indices]);
	}
	kDrawPathStroked(color, true, z);
}

void kDrawPolygonOutline(const kVec2 *vertices, u32 count, kVec4 color)
{
	kDrawPolygonOutline(vertices, count, 0, color);
}

void kDrawTexture(kTexture texture, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture texture, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTexture(kTexture texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureMasked(kTexture texture, kTexture mask, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawRoundedRect(kVec3 pos, kVec2 dim, kVec4 color, float radius)
{
	if (radius)
	{
		float rad_x = kMin(radius, 0.5f * dim.x);
		float rad_y = kMin(radius, 0.5f * dim.y);

		kVec2 pos2d = pos.xy;

		kVec2 p0, p1, p2, p3;

		p0 = pos2d + kVec2(rad_x, rad_y);
		p1 = pos2d + kVec2(dim.x - rad_x, rad_y);
		p2 = pos2d + dim - kVec2(rad_x, rad_y);
		p3 = pos2d + kVec2(rad_x, dim.y - rad_y);

		kArcTo(p0, rad_x, rad_y, 2.0f / 4.0f, 3.0f / 4.0f);
		kArcTo(p1, rad_x, rad_y, 3.0f / 4.0f, 4.0f / 4.0f);
		kArcTo(p2, rad_x, rad_y, 0.0f / 4.0f, 1.0f / 4.0f);
		kArcTo(p3, rad_x, rad_y, 1.0f / 4.0f, 2.0f / 4.0f);

		kDrawPathFilled(color, pos.z);
	}
	else
	{
		kDrawRect(pos, dim, color);
	}
}

void kDrawRoundedRect(kVec2 pos, kVec2 dim, kVec4 color, float radius)
{
	kDrawRoundedRect(kVec3(pos, 0), dim, color, radius);
}

void kDrawRoundedRectOutline(kVec3 pos, kVec2 dim, kVec4 color, float radius)
{
	if (radius)
	{
		float rad_x = kMin(radius, 0.5f * dim.x);
		float rad_y = kMin(radius, 0.5f * dim.y);

		kVec2 pos2d = pos.xy;

		kVec2 p0, p1, p2, p3;

		p0 = pos2d + kVec2(rad_x, rad_y);
		p1 = pos2d + kVec2(dim.x - rad_x, rad_y);
		p2 = pos2d + dim - kVec2(rad_x, rad_y);
		p3 = pos2d + kVec2(rad_x, dim.y - rad_y);

		kArcTo(p0, rad_x, rad_y, 2.0f / 4.0f, 3.0f / 4.0f);
		kArcTo(p1, rad_x, rad_y, 3.0f / 4.0f, 4.0f / 4.0f);
		kArcTo(p2, rad_x, rad_y, 0.0f / 4.0f, 1.0f / 4.0f);
		kArcTo(p3, rad_x, rad_y, 1.0f / 4.0f, 2.0f / 4.0f);

		kDrawPathStroked(color, true, pos.z);
	}
	else
	{
		kDrawRect(pos, dim, color);
	}
}

void kDrawRoundedRectOutline(kVec2 pos, kVec2 dim, kVec4 color, float radius)
{
	kDrawRoundedRectOutline(kVec3(pos, 0), dim, color, radius);
}

kGlyph *kFindFontGlyph(const kFont *font, u32 codepoint)
{
	if (codepoint >= font->MinCodepoint && codepoint <= font->MaxCodepoint)
	{
		u32 index = codepoint - font->MinCodepoint;
		u16 pos   = font->CodepointMap[index];
		return &font->Glyphs[pos];
	}
	return font->Fallback;
}

kVec2 kAdjectCursorToBaseline(const kFont *font, kVec2 cursor, float scale)
{
	cursor.y += (float)(font->Ascent) * scale;
	return cursor;
}

bool kCalculateGlyphMetrics(const kFont *font, u32 codepoint, float scale, kVec2 *cursor, kVec2 *rpos, kVec2 *rsize,
                            kRect *rect)
{
	if (codepoint == '\r')
		return false;

	if (codepoint == '\n')
	{
		cursor->x = 0;
		cursor->y += (float)font->Linegap * scale;
		return false;
	}

	kGlyph *glyph = kFindFontGlyph(font, codepoint);

	rpos->x       = (cursor->x + scale * (float)glyph->Bearing.x);
	rpos->y       = (cursor->y - scale * (float)(glyph->Bearing.y + glyph->Size.y));

	rsize->x      = (scale * (float)glyph->Size.x);
	rsize->y      = (scale * (float)glyph->Size.y);
	*rect         = glyph->Rect;

	cursor->x += scale * glyph->Advance.x;

	return true;
}

void kDrawTextQuad(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale)
{
	kPushTexture(g_Builtin.Textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->Atlas, kTextureType_MaskSDF);

	kVec3 rpos = pos;
	kVec2 rsize;
	kRect rect;

	kVec2 offset = kAdjectCursorToBaseline(font, pos.xy, scale);
	kVec2 cursor = kVec2(0, 0);

	if (kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos.xy, &rsize, &rect))
	{
		rpos.xy += offset;
		kDrawRect(rpos, rsize, rect, color);
	}

	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextQuad(u32 codepoint, const kFont *font, kVec2 pos, kVec4 color, float scale)
{
	kDrawTextQuad(codepoint, font, kVec3(pos, 0), color, scale);
}

void kDrawTextQuadRotated(u32 codepoint, const kFont *font, kVec3 pos, float angle, kVec4 color, float scale)
{
	kPushTexture(g_Builtin.Textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->Atlas, kTextureType_MaskSDF);

	kVec3 rpos = pos;
	kVec2 rsize;
	kRect rect;

	kVec2 offset = kAdjectCursorToBaseline(font, pos.xy, scale);
	kVec2 cursor = kVec2(0, 0);

	if (kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos.xy, &rsize, &rect))
	{
		rpos.xy += offset;
		kDrawRectRotated(rpos, rsize, angle, rect, color);
	}

	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextQuadRotated(u32 codepoint, const kFont *font, kVec2 pos, float angle, kVec4 color, float scale)
{
	kDrawTextQuadRotated(codepoint, font, kVec3(pos, 0), angle, color, scale);
}

void kDrawTextQuadCentered(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale)
{
	kPushTexture(g_Builtin.Textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->Atlas, kTextureType_MaskSDF);

	kVec3 rpos = pos;
	kVec2 rsize;
	kRect rect;

	kVec2 offset = kAdjectCursorToBaseline(font, pos.xy, scale);
	kVec2 cursor = kVec2(0, 0);

	if (kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos.xy, &rsize, &rect))
	{
		rpos.xy += offset;
		kDrawRectCentered(rpos, rsize, rect, color);
	}

	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextQuadCentered(u32 codepoint, const kFont *font, kVec2 pos, kVec4 color, float scale)
{
	kDrawTextQuadCentered(codepoint, font, kVec3(pos, 0), color, scale);
}

void kDrawTextQuadCenteredRotated(u32 codepoint, const kFont *font, kVec3 pos, float angle, kVec4 color, float scale)
{
	kPushTexture(g_Builtin.Textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->Atlas, kTextureType_MaskSDF);

	kVec3 rpos = pos;
	kVec2 rsize;
	kRect rect;

	kVec2 offset = kAdjectCursorToBaseline(font, pos.xy, scale);
	kVec2 cursor = kVec2(0, 0);

	if (kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos.xy, &rsize, &rect))
	{
		rpos.xy += offset;
		kDrawRectCenteredRotated(rpos, rsize, angle, rect, color);
	}

	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextQuadCenteredRotated(u32 codepoint, const kFont *font, kVec2 pos, float angle, kVec4 color, float scale)
{
	kDrawTextQuadCenteredRotated(codepoint, font, kVec3(pos, 0), angle, color, scale);
}

void kDrawTextQuad(u32 codepoint, kVec3 pos, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuad(codepoint, font, pos, color, scale);
}

void kDrawTextQuad(u32 codepoint, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuad(codepoint, font, pos, color, scale);
}

void kDrawTextQuadRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuadRotated(codepoint, font, pos, angle, color, scale);
}

void kDrawTextQuadRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuadRotated(codepoint, font, pos, angle, color, scale);
}

void kDrawTextQuadCentered(u32 codepoint, kVec3 pos, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuadCentered(codepoint, font, pos, color, scale);
}

void kDrawTextQuadCentered(u32 codepoint, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuadCentered(codepoint, font, pos, color, scale);
}

void kDrawTextQuadCenteredRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuadCenteredRotated(codepoint, font, pos, angle, color, scale);
}

void kDrawTextQuadCenteredRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawTextQuadCenteredRotated(codepoint, font, pos, angle, color, scale);
}

kVec2 kCalculateText(kString text, const kFont *font, float scale)
{
	u32   codepoint;
	u8 *  ptr = text.begin();
	u8 *  end = text.end();

	kVec2 rpos;
	kVec2 rsize;
	kRect rect;

	kVec2 cursor  = kAdjectCursorToBaseline(font, kVec2(0), scale);
	float ycounts = 1.0f;

	while (ptr < end)
	{
		ptr += kUTF8ToCodepoint(ptr, end, &codepoint);
		if (codepoint == '\n')
			ycounts += 1;
		kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos, &rsize, &rect);
	}

	kVec2 r;
	r.x = cursor.x;
	r.y = ycounts * scale * font->Height;

	return r;
}

kVec2 kCalculateText(kString text, float scale)
{
	kFont *font = g_Builtin.Font;
	return kCalculateText(text, font, scale);
}

void kDrawText(kString text, kVec3 pos, const kFont *font, kVec4 color, float scale)
{
	kPushTexture(g_Builtin.Textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->Atlas, kTextureType_MaskSDF);

	u32   codepoint;
	u8 *  ptr  = text.begin();
	u8 *  end  = text.end();

	kVec3 rpos = pos;
	kVec2 rsize;
	kRect rect;

	kVec2 offset = kAdjectCursorToBaseline(font, pos.xy, scale);
	kVec2 cursor = kVec2(0, 0);

	while (ptr < end)
	{
		ptr += kUTF8ToCodepoint(ptr, end, &codepoint);
		if (kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos.xy, &rsize, &rect))
		{
			rpos.xy += offset;
			kDrawRect(rpos, rsize, rect, color);
		}
	}

	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawText(kString text, kVec2 pos, const kFont *font, kVec4 color, float scale)
{
	kDrawText(text, kVec3(pos, 0), font, color, scale);
}

void kDrawText(kString text, kVec3 pos, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawText(text, pos, font, color, scale);
}

void kDrawText(kString text, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = g_Builtin.Font;
	kDrawText(text, pos, font, color, scale);
}

//
//
//

void kCreateRenderContext(const kRenderSpec &spec, kTexture textures[kTextureType_Count], kFont *font)
{
	for (u32 i = 0; i < K_MAX_CIRCLE_SEGMENTS; ++i)
	{
		float theta = ((float)i / (float)(K_MAX_CIRCLE_SEGMENTS - 1));
		Cosines[i]  = kCos(theta);
		Sines[i]    = kSin(theta);
	}

	Cosines[K_MAX_CIRCLE_SEGMENTS - 1] = 1;
	Sines[K_MAX_CIRCLE_SEGMENTS - 1]   = 0;

	for (u32 i = 0; i < kTextureType_Count; ++i)
		g_Builtin.Textures[i] = textures[i];
	g_Builtin.Font             = font;

	g_Render2D.Stack.Thickness = 1.0f;

	g_Render2D.Vertices.Reserve(spec.Vertices);
	g_Render2D.Indices.Reserve(spec.Indices);
	g_Render2D.Commands.Reserve(spec.Commands);
	g_Render2D.Scenes.Reserve(spec.Scenes);
	g_Render2D.Rects.Reserve(spec.Commands);

	g_Render2D.Stack.Rects.Reserve(64);
	g_Render2D.Stack.Transforms.Reserve(64);
	for (u32 i = 0; i < kTextureType_Count; ++i)
		g_Render2D.Stack.Textures[i].Reserve(64);
	g_Render2D.Stack.Paths.Reserve(4096);
	g_Render2D.Stack.Normals.Reserve(4096);
	g_Render2D.Stack.BlendModes.Reserve(32);

	g_Render2D.Stack.Rects.Add(kRect{});
	g_Render2D.Stack.Transforms.Add(kIdentity3x3());
	for (u32 i = 0; i < kTextureType_Count; ++i)
		g_Render2D.Stack.Textures[i].Add(g_Builtin.Textures[i]);
	g_Render2D.Stack.OutLineStyles.Add(kVec4(0));
	g_Render2D.Stack.BlendModes.Add(kBlendMode_Normal);
	g_Render2D.Stack.TextureFilters.Add(kTextureFilter_LinearWrap);

	kResetFrame();
}

void kDestroyRenderContext(void)
{
	kFree(&g_Render2D.Vertices);
	kFree(&g_Render2D.Indices);
	kFree(&g_Render2D.Commands);
	kFree(&g_Render2D.Scenes);
	kFree(&g_Render2D.Rects);
	kFree(&g_Render2D.OutLineStyles);

	kFree(&g_Render2D.Stack.Rects);
	kFree(&g_Render2D.Stack.Transforms);
	for (u32 i = 0; i < kTextureType_Count; ++i)
		kFree(&g_Render2D.Stack.Textures[i]);
	kFree(&g_Render2D.Stack.Paths);
	kFree(&g_Render2D.Stack.Normals);
	kFree(&g_Render2D.Stack.BlendModes);
	kFree(&g_Render2D.Stack.OutLineStyles);

	memset(&g_Render2D, 0, sizeof(g_Render2D));
}

void kResetFrame(void)
{
	g_Render2D.Vertices.Count         = 0;
	g_Render2D.Indices.Count          = 0;
	g_Render2D.Commands.Count         = 0;
	g_Render2D.Scenes.Count           = 0;
	g_Render2D.Rects.Count            = 0;
	g_Render2D.OutLineStyles.Count    = 0;

	g_Render2D.Stack.Rects.Count      = 1;
	g_Render2D.Stack.Transforms.Count = 1;
	for (u32 i = 0; i < kTextureType_Count; ++i)
		g_Render2D.Stack.Textures[i].Count = 1;
	g_Render2D.Stack.OutLineStyles.Count  = 1;
	g_Render2D.Stack.BlendModes.Count     = 1;
	g_Render2D.Stack.TextureFilters.Count = 1;
	g_Render2D.Stack.Paths.Count          = 0;
	g_Render2D.Stack.Normals.Count        = 0;
	g_Render2D.Stack.Thickness            = 1.0f;
}

void kGetRenderMemoryUsage(kRenderMemory *usage)
{
	memcpy(usage, &g_Memory.Used, sizeof(*usage));
}

void kGetRenderMemoryCaps(kRenderMemory *usage)
{
	memcpy(usage, &g_Memory.Caps, sizeof(*usage));
}

static void kUpdateMemoryStatictics(void)
{
	g_Memory.Caps.VertexSize  = (u32)(g_Render2D.Vertices.Capacity * sizeof(kVertex2D));
	g_Memory.Caps.IndexSize   = (u32)(g_Render2D.Indices.Capacity * sizeof(kIndex2D));
	g_Memory.Caps.CommandSize = (u32)(g_Render2D.Commands.Capacity * sizeof(kRenderCommand2D));
	g_Memory.Caps.SceneSize   = (u32)(g_Render2D.Scenes.Capacity * sizeof(kRenderScene2D));

	g_Memory.Caps.MB          = 0;
	g_Memory.Caps.MB += (float)g_Memory.Caps.VertexSize;
	g_Memory.Caps.MB += (float)g_Memory.Caps.IndexSize;
	g_Memory.Caps.MB += (float)g_Memory.Caps.CommandSize;
	g_Memory.Caps.MB += (float)g_Memory.Caps.SceneSize;
	g_Memory.Caps.MB /= (1024 * 1024);

	g_Memory.Used.VertexSize  = (u32)(g_Render2D.Vertices.Count * sizeof(kVertex2D));
	g_Memory.Used.IndexSize   = (u32)(g_Render2D.Indices.Count * sizeof(kIndex2D));
	g_Memory.Used.CommandSize = (u32)(g_Render2D.Commands.Count * sizeof(kRenderCommand2D));
	g_Memory.Used.SceneSize   = (u32)(g_Render2D.Scenes.Count * sizeof(kRenderScene2D));

	g_Memory.Used.MB          = 0;
	g_Memory.Used.MB += (float)g_Memory.Used.VertexSize;
	g_Memory.Used.MB += (float)g_Memory.Used.IndexSize;
	g_Memory.Used.MB += (float)g_Memory.Used.CommandSize;
	g_Memory.Used.MB += (float)g_Memory.Used.SceneSize;
	g_Memory.Used.MB /= (1024 * 1024);
}

void kGetFrameData(kRenderFrame2D *frame)
{
	frame->Scenes        = g_Render2D.Scenes;
	frame->Commands      = g_Render2D.Commands;
	frame->OutLineStyles = g_Render2D.OutLineStyles;
	frame->Rects         = g_Render2D.Rects;
	frame->Vertices      = g_Render2D.Vertices;
	frame->Indices       = g_Render2D.Indices;

	kUpdateMemoryStatictics();
}
