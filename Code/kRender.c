#pragma once
#include "kMath.h"
#include "kRender.h"
#include "kContext.h"

#include <string.h>

//
//
//

typedef struct kVertexList2D {
	kVertex2D *data;
	u32        count;
	u32        allocated;
} kVertexList2D;

typedef struct kIndexList2D {
	kIndex2D *data;
	u32       count;
	u32       allocated;
} kIndexList2D;

typedef struct kRenderState2D {
	u32       count;
	u32       next;
	float     thickness;
} kRenderState2D;

typedef struct kVectorList2D {
	kVec2 *data;
	u32    count;
	u32    allocated;
} kVectorList2D;

typedef struct kRenderContext2D {
	kVertexList2D  vertex;
	kIndexList2D   index;
	kRenderState2D state;

	kVectorList2D  builder;
} kRenderContext2D;

typedef struct kRenderContext {
	kRenderContext2D context2d;
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
//
//void SetTexture(PL_Texture *texture, uint idx) {
//	Assert(idx <= PL_MAX_SHADER_BINDING);
//	PL_Texture *prev = Last(Render.Texture[idx]);
//	if (prev != texture) {
//		OnTextureTransition(idx);
//	}
//	Last(Render.Texture[idx]) = texture;
//}
//
//void PushTexture(PL_Texture *texture, uint idx) {
//	Assert(idx <= PL_MAX_SHADER_BINDING);
//	Push(&Render.Texture[idx], Last(Render.Texture[idx]));
//	SetTexture(texture, idx);
//}
//
//void PopTexture(uint idx) {
//	Assert(Render.Texture[idx].Count > 1);
//	SetTexture(Render.Texture[idx][Render.Texture[idx].Count - 2], idx);
//	Render.Texture[idx].Count -= 1;
//}
//
//void SetRect(PL_Rect rect) {
//	PL_Rect prev = Last(Render.Rect);
//	if (memcmp(&prev, &rect, sizeof(rect)) != 0) {
//		OnRectTransition();
//	}
//	Last(Render.Rect) = rect;
//}
//
//void SetRect(float x, float y, float w, float h) {
//	PL_Rect rect;
//	rect.MinX = x;
//	rect.MinY = y;
//	rect.MaxX = x + w;
//	rect.MaxY = y + h;
//	SetRect(rect);
//}
//
//void PushRect(PL_Rect rect) {
//	Push(&Render.Rect, Last(Render.Rect));
//	SetRect(rect);
//}
//
//void PushRect(float x, float y, float w, float h) {
//	PL_Rect rect;
//	rect.MinX = x;
//	rect.MinY = y;
//	rect.MaxX = x + w;
//	rect.MaxY = y + h;
//	PushRect(rect);
//}
//
//void PopRect() {
//	Assert(Render.Rect.Count > 1);
//	SetRect(Render.Rect[Render.Rect.Count - 2]);
//	Render.Rect.Count -= 1;
//}
//
//void SetTransform(const Mat4 &transform) {
//	const Mat4 prev = Last(Render.Transform);
//	if (memcmp(&prev, &transform, sizeof(transform)) != 0) {
//		OnTransformTransition();
//	}
//	Last(Render.Transform) = transform;
//}
//
//void PushTransform(const Mat4 &transform) {
//	Push(&Render.Transform, Last(Render.Transform));
//	Mat4 t = Last(Render.Transform) * transform;
//	SetTransform(t);
//}
//
//void PushTransform(const Mat3 &transform) {
//	Mat4 m4;
//	m4.rows[0] = kVec4(transform.rows[0]._0.xy, 0.0f, transform.rows[0].z);
//	m4.rows[1] = kVec4(transform.rows[1]._0.xy, 0.0f, transform.rows[1].z);
//	m4.rows[2] = kVec4(transform.rows[2]._0.xy, 0.0f, 0.0f);
//	m4.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
//	PushTransform(m4);
//}
//
//void PushTransform(const Mat2 &transform) {
//	Mat4 m4;
//	m4.rows[0] = kVec4(transform.rows[0], 0.0f, 0.0f);
//	m4.rows[1] = kVec4(transform.rows[1], 0.0f, 0.0f);
//	m4.rows[2] = kVec4(0.0f, 0.0f, 1.0f, 0.0f);
//	m4.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
//	PushTransform(m4);
//}
//
//void PushTransform(kVec2 pos, float angle) {
//	kVec2 arm = Arm(angle);
//
//	Mat4 mat;
//	mat.rows[0] = kVec4(arm.x, -arm.y, 0.0f, pos.x);
//	mat.rows[1] = kVec4(arm.y, arm.x, 0.0f, pos.y);
//	mat.rows[2] = kVec4(0.0f, 0.0f, 1.0f, 0.0f);
//	mat.rows[2] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
//	PushTransform(mat);
//}
//
//void PopTransform() {
//	Assert(Render.Transform.Count > 1);
//	SetTransform(Render.Transform[Render.Transform.Count - 2]);
//	Render.Transform.Count -= 1;
//}

//
//
//

static u32 kNextVertexListCap(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	u32 next = ctx->vertex.allocated ? ctx->vertex.allocated << 1 : K_INITIAL_VERTEX_ALLOC;
	return kMax(next, ctx->vertex.count + count);
}

static u32 kNextIndexListCap(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	u32 next = ctx->index.allocated ? ctx->index.allocated << 1 : K_INITIAL_INDEX_ALLOC;
	return kMax(next, ctx->index.count + count);
}

static void kReserveVertex(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	if (ctx->vertex.count + count < ctx->vertex.allocated)
		return;
	u32 allocate          = kNextVertexListCap(count);
	ctx->vertex.data      = (kVertex2D *)kRealloc(ctx->vertex.data, ctx->vertex.allocated * sizeof(kVertex2D), allocate * sizeof(kVertex2D));
	ctx->vertex.allocated = allocate;
}

static void kReserveIndex(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	if (ctx->index.count + count < ctx->index.allocated)
		return;
	u32 allocate         = kNextIndexListCap(count);
	ctx->index.data      = (kIndex2D *)kRealloc(ctx->index.data, ctx->index.allocated * sizeof(kIndex2D), allocate * sizeof(kIndex2D));
	ctx->index.allocated = allocate;
}

static void kReservePrimitive(u32 vertex, u32 index) {
	kReserveVertex(vertex);
	kReserveIndex(index);
}

static void kCommitPrimitive(u32 vertex, u32 index) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kAssert(ctx->vertex.count + vertex < ctx->vertex.allocated &&
		ctx->index.count + index < ctx->index.allocated);
	ctx->vertex.count += vertex;
	ctx->index.count  += index;
	ctx->state.count  += index;
	ctx->state.next   += vertex;
}

