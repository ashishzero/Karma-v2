#pragma once
#include "kArray.h"
#include "kRender.h"

#include <string.h>

//
//
//

typedef struct kRenderParamState2D {
	i32 vertex;
	u32 index;
	u32 count;
	u32 next;
} kRenderParamState2D;

typedef struct kRenderCommandState2D {
	u32 params;
	u32 count;
} kRenderCommandState2D;

typedef struct kRenderState2D {
	kRenderParamState2D   param;
	kRenderCommandState2D command;
} kRenderState2D;

typedef struct kRenderContext2D {
	kRenderState2D     state;

	kVertex2D      *   vertices;
	kIndex2D       *   indices;
	kRenderParam2D *   params;
	kRenderCommand2D * commands;

	float              thickness;

	kRect         *    rects;
	kMat4         *    transforms;
	kTexture      *    textures[K_MAX_TEXTURE_SLOTS];

	kVec2         *    builder;
} kRenderContext2D;

typedef struct kRenderContextBuiltin {
	kFont font;
} kRenderContextBuiltin;

typedef struct kRenderContext {
	kRenderContext2D      context2d;
	kRenderContextBuiltin builtin;
} kRenderContext;

static kRenderContext render;

static float          sines[K_MAX_CIRCLE_SEGMENTS];
static float          cosines[K_MAX_CIRCLE_SEGMENTS];

//
//
//

static kRenderContext2D *kGetRenderContext2D(void) {
	return &render.context2d;
}

//
//
//

static float kWrapTurns(float turns) {
	while (turns < 0.0f)
		turns += 1.0f;
	while (turns > 1.0f)
		turns -= 1.0f;
	return turns;
}

static float kSinLookupEx(int index, int segments) {
	float lookup = ((float)index / (float)segments) * (K_MAX_CIRCLE_SEGMENTS - 1);
	int a = (int)lookup;
	int b = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(sines[a], sines[b], lookup - a);
}

static float kCosLookupEx(int index, int segments) {
	float lookup = ((float)index / (float)segments) * (K_MAX_CIRCLE_SEGMENTS - 1);
	int a = (int)lookup;
	int b = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(cosines[a], cosines[b], lookup - a);
}

static float kSinLookup(float turns) {
	float lookup = turns * (K_MAX_CIRCLE_SEGMENTS - 1);
	int a = (int)lookup;
	int b = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(sines[a], sines[b], lookup - a);
}

static float kCosLookup(float turns) {
	float lookup = turns * (K_MAX_CIRCLE_SEGMENTS - 1);
	int a = (int)lookup;
	int b = (int)(a + 1) & (K_MAX_CIRCLE_SEGMENTS - 1);
	return kLerp(cosines[a], cosines[b], lookup - a);
}

//
//
//

//void BeginCameraRegion(float left, float right, float bottom, float top) {
//	Mat4 proj = OrthographicLH(left, right, top, bottom, -1, 1);
//	Push(&Render.Transform, Last(Render.Transform));
//	SetTransform(proj);
//}
//
//void BeginCameraRegion(float width, float height) {
//	float half_width  = 0.5f * width;
//	float half_height = 0.5f * height;
//	BeginCameraRegion(-half_width, half_width, -half_height, half_height);
//}
//
//void BeginCamera(float aspect_ratio, float height) {
//	float width = aspect_ratio * height;
//
//	float arx = aspect_ratio;
//	float ary = 1;
//
//	if (width < height) {
//		ary = 1.0f / arx;
//		arx = 1.0f;
//	}
//
//	float half_height = 0.5f * height;
//
//	BeginCameraRegion(-half_height * arx, half_height * arx, -half_height * ary, half_height * ary);
//}
//
//void EndCamera() {
//	PopTransform();
//}
//
//void SetLineThickness(float thickness) {
//	Render.Thickness = thickness;
//}
//
//void SetShader(PL_Shader *shader) {
//	PL_Shader *prev = Last(Render.Shader);
//	if (prev != shader) {
//		OnShaderTransition();
//	}
//	Last(Render.Shader) = shader;
//}
//
//void PushShader(PL_Shader *shader) {
//	Push(&Render.Shader, Last(Render.Shader));
//	SetShader(shader);
//}
//
//void PopShader() {
//	Assert(Render.Shader.Count > 1);
//	SetShader(Render.Shader[Render.Shader.Count - 2]);
//	Render.Shader.Count -= 1;
//}

void kFlushRenderData(void) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	if (ctx->state.param.count)
		return;

	kRenderParam2D *param   = kArrayAddEx(&ctx->params);

	for (u32 i = 0; i < K_MAX_TEXTURE_SLOTS; ++i)
		param->textures[i]  = kArrayLast(&ctx->textures[i]);

	param->transform        = kArrayLast(&ctx->transforms);
	param->rect             = kArrayLast(&ctx->rects);
	param->vertex           = ctx->state.param.vertex;
	param->index            = ctx->state.param.index;
	param->count            = ctx->state.param.count;

	ctx->state.param.vertex = (i32)kArrayCount(&ctx->vertices);
	ctx->state.param.index  = (u32)kArrayCount(&ctx->indices);
	ctx->state.param.count  = 0;

	ctx->state.command.count += 1;
}

