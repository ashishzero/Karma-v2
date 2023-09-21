#pragma once
#include "kContext.h"

typedef struct kVertex2d {
	kVec2 pos;
	kVec2 tex;
	kVec4 col;
} kVertex2d;

typedef u32 kIndex2d;

typedef struct kVertex2dList {
	kVertex2d *data;
	u32        count;
	u32        allocated;
} kVertex2dList;

typedef struct kIndex2dList {
	kIndex2d *data;
	u32       count;
	u32       allocated;
} kIndex2dList;

typedef struct kRenderContext2d {
	kVertex2dList vertex;
	kIndex2dList  index;
} kRenderContext2d;

void kEnsureVertexSize(kVertex2dList *vertex, u32 count) {
	if (vertex->count + count < vertex->allocated)
		return;
	u32 allocate      = kMax(vertex->allocated ? (vertex->allocated << 1) : 64 * 4, vertex->count + 4);
	vertex->data      = (kVertex2d *)kRealloc(vertex->data, vertex->allocated * sizeof(kVertex2d), allocate * sizeof(kVertex2d));
	vertex->allocated = allocate;
}

void kEnsureIndexSize(kIndex2dList *index, u32 count) {
	if (index->count + count < index->allocated)
		return;
	u32 allocate     = kMax(index->allocated ? (index->allocated << 1) : 64 * 6, index->count + 6);
	index->data      = (kIndex2d *)kRealloc(index->data, index->allocated * sizeof(kIndex2d), allocate * sizeof(kIndex2d));
	index->allocated = allocate;
}

void kEnsurePrimitive(kRenderContext2d *ctx, u32 vertex, u32 index) {
	kEnsureVertexSize(&ctx->vertex, vertex);
	kEnsureIndexSize(&ctx->index, index);
}

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
//	m4.rows[0] = Vec4(transform.rows[0]._0.xy, 0.0f, transform.rows[0].z);
//	m4.rows[1] = Vec4(transform.rows[1]._0.xy, 0.0f, transform.rows[1].z);
//	m4.rows[2] = Vec4(transform.rows[2]._0.xy, 0.0f, 0.0f);
//	m4.rows[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
//	PushTransform(m4);
//}
//
//void PushTransform(const Mat2 &transform) {
//	Mat4 m4;
//	m4.rows[0] = Vec4(transform.rows[0], 0.0f, 0.0f);
//	m4.rows[1] = Vec4(transform.rows[1], 0.0f, 0.0f);
//	m4.rows[2] = Vec4(0.0f, 0.0f, 1.0f, 0.0f);
//	m4.rows[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
//	PushTransform(m4);
//}
//
//void PushTransform(Vec2 pos, float angle) {
//	Vec2 arm = Arm(angle);
//
//	Mat4 mat;
//	mat.rows[0] = Vec4(arm.x, -arm.y, 0.0f, pos.x);
//	mat.rows[1] = Vec4(arm.y, arm.x, 0.0f, pos.y);
//	mat.rows[2] = Vec4(0.0f, 0.0f, 1.0f, 0.0f);
//	mat.rows[2] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
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

void kRender2d_DrawTriangleTexGrad(
		kRenderContext2d *ctx,
		kVec2 va, kVec2 vb, kVec2 vc,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 ca, kVec4 cb, kVec4 cc) {
	kEnsurePrimitive(ctx, 3, 3);

	kVertex2d *vtx = ctx->vertex.data + ctx->vertex.count;
	vtx[0].pos = va; vtx[0].tex = ta; vtx[0].col = ca;
	vtx[1].pos = vb; vtx[1].tex = tb; vtx[1].col = cb;
	vtx[2].pos = vc; vtx[2].tex = tc; vtx[2].col = cc;

	kIndex2d *idx = ctx->index.data + ctx->index.count;
	idx[0] = ctx->draw.next + 0;
	idx[1] = ctx->draw.next + 1;
	idx[2] = ctx->draw.next + 2;

	ctx->vertex.count += 3;
	ctx->index.count  += 6;
	ctx->draw.next    += 3;
	ctx->draw.count   += 6;
}

void kRender2d_DrawTriangleTex(
		kRenderContext2d *ctx,
		kVec2 a, kVec2 b, kVec2 c,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 col) {
	kRender2d_DrawTriangleTexGrad(ctx, a, b, c, ta, tb, tc, col, col , col);
}

void kRender2d_DrawTriangleGrad(
		kRenderContext2d *ctx,
		kVec2 a, kVec2 b, kVec2 c,
		kVec4 ca, kVec4 cb, kVec4 cc) {
	kVec2 zero = { 0 };
	kRender2d_DrawTriangleTexGrad(ctx, a, b, c, zero, zero, zero, ca, cb, cc);
}

