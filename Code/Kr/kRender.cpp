#include "kArray.h"
#include "kRender.h"

#include <string.h>

//
//
//

typedef struct kRenderState2D
{
	i32             vertex;
	u32             index;
	u32             count;
	u32             next;
	u32             param;
	u32             command;
	u32             pass;
	kTexture        render_target;
	kTexture        depth_stenil;
	kViewport       viewport;
	u32             flags;
	kRenderClear2D  clear;
	kResolveTexture resolve;
	kTextureFilter  filter;
} kRenderState2D;

typedef struct kRenderContext2D
{
	kRenderState2D           state;

	kArray<kVertex2D>        vertices;
	kArray<kIndex2D>         indices;
	kArray<kRenderParam2D>   params;
	kArray<kRenderCommand2D> commands;
	kArray<kRenderPass2D>    passes;

	float                    thickness;
	kArray<kRect>            rects;
	kArray<kMat4>            transforms;
	kArray<kTexture>         textures[K_MAX_TEXTURE_SLOTS];
	kArray<kRenderShader2D>  shaders;

	kArray<kVec2>            builder;
} kRenderContext2D;

typedef struct kRenderContextTarget
{
	u32      multisamples;
	kTexture msaa;
	kTexture msaa_resolved;
} kRenderContextTarget;

typedef struct kRenderContextBuiltin
{
	kTexture             texture;
	kFont                font;
	kRenderContextTarget target;
} kRenderContextBuiltin;

typedef struct kRenderContextFrame
{
	kRenderFeatures features;
	kTexture        target;
	kTexture        resolve;
} kRenderContextFrame;

typedef struct kRenderContext
{
	kRenderContextFrame   frame;
	kRenderContext2D      context2d;
	kRenderContextBuiltin builtin;
	kRenderBackend        backend;
	uint                  req_features;
} kRenderContext;

static kRenderContext render;

static float          Sines[K_MAX_CIRCLE_SEGMENTS];
static float          Cosines[K_MAX_CIRCLE_SEGMENTS];

static int            FallbackFontHeight = 32;
static int            FallbackFontWidth  = (3 * FallbackFontHeight) / 2;

static const kGlyph   FallbackGlyph      = {
    kRect{kVec2(0), kVec2(0)},
    kVec2i(0),
    kVec2i(FallbackFontWidth, FallbackFontHeight),
    kVec2i(FallbackFontWidth + 2, FallbackFontHeight + 2),
};

//
//
//

static kRenderContext2D *kGetRenderContext2D(void)
{
	return &render.context2d;
}

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

static void kDestroyRenderTargetMSAA(kRenderContextTarget *target)
{
	if (target->msaa)
	{
		render.backend.destroy_texture(target->msaa);
		target->msaa = nullptr;
	}

	if (target->msaa_resolved)
	{
		render.backend.destroy_texture(target->msaa_resolved);
		target->msaa_resolved = nullptr;
	}

	target->multisamples = 0;
}

static void kRecreateRenderTargetMSAA(kRenderContextTarget *target, u32 w, u32 h, u32 multisamples)
{
	kAssert(w && h && multisamples);

	if (target->msaa && target->msaa_resolved)
	{
		u32 cw, ch;
		render.backend.texture_size(target->msaa, &cw, &ch);

		if (target->multisamples == multisamples && cw == w && ch == h)
			return;
	}

	kDestroyRenderTargetMSAA(target);

	kTextureSpec spec     = {};
	spec.format           = kFormat_RGBA16_FLOAT;
	spec.width            = w;
	spec.height           = h;
	spec.bind_flags       = kBind_RenderTarget;
	spec.num_samples      = multisamples;
	spec.usage            = kUsage_Default;

	kTextureSpec resolved = spec;
	resolved.num_samples  = 1;
	resolved.bind_flags   = kBind_ShaderResource;

	target->multisamples  = multisamples;
	target->msaa          = render.backend.create_texture(spec);
	target->msaa_resolved = render.backend.create_texture(resolved);
}