void kSetTexture(kTexture texture, uint idx) {
	kAssert(idx < K_MAX_TEXTURE_SLOTS);
	kRenderContext2D *ctx = kGetRenderContext2D();
	kTexture *prev = &kArrayLast(&ctx->textures[idx]);
	if (prev->ptr != texture.ptr) {
		kFlushRenderData();
	}
	*prev = texture;
}

void kPushTexture(kTexture texture, uint idx) {
	kAssert(idx < K_MAX_TEXTURE_SLOTS);
	kRenderContext2D *ctx = kGetRenderContext2D();
	kTexture last = kArrayLast(&ctx->textures[idx]);
	kArrayAdd(&ctx->textures[idx], last);
	kSetTexture(texture, idx);
}

void kPopTexture(uint idx) {
	kAssert(idx < K_MAX_TEXTURE_SLOTS);
	kRenderContext2D *ctx = kGetRenderContext2D();
	imem count = kArrayCount(&ctx->textures[idx]);
	kAssert(count > 1);
	kSetTexture(ctx->textures[idx][count - 2], idx);
	kArrayPop(&ctx->textures[idx]);
}

void kSetRect(kRect rect) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kRect *prev = &kArrayLast(&ctx->rects);
	if (memcmp(prev, &rect, sizeof(rect)) != 0) {
		kFlushRenderData();
	}
	*prev = rect;
}

void kSetRectEx(float x, float y, float w, float h) {
	kRect rect;
	rect.min.x = x;
	rect.min.y = y;
	rect.max.x = x + w;
	rect.max.y = y + h;
	kSetRect(rect);
}

void kPushRect(kRect rect) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kRect last = kArrayLast(&ctx->rects);
	kArrayAdd(&ctx->rects, last);
	kSetRect(rect);
}

void kPushRectEx(float x, float y, float w, float h) {
	kRect rect;
	rect.min.x = x;
	rect.min.y = y;
	rect.max.x = x + w;
	rect.max.y = y + h;
	kPushRect(rect);
}

void PopRect(void) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	imem count = kArrayCount(&ctx->rects);
	kAssert(count > 1);
	kSetRect(ctx->rects[count - 2]);
	kArrayPop(&ctx->rects);
}

void kSetTransform(const kMat4 *transform) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kMat4 *prev = &kArrayLast(&ctx->transforms);
	if (memcmp(prev, transform, sizeof(kMat4)) != 0) {
		kFlushRenderData();
	}
	*prev = *transform;
}

void kPushTransform(const kMat4 *transform) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kMat4 last = kArrayLast(&ctx->transforms);
	kArrayAdd(&ctx->transforms, last);
	kMat4 t = kMul(&last, transform);
	kSetTransform(&t);
}

void kTranslateRotate(kVec2 pos, float angle) {
	kVec2 arm = kArm(angle);

	kMat4 mat;
	mat.rows[0] = (kVec4){ arm.x, -arm.y, 0.0f, pos.x };
	mat.rows[1] = (kVec4){ arm.y, arm.x, 0.0f, pos.y };
	mat.rows[2] = (kVec4){ 0.0f, 0.0f, 1.0f, 0.0f };
	mat.rows[2] = (kVec4){ 0.0f, 0.0f, 0.0f, 1.0f };
	kPushTransform(&mat);
}

void kPopTransform(void) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	imem count = kArrayCount(&ctx->transforms);
	kAssert(count > 1);
	kSetTransform(&ctx->transforms[count - 2]);
	kArrayPop(&ctx->transforms);
}

//
//
//

static void kCommitPrimitive(u32 vertex, u32 index) {
	kRenderContext2D *ctx   = kGetRenderContext2D();
	ctx->state.param.count += index;
	ctx->state.param.next  += vertex;
}

//
//
//

void kDrawTriangleUvGradient(
		kVec2 va, kVec2 vb, kVec2 vc,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 ca, kVec4 cb, kVec4 cc) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	kVertex2D *vtx = kArrayExtend(&ctx->vertices, 3);
	vtx[0].pos = va; vtx[0].tex = ta; vtx[0].col = ca;
	vtx[1].pos = vb; vtx[1].tex = tb; vtx[1].col = cb;
	vtx[2].pos = vc; vtx[2].tex = tc; vtx[2].col = cc;

	kIndex2D *idx = kArrayExtend(&ctx->indices, 3);
	idx[0] = ctx->state.param.next + 0;
	idx[1] = ctx->state.param.next + 1;
	idx[2] = ctx->state.param.next + 2;

	kCommitPrimitive(3, 3);
}

void kDrawTriangleUv(
		kVec2 a, kVec2 b, kVec2 c,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 col) {
	kDrawTriangleUvGradient(a, b, c, ta, tb, tc, col, col, col);
}

void kDrawTriangleGradient(
		kVec2 a, kVec2 b, kVec2 c,
		kVec4 ca, kVec4 cb, kVec4 cc) {
	kVec2 zero = { 0 };
	kDrawTriangleUvGradient(a, b, c, zero, zero, zero, ca, cb, cc);
}

void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec4 color) {
	kVec2 zero = { 0 };
	kDrawTriangleUvGradient(a, b, c, zero, zero, zero, color, color, color);
}