void kRender2d_DrawTriangle(
		kRenderContext2d *ctx,
		kVec2 a, kVec2 b, kVec2 c,
		kVec4 color) {
	kVec2 zero = { 0 };
	kRender2d_DrawTriangleTexGrad(ctx, a, b, c, zero, zero, zero, color, color, color);
}

void kAddQuadTexGrad(kRenderContext2d *ctx, kVec2 va, kVec2 vb, kVec2 vc, kVec2 vd, kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd) {
	kEnsurePrimitive(ctx, 4, 6);

	kVertex2d *vtx = ctx->vertex.data + ctx->vertex.count;
	vtx[0].pos = va; vtx[0].tex = ta; vtx[0].col = ca;
	vtx[1].pos = vb; vtx[1].tex = tb; vtx[1].col = cb;
	vtx[2].pos = vc; vtx[2].tex = tc; vtx[2].col = cc;
	vtx[3].pos = vd; vtx[3].tex = td; vtx[3].col = cd;

	kIndex2d *idx = ctx->index.data + ctx->index.count;
	idx[0] = ctx->draw.next + 0;
	idx[1] = ctx->draw.next + 1;
	idx[2] = ctx->draw.next + 2;
	idx[3] = ctx->draw.next + 0;
	idx[4] = ctx->draw.next + 2;
	idx[5] = ctx->draw.next + 3;

	ctx->vertex.count += 4;
	ctx->index.count  += 6;
	ctx->draw.next    += 4;
	ctx->draw.count   += 6;
}

void kAddQuadTex(kRenderContext2d *ctx, kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color) {
	kAddQuadTexGrad(ctx, a, b, c, d, ta, tb, tc, td, color, color, color, color);
}

void kAddQuad(kRenderContext2d *ctx, kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color) {
	kVec2 ta = { 0, 0 };
	kVec2 tb = { 0, 1 };
	kVec2 tc = { 1, 1 };
	kVec2 td = { 1, 0 };
	kAddQuadTexGrad(ctx, a, b, c, d, ta, tb, tc, td, color, color, color, color);
}