static u32 kNextBuilderListCap(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	u32 next = ctx->builder.allocated ? ctx->builder.allocated << 2 : K_INITIAL_PATH_ALLOC;
	return kMax(next, ctx->builder.count + count);
}

static void kReserveBuilder(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kVectorList2D    *vec = &ctx->builder;
	if (vec->count + count < vec->allocated)
		return;
	u32 allocate   = kNextBuilderListCap(count);
	vec->data      = (kVec2 *)kRealloc(vec->data, vec->allocated * sizeof(kVec2), allocate * sizeof(kVec2));
	vec->allocated = allocate;
}

static void kCommitBuilder(u32 count) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	kVectorList2D    *vec = &ctx->builder;
	kAssert(vec->count + count < vec->allocated);
	vec->count += count;
}

//
//
//

void kDrawTriangleUvGradient(
		kVec2 va, kVec2 vb, kVec2 vc,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 ca, kVec4 cb, kVec4 cc) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	kReservePrimitive(3, 3);

	kVertex2D *vtx = ctx->vertex.data + ctx->vertex.count;
	vtx[0].pos = va; vtx[0].tex = ta; vtx[0].col = ca;
	vtx[1].pos = vb; vtx[1].tex = tb; vtx[1].col = cb;
	vtx[2].pos = vc; vtx[2].tex = tc; vtx[2].col = cc;

	kIndex2D *idx = ctx->index.data + ctx->index.count;
	idx[0] = ctx->state.next + 0;
	idx[1] = ctx->state.next + 1;
	idx[2] = ctx->state.next + 2;

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

	kReservePrimitive(4, 6);

	kVertex2D *vtx = ctx->vertex.data + ctx->vertex.count;
	vtx[0].pos = va; vtx[0].tex = ta; vtx[0].col = ca;
	vtx[1].pos = vb; vtx[1].tex = tb; vtx[1].col = cb;
	vtx[2].pos = vc; vtx[2].tex = tc; vtx[2].col = cc;
	vtx[3].pos = vd; vtx[3].tex = td; vtx[3].col = cd;

	u32 next = ctx->state.next;

	kIndex2D *idx = ctx->index.data + ctx->index.count;
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
	kReservePrimitive(count, count);

	kRenderContext2D *ctx = kGetRenderContext2D();
	memcpy(ctx->vertex.data + ctx->vertex.count, vertices, count * sizeof(kVertex2D));

	u32 next = ctx->state.next;
	u32 *idx = ctx->index.data;

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

	float thickness = ctx->state.thickness * 0.5f;
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

	if (ctx->builder.count) {
		kVec2 last = ctx->builder.data[ctx->builder.count - 1];
		if (kAlmostEqual(last.x - a.x, 0.0f) &&
			kAlmostEqual(last.y - a.y, 0.0f))
			return;
	}

	kReserveBuilder(1);
	ctx->builder.data[ctx->builder.count] = a;
	kCommitBuilder(1);
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
	kReserveBuilder(segments + 1);
	kBuildBezierQuadratic(a, b, c, ctx->builder.data + ctx->builder.count, segments);
	kCommitBuilder(segments + 1);
}