void kDrawQuadUvGradient(
	kVec2 va, kVec2 vb, kVec2 vc, kVec2 vd,
	kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	kVertex2D *vtx = kArrayExtend(&ctx->vertices, 4);
	vtx[0].pos = va; vtx[0].tex = ta; vtx[0].col = ca;
	vtx[1].pos = vb; vtx[1].tex = tb; vtx[1].col = cb;
	vtx[2].pos = vc; vtx[2].tex = tc; vtx[2].col = cc;
	vtx[3].pos = vd; vtx[3].tex = td; vtx[3].col = cd;

	u32 next = ctx->state.param.next;

	kIndex2D *idx = kArrayExtend(&ctx->indices, 6);
	idx[0] = next + 0;
	idx[1] = next + 1;
	idx[2] = next + 2;
	idx[3] = next + 0;
	idx[4] = next + 2;
	idx[5] = next + 3;

	kCommitPrimitive(4, 6);
}

void kDrawQuadUv(
	kVec2 a, kVec2 b, kVec2 c, kVec2 d,
	kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color) {
	kDrawQuadUvGradient(a, b, c, d, ta, tb, tc, td, color, color, color, color);
}

void kDrawQuadGradient(
	kVec2 a, kVec2 b, kVec2 c, kVec2 d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawQuadUvGradient(a, b, c, d, ta, tb, tc, td, ca, cb, cc, cd);
}

void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawQuadUvGradient(a, b, c, d, ta, tb, tc, td, color, color, color, color);
}

void kDrawRectUvGradient(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 a = pos;
	kVec2 b = { pos.x, pos.y + dim.y };
	kVec2 c = kAdd(pos, dim);
	kVec2 d = { pos.x + dim.x, pos.y };
	kDrawQuadUvGradient(a, b, c, d, uv_a, uv_b, uv_c, uv_d, ca, cb, cc, cd);
}

void kDrawRectUv(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color) {
	kVec2 a = pos;
	kVec2 b = { pos.x, pos.y + dim.y };
	kVec2 c = kAdd(pos, dim);
	kVec2 d = { pos.x + dim.x, pos.y };
	kDrawQuadUv(a, b, c, d, uv_a, uv_b, uv_c, uv_d, color);
}

void kDrawRectGradient(
	kVec2 pos, kVec2 dim,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 a  = pos;
	kVec2 b  = { pos.x, pos.y + dim.y };
	kVec2 c  = kAdd(pos, dim);
	kVec2 d  = { pos.x + dim.x, pos.y };
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawQuadUvGradient(a, b, c, d, ta, tb, tc, td, ca, cb, cc, cd);
}

void kDrawRect(kVec2 pos, kVec2 dim, kVec4 color) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectUv(pos, dim, ta, tb, tc, td, color);
}

void kDrawRectRotatedUvGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 center = {
		.x = pos.x + 0.5f * dim.x,
		.y = pos.y + 0.5f * dim.y
	};

	kVec2 a  = pos;
	kVec2 b  = { pos.x, pos.y + dim.y };
	kVec2 c  = kAdd(pos, dim);
	kVec2 d  = { pos.x + dim.x, pos.y };

	kVec2 t0 = kSub(a, center);
	kVec2 t1 = kSub(b, center);
	kVec2 t2 = kSub(c, center);
	kVec2 t3 = kSub(d, center);

	float cv = kCos(angle);
	float sv = kSin(angle);

	a.x = t0.x * cv - t0.y * sv;
	a.y = t0.x * sv + t0.y * cv;
	b.x = t1.x * cv - t1.y * sv;
	b.y = t1.x * sv + t1.y * cv;
	c.x = t2.x * cv - t2.y * sv;
	c.y = t2.x * sv + t2.y * cv;
	d.x = t3.x * cv - t3.y * sv;
	d.y = t3.x * sv + t3.y * cv;

	a = kAdd(a, center);
	b = kAdd(b, center);
	c = kAdd(c, center);
	d = kAdd(d, center);

	kDrawQuadUvGradient(a, b, c, d, uv_a, uv_b, uv_c, uv_d, ca, cb, cc, cd);
}

void kDrawRectRotatedUv(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color) {
	kDrawRectRotatedUvGradient(pos, dim, angle, uv_a, uv_b, uv_c, uv_d, color, color, color, color);
}

void kDrawRectRotatedGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectRotatedUvGradient(pos, dim, angle, ta, tb, tc, td, ca, cb, cc, cd);
}

void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectRotatedUvGradient(pos, dim, angle, ta, tb, tc, td, color, color, color, color);
}

void kDrawRectCenteredUvGradient(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 half_dim = {
		.x = 0.5f * dim.x,
		.y = 0.5f * dim.y,
	};

	kVec2 a = kSub(pos, half_dim);
	kVec2 b = { .x = pos.x - half_dim.x, .y = pos.y + half_dim.y };
	kVec2 c = kAdd(pos, half_dim);
	kVec2 d = { .x = pos.x + half_dim.x, .y = pos.y - half_dim.y };

	kDrawQuadUvGradient(a, b, c, d, uv_a, uv_b, uv_c, uv_d, ca, cb, cc, cd);
}

void kDrawRectCenteredUv(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color) {
	kDrawRectCenteredUvGradient(pos, dim, uv_a, uv_b, uv_c, uv_d, color, color, color, color);
}

void kDrawRectCenteredGradient(
	kVec2 pos, kVec2 dim,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectCenteredUvGradient(pos, dim, ta, tb, tc, td, ca, cb, cc, cd);
}

