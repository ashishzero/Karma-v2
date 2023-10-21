#include "kMath.h"
#include "kArray.h"
#include "kRender.h"

#include <string.h>

//
//
//

typedef struct kRenderStack2D
{
	kArray<kTexture *>     textures[kTextureType_Count];
	kArray<kMat3>          transforms;
	kArray<kRect>          rects;
	kArray<kBlendMode>     blends;
	kArray<kTextureFilter> filters;
	kArray<kVec2>          scratch;
	float                  thickness;
} kRenderStack2D;

typedef struct kRenderContext2D
{
	kArray<kVertex2D>        vertices;
	kArray<kIndex2D>         indices;
	kArray<kTexture *>       textures[kTextureType_Count];
	kArray<kRect>            rects;
	kArray<kRenderScene2D>   scenes;
	kArray<kRenderCommand2D> commands;
	kRenderStack2D           stack;
} kRenderContext2D;

typedef struct kRenderContextBuiltin
{
	kTexture *textures[kTextureType_Count];
	kFont    *font;
} kRenderContextBuiltin;

typedef struct kRenderContext
{
	kRenderContext2D        context2d;
	kRenderContextBuiltin   builtin;
	kRenderMemoryStatistics memory;
} kRenderContext;

static kRenderContext render;

static float          Sines[K_MAX_CIRCLE_SEGMENTS];
static float          Cosines[K_MAX_CIRCLE_SEGMENTS];

//
//
//

static kRenderContext2D *kGetRenderContext2D(void) { return &render.context2d; }

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
	kRenderContext2D *ctx = kGetRenderContext2D();
	kRenderCommand2D *cmd = ctx->commands.Add();
	cmd->flags            = dirty_flags;
	for (int i = 0; i < kTextureType_Count; ++i)
		cmd->textures[i] = (u32)ctx->textures[i].count - 1;
	cmd->rect   = (u32)ctx->rects.count - 1;
	cmd->blend  = ctx->stack.blends.Last();
	cmd->filter = ctx->stack.filters.Last();
	cmd->vertex = 0;
	cmd->index  = 0;
}

static void kHandleBlendModeChange(void)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kBlendMode        mode = ctx->stack.blends.Last();
	kRenderCommand2D &cmd  = ctx->commands.Last();

	if (cmd.blend != mode)
	{
		if (cmd.index)
		{
			kPushRenderCommand(kRenderDirty_Blend);
		}
		else
		{
			cmd.blend = mode;
			if (ctx->commands.count > 1)
			{
				kRenderCommand2D &prev = ctx->commands[ctx->commands.count - 2];
				if (prev.blend != mode)
				{
					cmd.flags |= kRenderDirty_Blend;
				}
				else
				{
					cmd.flags &= ~kRenderDirty_Blend;
					if (!cmd.flags)
					{
						ctx->commands.Pop();
					}
				}
			}
		}
	}
}

static void kHandleTextureFilterChange(void)
{
	kRenderContext2D *ctx    = kGetRenderContext2D();
	kTextureFilter    filter = ctx->stack.filters.Last();
	kRenderCommand2D &cmd    = ctx->commands.Last();

	if (cmd.filter != filter)
	{
		if (cmd.index)
		{
			kPushRenderCommand(kRenderDirty_TextureFilter);
		}
		else
		{
			cmd.filter = filter;
			if (ctx->commands.count > 1)
			{
				kRenderCommand2D &prev = ctx->commands[ctx->commands.count - 2];
				if (prev.filter != filter)
				{
					cmd.flags |= kRenderDirty_TextureFilter;
				}
				else
				{
					cmd.flags &= ~kRenderDirty_TextureFilter;
					if (!cmd.flags)
					{
						ctx->commands.Pop();
					}
				}
			}
		}
	}
}