static void kResizeSwapChainTargetMSAA(kSwapChain swap_chain, u32 w, u32 h, void *data)
{
	if (w && h)
	{
		if (!render.builtin.target.multisamples)
			render.builtin.target.multisamples = 1;
		kRecreateRenderTargetMSAA(&render.builtin.target, w, h, render.builtin.target.multisamples);
	}
}

static void kDestroySwapChainRenderTargetMSAA(void)
{
	kDestroyRenderTargetMSAA(&render.builtin.target);
}

static void kRecreateSwapChainRenderTargetMSAA(void)
{
	render.builtin.target.multisamples = render.frame.features.msaa.samples;

	u32        w, h;
	kSwapChain swap_chain = render.backend.window_swap_chain();
	kTexture   target     = render.backend.swap_chain_target(swap_chain);
	render.backend.texture_size(target, &w, &h);
	kResizeSwapChainTargetMSAA(swap_chain, w, h, 0);
	render.backend.swap_chain_resize_cb(swap_chain, kResizeSwapChainTargetMSAA, 0);
}

static void kCreateBuiltinResources(void);
static void kDestroyBuiltinResources(void);

//
//
//

void kSetRenderFeatures(uint flags)
{
	render.req_features = flags;
}

bool kIsRenderFeatureEnabled(uint flags)
{
	return (render.req_features & flags);
}

void kEnableRenderFeatures(uint flags)
{
	render.req_features |= flags;
}

void kDisableRenderFeatures(uint flags)
{
	render.req_features &= ~flags;
}

u32 kGetMSAASampleCount(void)
{
	if (render.req_features & kRenderFeature_MSAA)
		return render.frame.features.msaa.samples;
	return 1;
}

void kSetMSAASampleCount(u32 count)
{
	render.frame.features.msaa.samples = count;
}

void kGetRenderFeatures(kRenderFeatures *features)
{
	*features = render.frame.features;
}

void kCreateRenderContext(kRenderBackend backend, const kRenderFeatures &features, const kRenderSpec &spec)
{
	render.backend = backend;

	for (u32 i = 0; i < K_MAX_CIRCLE_SEGMENTS; ++i)
	{
		float theta = ((float)i / (float)(K_MAX_CIRCLE_SEGMENTS - 1));
		Cosines[i]  = kCos(theta);
		Sines[i]    = kSin(theta);
	}

	Cosines[K_MAX_CIRCLE_SEGMENTS - 1] = 1;
	Sines[K_MAX_CIRCLE_SEGMENTS - 1]   = 0;

	render.frame.features              = features;
	render.req_features                = features.flags;

	if (kIsRenderFeatureEnabled(kRenderFeature_MSAA))
	{
		if (!render.frame.features.msaa.samples)
			render.frame.features.msaa.samples = 1;
	}

	kCreateBuiltinResources();

	render.context2d.thickness = spec.thickness;

	render.context2d.vertices.Reserve(spec.vertices);
	render.context2d.indices.Reserve(spec.indices);
	render.context2d.params.Reserve(spec.params);
	render.context2d.commands.Reserve(spec.commands);
	render.context2d.passes.Reserve(spec.passes);

	render.context2d.rects.Reserve(spec.rects);
	render.context2d.transforms.Reserve(spec.transforms);

	for (u32 i = 0; i < K_MAX_TEXTURE_SLOTS; ++i)
		render.context2d.textures[i].Reserve(spec.textures);

	render.context2d.builder.Reserve(spec.builder);

	render.context2d.rects.Add(kRect{});
	render.context2d.transforms.Add(kIdentity());

	render.context2d.textures[0].Add(render.builtin.texture);
	render.context2d.textures[1].Add(render.builtin.texture);

	kRenderShader2D shader = {};
	shader.blend           = kBlendMode_Alpha;
	render.context2d.shaders.Add(shader);

	kBeginFrame();
}