void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec4 color) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectCenteredUvGradient(pos, dim, ta, tb, tc, td, color, color, color, color);
}

void kDrawRectCenteredRotatedUvGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 center = pos;

	kVec2 half_dim = {
		.x = 0.5f * dim.x,
		.y = 0.5f * dim.y,
	};

	kVec2  a = kSub(pos, half_dim);
	kVec2  b = { .x = pos.x - half_dim.x, .y = pos.y + half_dim.y };
	kVec2  c = kAdd(pos, half_dim);
	kVec2  d = { .x = pos.x + half_dim.x, .y = pos.y - half_dim.y };

	kVec2 t0 = kSub(a, center);
	kVec2 t1 = kSub(b, center);
	kVec2 t2 = kSub(c, center);
	kVec2 t3 = kSub(d, center);

	float cv = kCos(angle);
	float sv = kSin(angle);

	a.x = t0.x * cv - t0.y * sv;
	a.y = t0.x * sv + t0.y * cv;
	b.x = t1.x * cv - t1.y * sv;
	b.y = t1.x * sv + t1.y * cv;
	c.x = t2.x * cv - t2.y * sv;
	c.y = t2.x * sv + t2.y * cv;
	d.x = t3.x * cv - t3.y * sv;
	d.y = t3.x * sv + t3.y * cv;

	a = kAdd(a, center);
	b = kAdd(b, center);
	c = kAdd(c, center);
	d = kAdd(d, center);

	kDrawQuadUvGradient(a, b, c, d, uv_a, uv_b, uv_c, uv_d, ca, cb, cc, cd);
}

void kDrawRectCenteredRotatedUv(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color) {
	kDrawRectCenteredRotatedUvGradient(pos, dim, angle, uv_a, uv_b, uv_c, uv_d, color, color, color, color);
}

void kDrawRectCenteredRotatedGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectCenteredRotatedUvGradient(pos, dim, angle, ta, tb, tc, td, ca, cb, cc, cd);
}

void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kDrawRectCenteredRotatedUvGradient(pos, dim, angle, ta, tb, tc, td, color, color, color, color);
}

void kDrawRoundedRect(kVec2 pos, kVec2 dim, kVec4 color, float radius) {
	if (radius) {
		float rad_x = kMin(radius, 0.5f * dim.x);
		float rad_y = kMin(radius, 0.5f * dim.y);

		kVec2 p0 = { pos.x + rad_x, pos.y + rad_y };
		kVec2 p1 = { pos.x + dim.x - rad_x, pos.y + rad_y };
		kVec2 p2 = { pos.x + dim.x - rad_x, pos.y + dim.y - rad_y };
		kVec2 p3 = { pos.x + rad_x, pos.y + dim.y - rad_y };

		kArcTo(p0, rad_x, rad_y, 2.0f / 4.0f, 3.0f / 4.0f);
		kArcTo(p1, rad_x, rad_y, 3.0f / 4.0f, 4.0f / 4.0f);
		kArcTo(p2, rad_x, rad_y, 0.0f / 4.0f, 1.0f / 4.0f);
		kArcTo(p3, rad_x, rad_y, 1.0f / 4.0f, 2.0f / 4.0f);

		kDrawPathFilled(color);
	} else {
		kDrawRect(pos, dim, color);
	}
}

void kDrawEllipseGradient(kVec2 pos, float radius_a, float radius_b, kVec4 center, kVec4 first, kVec4 last) {
	int segments = (int)kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));

	float px = kCosLookup(0) * radius_a;
	float py = kSinLookup(0) * radius_b;

	kVec4 pc = first;

	float npx, npy;
	for (int index = 1; index <= segments; ++index) {
		float t    = (float)index / (float)segments;

		npx = kCosLookupEx(index, segments) * radius_a;
		npy = kSinLookupEx(index, segments) * radius_b;

		kVec2 pos1 = { .x = npx, .y = npy };
		kVec2 pos2 = { .x =  px, .y =  py };

		kVec4 nc   = kLerp(first, last, t);

		kDrawTriangleGradient(pos, kAdd(pos, pos1), kAdd(pos, pos2), center, nc, pc);

		px = npx;
		py = npy;
		pc = nc;
	}
}

void kDrawEllipse(kVec2 pos, float radius_a, float radius_b, kVec4 color) {
	int segments = (int)kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));

	float px = kCosLookup(0) * radius_a;
	float py = kSinLookup(0) * radius_b;

	float npx, npy;
	for (int index = 1; index <= segments; ++index) {
		npx = kCosLookupEx(index, segments) * radius_a;
		npy = kSinLookupEx(index, segments) * radius_b;

		kVec2 pos1 = { .x = npx, .y = npy };
		kVec2 pos2 = { .x =  px, .y =  py };

		kDrawTriangle(pos, kAdd(pos, pos1), kAdd(pos, pos2), color);

		px = npx;
		py = npy;
	}
}

void kDrawCircleGradient(kVec2 pos, float radius, kVec4 center, kVec4 first, kVec4 last) {
	kDrawEllipseGradient(pos, radius, radius, center, first, last);
}

void kDrawCircle(kVec2 pos, float radius, kVec4 color) {
	kDrawEllipse(pos, radius, radius, color);
}

