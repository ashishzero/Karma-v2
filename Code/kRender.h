#pragma once
#include "kMath.h"
#include "kRenderApi.h"

#define K_MAX_CIRCLE_SEGMENTS  512
#define K_MAX_TEXTURE_SLOTS    2

//
//
//

typedef struct kVertex2D {
	kVec2 pos;
	kVec2 tex;
	kVec4 col;
} kVertex2D;

typedef u32 kIndex2D;

typedef struct kRect {
	kVec2 min;
	kVec2 max;
} kRect;

typedef struct kGlyph {
	kRect rect;
	kVec2 bearing;
	kVec2 size;
	float advance;
} kGlyph;

typedef struct kFont {
	kTexture   texture;
	u16      * map;
	kGlyph   * glyphs;
	kGlyph   * fallback;
	u32        largest;
	u32        count;
} kFont;

typedef struct kRenderParam2D {
	kTexture textures[K_MAX_TEXTURE_SLOTS];
	kMat4    transform;
	kRect    rect;
	i32      vertex;
	u32      index;
	u32      count;
} kRenderParam2D;

typedef struct kRenderCommand2D {
	kShader shader;
	u32     params;
	u32     count;
} kRenderCommand2D;

enum kRenderPassFlags {
	kRenderPass_ClearTarget = 0x1
};

typedef struct kRenderPass2D {
	kTexture target;
	u32      commands;
	u32      count;
	u32      flags;
	kVec4    color;
} kRenderPass2D;

typedef struct kRenderPassList2D {
	u32                count;
	kRenderPass2D    * passes;
	kRenderCommand2D * commands;
	kRenderParam2D   * params;
} kRenderPassList2D;

//
//
//

void    kCreateRenderContext(void);
void    kDestroyRenderContext(void);

void    kFlushFrame(void);
void    kGetRenderPassList2D(kRenderPassList2D *passes);

void    kBeginRenderPass(kTexture texture, kVec4 *color);
void    kBeginDefaultRenderPass(kVec4 *color);
void    kEndRenderPass(void);

void    kBeginCameraRect(float left, float right, float bottom, float top);
void    kBeginCamera(float aspect_ratio, float height);
void    kEndCamera(void);

void    kLineThickness(float thickness);

void    kFlushRenderCommand(void);
void    kSetShader(kShader shader);
void    kPushShader(kShader shader);
void    kPopShader(void);

void    kFlushRenderParam(void);

void    kSetTexture(kTexture texture, uint idx);
void    kPushTexture(kTexture texture, uint idx);
void    kPopTexture(uint idx);

void    kSetRect(kRect rect);
void    kSetRectEx(float x, float y, float w, float h);
void    kPushRect(kRect rect);
void    kPushRectEx(float x, float y, float w, float h);
void    PopRect(void);

void    kSetTransform(const kMat4 *transform);
void    kPushTransform(const kMat4 *transform);
void    kTranslateRotate(kVec2 pos, float angle);
void    kPopTransform(void);

void    kDrawTriangleUvGradient(
		kVec2 va, kVec2 vb, kVec2 vc,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 ca, kVec4 cb, kVec4 cc);
void    kDrawTriangleUv(
		kVec2 a, kVec2 b, kVec2 c,
		kVec2 ta, kVec2 tb, kVec2 tc,
		kVec4 col);
void    kDrawTriangleGradient(
		kVec2 a, kVec2 b, kVec2 c,
		kVec4 ca, kVec4 cb, kVec4 cc);
void    kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec4 color);