void kDestroyRenderContext(void)
{
	kDestroyBuiltinResources();

	kFree(&render.context2d.vertices);
	kFree(&render.context2d.indices);
	kFree(&render.context2d.params);
	kFree(&render.context2d.commands);
	kFree(&render.context2d.passes);

	kFree(&render.context2d.rects);
	kFree(&render.context2d.transforms);

	for (u32 i = 0; i < K_MAX_TEXTURE_SLOTS; ++i)
		kFree(&render.context2d.textures[i]);

	kFree(&render.context2d.builder);

	memset(&render, 0, sizeof(render));
}

void kBeginFrame(void)
{
	memset(&render.context2d.state, 0, sizeof(render.context2d.state));

	render.context2d.vertices.count   = 0;
	render.context2d.indices.count    = 0;
	render.context2d.params.count     = 0;
	render.context2d.commands.count   = 0;
	render.context2d.passes.count     = 0;
	render.context2d.rects.count      = 1;
	render.context2d.transforms.count = 1;

	for (u32 i = 0; i < K_MAX_TEXTURE_SLOTS; ++i)
		render.context2d.textures[i].count = 1;

	render.context2d.shaders.count = 1;

	//
	//
	//

	uint requested = render.frame.features.flags ^ render.req_features;

	if (requested & kRenderFeature_MSAA)
	{
		if (render.req_features & kRenderFeature_MSAA)
		{
			kRecreateSwapChainRenderTargetMSAA();
		}
		else
		{
			kDestroySwapChainRenderTargetMSAA();
		}
	}

	render.frame.features.flags = render.req_features;

	if (render.frame.features.flags & kRenderFeature_MSAA)
	{
		if (render.builtin.target.multisamples != render.frame.features.msaa.samples)
		{
			kRecreateSwapChainRenderTargetMSAA();
		}
		render.frame.target  = render.builtin.target.msaa;
		render.frame.resolve = render.builtin.target.msaa_resolved;
	}
	else
	{
		kSwapChain swap_chain = render.backend.window_swap_chain();
		render.frame.target   = render.backend.swap_chain_target(swap_chain);
		render.frame.resolve  = nullptr;
	}
}

void kEndFrame(void)
{
	if (render.frame.features.flags & kRenderFeature_MSAA)
	{
		kSwapChain swap_chain = render.backend.window_swap_chain();
		kTexture   target     = render.backend.swap_chain_target(swap_chain);

		// TODO: Optimize
		kBeginRenderPass(target);
		kBeginBlendMode(kBlendMode_None);
		kPushTexture(render.frame.resolve, 0);
		kDrawRect(kVec2(-1), kVec2(2), kRect(0, 1, 1, 0), kVec4(1));
		kEndBlendMode();
		kEndRenderPass();
	}

	kRenderData2D data;
	kGetRenderData2D(&data);
	render.backend.execute_commands(data);
}

void kGetRenderData2D(kRenderData2D *data)
{
	data->passes   = render.context2d.passes;
	data->vertices = render.context2d.vertices;
	data->indices  = render.context2d.indices;
}

void kBeginRenderPass(kTexture texture, kTexture depth_stencil, uint flags, kVec4 color, float depth,
                      kResolveTexture *resolve)
{
	kRenderContext2D *ctx = kGetRenderContext2D();
	kAssert(ctx->state.render_target == 0 && ctx->state.depth_stenil == 0);

	u32 w, h;
	render.backend.texture_size(texture, &w, &h);

	kViewport viewport;
	viewport.x               = 0;
	viewport.y               = 0;
	viewport.w               = (float)w;
	viewport.h               = (float)h;
	viewport.n               = 0;
	viewport.f               = 1;

	ctx->state.render_target = texture;
	ctx->state.depth_stenil  = depth_stencil;
	ctx->state.viewport      = viewport;
	ctx->state.clear.color   = color;
	ctx->state.clear.depth   = depth;
	ctx->state.clear.stencil = 0;
	ctx->state.flags         = flags;
	ctx->state.command       = (u32)ctx->commands.count;

	if (resolve)
	{
		ctx->state.resolve = *resolve;
	}
	else
	{
		ctx->state.resolve = {};
	}

	kPushRectEx(0, 0, (float)w, (float)h);
}