void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d) {
	kRenderContext2D *ctx = kGetRenderContext2D();
	int segments = (int)kCeil(kLength(a) + kLength(b) + kLength(c) + kLength(d));
	kReserveBuilder(segments + 1);
	kBuildBezierCubic(a, b, c, d, ctx->builder.data + ctx->builder.count, segments);
	kCommitBuilder(segments + 1);
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

	if (ctx->builder.count < 2) {
		ctx->builder.count = 0;
		return;
	}

	if (ctx->builder.count == 2 && closed) {
		ctx->builder.count = 0;
		return;
	}

	if (closed) {
		kPathTo(ctx->builder.data[0]);
	}

	u32 path_count   = ctx->builder.count;
	u32 normal_count = path_count - 1;

	kReserveBuilder(normal_count);

	kVec2 *paths   = ctx->builder.data;
	kVec2 *normals = ctx->builder.data + ctx->builder.count;

	for (u32 i = 0; i < normal_count; ++i) {
		kVec2 diff = kSub(paths[i + 1], paths[i]);
		kVec2 v    = kNormalize(diff);
		normals[i] = v;
	}

	kVec2 thickness = kVec2Factor(0.5f * ctx->state.thickness);

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

	ctx->builder.count = 0;
}

void kDrawPathFilled(kVec4 color) {
	kRenderContext2D *ctx = kGetRenderContext2D();

	if (ctx->builder.count < 3) {
		ctx->builder.count = 0;
		return;
	}

	kVec2 *paths = ctx->builder.data;
	u32 triangle_count = ctx->builder.count - 2;
	for (u32 ti = 0; ti < triangle_count; ++ti) {
		kDrawTriangle(paths[0], paths[ti + 1], paths[ti + 2], color);
	}

	ctx->builder.count = 0;
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


//void kDrawTexture(PL_Texture *texture, kVec3 pos, kVec2 dim, kVec4 color) {
//	PushTexture(texture);
//	kDrawRect(pos, dim, color);
//	PopTexture();
//}
//
//void kDrawTexture(PL_Texture *texture, kVec2 pos, kVec2 dim, kVec4 color) {
//	PushTexture(texture);
//	kDrawRect(pos, dim, color);
//	PopTexture();
//}
//
//void kDrawTextureCentered(PL_Texture *texture, kVec3 pos, kVec2 dim, kVec4 color) {
//	PushTexture(texture);
//	kDrawRectCentered(pos, dim, color);
//	PopTexture();
//}
//
//void kDrawTextureCentered(PL_Texture *texture, kVec2 pos, kVec2 dim, kVec4 color) {
//	PushTexture(texture);
//	kDrawRectCentered(pos, dim, color);
//	PopTexture();
//}
//
//void kDrawTexture(PL_Texture *texture, kVec3 pos, kVec2 dim, Region  rect, kVec4 color) {
//	PushTexture(texture);
//	kDrawRect(pos, dim, rect, color);
//	PopTexture();
//}
//
//void kDrawTexture(PL_Texture *texture, kVec2 pos, kVec2 dim, Region rect, kVec4 color) {
//	PushTexture(texture);
//	kDrawRect(pos, dim, rect, color);
//	PopTexture();
//}
//
//void kDrawTextureCentered(PL_Texture *texture, kVec3 pos, kVec2 dim, Region rect, kVec4 color) {
//	PushTexture(texture);
//	kDrawRectCentered(pos, dim, rect, color);
//	PopTexture();
//}
//
//void kDrawTextureCentered(PL_Texture *texture, kVec2 pos, kVec2 dim, Region rect, kVec4 color) {
//	PushTexture(texture);
//	kDrawRectCentered(pos, dim, rect, color);
//	PopTexture();
//}
//
//static const GlyphInfo *FindGlyphInfo(Font *font, u32 codepoint) {
//	if (codepoint < font->Index.Count) {
//		u16 index = font->Index[codepoint];
//		return &font->Glyphs[index];
//	}
//	return font->Fallback;
//}
//
//float CalculateText(String text, Font *font, float height) {
//	float dist = 0;
//
//	u32 codepoint;
//	u8 *ptr = text.begin();
//	u8 *end = text.end();
//
//	while (ptr < end) {
//		int len = UTF8ToCodepoint(ptr, end, &codepoint);
//		ptr += len;
//
//		auto info = FindGlyphInfo(font, codepoint);
//		dist += height * info->Advance;
//	}
//
//	return dist;
//}
//
//float CalculateText(String text, float height) {
//	Font *font = &Render.BuiltinFont;
//	return CalculateText(text, font, height);
//}
//
//void kDrawText(String text, kVec3 pos, Font *font, kVec4 color, float height) {
//	PushShader(Render.BuiltinShaders[PL_EmbeddedShader_Font]);
//	PushTexture(font->Texture);
//
//	u32 codepoint;
//	u8 *ptr = text.begin();
//	u8 *end = text.end();
//
//	while (ptr < end) {
//		int len = UTF8ToCodepoint(ptr, end, &codepoint);
//		ptr += len;
//
//		auto info = FindGlyphInfo(font, codepoint);
//
//		kVec3 kDraw_pos = pos;
//		draw_pos._0.xy += height * kVec2(info->Bearing.x, info->Bearing.y);
//		kDrawRect(draw_pos, height * info->Size, info->Rect, color);
//		pos.x += height * info->Advance;
//	}
//
//	PopTexture();
//	PopShader();
//}
//
//void kDrawText(String text, kVec2 pos, Font *font, kVec4 color, float height) {
//	kDrawText(text, kVec3(pos, 0), font, color, height);
//}
//
//void kDrawText(String text, kVec3 pos, kVec4 color, float height) {
//	Font *font = &Render.BuiltinFont;
//	kDrawText(text, pos, font, color, height);
//}
//
//void kDrawText(String text, kVec2 pos, kVec4 color, float height) {
//	Font *font = &Render.BuiltinFont;
//	kDrawText(text, pos, font, color, height);
//}
//