void kDrawEllipticPieGradient(
	kVec2 pos, float radius_a, float radius_b,
	float theta_a, float theta_b,
	kVec4 center, kVec4 first, kVec4 last) {
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float px = kCosLookup(theta_a) * radius_a;
	float py = kSinLookup(theta_a) * radius_b;
	kVec4 pc = first;

	int index = 1;

	float npx, npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt) {
		npx = kCosLookup(theta_a) * radius_a;
		npy = kSinLookup(theta_a) * radius_b;

		kVec2 pos1 = { .x = npx, .y = npy };
		kVec2 pos2 = { .x =  px, .y =  py };

		float t    = (float)index / (float)segments;
		kVec4 nc   = kLerp(first, last, t);

		kDrawTriangleGradient(pos, kAdd(pos, pos1), kAdd(pos, pos2), center, nc, pc);

		px = npx;
		py = npy;
		pc = nc;

		index += 1;
	}
}

void kDrawEllipticPie(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color) {
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float px = kCosLookup(theta_a) * radius_a;
	float py = kSinLookup(theta_a) * radius_b;

	float npx, npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt) {
		npx = kCosLookup(theta_a) * radius_a;
		npy = kSinLookup(theta_a) * radius_b;

		kVec2 pos1 = { .x = npx, .y = npy };
		kVec2 pos2 = { .x =  px, .y =  py };

		kDrawTriangle(pos, kAdd(pos, pos1), kAdd(pos, pos2), color);

		px = npx;
		py = npy;
	}
}

void kDrawPieGradient(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 center, kVec4 first, kVec4 last) {
	kDrawEllipticPieGradient(pos, radius, radius, theta_a, theta_b, center, first, last);
}

void kDrawPie(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color) {
	kDrawEllipticPie(pos, radius, radius, theta_a, theta_b, color);
}

void kDrawEllipticPiePartGradient(
	kVec2 pos, float radius_a_min, float radius_b_min,
	float radius_a_max, float radius_b_max,
	float theta_a, float theta_b,
	kVec4 minc_a, kVec4 minc_b, kVec4 maxc_a, kVec4 maxc_b) {
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a_max * radius_a_max + radius_b_max * radius_b_max)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float min_px = kCosLookup(theta_a) * radius_a_min;
	float min_py = kSinLookup(theta_a) * radius_b_min;
	float max_px = kCosLookup(theta_a) * radius_a_max;
	float max_py = kSinLookup(theta_a) * radius_b_max;

	kVec4 min_pc = minc_a;
	kVec4 max_pc = maxc_a;

	int index = 1;

	float min_npx, min_npy;
	float max_npx, max_npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt) {
		min_npx = kCosLookup(theta_a) * radius_a_min;
		min_npy = kSinLookup(theta_a) * radius_b_min;
		max_npx = kCosLookup(theta_a) * radius_a_max;
		max_npy = kSinLookup(theta_a) * radius_b_max;

		kVec2 q0 = { pos.x + min_npx, pos.y + min_npy };
		kVec2 q1 = { pos.x + max_npx, pos.y + max_npy };
		kVec2 q2 = { pos.x + max_px,  pos.y + max_py  };
		kVec2 q3 = { pos.x + min_px,  pos.y + min_py  };

		float t  = (float)index / (float)segments;

		kVec4 min_nc = kLerp(minc_a, minc_b, t);
		kVec4 max_nc = kLerp(maxc_a, maxc_b, t);

		kDrawQuadGradient(q0, q1, q2, q3, min_nc, max_nc, max_pc, min_pc);

		min_px = min_npx;
		min_py = min_npy;
		max_px = max_npx;
		max_py = max_npy;
		min_pc = min_nc;
		max_pc = max_nc;

		index += 1;
	}
}

void kDrawEllipticPiePart(
	kVec3 pos, float radius_a_min, float radius_b_min,
	float radius_a_max, float radius_b_max,
	float theta_a, float theta_b, kVec4 color) {
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a_max * radius_a_max + radius_b_max * radius_b_max)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float min_px = kCosLookup(theta_a) * radius_a_min;
	float min_py = kSinLookup(theta_a) * radius_b_min;
	float max_px = kCosLookup(theta_a) * radius_a_max;
	float max_py = kSinLookup(theta_a) * radius_b_max;

	float min_npx, min_npy;
	float max_npx, max_npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt) {
		min_npx = kCosLookup(theta_a) * radius_a_min;
		min_npy = kSinLookup(theta_a) * radius_b_min;
		max_npx = kCosLookup(theta_a) * radius_a_max;
		max_npy = kSinLookup(theta_a) * radius_b_max;

		kVec2 q0 = { pos.x + min_npx, pos.y + min_npy };
		kVec2 q1 = { pos.x + max_npx, pos.y + max_npy };
		kVec2 q2 = { pos.x + max_px,  pos.y + max_py  };
		kVec2 q3 = { pos.x + min_px,  pos.y + min_py  };

		kDrawQuad(q0, q1, q2, q3, color);

		min_px = min_npx;
		min_py = min_npy;
		max_px = max_npx;
		max_py = max_npy;
	}
}

void kDrawPiePartGradient(
	kVec2 pos, float radius_min, float radius_max,
	float theta_a, float theta_b,
	kVec4 minc_a, kVec4 minc_b, kVec4 maxc_a, kVec4 maxc_b) {
	kDrawEllipticPiePartGradient(pos, radius_min, radius_min, radius_max, radius_max, theta_a, theta_b, minc_a, minc_b, maxc_a, maxc_b);
}