void kBeginDefaultRenderPass(uint flags, kVec4 color, float depth)
{
	kResolveTexture resolve;
	resolve.target  = render.frame.resolve;
	resolve.format  = kFormat_RGBA16_FLOAT;
	kTexture target = render.frame.target;
	kBeginRenderPass(target, nullptr, flags, color, depth, &resolve);
}

void kEndRenderPass(void)
{
	kPopRect();
	kFlushRenderCommand();

	kRenderContext2D *ctx = kGetRenderContext2D();

	if (ctx->state.command != ctx->commands.count)
	{
		kRenderPass2D *pass  = ctx->passes.Add();
		pass->rt             = ctx->state.render_target;
		pass->ds             = ctx->state.depth_stenil;
		pass->viewport       = ctx->state.viewport;
		pass->flags          = ctx->state.flags;
		pass->clear          = ctx->state.clear;
		pass->commands.data  = ctx->commands.data + ctx->state.command;
		pass->commands.count = ctx->commands.count - ctx->state.command;
		pass->resolve        = ctx->state.resolve;
		ctx->state.command   = (u32)ctx->commands.count;
	}

	ctx->state.render_target = nullptr;
	ctx->state.depth_stenil  = nullptr;
	ctx->state.flags         = 0;
	ctx->state.resolve       = {};
}

void kBeginCameraRect(float left, float right, float bottom, float top)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kMat4             proj = kOrthographicLH(left, right, top, bottom, -1, 1);
	ctx->transforms.Add();
	kSetTransform(proj);
}

void kBeginCamera(float aspect_ratio, float height)
{
	float width = aspect_ratio * height;

	float arx   = aspect_ratio;
	float ary   = 1;

	if (width < height)
	{
		ary = 1.0f / arx;
		arx = 1.0f;
	}

	float half_height = 0.5f * height;

	kBeginCameraRect(-half_height * arx, half_height * arx, -half_height * ary, half_height * ary);
}

void kEndCamera(void)
{
	kPopTransform();
}

void kLineThickness(float thickness)
{
	kRenderContext2D *ctx = kGetRenderContext2D();
	ctx->thickness        = thickness;
}

void kFlushRenderCommand(void)
{
	kFlushRenderParam();

	kRenderContext2D *ctx = kGetRenderContext2D();

	if (ctx->state.param == ctx->params.count)
		return;

	kRenderCommand2D *command = ctx->commands.Add();
	command->shader           = ctx->shaders.Last();
	command->filter           = ctx->state.filter;
	command->params.data      = ctx->params.data + ctx->state.param;
	command->params.count     = ctx->params.count - ctx->state.param;
	ctx->state.param          = (u32)ctx->params.count;
}

void kSetBlendMode(kBlendMode mode)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kRenderShader2D * prev = &ctx->shaders.Last();
	if (prev->blend != mode)
	{
		kFlushRenderCommand();
	}
	prev->blend = mode;
}

void kBeginBlendMode(kBlendMode mode)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kRenderShader2D   last = ctx->shaders.Last();
	ctx->shaders.Add(last);
	kSetBlendMode(mode);
}

void kEndBlendMode(void)
{
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->shaders.count;
	kAssert(count > 1);
	kSetBlendMode(ctx->shaders.data[count - 2].blend);
	ctx->shaders.Pop();
}

void kSetTextureFilter(kTextureFilter filter)
{
	kRenderContext2D *ctx = kGetRenderContext2D();
	if (filter == ctx->state.filter)
		return;

	kFlushRenderCommand();

	ctx->state.filter = filter;
}

