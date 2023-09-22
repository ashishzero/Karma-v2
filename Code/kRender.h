#pragma once
#include "kCommon.h"

#define K_MAX_CIRCLE_SEGMENTS  512
#define K_INITIAL_VERTEX_ALLOC 64*4
#define K_INITIAL_INDEX_ALLOC  64*6
#define K_INITIAL_PATH_ALLOC   256

//
//
//

typedef struct kVertex2D {
	kVec2 pos;
	kVec2 tex;
	kVec4 col;
} kVertex2D;

typedef u32 kIndex2D;

//
//
//

void kDrawTriangleUvGradient(
		kVec2 va, kVec2 vb, kVec2 vc,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 ca, kVec4 cb, kVec4 cc);
void kDrawTriangleUv(
		kVec2 a, kVec2 b, kVec2 c,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 col);
void kDrawTriangleGradient(
		kVec2 a, kVec2 b, kVec2 c,
		kVec4 ca, kVec4 cb, kVec4 cc);
void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec4 color);

void kDrawQuadUvGradient(
	kVec2 va, kVec2 vb, kVec2 vc, kVec2 vd,
	kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawQuadUv(
	kVec2 a, kVec2 b, kVec2 c, kVec2 d,
	kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color);
void kDrawQuadGradient(
	kVec2 a, kVec2 b, kVec2 c, kVec2 d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);

void kDrawRectUvGradient(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectUv(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectGradient(
	kVec2 pos, kVec2 dim,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRect(kVec2 pos, kVec2 dim, kVec4 color);

void kDrawRectRotatedUvGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectRotatedUv(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectRotatedGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);

void kDrawRectCenteredUvGradient(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectCenteredUv(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectCenteredGradient(
	kVec2 pos, kVec2 dim,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec4 color);

void kDrawRectCenteredRotatedUvGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectCenteredRotatedUv(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectCenteredRotatedGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);

void kDrawRoundedRect(kVec2 pos, kVec2 dim, kVec4 color, float radius);

void kDrawEllipseGradient(kVec2 pos, float radius_a, float radius_b, kVec4 center, kVec4 first, kVec4 last);
void kDrawEllipse(kVec2 pos, float radius_a, float radius_b, kVec4 color);
void kDrawCircleGradient(kVec2 pos, float radius, kVec4 center, kVec4 first, kVec4 last);
void kDrawCircle(kVec2 pos, float radius, kVec4 color);

void kDrawEllipticPieGradient(
	kVec2 pos, float radius_a, float radius_b,
	float theta_a, float theta_b,
	kVec4 center, kVec4 first, kVec4 last);
void kDrawEllipticPie(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color);
void kDrawPieGradient(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 center, kVec4 first, kVec4 last);
void kDrawPie(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color);

void kDrawEllipticPiePartGradient(
	kVec2 pos, float radius_a_min, float radius_b_min,
	float radius_a_max, float radius_b_max,
	float theta_a, float theta_b,
	kVec4 minc_a, kVec4 minc_b, kVec4 maxc_a, kVec4 maxc_b);
void kDrawEllipticPiePart(
	kVec3 pos, float radius_a_min, float radius_b_min,
	float radius_a_max, float radius_b_max,
	float theta_a, float theta_b, kVec4 color);
void kDrawPiePartGradient(
	kVec2 pos, float radius_min, float radius_max,
	float theta_a, float theta_b,
	kVec4 minc_a, kVec4 minc_b, kVec4 maxc_a, kVec4 maxc_b);
void kDrawPiePart(kVec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color);

void kDrawPolygon(kVec2 *vertices, u32 count, kVec4 color);
void kDrawPolygonGradient(kVec2 *vertices, kVec4 *colors, u32 count);
void kDrawPolygonUvGradient(kVec2 *vertices, const kVec2 *uvs, kVec4 *colors, u32 count);
void kDrawTriangleList(kVertex2D *vertices, u32 count);

void kDrawLine(kVec3 a, kVec3 b, kVec4 color);
void kPathTo(kVec2 a);
void kArcTo(kVec2 position, float radius_a, float radius_b, float theta_a, float theta_b);
void kBezierQuadraticTo(kVec2 a, kVec2 b, kVec2 c);
void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d);

void kDrawPathStroked(kVec4 color, bool closed);
void kDrawPathFilled(kVec4 color);
void kDrawBezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec4 color);
void kDrawBezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);

void kDrawTriangleOutline(kVec2 a, kVec2 b, kVec2 c, kVec4 color);
void kDrawQuadOutline(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);
void kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color);
void kDrawRectCenteredOutline(kVec2 pos, kVec2 dim, kVec4 color);
void kDrawEllipseOutline(kVec2 pos, float radius_a, float radius_b, kVec4 color);
void kDrawCircleOutline(kVec2 pos, float radius, kVec4 color);
void kDrawEllipticArcOutline(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed);
void kDrawArcOutline(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed);
void kDrawPolygonOutline(const kVec2 *vertices, u32 count, kVec4 color);
void kDrawRoundedRectOutline(kVec2 pos, kVec2 dim, kVec4 color, float radius);