void kDrawPiePart(kVec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color) {
	kDrawEllipticPiePartGradient(pos, radius_min, radius_min, radius_max, radius_max, theta_a, theta_b, color, color, color, color);
}

void kDrawPolygon(kVec2 *vertices, u32 count, kVec4 color) {
	kAssert(count >= 3);
	u32 triangle_count = count - 2;
	for (u32 ti = 0; ti < triangle_count; ++ti) {
		kDrawTriangle(vertices[0], vertices[ti + 1], vertices[ti + 2], color);
	}
}

void kDrawPolygonGradient(kVec2 *vertices, kVec4 *colors, u32 count) {
	kAssert(count >= 3);
	u32 triangle_count = count - 2;
	for (u32 ti = 0; ti < triangle_count; ++ti) {
		kDrawTriangleGradient(vertices[0], vertices[ti + 1], vertices[ti + 2], colors[0], colors[ti + 1], colors[ti + 2]);
	}
}

void kDrawPolygonUvGradient(kVec2 *vertices, const kVec2 *uvs, kVec4 *colors, u32 count) {
	kAssert(count >= 3);
	u32 triangle_count = count - 2;
	for (u32 ti = 0; ti < triangle_count; ++ti) {
		kDrawTriangleUvGradient(vertices[0], vertices[ti + 1], vertices[ti + 2],
			uvs[0], uvs[ti + 1], uvs[ti + 2], colors[0], colors[ti + 1], colors[ti + 2]);
	}
}

void kDrawTriangleList(kVertex2D *vertices, u32 count) {
	kRenderContext2D * ctx = kGetRenderContext2D();
	kVertex2D        * vtx = kArrayExtend(&ctx->vertices, count);
	u32              * idx = kArrayExtend(&ctx->indices, count);

	memcpy(vtx, vertices, count * sizeof(kVertex2D));

	u32 next = ctx->state.param.next;
	for (u32 i = 0; i < count; ++i) {
		idx[i] = next + i;
	}

	kCommitPrimitive(count, count);
}

void kDrawLine(kVec3 a, kVec3 b, kVec4 color) {
	if (kAlmostEqual(b.x - a.x, 0.0f) &&
		kAlmostEqual(b.y - a.y, 0.0f))
		return;

	kRenderContext2D *ctx = kGetRenderContext2D();

	float thickness = ctx->thickness * 0.5f;
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float ilen = 1.0f / kSquareRoot(dx * dx + dy * dy);
	dx *= (thickness * ilen);
	dy *= (thickness * ilen);

	kVec2 c0 = { a.x - dy, a.y + dx };
	kVec2 c1 = { b.x - dy, b.y + dx };
	kVec2 c2 = { b.x + dy, b.y - dx };
	kVec2 c3 = { a.x + dy, a.y - dx };

	kDrawQuad(c0, c1, c2, c3, color);
}

void kPathTo(kVec2 a) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	imem count = kArrayCount(&ctx->builder);
	if (count) {
		kVec2 last = ctx->builder[count - 1];
		if (kAlmostEqual(last.x - a.x, 0.0f) &&
			kAlmostEqual(last.y - a.y, 0.0f))
			return;
	}

	kArrayAdd(&ctx->builder, a);
}

void kArcTo(kVec2 position, float radius_a, float radius_b, float theta_a, float theta_b) {
	theta_a = kWrapTurns(theta_a);
	theta_b = kWrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float npx, npy;
	for (; theta_a <= theta_b; theta_a += dt) {
		npx = kCosLookup(theta_a) * radius_a;
		npy = kSinLookup(theta_a) * radius_b;
		kVec2 offset = { npx, npy };
		kVec2 point  = kAdd(position, offset);
		kPathTo(point);
	}
}

void kBezierQuadraticTo(kVec2 a, kVec2 b, kVec2 c) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	int segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c));
	kVec2 *paths = kArrayExtend(&ctx->builder, segments + 1);
	kBuildBezierQuadratic(a, b, c, paths, segments);
}

void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	int segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c) + kLength(d));
	kVec2 *paths = kArrayExtend(&ctx->builder, segments + 1);
	kBuildBezierCubic(a, b, c, d, paths, segments);
}

static inline kVec2 kLineLineIntersect(kVec2 p1, kVec2 q1, kVec2 p2, kVec2 q2) {
	kVec2 d1 = kSub(p1, q1);
	kVec2 d2 = kSub(p2, q2);

	float d = d1.x * d2.y - d1.y * d2.x;
	float n2 = -d1.x * (p1.y - p2.y) + d1.y * (p1.x - p2.x);

	if (d != 0) {
		float u   = n2 / d;
		kVec2 ud2 = { u * d2.x, u * d2.y };
		p2 = kSub(p2, ud2);
		return p2;
	}

	return p1;
}