void    kDrawQuadUvGradient(
	kVec2 va, kVec2 vb, kVec2 vc, kVec2 vd,
	kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawQuadUv(
	kVec2 a, kVec2 b, kVec2 c, kVec2 d,
	kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color);
void    kDrawQuadGradient(
	kVec2 a, kVec2 b, kVec2 c, kVec2 d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);

void    kDrawRectUvGradient(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectUv(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void    kDrawRectGradient(
	kVec2 pos, kVec2 dim,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRect(kVec2 pos, kVec2 dim, kVec4 color);

void    kDrawRectRotatedUvGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectRotatedUv(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void    kDrawRectRotatedGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);

void    kDrawRectCenteredUvGradient(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectCenteredUv(
	kVec2 pos, kVec2 dim,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void    kDrawRectCenteredGradient(
	kVec2 pos, kVec2 dim,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectCentered(kVec2 pos, kVec2 dim, kVec4 color);

void    kDrawRectCenteredRotatedUvGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectCenteredRotatedUv(
	kVec2 pos, kVec2 dim, float angle,
	kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void    kDrawRectCenteredRotatedGradient(
	kVec2 pos, kVec2 dim, float angle,
	kVec4 ca, kVec4 cb, kVec4 cc, kVec4 cd);
void    kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);

void    kDrawRoundedRect(kVec2 pos, kVec2 dim, kVec4 color, float radius);

void    kDrawEllipseGradient(kVec2 pos, float radius_a, float radius_b, kVec4 center, kVec4 first, kVec4 last);
void    kDrawEllipse(kVec2 pos, float radius_a, float radius_b, kVec4 color);
void    kDrawCircleGradient(kVec2 pos, float radius, kVec4 center, kVec4 first, kVec4 last);
void    kDrawCircle(kVec2 pos, float radius, kVec4 color);

void    kDrawEllipticPieGradient(
	kVec2 pos, float radius_a, float radius_b,
	float theta_a, float theta_b,
	kVec4 center, kVec4 first, kVec4 last);
void    kDrawEllipticPie(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color);
void    kDrawPieGradient(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 center, kVec4 first, kVec4 last);
void    kDrawPie(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color);

void    kDrawEllipticPiePartGradient(
	kVec2 pos, float radius_a_min, float radius_b_min,
	float radius_a_max, float radius_b_max,
	float theta_a, float theta_b,
	kVec4 minc_a, kVec4 minc_b, kVec4 maxc_a, kVec4 maxc_b);
void    kDrawEllipticPiePart(
	kVec3 pos, float radius_a_min, float radius_b_min,
	float radius_a_max, float radius_b_max,
	float theta_a, float theta_b, kVec4 color);
void    kDrawPiePartGradient(
	kVec2 pos, float radius_min, float radius_max,
	float theta_a, float theta_b,
	kVec4 minc_a, kVec4 minc_b, kVec4 maxc_a, kVec4 maxc_b);
void    kDrawPiePart(kVec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color);

void    kDrawPolygon(kVec2 *vertices, u32 count, kVec4 color);
void    kDrawPolygonGradient(kVec2 *vertices, kVec4 *colors, u32 count);
void    kDrawPolygonUvGradient(kVec2 *vertices, const kVec2 *uvs, kVec4 *colors, u32 count);
void    kDrawTriangleList(kVertex2D *vertices, u32 count);

void    kDrawLine(kVec3 a, kVec3 b, kVec4 color);
void    kPathTo(kVec2 a);
void    kArcTo(kVec2 position, float radius_a, float radius_b, float theta_a, float theta_b);
void    kBezierQuadraticTo(kVec2 a, kVec2 b, kVec2 c);
void    kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d);

void    kDrawPathStroked(kVec4 color, bool closed);
void    kDrawPathFilled(kVec4 color);
void    kDrawBezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec4 color);
void    kDrawBezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);

void    kDrawTriangleOutline(kVec2 a, kVec2 b, kVec2 c, kVec4 color);
void    kDrawQuadOutline(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);
void    kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color);
void    kDrawRectCenteredOutline(kVec2 pos, kVec2 dim, kVec4 color);
void    kDrawEllipseOutline(kVec2 pos, float radius_a, float radius_b, kVec4 color);
void    kDrawCircleOutline(kVec2 pos, float radius, kVec4 color);
void    kDrawEllipticArcOutline(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed);
void    kDrawArcOutline(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed);
void    kDrawPolygonOutline(const kVec2 *vertices, u32 count, kVec4 color);
void    kDrawRoundedRectOutline(kVec2 pos, kVec2 dim, kVec4 color, float radius);

void    kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color);
void    kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color);

void    kDrawTextureMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color);
void    kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color);

kGlyph *kFindFontGlyph(kFont *font, u32 codepoint);
float   kCalculateText(kString text, kFont *font, float height);
float   kCalculateBuiltinText(kString text, float height);
void    kDrawText(kString text, kVec2 pos, kFont *font, kVec4 color, float height);
void    kDrawBuilinText(kString text, kVec2 pos, kVec4 color, float height);