void kFlushRenderParam(void)
{
	kRenderContext2D *ctx = kGetRenderContext2D();

	if (!ctx->state.count)
		return;

	kRenderParam2D *param = ctx->params.Add();

	for (u32 i = 0; i < K_MAX_TEXTURE_SLOTS; ++i)
		param->textures[i] = ctx->textures[i].Last();

	param->transform  = ctx->transforms.Last();
	param->rect       = ctx->rects.Last();
	param->vertex     = ctx->state.vertex;
	param->index      = ctx->state.index;
	param->count      = ctx->state.count;

	ctx->state.vertex = (i32)ctx->vertices.count;
	ctx->state.index  = (u32)ctx->indices.count;
	ctx->state.next   = 0;
	ctx->state.count  = 0;
}

void kSetTexture(kTexture texture, uint idx)
{
	kAssert(idx < K_MAX_TEXTURE_SLOTS);
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kTexture *        prev = &ctx->textures[idx].Last();
	if (*prev != texture)
	{
		kFlushRenderParam();
	}
	*prev = texture;
}

void kPushTexture(kTexture texture, uint idx)
{
	kAssert(idx < K_MAX_TEXTURE_SLOTS);
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kTexture          last = ctx->textures[idx].Last();
	ctx->textures[idx].Add(last);
	kSetTexture(texture, idx);
}

void kPopTexture(uint idx)
{
	kAssert(idx < K_MAX_TEXTURE_SLOTS);
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->textures[idx].count;
	kAssert(count > 1);
	kSetTexture(ctx->textures[idx].data[count - 2], idx);
	ctx->textures[idx].Pop();
}

void kSetRect(kRect rect)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kRect *           prev = &ctx->rects.Last();
	if (memcmp(prev, &rect, sizeof(rect)) != 0)
	{
		kFlushRenderParam();
	}
	*prev = rect;
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
	kRect             last = ctx->rects.Last();
	ctx->rects.Add(last);
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
	imem              count = ctx->rects.count;
	kAssert(count > 1);
	kSetRect(ctx->rects.data[count - 2]);
	ctx->rects.Pop();
}

void kSetTransform(const kMat4 &transform)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kMat4 *           prev = &ctx->transforms.Last();
	if (memcmp(prev, &transform, sizeof(kMat4)) != 0)
	{
		kFlushRenderParam();
	}
	*prev = transform;
}

void kPushTransform(const kMat4 &transform)
{
	kRenderContext2D *ctx  = kGetRenderContext2D();
	kMat4             last = ctx->transforms.Last();
	ctx->transforms.Add(last);
	kMat4 t = last * transform;
	kSetTransform(t);
}

void kPushTransform(kVec2 pos, float angle)
{
	kVec2 arm = kArm(angle);

	kMat4 mat;
	mat.rows[0] = kVec4(arm.x, -arm.y, 0.0f, pos.x);
	mat.rows[1] = kVec4(arm.y, arm.x, 0.0f, pos.y);
	mat.rows[2] = kVec4(0.0f, 0.0f, 1.0f, 0.0f);
	mat.rows[2] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	kPushTransform(mat);
}

void kPopTransform(void)
{
	kRenderContext2D *ctx   = kGetRenderContext2D();
	imem              count = ctx->transforms.count;
	kAssert(count > 1);
	kSetTransform(ctx->transforms.data[count - 2]);
	ctx->transforms.Pop();
}

//
//
//