void kDrawPathStroked(kVec4 color, bool closed) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	imem path_count = kArrayCount(&ctx->builder);

	if (path_count < 2) {
		kArrayReset(&ctx->builder);
		return;
	}

	if (path_count == 2 && closed) {
		kArrayReset(&ctx->builder);
		return;
	}

	if (closed) {
		kPathTo(ctx->builder[0]);
	}

	imem normal_count = path_count - 1;

	kArrayExtend(&ctx->builder, normal_count);

	kVec2 *paths   = ctx->builder;
	kVec2 *normals = ctx->builder + path_count;

	for (u32 i = 0; i < normal_count; ++i) {
		kVec2 diff = kSub(paths[i + 1], paths[i]);
		kVec2 v    = kNormalize(diff);
		normals[i] = v;
	}

	kVec2 thickness = kVec2Factor(0.5f * ctx->thickness);

	kVec2 p, q;
	kVec2 start_p, start_q;
	kVec2 prev_p, prev_q;
	kVec2 a, b, c;
	kVec2 v1, v2, n1;
	kVec2 n2, t1, t2;
	kVec2 p1, q1, p2, q2;

	if (closed) {
		a = paths[path_count - 1];
		b = paths[0];
		c = paths[1];

		v1 = normals[normal_count - 1];
		v2 = normals[0];

		n1 = (kVec2){ -v1.y, v1.x };
		n2 = (kVec2){ -v2.y, v2.x };

		t1 = kMul(n1, thickness);
		t2 = kMul(n2, thickness);

		p1 = kAdd(b, t1);
		q1 = kAdd(p1, v1);
		p2 = kAdd(b, t2);
		q2 = kAdd(p2, v2);
		start_p = prev_p = kLineLineIntersect(p1, q1, p2, q2);

		p1 = kSub(b, t1);
		q1 = kAdd(p1, v1);
		p2 = kSub(b, t2);
		q2 = kAdd(p2, v2);
		start_q = prev_q = kLineLineIntersect(p1, q1, p2, q2);
	} else {
		kVec2 v, t, n;

		v = normals[0];
		n = (kVec2){ -v.y, v.x };
		t = kMul(thickness, n);
		prev_p = kAdd(paths[0], t);
		prev_q = kSub(paths[0], t);

		v = normals[normal_count - 1];
		n = (kVec2){ -v.y, v.x };
		t = kMul(thickness, n);
		start_p = kAdd(paths[path_count - 1], t);
		start_q = kSub(paths[path_count - 1], t);
	}

	for (u32 i = 0; i + 1 < normal_count; ++i) {
		a = paths[i + 0];
		b = paths[i + 1];
		c = paths[i + 2];

		v1 = normals[i + 0];
		v2 = normals[i + 1];

		n1 = (kVec2){ -v1.y, v1.x };
		n2 = (kVec2){ -v2.y, v2.x };

		t1 = kMul(n1, thickness);
		t2 = kMul(n2, thickness);

		{
			p1 = kAdd(b, t1);
			q1 = kAdd(p1, v1);
			p2 = kAdd(b, t2);
			q2 = kAdd(p2, v2);
			p = kLineLineIntersect(p1, q1, p2, q2);
		}

		{
			p1 = kSub(b, t1);
			q1 = kAdd(p1, v1);
			p2 = kSub(b, t2);
			q2 = kAdd(p2, v2);
			q = kLineLineIntersect(p1, q1, p2, q2);
		}

		kDrawQuad(prev_q, prev_p, p, q, color);

		prev_p = p;
		prev_q = q;
	}

	kDrawQuad(prev_q, prev_p, start_p, start_q, color);

	kArrayReset(&ctx->builder);
}

void kDrawPathFilled(kVec4 color) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	imem count = kArrayCount(&ctx->builder);

	if (count < 3) {
		kArrayReset(&ctx->builder);
		return;
	}

	kVec2 *paths = ctx->builder;
	imem triangle_count = count - 2;
	for (u32 ti = 0; ti < triangle_count; ++ti) {
		kDrawTriangle(paths[0], paths[ti + 1], paths[ti + 2], color);
	}

	kArrayReset(&ctx->builder);
}

void kDrawBezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec4 color) {
	kBezierQuadraticTo(a, b, c);
	kDrawPathStroked(color, false);
}

void kDrawBezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color) {
	kBezierCubicTo(a, b, c, d);
	kDrawPathStroked(color, false);
}

void kDrawTriangleOutline(kVec2 a, kVec2 b, kVec2 c, kVec4 color) {
	kPathTo(a);
	kPathTo(b);
	kPathTo(c);
	kDrawPathStroked(color, true);
}

void kDrawQuadOutline(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color) {
	kPathTo(a);
	kPathTo(b);
	kPathTo(c);
	kPathTo(d);
	kDrawPathStroked(color, true);
}

void kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color) {
	kVec2 a = pos;
	kVec2 b = { pos.x, pos.y + dim.y };
	kVec2 c = kAdd(pos, dim);
	kVec2 d = { pos.x + dim.x, pos.y };
	kDrawQuadOutline(a, b, c, d, color);
}

void kDrawRectCenteredOutline(kVec2 pos, kVec2 dim, kVec4 color) {
	kVec2 half_dim = { 0.5f * dim.x, 0.5f * dim.y };

	kVec2 a = kSub(pos, half_dim);
	kVec2 b = { pos.x - half_dim.x, pos.y + half_dim.y };
	kVec2 c = kAdd(pos, half_dim);
	kVec2 d = { pos.x + half_dim.x, pos.y - half_dim.y };

	kDrawQuadOutline(a, b, c, d, color);
}