static void kHandleRectChange(void)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kRect            &rect = ctx->stack.rects.Last();
	kRenderCommand2D &cmd  = ctx->commands.Last();

	if (memcmp(&ctx->rects[cmd.rect], &rect, sizeof(kRect)) != 0)
	{
		if (cmd.index)
		{
			ctx->rects.Add(rect);
			kPushRenderCommand(kRenderDirty_Rect);
		}
		else
		{
			if (ctx->commands.count > 1)
			{
				if (cmd.flags & kRenderDirty_Rect)
				{
					ctx->rects.Pop();
				}

				kRenderCommand2D &prev = ctx->commands[ctx->commands.count - 2];
				if (memcmp(&ctx->rects[prev.rect], &rect, sizeof(kRect)) != 0)
				{
					ctx->rects.Add(rect);
					cmd.flags |= kRenderDirty_Rect;
					cmd.rect = (u32)ctx->rects.count - 1;
				}
				else
				{
					cmd.flags &= ~kRenderDirty_Rect;
					cmd.rect = (u32)ctx->rects.count - 1;
					if (!cmd.flags)
					{
						ctx->commands.Pop();
					}
				}
			}
			else
			{
				ctx->rects[0] = rect;
			}
		}
	}
}

static void kHandleTextureChange(kTextureType type)
{
	kRenderContext2D *ctx     = kGetRenderContext2D();
	kTexture         *texture = ctx->stack.textures[type].Last();
	kRenderCommand2D &cmd     = ctx->commands.Last();
	u32               bit     = type == kTextureType_Color ? kRenderDirty_TextureColor : kRenderDirty_TextureMaskSDF;

	if (ctx->textures[type][cmd.textures[type]] != texture)
	{
		if (cmd.index)
		{
			ctx->textures[type].Add(texture);
			kPushRenderCommand(bit);
		}
		else
		{
			if (ctx->commands.count > 1)
			{
				if (cmd.flags & bit)
				{
					ctx->textures[type].Pop();
				}

				kRenderCommand2D &prev = ctx->commands[ctx->commands.count - 2];
				if (ctx->textures[type][prev.textures[type]] != texture)
				{
					ctx->textures[type].Add(texture);
					cmd.flags |= bit;
					cmd.textures[type] = (u32)ctx->textures[type].count - 1;
				}
				else
				{
					cmd.flags &= ~bit;
					cmd.textures[type] = (u32)ctx->textures[type].count - 1;
					if (!cmd.flags)
					{
						ctx->commands.Pop();
					}
				}
			}
			else
			{
				ctx->textures[type][0] = texture;
			}
		}
	}
}

//
//
//

void kBeginScene(const kCamera2D &camera, kRect region)
{
	kRenderContext2D *ctx   = kGetRenderContext2D();
	kRenderScene2D   *scene = ctx->scenes.Add();
	scene->camera           = camera;
	scene->region           = region;
	scene->commands         = kRange<u32>((u32)ctx->commands.count);

	for (int i = 0; i < kTextureType_Count; ++i)
		render.context2d.textures[i].Add(render.context2d.stack.textures[i].Last());
	render.context2d.rects.Add(render.context2d.stack.rects.Last());

	kPushRenderCommand(kRenderDirty_Everything);
	kPushRect(region);
}

void kBeginScene(float left, float right, float top, float bottom, float near, float far, kRect region)
{
	kCamera2D camera;
	camera.left   = left;
	camera.right  = right;
	camera.top    = top;
	camera.bottom = bottom;
	camera.near   = near;
	camera.far    = far;
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
	kRenderContext2D *ctx = kGetRenderContext2D();

	kRenderCommand2D &cmd = ctx->commands.Last();
	if (!cmd.index)
	{
		u32 flags = cmd.flags;
		if (flags & kRenderDirty_TextureColor) ctx->textures[kTextureType_Color].Pop();
		if (flags & kRenderDirty_TextureMaskSDF) ctx->textures[kTextureType_MaskSDF].Pop();
		if (flags & kRenderDirty_Rect) ctx->rects.Pop();
		ctx->commands.Pop();
	}

	kRenderScene2D *pass = &ctx->scenes.Last();
	pass->commands.end   = (u32)ctx->commands.count;

	if (!pass->commands.Length())
	{
		ctx->scenes.Pop();
	}
}

void kLineThickness(float thickness)
{
	kRenderContext2D *ctx = kGetRenderContext2D();
	ctx->stack.thickness  = thickness;
}

void kSetBlendMode(kBlendMode mode)
{
	kRenderContext2D *ctx    = kGetRenderContext2D();
	ctx->stack.blends.Last() = mode;
	kHandleBlendModeChange();
}