static void kCommitPrimitive(u32 vertex, u32 index)
{
	kRenderContext2D *ctx = kGetRenderContext2D();
	ctx->state.count += index;
	ctx->state.next += vertex;
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

	kIndex2D *idx  = render.context2d.indices.Extend(3);
	idx[0]         = render.context2d.state.next + 0;
	idx[1]         = render.context2d.state.next + 1;
	idx[2]         = render.context2d.state.next + 2;

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

	kIndex2D *idx  = render.context2d.indices.Extend(6);
	idx[0]         = render.context2d.state.next + 0;
	idx[1]         = render.context2d.state.next + 1;
	idx[2]         = render.context2d.state.next + 2;
	idx[3]         = render.context2d.state.next + 0;
	idx[4]         = render.context2d.state.next + 2;
	idx[5]         = render.context2d.state.next + 3;

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
	auto uv_a = rect.min;
	auto uv_b = kVec2(rect.min.x, rect.max.y);
	auto uv_c = rect.max;
	auto uv_d = kVec2(rect.max.x, rect.min.y);
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

	float thickness = render.context2d.thickness * 0.5f;
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
	if (render.context2d.builder.count)
	{
		if (kIsNull(render.context2d.builder.Last() - a))
		{
			return;
		}
	}
	render.context2d.builder.Add(a);
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
	imem index    = render.context2d.builder.count;
	int  segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	if (render.context2d.builder.Resize(render.context2d.builder.count + segments + 1))
		kBuildBezierQuadratic(a, b, c, &render.context2d.builder[index], segments);
}

void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d)
{
	imem index    = render.context2d.builder.count;
	int  segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	if (render.context2d.builder.Resize(render.context2d.builder.count + segments + 1))
		kBuildBezierCubic(a, b, c, d, &render.context2d.builder[index], segments);
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
	kArray<kVec2> &path = render.context2d.builder;

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

	float thickness = 0.5f * render.context2d.thickness;

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
	kArray<kVec2> &path = render.context2d.builder;

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
	kPushTexture(texture, 0);
	kDrawRect(pos, dim, color);
	kPopTexture(0);
}

void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRect(pos, dim, color);
	kPopTexture(0);
}

void kDrawTextureCentered(kTexture texture, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(0);
}

void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(0);
}

void kDrawTexture(kTexture texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRect(pos, dim, rect, color);
	kPopTexture(0);
}

void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRect(pos, dim, rect, color);
	kPopTexture(0);
}

void kDrawTextureCentered(kTexture texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRectCentered(pos, dim, rect, color);
	kPopTexture(0);
}

void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color)
{
	kPushTexture(texture, 0);
	kDrawRectCentered(pos, dim, rect, color);
	kPopTexture(0);
}

void kDrawTextureMasked(kTexture texture, kTexture mask, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kPushTexture(mask, 1);
	kDrawRect(pos, dim, color);
	kPopTexture(1);
	kPopTexture(0);
}

void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec3 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kPushTexture(mask, 1);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(1);
	kPopTexture(0);
}

void kDrawTextureMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kPushTexture(mask, 1);
	kDrawRect(pos, dim, color);
	kPopTexture(1);
	kPopTexture(0);
}