void kDrawEllipseOutline(kVec2 pos, float radius_a, float radius_b, kVec4 color) {
	int segments = (int)kCeil(2 * K_PI * kSquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));
	float npx, npy;
	for (int index = 0; index <= segments; ++index) {
		npx = kCosLookupEx(index, segments) * radius_a;
		npy = kSinLookupEx(index, segments) * radius_b;
		kVec2 npos = { pos.x + npx, pos.y + npy };
		kPathTo(npos);
	}
	kDrawPathStroked(color, true);
}

void kDrawCircleOutline(kVec2 pos, float radius, kVec4 color) {
	kDrawEllipseOutline(pos, radius, radius, color);
}

void kDrawEllipticArcOutline(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed) {
	kArcTo(pos, radius_a, radius_b, theta_a, theta_b);
	if (closed) {
		kPathTo(pos);
	}
	kDrawPathStroked(color, closed);
}

void kDrawArcOutline(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed) {
	kDrawEllipticArcOutline(pos, radius, radius, theta_a, theta_b, color, closed);
}

void kDrawPolygonOutline(const kVec2 *vertices, u32 count, kVec4 color) {
	for (u32 index = 0; index < count; ++index) {
		kPathTo(vertices[index]);
	}
	kDrawPathStroked(color, true);
}

void kDrawRoundedRectOutline(kVec2 pos, kVec2 dim, kVec4 color, float radius) {
	if (radius) {
		float rad_x = kMin(radius, 0.5f * dim.x);
		float rad_y = kMin(radius, 0.5f * dim.y);

		kVec2 p0 = { pos.x + rad_x, pos.y + rad_y };
		kVec2 p1 = { pos.x + dim.x - rad_x, pos.y + rad_y };
		kVec2 p2 = { pos.x + dim.x - rad_x, pos.y + dim.y - rad_y };
		kVec2 p3 = { pos.x + rad_x, pos.y + dim.y - rad_y };

		kArcTo(p0, rad_x, rad_y, 2.0f / 4.0f, 3.0f / 4.0f);
		kArcTo(p1, rad_x, rad_y, 3.0f / 4.0f, 4.0f / 4.0f);
		kArcTo(p2, rad_x, rad_y, 0.0f / 4.0f, 1.0f / 4.0f);
		kArcTo(p3, rad_x, rad_y, 1.0f / 4.0f, 2.0f / 4.0f);

		kDrawPathStroked(color, true);
	} else {
		kDrawRect(pos, dim, color);
	}
}

void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color) {
	kPushTexture(texture, 0);
	kDrawRect(pos, dim, color);
	kPopTexture(0);
}

void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color) {
	kPushTexture(texture, 0);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(0);
}

void kDrawTextureMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color) {
	kPushTexture(texture, 0);
	kPushTexture(mask, 1);
	kDrawRect(pos, dim, color);
	kPopTexture(1);
	kPopTexture(0);
}

void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color) {
	kPushTexture(texture, 0);
	kPushTexture(mask, 1);
	kDrawRectCentered(pos, dim, color);
	kPopTexture(1);
	kPopTexture(0);
}

kGlyph *kFindFontGlyph(kFont *font, u32 codepoint) {
	if (codepoint < font->largest) {
		u16 pos = font->map[codepoint];
		return &font->glyphs[pos];
	}
	return font->fallback;
}

float kCalculateText(kString text, kFont *font, float height) {
	float dist = 0;

	u32 codepoint;
	u8 *ptr = text.data;
	u8 *end = text.data + text.count;

	while (ptr < end) {
		int len = kUTF8ToCodepoint(ptr, end, &codepoint);
		ptr += len;

		kGlyph *glyph = kFindFontGlyph(font, codepoint);
		dist += height * glyph->advance;
	}

	return dist;
}

float kCalculateBuiltinText(kString text, float height) {
	kFont *font = &render.builtin.font;
	return kCalculateText(text, font, height);
}

void kDrawText(kString text, kVec2 pos, kFont *font, kVec4 color, float height) {
	kPushTexture(font->texture, 0);

	u32 codepoint;
	u8 *ptr = text.data;
	u8 *end = text.data + text.count;

	while (ptr < end) {
		int len = kUTF8ToCodepoint(ptr, end, &codepoint);
		ptr += len;

		kGlyph *glyph = kFindFontGlyph(font, codepoint);

		kVec2 draw_pos;
		draw_pos.x = pos.x + height * glyph->bearing.x;
		draw_pos.y = pos.y + height * glyph->bearing.y;

		kVec2 draw_size;
		draw_size.x = height * glyph->size.x;
		draw_size.y = height * glyph->size.y;

		kVec2 uv_a = { glyph->rect.min.x, glyph->rect.min.y };
		kVec2 uv_b = { glyph->rect.min.x, glyph->rect.max.y };
		kVec2 uv_c = { glyph->rect.max.x, glyph->rect.max.y };
		kVec2 uv_d = { glyph->rect.max.x, glyph->rect.min.y };

		kDrawRectUv(draw_pos, draw_size, uv_a, uv_b, uv_c, uv_d, color);

		pos.x += height * glyph->advance;
	}

	kPopTexture(0);
}

void kDrawBuilinText(kString text, kVec2 pos, kVec4 color, float height) {
	kFont *font = &render.builtin.font;
	kDrawText(text, pos, font, color, height);
}