void kBeginBlendMode(kBlendMode mode)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kBlendMode        last = ctx->stack.blends.Last();
	ctx->stack.blends.Add(last);
	kSetBlendMode(mode);
}

void kEndBlendMode(void)
{
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->stack.blends.count;
	kAssert(count > 1);
	kSetBlendMode(ctx->stack.blends[count - 2]);
	ctx->stack.blends.Pop();
}

void kSetTextureFilter(kTextureFilter filter)
{
	kRenderContext2D *ctx     = kGetRenderContext2D();
	ctx->stack.filters.Last() = filter;
	kHandleTextureFilterChange();
}

void kSetTexture(kTexture *texture, kTextureType idx)
{
	kRenderContext2D *ctx           = kGetRenderContext2D();
	ctx->stack.textures[idx].Last() = texture;
	kHandleTextureChange(idx);
}

void kPushTexture(kTexture *texture, kTextureType idx)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kTexture         *last = ctx->stack.textures[idx].Last();
	ctx->stack.textures[idx].Add(last);
	kSetTexture(texture, idx);
}

void kPopTexture(kTextureType idx)
{
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->stack.textures[idx].count;
	kAssert(count > 1);
	kSetTexture(ctx->stack.textures[idx].data[count - 2], idx);
	ctx->stack.textures[idx].Pop();
}

void kSetRect(kRect rect)
{
	kRenderContext2D *ctx   = kGetRenderContext2D();
	ctx->stack.rects.Last() = rect;
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
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kRect             last = ctx->stack.rects.Last();
	ctx->stack.rects.Add(last);
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
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->stack.rects.count;
	kAssert(count > 1);
	kSetRect(ctx->stack.rects.data[count - 2]);
	ctx->stack.rects.Pop();
}

void kSetTransform(const kMat3 &transform)
{
	kRenderContext2D *ctx        = kGetRenderContext2D();
	ctx->stack.transforms.Last() = transform;
}

void kPushTransform(const kMat3 &transform)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kMat3             last = ctx->stack.transforms.Last();
	ctx->stack.transforms.Add(last);
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
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->stack.transforms.count;
	kAssert(count > 1);
	kSetTransform(ctx->stack.transforms.data[count - 2]);
	ctx->stack.transforms.Pop();
}

//
//
//

static void kCommitPrimitive(u32 vertex, u32 index)
{
	kRenderContext2D *ctx = kGetRenderContext2D();
	kRenderCommand2D &cmd = ctx->commands.Last();
	cmd.vertex += vertex;
	cmd.index += index;
}

//
//
//