void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color)
{
	kPushTexture(texture, 0);
	kPushTexture(mask, 1);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(1);
	kPopTexture(0);
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

kGlyph *kFindFontGlyph(kFont *font, u32 codepoint)
{
	if (codepoint >= font->mincp && codepoint <= font->maxcp)
	{
		u32 index = codepoint - font->mincp;
		u16 pos   = font->map[index];
		return &font->glyphs[pos];
	}
	return font->fallback;
}

float kCalculateText(kString text, kFont *font, float scale)
{
	float dist = 0;

	u32   codepoint;
	u8 *  ptr = text.begin();
	u8 *  end = text.end();

	while (ptr < end)
	{
		int len = kUTF8ToCodepoint(ptr, end, &codepoint);
		ptr += len;

		auto info = kFindFontGlyph(font, codepoint);
		dist += scale * (float)info->advance.x;
	}

	return dist;
}

float kCalculateText(kString text, float scale)
{
	kFont *font = &render.builtin.font;
	return kCalculateText(text, font, scale);
}

void kDrawText(kString text, kVec3 pos, kFont *font, kVec4 color, float scale)
{
	kPushTexture(font->texture, 0);

	u32 codepoint;
	u8 *ptr = text.begin();
	u8 *end = text.end();

	while (ptr < end)
	{
		int len = kUTF8ToCodepoint(ptr, end, &codepoint);
		ptr += len;

		kGlyph *glyph    = kFindFontGlyph(font, codepoint);

		kVec2   bearing  = kVec2((float)glyph->bearing.x, (float)glyph->bearing.y);
		kVec2   size     = kVec2((float)glyph->size.x, (float)glyph->size.y);

		kVec3   draw_pos = pos;
		draw_pos.xy += scale * bearing;
		kDrawRect(draw_pos, scale * size, glyph->rect, color);
		pos.x += scale * (float)glyph->advance.x;
	}

	kPopTexture(0);
}

void kDrawText(kString text, kVec2 pos, kFont *font, kVec4 color, float scale)
{
	kDrawText(text, kVec3(pos, 0), font, color, scale);
}

void kDrawText(kString text, kVec3 pos, kVec4 color, float scale)
{
	kFont *font = &render.builtin.font;
	kDrawText(text, pos, font, color, scale);
}

void kDrawText(kString text, kVec2 pos, kVec4 color, float scale)
{
	kFont *font = &render.builtin.font;
	kDrawText(text, pos, font, color, scale);
}

//
//
//

#include "Font/kFont.h"

static void kCreateBuiltinResources(void)
{
	{
		u8           pixels[]  = {0xff, 0xff, 0xff, 0xff};

		kTextureSpec spec      = {};
		spec.width             = 1;
		spec.height            = 1;
		spec.num_samples       = 1;
		spec.pitch             = 1 * sizeof(u32);
		spec.format            = kFormat_RGBA8_UNORM;
		spec.bind_flags        = kBind_ShaderResource;
		spec.usage             = kUsage_Default;
		spec.pixels            = pixels;

		render.builtin.texture = render.backend.create_texture(spec);
	}

	{
		kFont *      font = &render.builtin.font;

		kTextureSpec spec = {};
		spec.num_samples  = 1;
		spec.format       = kFormat_RGBA8_UNORM;
		spec.bind_flags   = kBind_ShaderResource;
		spec.usage        = kUsage_Default;
		spec.width        = kEmFontAtlasWidth;
		spec.height       = kEmFontAtlasHeight;
		spec.pitch        = kEmFontAtlasWidth * 4;
		spec.pixels       = (u8 *)kEmFontAtlasPixels;

		font->texture     = render.backend.create_texture(spec);
		if (!font->texture)
		{
			font->texture  = render.builtin.texture;
			font->map      = nullptr;
			font->glyphs   = nullptr;
			font->fallback = (kGlyph *)&FallbackGlyph;
			font->height   = FallbackFontHeight;
			font->mincp    = 0;
			font->maxcp    = 0;
			font->count    = 0;
			return;
		}

		font->map      = (u16 *)kEmFontGlyphMap;
		font->glyphs   = (kGlyph *)kEmFontGlyphs;
		font->fallback = (kGlyph *)&font->glyphs[kEmFontGlyphCount - 1];
		font->height   = kEmFontHeight;
		font->mincp    = kEmFontMinCodepoint;
		font->maxcp    = kEmFontMaxCodepoint;
		font->count    = kEmFontGlyphCount;
	}

	if (render.frame.features.flags & kRenderFeature_MSAA)
	{
		kRecreateSwapChainRenderTargetMSAA();
	}
}

static void kDestroyBuiltinResources(void)
{
	if (render.builtin.font.texture != render.builtin.texture)
	{
		render.backend.destroy_texture(render.builtin.font.texture);
	}

	if (render.builtin.texture)
	{
		render.backend.destroy_texture(render.builtin.texture);
	}

	kDestroySwapChainRenderTargetMSAA();
}