void DrawRect(Vec3 pos, Vec2 dim, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	Vec3 a = pos;
	Vec3 b = Vec3(pos.x, pos.y + dim.y, pos.z);
	Vec3 c = Vec3(pos._0.xy + dim, pos.z);
	Vec3 d = Vec3(pos.x + dim.x, pos.y, pos.z);
	DrawQuad(a, b, c, d, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRect(Vec2 pos, Vec2 dim, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	DrawRect(Vec3(pos, 0), dim, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRect(Vec3 pos, Vec2 dim, Vec4 color) {
	DrawRect(pos, dim, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawRect(Vec2 pos, Vec2 dim, Vec4 color) {
	DrawRect(Vec3(pos, 0), dim, color);
}

void DrawRect(Vec3 pos, Vec2 dim, Region rect, Vec4 color) {
	Vec2 uv_a = rect.min;
	Vec2 uv_b = Vec2(rect.min.x, rect.max.y);
	Vec2 uv_c = rect.max;
	Vec2 uv_d = Vec2(rect.max.x, rect.min.y);
	DrawRect(pos, dim, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRect(Vec2 pos, Vec2 dim, Region rect, Vec4 color) {
	DrawRect(Vec3(pos, 0), dim, rect, color);
}

void DrawRectRotated(Vec3 pos, Vec2 dim, float angle, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	Vec2  center = 0.5f * (2.0f * pos._0.xy + dim);

	Vec2  a = pos._0.xy;
	Vec2  b = Vec2(pos.x, pos.y + dim.y);
	Vec2  c = pos._0.xy + dim;
	Vec2  d = Vec2(pos.x + dim.x, pos.y);

	auto  t0 = a - center;
	auto  t1 = b - center;
	auto  t2 = c - center;
	auto  t3 = d - center;

	float cv = Cos(angle);
	float sv = Sin(angle);

	a.x = t0.x * cv - t0.y * sv;
	a.y = t0.x * sv + t0.y * cv;
	b.x = t1.x * cv - t1.y * sv;
	b.y = t1.x * sv + t1.y * cv;
	c.x = t2.x * cv - t2.y * sv;
	c.y = t2.x * sv + t2.y * cv;
	d.x = t3.x * cv - t3.y * sv;
	d.y = t3.x * sv + t3.y * cv;

	a += center;
	b += center;
	c += center;
	d += center;

	DrawQuad(Vec3(a, pos.z), Vec3(b, pos.z), Vec3(c, pos.z), Vec3(d, pos.z), uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectRotated(Vec2 pos, Vec2 dim, float angle, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	DrawRectRotated(Vec3(pos, 0), dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectRotated(Vec3 pos, Vec2 dim, float angle, Vec4 color) {
	DrawRectRotated(pos, dim, angle, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawRectRotated(Vec2 pos, Vec2 dim, float angle, Vec4 color) {
	DrawRectRotated(Vec3(pos, 0), dim, angle, color);
}

void DrawRectRotated(Vec3 pos, Vec2 dim, float angle, Region rect, Vec4 color) {
	Vec2 uv_a = rect.min;
	Vec2 uv_b = Vec2(rect.min.x, rect.max.y);
	Vec2 uv_c = rect.max;
	Vec2 uv_d = Vec2(rect.max.x, rect.min.y);
	DrawRectRotated(pos, dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectRotated(Vec2 pos, Vec2 dim, float angle, Region rect, Vec4 color) {
	DrawRectRotated(Vec3(pos, 0), dim, angle, rect, color);
}

void DrawRectCentered(Vec3 pos, Vec2 dim, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	Vec2 half_dim = 0.5f * dim;

	Vec3 a, b, c, d;
	a._0.xy = pos._0.xy - half_dim;
	b._0.xy = Vec2(pos.x - half_dim.x, pos.y + half_dim.y);
	c._0.xy = pos._0.xy + half_dim;
	d._0.xy = Vec2(pos.x + half_dim.x, pos.y - half_dim.y);

	a.z = pos.z;
	b.z = pos.z;
	c.z = pos.z;
	d.z = pos.z;

	DrawQuad(a, b, c, d, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectCentered(Vec2 pos, Vec2 dim, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	DrawRectCentered(Vec3(pos, 0), dim, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectCentered(Vec3 pos, Vec2 dim, Vec4 color) {
	DrawRectCentered(pos, dim, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawRectCentered(Vec2 pos, Vec2 dim, Vec4 color) {
	DrawRectCentered(Vec3(pos, 0), dim, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawRectCentered(Vec3 pos, Vec2 dim, Region rect, Vec4 color) {
	Vec2 uv_a = rect.min;
	Vec2 uv_b = Vec2(rect.min.x, rect.max.y);
	Vec2 uv_c = rect.max;
	Vec2 uv_d = Vec2(rect.max.x, rect.min.y);
	DrawRectCentered(pos, dim, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectCentered(Vec2 pos, Vec2 dim, Region rect, Vec4 color) {
	DrawRectCentered(Vec3(pos, 0), dim, rect, color);
}

void DrawRectCenteredRotated(Vec3 pos, Vec2 dim, float angle, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	Vec2 center = pos._0.xy;

	Vec2 half_dim = 0.5f * dim;
	Vec2 a, b, c, d;
	a = pos._0.xy - half_dim;
	b = Vec2(pos.x - half_dim.x, pos.y + half_dim.y);
	c = pos._0.xy + half_dim;
	d = Vec2(pos.x + half_dim.x, pos.y - half_dim.y);

	auto  t0 = a - center;
	auto  t1 = b - center;
	auto  t2 = c - center;
	auto  t3 = d - center;

	float cv = Cos(angle);
	float sv = Sin(angle);

	a.x = t0.x * cv - t0.y * sv;
	a.y = t0.x * sv + t0.y * cv;
	b.x = t1.x * cv - t1.y * sv;
	b.y = t1.x * sv + t1.y * cv;
	c.x = t2.x * cv - t2.y * sv;
	c.y = t2.x * sv + t2.y * cv;
	d.x = t3.x * cv - t3.y * sv;
	d.y = t3.x * sv + t3.y * cv;

	a += center;
	b += center;
	c += center;
	d += center;

	DrawQuad(Vec3(a, pos.z), Vec3(b, pos.z), Vec3(c, pos.z), Vec3(d, pos.z), uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectCenteredRotated(Vec2 pos, Vec2 dim, float angle, Vec2 uv_a, Vec2 uv_b, Vec2 uv_c, Vec2 uv_d, Vec4 color) {
	DrawRectCenteredRotated(Vec3(pos, 0), dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectCenteredRotated(Vec3 pos, Vec2 dim, float angle, Vec4 color) {
	DrawRectCenteredRotated(pos, dim, angle, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawRectCenteredRotated(Vec2 pos, Vec2 dim, float angle, Vec4 color) {
	DrawRectCenteredRotated(Vec3(pos, 0), dim, angle, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawRectCenteredRotated(Vec3 pos, Vec2 dim, float angle, Region rect, Vec4 color) {
	Vec2 uv_a = rect.min;
	Vec2 uv_b = Vec2(rect.min.x, rect.max.y);
	Vec2 uv_c = rect.max;
	Vec2 uv_d = Vec2(rect.max.x, rect.min.y);
	DrawRectCenteredRotated(pos, dim, angle, uv_a, uv_b, uv_c, uv_d, color);
}

void DrawRectCenteredRotated(Vec2 pos, Vec2 dim, float angle, Region rect, Vec4 color) {
	DrawRectCenteredRotated(Vec3(pos, 0), dim, angle, rect, color);
}

void DrawEllipse(Vec3 pos, float radius_a, float radius_b, Vec4 color) {
	int length = (int)Ceil(2 * PI * SquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));
	int segments = Max(MIN_CIRCLE_SEGMENTS, (int)length);

	float px = cos_map[0] * radius_a;
	float py = sin_map[0] * radius_b;

	float npx, npy;
	for (int index = 1; index <= segments; ++index) {
		npx = CosLookup(index, segments) * radius_a;
		npy = SinLookup(index, segments) * radius_b;

		DrawTriangle(pos, pos + Vec3(npx, npy, 0), pos + Vec3(px, py, 0), color);

		px = npx;
		py = npy;
	}
}

void DrawEllipse(Vec2 pos, float radius_a, float radius_b, Vec4 color) {
	DrawEllipse(Vec3(pos, 0), radius_a, radius_b, color);
}

void DrawCircle(Vec3 pos, float radius, Vec4 color) {
	DrawEllipse(pos, radius, radius, color);
}

void DrawCircle(Vec2 pos, float radius, Vec4 color) {
	DrawEllipse(Vec3(pos, 0), radius, radius, color);
}

void DrawPie(Vec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, Vec4 color) {
	theta_a = WrapTurns(theta_a);
	theta_b = WrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = Ceil(2 * PI * SquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float px = CosLookup(theta_a) * radius_a;
	float py = SinLookup(theta_a) * radius_b;

	float npx, npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt) {
		npx = CosLookup(theta_a) * radius_a;
		npy = SinLookup(theta_a) * radius_b;

		DrawTriangle(pos, pos + Vec3(npx, npy, 0), pos + Vec3(px, py, 0), color);

		px = npx;
		py = npy;
	}
}

void DrawPie(Vec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, Vec4 color) {
	DrawPie(Vec3(pos, 0), radius_a, radius_b, theta_a, theta_b, color);
}

void DrawPie(Vec3 pos, float radius, float theta_a, float theta_b, Vec4 color) {
	DrawPie(pos, radius, radius, theta_a, theta_b, color);
}

void DrawPie(Vec2 pos, float radius, float theta_a, float theta_b, Vec4 color) {
	DrawPie(Vec3(pos, 0), radius, radius, theta_a, theta_b, color);
}

void DrawPiePart(Vec3 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max, float theta_a, float theta_b, Vec4 color) {
	theta_a = WrapTurns(theta_a);
	theta_b = WrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = Ceil(2 * PI * SquareRoot(0.5f * (radius_a_max * radius_a_max + radius_b_max * radius_b_max)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float min_px = CosLookup(theta_a) * radius_a_min;
	float min_py = SinLookup(theta_a) * radius_b_min;
	float max_px = CosLookup(theta_a) * radius_a_max;
	float max_py = SinLookup(theta_a) * radius_b_max;

	float min_npx, min_npy;
	float max_npx, max_npy;
	for (theta_a += dt; theta_a <= theta_b; theta_a += dt) {
		min_npx = CosLookup(theta_a) * radius_a_min;
		min_npy = SinLookup(theta_a) * radius_b_min;
		max_npx = CosLookup(theta_a) * radius_a_max;
		max_npy = SinLookup(theta_a) * radius_b_max;

		DrawQuad(pos + Vec3(min_npx, min_npy, 0), pos + Vec3(max_npx, max_npy, 0), pos + Vec3(max_px, max_py, 0), pos + Vec3(min_px, min_py, 0), color);

		min_px = min_npx;
		min_py = min_npy;
		max_px = max_npx;
		max_py = max_npy;
	}
}

void DrawPiePart(Vec2 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max, float theta_a, float theta_b, Vec4 color) {
	DrawPiePart(Vec3(pos, 0), radius_a_min, radius_b_min, radius_a_max, radius_b_max, theta_a, theta_b, color);
}

void DrawPiePart(Vec3 pos, float radius_min, float radius_max, float theta_a, float theta_b, Vec4 color) {
	DrawPiePart(pos, radius_min, radius_min, radius_max, radius_max, theta_a, theta_b, color);
}

void DrawPiePart(Vec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, Vec4 color) {
	DrawPiePart(Vec3(pos, 0), radius_min, radius_min, radius_max, radius_max, theta_a, theta_b, color);
}

void DrawLine(Vec3 a, Vec3 b, Vec4 color) {
	if (IsNull(b - a))
		return;

	float thickness = Render.Thickness * 0.5f;
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float ilen = 1.0f / SquareRoot(dx * dx + dy * dy);
	dx *= (thickness * ilen);
	dy *= (thickness * ilen);

	Vec3 c0 = Vec3(a.x - dy, a.y + dx, a.z);
	Vec3 c1 = Vec3(b.x - dy, b.y + dx, b.z);
	Vec3 c2 = Vec3(b.x + dy, b.y - dx, b.z);
	Vec3 c3 = Vec3(a.x + dy, a.y - dx, a.z);

	DrawQuad(c0, c1, c2, c3, Vec2(0, 0), Vec2(0, 1), Vec2(1, 1), Vec2(1, 0), color);
}

void DrawLine(Vec2 a, Vec2 b, Vec4 color) {
	DrawLine(Vec3(a, 0), Vec3(b, 0), color);
}

void PathTo(Vec2 a) {
	if (Render.Path.Count) {
		if (!IsNull(Last(Render.Path) - a)) {
			Push(&Render.Path, a);
		}
		return;
	}
	Push(&Render.Path, a);
}

void ArcTo(Vec2 position, float radius_a, float radius_b, float theta_a, float theta_b) {
	theta_a = WrapTurns(theta_a);
	theta_b = WrapTurns(theta_b);

	if (theta_b < theta_a) {
		float t = theta_a;
		theta_a = theta_b;
		theta_b = t;
	}

	float segments = Ceil(2 * PI * SquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)) / (theta_b - theta_a));
	float dt = 1.0f / segments;

	float npx, npy;
	for (; theta_a <= theta_b; theta_a += dt) {
		npx = CosLookup(theta_a) * radius_a;
		npy = SinLookup(theta_a) * radius_b;
		PathTo(position + Vec2(npx, npy));
	}
}

void BezierQuadraticTo(Vec2 a, Vec2 b, Vec2 c) {
	imem Indices = Render.Path.Count;
	int segments = (int)Ceil(Length(a) + Length(b) + Length(c));
	if (Resize(&Render.Path, Render.Path.Count + segments + 1))
		BuildBezierQuadratic(a, b, c, &Render.Path[Indices], segments);
}

void BezierCubicTo(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
	imem Indices = Render.Path.Count;
	int segments = (int)Ceil(Length(a) + Length(b) + Length(c));
	if (Resize(&Render.Path, Render.Path.Count + segments + 1))
		BuildBezierCubic(a, b, c, d, &Render.Path[Indices], segments);
}

static inline Vec2 LineLineIntersect(Vec2 p1, Vec2 q1, Vec2 p2, Vec2 q2) {
	Vec2 d1 = p1 - q1;
	Vec2 d2 = p2 - q2;

	float d = d1.x * d2.y - d1.y * d2.x;
	float n2 = -d1.x * (p1.y - p2.y) + d1.y * (p1.x - p2.x);

	if (d != 0) {
		float u = n2 / d;
		return p2 - u * d2;
	}

	return p1;
}

void DrawPathStroked(Vec4 color, bool closed, float z) {
	if (Render.Path.Count < 2) {
		Reset(&Render.Path);
		Reset(&Render.Normal);
		return;
	}

	if (Render.Path.Count == 2 && closed) {
		Reset(&Render.Path);
		Reset(&Render.Normal);
		return;
	}

	if (closed) {
		PathTo(Render.Path[0]);
	}

	if (!Resize(&Render.Normal, Render.Path.Count - 1)) {
		Reset(&Render.Path);
		Reset(&Render.Normal);
		return;
	}

	for (imem index = 0; index < Render.Normal.Count; ++index) {
		Vec2 v = Normalize(Render.Path[index + 1] - Render.Path[index]);
		Render.Normal[index] = v;
	}

	float thickness = 0.5f * Render.Thickness;

	Vec2 p, q;
	Vec2 start_p, start_q;
	Vec2 prev_p, prev_q;
	Vec2 a, b, c;
	Vec2 v1, v2, n1;
	Vec2 n2, t1, t2;
	Vec2 p1, q1, p2, q2;

	if (closed) {
		a = Last(Render.Path);
		b = Render.Path[0];
		c = Render.Path[1];

		v1 = Last(Render.Normal);
		v2 = Render.Normal[0];

		n1 = Vec2(-v1.y, v1.x);
		n2 = Vec2(-v2.y, v2.x);

		t1 = n1 * thickness;
		t2 = n2 * thickness;

		p1 = b + t1;
		q1 = p1 + v1;
		p2 = b + t2;
		q2 = p2 + v2;
		start_p = prev_p = LineLineIntersect(p1, q1, p2, q2);

		p1 = b - t1;
		q1 = p1 + v1;
		p2 = b - t2;
		q2 = p2 + v2;
		start_q = prev_q = LineLineIntersect(p1, q1, p2, q2);
	} else {
		Vec2 v, t, n;

		v = Render.Normal[0];
		n = Vec2(-v.y, v.x);
		t = thickness * n;
		prev_p = Render.Path[0] + t;
		prev_q = Render.Path[0] - t;

		v = Last(Render.Normal);
		n = Vec2(-v.y, v.x);
		t = thickness * n;
		start_p = Last(Render.Path) + t;
		start_q = Last(Render.Path) - t;
	}

	for (imem index = 0; index + 1 < Render.Normal.Count; ++index) {
		a = Render.Path[index + 0];
		b = Render.Path[index + 1];
		c = Render.Path[index + 2];

		v1 = Render.Normal[index + 0];
		v2 = Render.Normal[index + 1];

		n1 = Vec2(-v1.y, v1.x);
		n2 = Vec2(-v2.y, v2.x);

		t1 = n1 * thickness;
		t2 = n2 * thickness;

		{
			p1 = b + t1;
			q1 = p1 + v1;
			p2 = b + t2;
			q2 = p2 + v2;
			p = LineLineIntersect(p1, q1, p2, q2);
		}

		{
			p1 = b - t1;
			q1 = p1 + v1;
			p2 = b - t2;
			q2 = p2 + v2;
			q = LineLineIntersect(p1, q1, p2, q2);
		}

		DrawQuad(prev_q, prev_p, p, q, color);

		prev_p = p;
		prev_q = q;
	}

	DrawQuad(prev_q, prev_p, start_p, start_q, color);

	Reset(&Render.Path);
	Reset(&Render.Normal);
}

void DrawPathFilled(Vec4 color, float z) {
	if (Render.Path.Count < 3) {
		Reset(&Render.Path);
		return;
	}

	Vec2 *path = Render.Path.Data;
	int triangle_count = (int)Render.Path.Count - 2;
	for (int ti = 0; ti < triangle_count; ++ti) {
		DrawTriangle(Vec3(path[0], z), Vec3(path[ti + 1], z), Vec3(path[ti + 2], z), color);
	}

	Reset(&Render.Path);
}

void DrawBezierQuadratic(Vec2 a, Vec2 b, Vec2 c, Vec4 color, float z) {
	BezierQuadraticTo(a, b, c);
	DrawPathStroked(color, false, z);
}

void DrawBezierCubic(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color, float z) {
	BezierCubicTo(a, b, c, d);
	DrawPathStroked(color, false, z);
}

void DrawPolygon(const Vec2 *vertices, u32 count, float z, Vec4 color) {
	Assert(count >= 3);
	u32 triangle_count = count - 2;
	for (u32 triangle_index = 0; triangle_index < triangle_count; ++triangle_index) {
		DrawTriangle(Vec3(vertices[0], z), Vec3(vertices[triangle_index + 1], z), Vec3(vertices[triangle_index + 2], z), color);
	}
}

void DrawPolygon(const Vec2 *vertices, u32 count, Vec4 color) {
	DrawPolygon(vertices, count, 0, color);
}

void DrawTriangleOutline(Vec3 a, Vec3 b, Vec3 c, Vec4 color) {
	PathTo(a._0.xy);
	PathTo(b._0.xy);
	PathTo(c._0.xy);
	DrawPathStroked(color, true, a.z);
}

void DrawTriangleOutline(Vec2 a, Vec2 b, Vec2 c, Vec4 color) {
	PathTo(a);
	PathTo(b);
	PathTo(c);
	DrawPathStroked(color, true, 0.0f);
}

void DrawQuadOutline(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec4 color) {
	PathTo(a._0.xy);
	PathTo(b._0.xy);
	PathTo(c._0.xy);
	PathTo(d._0.xy);
	DrawPathStroked(color, true, a.z);
}

void DrawQuadOutline(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec4 color) {
	PathTo(a);
	PathTo(b);
	PathTo(c);
	PathTo(d);
	DrawPathStroked(color, true, 0.0f);
}

void DrawRectOutline(Vec3 pos, Vec2 dim, Vec4 color) {
	Vec3 a = pos;
	Vec3 b = pos + Vec3(0, dim.y, 0);
	Vec3 c = pos + Vec3(dim, 0);
	Vec3 d = pos + Vec3(dim.x, 0, 0);
	DrawQuadOutline(a, b, c, d, color);
}

void DrawRectOutline(Vec2 pos, Vec2 dim, Vec4 color) {
	DrawRectOutline(Vec3(pos, 0), dim, color);
}

void DrawRectCenteredOutline(Vec3 pos, Vec2 dim, Vec4 color) {
	Vec2 half_dim = 0.5f * dim;

	Vec3 a, b, c, d;
	a._0.xy = pos._0.xy - half_dim;
	b._0.xy = Vec2(pos.x - half_dim.x, pos.y + half_dim.y);
	c._0.xy = pos._0.xy + half_dim;
	d._0.xy = Vec2(pos.x + half_dim.x, pos.y - half_dim.y);

	a.z = pos.z;
	b.z = pos.z;
	c.z = pos.z;
	d.z = pos.z;
	DrawQuadOutline(a, b, c, d, color);
}

void DrawRectCenteredOutline(Vec2 pos, Vec2 dim, Vec4 color) {
	DrawRectCenteredOutline(Vec3(pos, 0), dim, color);
}

void DrawEllipseOutline(Vec3 pos, float radius_a, float radius_b, Vec4 color) {
	int length = (int)Ceil(2 * PI * SquareRoot(0.5f * (radius_a * radius_a + radius_b * radius_b)));
	int segments = Max(MIN_CIRCLE_SEGMENTS, (int)length);
	float npx, npy;
	for (int index = 0; index <= segments; ++index) {
		npx = CosLookup(index, segments) * radius_a;
		npy = SinLookup(index, segments) * radius_b;
		PathTo(pos._0.xy + Vec2(npx, npy));
	}
	DrawPathStroked(color, true, pos.z);
}

void DrawEllipseOutline(Vec2 pos, float radius_a, float radius_b, Vec4 color) {
	DrawEllipseOutline(Vec3(pos, 0), radius_a, radius_b, color);
}

void DrawCircleOutline(Vec3 pos, float radius, Vec4 color) {
	DrawEllipseOutline(pos, radius, radius, color);
}

void DrawCircleOutline(Vec2 pos, float radius, Vec4 color) {
	DrawEllipseOutline(Vec3(pos, 0), radius, radius, color);
}

void DrawArcOutline(Vec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, Vec4 color, bool closed) {
	ArcTo(pos._0.xy, radius_a, radius_b, theta_a, theta_b);
	if (closed) {
		PathTo(pos._0.xy);
	}
	DrawPathStroked(color, closed, pos.z);
}

void DrawArcOutline(Vec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, Vec4 color, bool closed) {
	DrawArcOutline(Vec3(pos, 0), radius_a, radius_b, theta_a, theta_b, color, closed);
}

void DrawArcOutline(Vec3 pos, float radius, float theta_a, float theta_b, Vec4 color, bool closed) {
	DrawArcOutline(pos, radius, radius, theta_a, theta_b, color, closed);
}

void DrawArcOutline(Vec2 pos, float radius, float theta_a, float theta_b, Vec4 color, bool closed) {
	DrawArcOutline(Vec3(pos, 0), radius, radius, theta_a, theta_b, color, closed);
}

void DrawPolygonOutline(const Vec2 *vertices, u32 count, float z, Vec4 color) {
	for (u32 Indices = 0; Indices < count; ++Indices) {
		PathTo(vertices[Indices]);
	}
	DrawPathStroked(color, true, z);
}

void DrawPolygonOutline(const Vec2 *vertices, u32 count, Vec4 color) {
	DrawPolygonOutline(vertices, count, 0, color);
}

void DrawTexture(PL_Texture *texture, Vec3 pos, Vec2 dim, Vec4 color) {
	PushTexture(texture);
	DrawRect(pos, dim, color);
	PopTexture();
}

void DrawTexture(PL_Texture *texture, Vec2 pos, Vec2 dim, Vec4 color) {
	PushTexture(texture);
	DrawRect(pos, dim, color);
	PopTexture();
}

void DrawTextureCentered(PL_Texture *texture, Vec3 pos, Vec2 dim, Vec4 color) {
	PushTexture(texture);
	DrawRectCentered(pos, dim, color);
	PopTexture();
}

void DrawTextureCentered(PL_Texture *texture, Vec2 pos, Vec2 dim, Vec4 color) {
	PushTexture(texture);
	DrawRectCentered(pos, dim, color);
	PopTexture();
}

void DrawTexture(PL_Texture *texture, Vec3 pos, Vec2 dim, Region  rect, Vec4 color) {
	PushTexture(texture);
	DrawRect(pos, dim, rect, color);
	PopTexture();
}

void DrawTexture(PL_Texture *texture, Vec2 pos, Vec2 dim, Region rect, Vec4 color) {
	PushTexture(texture);
	DrawRect(pos, dim, rect, color);
	PopTexture();
}

void DrawTextureCentered(PL_Texture *texture, Vec3 pos, Vec2 dim, Region rect, Vec4 color) {
	PushTexture(texture);
	DrawRectCentered(pos, dim, rect, color);
	PopTexture();
}

void DrawTextureCentered(PL_Texture *texture, Vec2 pos, Vec2 dim, Region rect, Vec4 color) {
	PushTexture(texture);
	DrawRectCentered(pos, dim, rect, color);
	PopTexture();
}

void DrawRoundedRect(Vec3 pos, Vec2 dim, Vec4 color, float radius) {
	if (radius) {
		float rad_x = Min(radius, 0.5f * dim.x);
		float rad_y = Min(radius, 0.5f * dim.y);

		Vec2 pos2d = pos._0.xy;

		Vec2 p0, p1, p2, p3;

		p0 = pos2d + Vec2(rad_x, rad_y);
		p1 = pos2d + Vec2(dim.x - rad_x, rad_y);
		p2 = pos2d + dim - Vec2(rad_x, rad_y);
		p3 = pos2d + Vec2(rad_x, dim.y - rad_y);

		ArcTo(p0, rad_x, rad_y, 2.0f / 4.0f, 3.0f / 4.0f);
		ArcTo(p1, rad_x, rad_y, 3.0f / 4.0f, 4.0f / 4.0f);
		ArcTo(p2, rad_x, rad_y, 0.0f / 4.0f, 1.0f / 4.0f);
		ArcTo(p3, rad_x, rad_y, 1.0f / 4.0f, 2.0f / 4.0f);

		DrawPathFilled(color, pos.z);
	} else {
		DrawRect(pos, dim, color);
	}
}

void DrawRoundedRect(Vec2 pos, Vec2 dim, Vec4 color, float radius) {
	DrawRoundedRect(Vec3(pos, 0), dim, color, radius);
}

void DrawRoundedRectOutline(Vec3 pos, Vec2 dim, Vec4 color, float radius) {
	if (radius) {
		float rad_x = Min(radius, 0.5f * dim.x);
		float rad_y = Min(radius, 0.5f * dim.y);

		Vec2 pos2d = pos._0.xy;

		Vec2 p0, p1, p2, p3;

		p0 = pos2d + Vec2(rad_x, rad_y);
		p1 = pos2d + Vec2(dim.x - rad_x, rad_y);
		p2 = pos2d + dim - Vec2(rad_x, rad_y);
		p3 = pos2d + Vec2(rad_x, dim.y - rad_y);

		ArcTo(p0, rad_x, rad_y, 2.0f / 4.0f, 3.0f / 4.0f);
		ArcTo(p1, rad_x, rad_y, 3.0f / 4.0f, 4.0f / 4.0f);
		ArcTo(p2, rad_x, rad_y, 0.0f / 4.0f, 1.0f / 4.0f);
		ArcTo(p3, rad_x, rad_y, 1.0f / 4.0f, 2.0f / 4.0f);

		DrawPathStroked(color, true, pos.z);
	} else {
		DrawRect(pos, dim, color);
	}
}

void DrawRoundedRectOutline(Vec2 pos, Vec2 dim, Vec4 color, float radius) {
	DrawRoundedRectOutline(Vec3(pos, 0), dim, color, radius);
}

static const GlyphInfo *FindGlyphInfo(Font *font, u32 codepoint) {
	if (codepoint < font->Index.Count) {
		u16 index = font->Index[codepoint];
		return &font->Glyphs[index];
	}
	return font->Fallback;
}

float CalculateText(String text, Font *font, float height) {
	float dist = 0;

	u32 codepoint;
	u8 *ptr = text.begin();
	u8 *end = text.end();

	while (ptr < end) {
		int len = UTF8ToCodepoint(ptr, end, &codepoint);
		ptr += len;

		auto info = FindGlyphInfo(font, codepoint);
		dist += height * info->Advance;
	}

	return dist;
}

float CalculateText(String text, float height) {
	Font *font = &Render.BuiltinFont;
	return CalculateText(text, font, height);
}

void DrawText(String text, Vec3 pos, Font *font, Vec4 color, float height) {
	PushShader(Render.BuiltinShaders[PL_EmbeddedShader_Font]);
	PushTexture(font->Texture);

	u32 codepoint;
	u8 *ptr = text.begin();
	u8 *end = text.end();

	while (ptr < end) {
		int len = UTF8ToCodepoint(ptr, end, &codepoint);
		ptr += len;

		auto info = FindGlyphInfo(font, codepoint);

		Vec3 draw_pos = pos;
		draw_pos._0.xy += height * Vec2(info->Bearing.x, info->Bearing.y);
		DrawRect(draw_pos, height * info->Size, info->Rect, color);
		pos.x += height * info->Advance;
	}

	PopTexture();
	PopShader();
}

void DrawText(String text, Vec2 pos, Font *font, Vec4 color, float height) {
	DrawText(text, Vec3(pos, 0), font, color, height);
}

void DrawText(String text, Vec3 pos, Vec4 color, float height) {
	Font *font = &Render.BuiltinFont;
	DrawText(text, pos, font, color, height);
}

void DrawText(String text, Vec2 pos, Vec4 color, float height) {
	Font *font = &Render.BuiltinFont;
	DrawText(text, pos, font, color, height);
}