void kDrawTriangle(kVec3 va, kVec3 vb, kVec3 vc, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 ca, kVec4 cb, kVec4 cc)
{
	kVertex2D *vtx = render.context2d.vertices.Extend(3);
	vtx[0].pos     = va;
	vtx[0].tex     = ta;
	vtx[0].col     = ca;
	vtx[1].pos     = vb;
	vtx[1].tex     = tb;
	vtx[1].col     = cb;
	vtx[2].pos     = vc;
	vtx[2].tex     = tc;
	vtx[2].col     = cc;

	if (render.context2d.stack.transforms.count > 1)
	{
		kMat3 &transform = render.context2d.stack.transforms.Last();
		vtx[0].pos       = transform * vtx[0].pos;
		vtx[1].pos       = transform * vtx[1].pos;
		vtx[2].pos       = transform * vtx[2].pos;
	}

	u32       next = render.context2d.commands.Last().vertex;

	kIndex2D *idx  = render.context2d.indices.Extend(3);
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
	kVertex2D *vtx = render.context2d.vertices.Extend(4);
	vtx[0].pos     = va;
	vtx[0].tex     = ta;
	vtx[0].col     = color;
	vtx[1].pos     = vb;
	vtx[1].tex     = tb;
	vtx[1].col     = color;
	vtx[2].pos     = vc;
	vtx[2].tex     = tc;
	vtx[2].col     = color;
	vtx[3].pos     = vd;
	vtx[3].tex     = td;
	vtx[3].col     = color;

	if (render.context2d.stack.transforms.count > 1)
	{
		kMat3 &transform = render.context2d.stack.transforms.Last();
		vtx[0].pos       = transform * vtx[0].pos;
		vtx[1].pos       = transform * vtx[1].pos;
		vtx[2].pos       = transform * vtx[2].pos;
		vtx[3].pos       = transform * vtx[3].pos;
	}

	u32       next = render.context2d.commands.Last().vertex;

	kIndex2D *idx  = render.context2d.indices.Extend(6);
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

void kDrawRect(kVec2 pos, kVec2 dim, kVec4 color) { kDrawRect(kVec3(pos, 0), dim, color); }

void kDrawRect(kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kVec2 uv_a = rect.min;
	kVec2 uv_b = kVec2(rect.min.x, rect.max.y);
	kVec2 uv_c = rect.max;
	kVec2 uv_d = kVec2(rect.max.x, rect.min.y);
	kDrawRect(pos, dim, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRect(kVec2 pos, kVec2 dim, kRect rect, kVec4 color) { kDrawRect(kVec3(pos, 0), dim, rect, color); }

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

void kDrawCircle(kVec3 pos, float radius, kVec4 color) { kDrawEllipse(pos, radius, radius, color); }

void kDrawCircle(kVec2 pos, float radius, kVec4 color) { kDrawEllipse(kVec3(pos, 0), radius, radius, color); }

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
	if (kIsNull(b - a)) return;

	float thickness = render.context2d.stack.thickness * 0.5f;
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

void kDrawLine(kVec2 a, kVec2 b, kVec4 color) { kDrawLine(kVec3(a, 0), kVec3(b, 0), color); }

void kPathTo(kVec2 a)
{
	if (render.context2d.stack.scratch.count)
	{
		if (kIsNull(render.context2d.stack.scratch.Last() - a))
		{
			return;
		}
	}
	render.context2d.stack.scratch.Add(a);
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
	imem index    = render.context2d.stack.scratch.count;
	int  segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	if (render.context2d.stack.scratch.Resize(render.context2d.stack.scratch.count + segments + 1))
		kBuildBezierQuadratic(a, b, c, &render.context2d.stack.scratch[index], segments);
}

void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d)
{
	imem index    = render.context2d.stack.scratch.count;
	int  segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	if (render.context2d.stack.scratch.Resize(render.context2d.stack.scratch.count + segments + 1))
		kBuildBezierCubic(a, b, c, d, &render.context2d.stack.scratch[index], segments);
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
	kArray<kVec2> &path = render.context2d.stack.scratch;

	if (path.count < 2)
	{
		path.Reset();
		return;
	}

	if (path.count == 2 && closed)
	{
		path.Reset();
		return;
	}

	if (closed)
	{
		kPathTo(path[0]);
	}

	kSpan<kVec2> normals = kSpan<kVec2>(path.Extend(path.count - 1), path.count - 1);

	for (imem index = 0; index < normals.count; ++index)
	{
		kVec2 v        = kNormalize(path[index + 1] - path[index]);
		normals[index] = v;
	}

	float thickness = 0.5f * render.context2d.stack.thickness;

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

		v1      = normals.Last();
		v2      = normals[0];

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

		v       = normals[0];
		n       = kVec2(-v.y, v.x);
		t       = thickness * n;
		prev_p  = path[0] + t;
		prev_q  = path[0] - t;

		v       = normals.Last();
		n       = kVec2(-v.y, v.x);
		t       = thickness * n;
		start_p = path.Last() + t;
		start_q = path.Last() - t;
	}

	for (imem index = 0; index + 1 < normals.count; ++index)
	{
		a  = path[index + 0];
		b  = path[index + 1];
		c  = path[index + 2];

		v1 = normals[index + 0];
		v2 = normals[index + 1];

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
	kArray<kVec2> &path = render.context2d.stack.scratch;

	if (path.count < 3)
	{
		path.Reset();
		return;
	}

	int triangle_count = (int)path.count - 2;
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

void kDrawPolygon(const kVec2 *vertices, u32 count, kVec4 color) { kDrawPolygon(vertices, count, 0, color); }

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

void kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color) { kDrawRectOutline(kVec3(pos, 0), dim, color); }

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

void kDrawCircleOutline(kVec3 pos, float radius, kVec4 color) { kDrawEllipseOutline(pos, radius, radius, color); }

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

void kDrawTexture(kTexture *texture, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTexture(kTexture *texture, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture *texture, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture *texture, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTexture(kTexture *texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTexture(kTexture *texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRect(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture *texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCentered(kTexture *texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kDrawRectCentered(pos, dim, rect, color);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureMasked(kTexture *texture, kTexture *mask, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCenteredMasked(kTexture *texture, kTexture *mask, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureMasked(kTexture *texture, kTexture *mask, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, kTextureType_Color);
	kPushTexture(mask, kTextureType_MaskSDF);
	kDrawRect(pos, dim, color);
	kPopTexture(kTextureType_MaskSDF);
	kPopTexture(kTextureType_Color);
}

void kDrawTextureCenteredMasked(kTexture *texture, kTexture *mask, kVec2 pos, kVec2 dim, kVec4 color)
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
	if (codepoint >= font->mincp && codepoint <= font->maxcp)
	{
		u32 index = codepoint - font->mincp;
		u16 pos   = font->map[index];
		return &font->glyphs[pos];
	}
	return font->fallback;
}

kVec2 kAdjectCursorToBaseline(const kFont *font, kVec2 cursor, float scale)
{
	cursor.y += (float)(font->ascent) * scale;
	return cursor;
}

bool kCalculateGlyphMetrics(const kFont *font, u32 codepoint, float scale, kVec2 *cursor, kVec2 *rpos, kVec2 *rsize,
                            kRect *rect)
{
	if (codepoint == '\r') return false;

	if (codepoint == '\n')
	{
		cursor->x = 0;
		cursor->y += (float)font->linegap * scale;
		return false;
	}

	kGlyph *glyph = kFindFontGlyph(font, codepoint);

	rpos->x       = (cursor->x + scale * (float)glyph->bearing.x);
	rpos->y       = (cursor->y - scale * (float)(glyph->bearing.y + glyph->size.y));

	rsize->x      = (scale * (float)glyph->size.x);
	rsize->y      = (scale * (float)glyph->size.y);
	*rect         = glyph->rect;

	cursor->x += scale * glyph->advance.x;

	return true;
}

void kDrawTextQuad(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale)
{
	kPushTexture(render.builtin.textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->texture, kTextureType_MaskSDF);

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
	kPushTexture(render.builtin.textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->texture, kTextureType_MaskSDF);

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
	kPushTexture(render.builtin.textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->texture, kTextureType_MaskSDF);

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
	kPushTexture(render.builtin.textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->texture, kTextureType_MaskSDF);

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
	kFont *font = render.builtin.font;
	kDrawTextQuad(codepoint, font, pos, color, scale);
}

void kDrawTextQuad(u32 codepoint, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuad(codepoint, font, pos, color, scale);
}

void kDrawTextQuadRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuadRotated(codepoint, font, pos, angle, color, scale);
}

void kDrawTextQuadRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuadRotated(codepoint, font, pos, angle, color, scale);
}

void kDrawTextQuadCentered(u32 codepoint, kVec3 pos, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuadCentered(codepoint, font, pos, color, scale);
}

void kDrawTextQuadCentered(u32 codepoint, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuadCentered(codepoint, font, pos, color, scale);
}

void kDrawTextQuadCenteredRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuadCenteredRotated(codepoint, font, pos, angle, color, scale);
}

void kDrawTextQuadCenteredRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawTextQuadCenteredRotated(codepoint, font, pos, angle, color, scale);
}

kVec2 kCalculateText(kString text, const kFont *font, float scale)
{
	u32   codepoint;
	u8   *ptr = text.begin();
	u8   *end = text.end();

	kVec2 rpos;
	kVec2 rsize;
	kRect rect;

	kVec2 cursor  = kAdjectCursorToBaseline(font, kVec2(0), scale);
	float ycounts = 1.0f;

	while (ptr < end)
	{
		ptr += kUTF8ToCodepoint(ptr, end, &codepoint);
		if (codepoint == '\n') ycounts += 1;
		kCalculateGlyphMetrics(font, codepoint, scale, &cursor, &rpos, &rsize, &rect);
	}

	kVec2 r;
	r.x = cursor.x;
	r.y = ycounts * scale * font->size;

	return r;
}

kVec2 kCalculateText(kString text, float scale)
{
	kFont *font = render.builtin.font;
	return kCalculateText(text, font, scale);
}

void kDrawText(kString text, kVec3 pos, const kFont *font, kVec4 color, float scale)
{
	kPushTexture(render.builtin.textures[kTextureType_Color], kTextureType_Color);
	kPushTexture(font->texture, kTextureType_MaskSDF);

	u32   codepoint;
	u8   *ptr  = text.begin();
	u8   *end  = text.end();

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
	kFont *font = render.builtin.font;
	kDrawText(text, pos, font, color, scale);
}

void kDrawText(kString text, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = render.builtin.font;
	kDrawText(text, pos, font, color, scale);
}

//
//
//

void kCreateRenderContext(const kRenderSpec &spec, kTexture *textures[kTextureType_Count], kFont *font)
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
		render.builtin.textures[i] = textures[i];
	render.builtin.font              = font;

	render.context2d.stack.thickness = spec.thickness;

	render.context2d.vertices.Reserve(spec.vertices);
	render.context2d.indices.Reserve(spec.indices);
	render.context2d.commands.Reserve(spec.commands);
	render.context2d.scenes.Reserve(spec.passes);
	render.context2d.rects.Reserve(spec.rects);
	for (u32 i = 0; i < kTextureType_Count; ++i)
		render.context2d.textures[i].Reserve(spec.textures);

	render.context2d.stack.rects.Reserve((imem)spec.rects << 1);
	render.context2d.stack.transforms.Reserve((imem)spec.transforms << 1);
	for (u32 i = 0; i < kTextureType_Count; ++i)
		render.context2d.stack.textures[i].Reserve((imem)spec.textures << 1);
	render.context2d.stack.scratch.Reserve(spec.scratch);
	render.context2d.stack.blends.Reserve(32);

	render.context2d.stack.rects.Add(kRect{});
	render.context2d.stack.transforms.Add(kIdentity3x3());
	for (u32 i = 0; i < kTextureType_Count; ++i)
		render.context2d.stack.textures[i].Add(render.builtin.textures[i]);
	render.context2d.stack.blends.Add(kBlendMode_Normal);
	render.context2d.stack.filters.Add(kTextureFilter_Linear);

	kResetFrame();
}

void kDestroyRenderContext(void)
{
	kFree(&render.context2d.vertices);
	kFree(&render.context2d.indices);
	kFree(&render.context2d.commands);
	kFree(&render.context2d.scenes);
	kFree(&render.context2d.rects);
	for (u32 i = 0; i < kTextureType_Count; ++i)
		kFree(&render.context2d.textures[i]);

	kFree(&render.context2d.stack.rects);
	kFree(&render.context2d.stack.transforms);
	for (u32 i = 0; i < kTextureType_Count; ++i)
		kFree(&render.context2d.stack.textures[i]);
	kFree(&render.context2d.stack.scratch);
	kFree(&render.context2d.stack.blends);

	memset(&render, 0, sizeof(render));
}

void kResetFrame(void)
{
	render.context2d.vertices.count   = 0;
	render.context2d.indices.count    = 0;
	render.context2d.commands.count   = 0;
	render.context2d.scenes.count     = 0;
	render.context2d.rects.count      = 0;
	for (u32 i = 0; i < kTextureType_Count; ++i)
		render.context2d.textures[i].count = 0;

	render.context2d.stack.rects.count      = 1;
	render.context2d.stack.transforms.count = 1;
	for (u32 i = 0; i < kTextureType_Count; ++i)
		render.context2d.stack.textures[i].count = 1;
	render.context2d.stack.blends.count  = 1;
	render.context2d.stack.filters.count = 1;
	render.context2d.stack.scratch.count = 0;
	render.context2d.stack.thickness     = 1.0f;
}

static void kCalculateTotalMegaBytes(kRenderMemoryUsage *usage)
{
	umem total = 0;

	total += usage->vertices * sizeof(kVertex2D);
	total += usage->indices * sizeof(kIndex2D);
	for (int i = 0; i < kTextureType_Count; ++i)
		total += usage->textures[i] * sizeof(kTexture *);
	total += usage->rects * sizeof(kRect);
	total += usage->passes * sizeof(kRenderScene2D);
	total += usage->commands * sizeof(kRenderCommand2D);

	for (int i = 0; i < kTextureType_Count; ++i)
		total += usage->stack.textures[i] * sizeof(kTexture *);
	total += usage->stack.transforms * sizeof(kMat4);
	total += usage->stack.rects * sizeof(kRect);
	total += usage->stack.blends * sizeof(kBlendMode);
	total += usage->stack.scratch * sizeof(kVec2);

	usage->megabytes = (float)((double)total / (double)(1024 * 1024));
}

static void kUpdateMemoryStatictics(void)
{
	kRenderMemoryUsage &allocated = render.memory.allocated;

	allocated.vertices            = render.context2d.vertices.allocated;
	allocated.indices             = render.context2d.indices.allocated;
	for (int i = 0; i < kTextureType_Count; ++i)
		allocated.textures[i] = render.context2d.textures[i].allocated;
	allocated.rects      = render.context2d.rects.allocated;
	allocated.passes     = render.context2d.scenes.allocated;
	allocated.commands   = render.context2d.commands.allocated;

	for (int i = 0; i < kTextureType_Count; ++i)
		allocated.stack.textures[i] = render.context2d.stack.textures[i].allocated;
	allocated.stack.transforms = render.context2d.stack.transforms.allocated;
	allocated.stack.rects      = render.context2d.stack.rects.allocated;
	allocated.stack.blends     = render.context2d.stack.blends.allocated;
	allocated.stack.scratch    = render.context2d.stack.scratch.allocated;

	kCalculateTotalMegaBytes(&allocated);

	kRenderMemoryUsage &last_frame = render.memory.last_frame;

	last_frame.vertices            = render.context2d.vertices.count;
	last_frame.indices             = render.context2d.indices.count;
	for (int i = 0; i < kTextureType_Count; ++i)
		last_frame.textures[i] = render.context2d.textures[i].count;
	last_frame.rects      = render.context2d.rects.count;
	last_frame.passes     = render.context2d.scenes.count;
	last_frame.commands   = render.context2d.commands.count;

	for (int i = 0; i < kTextureType_Count; ++i)
		last_frame.stack.textures[i] = render.context2d.stack.textures[i].count;
	last_frame.stack.transforms = render.context2d.stack.transforms.count;
	last_frame.stack.rects      = render.context2d.stack.rects.count;
	last_frame.stack.blends     = render.context2d.stack.blends.count;
	last_frame.stack.scratch    = render.context2d.stack.scratch.count;

	kCalculateTotalMegaBytes(&last_frame);

	kRenderMemoryUsage &max_used = render.memory.max_used;

	max_used.vertices            = kMax(max_used.vertices, last_frame.vertices);
	max_used.indices             = kMax(max_used.indices, last_frame.indices);
	for (int i = 0; i < kTextureType_Count; ++i)
		max_used.textures[i] = kMax(max_used.textures[i], last_frame.textures[i]);
	max_used.rects      = kMax(max_used.rects, last_frame.rects);
	max_used.passes     = kMax(max_used.passes, last_frame.passes);
	max_used.commands   = kMax(max_used.commands, last_frame.commands);

	for (int i = 0; i < kTextureType_Count; ++i)
		max_used.stack.textures[i] = kMax(max_used.stack.textures[i], last_frame.stack.textures[i]);
	max_used.stack.transforms = kMax(max_used.stack.transforms, last_frame.stack.transforms);
	max_used.stack.rects      = kMax(max_used.stack.rects, last_frame.stack.rects);
	max_used.stack.blends     = kMax(max_used.stack.blends, last_frame.stack.blends);
	max_used.stack.scratch    = kMax(max_used.stack.scratch, last_frame.stack.scratch);

	kCalculateTotalMegaBytes(&max_used);
}

void kGetFrameData(kRenderFrame2D *frame)
{
	frame->scenes   = render.context2d.scenes;
	frame->commands = render.context2d.commands;
	for (int i = 0; i < kTextureType_Count; ++i)
		frame->textures[i] = render.context2d.textures[i];
	frame->rects      = render.context2d.rects;
	frame->vertices   = render.context2d.vertices;
	frame->indices    = render.context2d.indices;

	kUpdateMemoryStatictics();
}

const kRenderMemoryStatistics *kGetRenderMemoryStatistics(void) { return &render.memory; }
