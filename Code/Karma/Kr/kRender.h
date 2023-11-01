#pragma once
#include "kRenderShared.h"

typedef struct kGlyph
{
	kRect  Rect;
	kVec2i Bearing;
	kVec2i Size;
	kVec2i Advance;
} kGlyph;

typedef struct kFont
{
	kTexture *Atlas;
	u16      *CodepointMap;
	kGlyph   *Glyphs;
	kGlyph   *Fallback;
	u32       MinCodepoint;
	u32       MaxCodepoint;
	u32       GlyphsCount;
	i16       Height;
	i16       Ascent;
	i16       Descent;
	i16       Linegap;
} kFont;

//
//
//

typedef struct kRenderMemory
{
	u32   VertexSize;
	u32   IndexSize;
	u32   SceneSize;
	u32   CommandSize;
	float MB;
} kRenderMemoryUsage;

//
//
//

typedef struct kRenderSpec
{
	u32 Vertices;
	u32 Indices;
	u32 Commands;
	u32 Scenes;
} kRenderSpec;

static constexpr kRenderSpec kDefaultRenderSpec = {.Vertices = 1048576,
                                                   .Indices  = 1048576 * 6,
                                                   .Commands = 512,
                                                   .Scenes   = 64};

//
//
//

void kCreateRenderContext(const kRenderSpec &spec, kTexture *textures[kTextureType_Count], kFont *font);
void kDestroyRenderContext(void);
void kGetRenderMemoryUsage(kRenderMemory *usage);
void kGetRenderMemoryCaps(kRenderMemory *usage);

void kResetFrame(void);
void kGetFrameData(kRenderFrame2D *frame);

//
//
//

void kBeginScene(const kCamera2D &camera, kRect region);
void kBeginScene(float left, float right, float top, float bottom, float near, float far, kRect region);
void kBeginScene(float ar, float height, kRect region);
void kEndScene(void);

void kLineThickness(float thickness);

void kSetBlendMode(kBlendMode mode);
void kBeginBlendMode(kBlendMode mode);
void kEndBlendMode(void);

void kSetTextureFilter(kTextureFilter filter);

void kSetTexture(kTexture *texture, kTextureType idx);
void kPushTexture(kTexture *texture, kTextureType idx);
void kPopTexture(kTextureType idx);

void kSetOutLineStyle(kVec3 color, float width);
void kPushOutLineStyle(kVec3 color, float width);
void kPopOutLineStyle(void);

void kSetRect(kRect rect);
void kSetRectEx(float x, float y, float w, float h);
void kPushRect(kRect rect);
void kPushRectEx(float x, float y, float w, float h);
void kPopRect(void);

void kSetTransform(const kMat3 &transform);
void kPushTransform(const kMat3 &transform);
void kPushTransform(kVec2 pos, float angle);
void kPopTransform(void);

void kDrawTriangle(kVec3 va, kVec3 vb, kVec3 vc, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 ca, kVec4 cb, kVec4 cc);
void kDrawTriangle(kVec3 a, kVec3 b, kVec3 c, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 col);
void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 col);
void kDrawTriangle(kVec3 a, kVec3 b, kVec3 c, kVec4 color);
void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec4 color);

void kDrawQuad(kVec3 va, kVec3 vb, kVec3 vc, kVec3 vd, kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color);
void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawQuad(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec4 color);
void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);
void kDrawQuad(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kRect rect, kVec4 color);
void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kRect rect, kVec4 color);

void kDrawRect(kVec3 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRect(kVec2 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRect(kVec3 pos, kVec2 dim, kVec4 color);
void kDrawRect(kVec2 pos, kVec2 dim, kVec4 color);
void kDrawRect(kVec3 pos, kVec2 dim, kRect rect, kVec4 color);
void kDrawRect(kVec2 pos, kVec2 dim, kRect rect, kVec4 color);

void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kVec4 color);
void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);
void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kRect rect, kVec4 color);
void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kRect rect, kVec4 color);

void kDrawRectCentered(kVec3 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
void kDrawRectCentered(kVec3 pos, kVec2 dim, kVec4 color);
void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec4 color);
void kDrawRectCentered(kVec3 pos, kVec2 dim, kRect rect, kVec4 color);
void kDrawRectCentered(kVec2 pos, kVec2 dim, kRect rect, kVec4 color);

void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
                              kVec4 color);
void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
                              kVec4 color);
void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kVec4 color);
void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);
void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kRect rect, kVec4 color);
void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kRect rect, kVec4 color);

void kDrawEllipse(kVec3 pos, float radius_a, float radius_b, kVec4 color);
void kDrawEllipse(kVec2 pos, float radius_a, float radius_b, kVec4 color);

void kDrawCircle(kVec3 pos, float radius, kVec4 color);
void kDrawCircle(kVec2 pos, float radius, kVec4 color);

void kDrawPie(kVec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color);
void kDrawPie(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color);
void kDrawPie(kVec3 pos, float radius, float theta_a, float theta_b, kVec4 color);
void kDrawPie(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color);

void kDrawPiePart(kVec3 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max,
                  float theta_a, float theta_b, kVec4 color);
void kDrawPiePart(kVec2 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max,
                  float theta_a, float theta_b, kVec4 color);
void kDrawPiePart(kVec3 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color);
void kDrawPiePart(kVec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color);

void kDrawLine(kVec3 a, kVec3 b, kVec4 color);
void kDrawLine(kVec2 a, kVec2 b, kVec4 color);

void kPathTo(kVec2 a);
void kArcTo(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b);
void kBezierQuadraticTo(kVec2 a, kVec2 b, kVec2 c);
void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d);

void kDrawPathStroked(kVec4 color, bool closed = false, float z = 1.0f);
void kDrawPathFilled(kVec4 color, float z = 1.0f);

void kDrawBezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec4 color, float z = 0);
void kDrawBezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color, float z = 0);

void kDrawPolygon(const kVec2 *vertices, u32 count, float z, kVec4 color);
void kDrawPolygon(const kVec2 *vertices, u32 count, kVec4 color);

void kDrawTriangleOutline(kVec3 a, kVec3 b, kVec3 c, kVec4 color);
void kDrawTriangleOutline(kVec2 a, kVec2 b, kVec2 c, kVec4 color);

void kDrawQuadOutline(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec4 color);
void kDrawQuadOutline(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);

void kDrawRectOutline(kVec3 pos, kVec2 dim, kVec4 color);
void kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color);

void kDrawRectCenteredOutline(kVec3 pos, kVec2 dim, kVec4 color);
void kDrawRectCenteredOutline(kVec2 pos, kVec2 dim, kVec4 color);

void kDrawEllipseOutline(kVec3 pos, float radius_a, float radius_b, kVec4 color);
void kDrawEllipseOutline(kVec2 pos, float radius_a, float radius_b, kVec4 color);

void kDrawCircleOutline(kVec3 pos, float radius, kVec4 color);
void kDrawCircleOutline(kVec2 pos, float radius, kVec4 color);

void kDrawArcOutline(kVec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed);
void kDrawArcOutline(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color, bool closed);
void kDrawArcOutline(kVec3 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed);
void kDrawArcOutline(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed);

void kDrawPolygonOutline(const kVec2 *vertices, u32 count, float z, kVec4 color);
void kDrawPolygonOutline(const kVec2 *vertices, u32 count, kVec4 color);

void kDrawTexture(kTexture *texture, kVec3 pos, kVec2 dim, kVec4 color = kVec4(1));
void kDrawTexture(kTexture *texture, kVec2 pos, kVec2 dim, kVec4 color = kVec4(1));
void kDrawTextureCentered(kTexture *texture, kVec3 pos, kVec2 dim, kVec4 color = kVec4(1));
void kDrawTextureCentered(kTexture *texture, kVec2 pos, kVec2 dim, kVec4 color = kVec4(1));
void kDrawTexture(kTexture *texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
void kDrawTexture(kTexture *texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
void kDrawTextureCentered(kTexture *texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
void kDrawTextureCentered(kTexture *texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
void kDrawTextureMasked(kTexture *texture, kTexture *mask, kVec3 pos, kVec2 dim, kVec4 color);
void kDrawTextureCenteredMasked(kTexture *texture, kTexture *mask, kVec3 pos, kVec2 dim, kVec4 color);
void kDrawTextureMasked(kTexture *texture, kTexture *mask, kVec2 pos, kVec2 dim, kVec4 color);
void kDrawTextureCenteredMasked(kTexture *texture, kTexture *mask, kVec2 pos, kVec2 dim, kVec4 color);

void kDrawRoundedRect(kVec3 pos, kVec2 dim, kVec4 color, float radius = 1.0f);
void kDrawRoundedRect(kVec2 pos, kVec2 dim, kVec4 color, float radius = 1.0f);

void kDrawRoundedRectOutline(kVec3 pos, kVec2 dim, kVec4 color, float radius = 1.0f);
void kDrawRoundedRectOutline(kVec2 pos, kVec2 dim, kVec4 color, float radius = 1.0f);

kGlyph *kFindFontGlyph(const kFont *font, u32 codepoint);
kVec2   kAdjectCursorToBaseline(const kFont *font, kVec2 cursor, float scale);
bool    kCalculateGlyphMetrics(const kFont *font, u32 codepoint, float scale, kVec2 *cursor, kVec2 *rpos, kVec2 *rsize,
                               kRect *rect);

void    kDrawTextQuad(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale = 1);
void    kDrawTextQuad(u32 codepoint, const kFont *font, kVec2 pos, kVec4 color, float scale = 1);
void    kDrawTextQuadRotated(u32 codepoint, const kFont *font, kVec3 pos, float angle, kVec4 color, float scale = 1);
void    kDrawTextQuadRotated(u32 codepoint, const kFont *font, kVec2 pos, float angle, kVec4 color, float scale = 1);
void    kDrawTextQuadCentered(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale = 1);
void    kDrawTextQuadCentered(u32 codepoint, const kFont *font, kVec2 pos, kVec4 color, float scale = 1);
void    kDrawTextQuadCenteredRotated(u32 codepoint, const kFont *font, kVec3 pos, float angle, kVec4 color,
                                     float scale = 1);
void    kDrawTextQuadCenteredRotated(u32 codepoint, const kFont *font, kVec2 pos, float angle, kVec4 color,
                                     float scale = 1);

void    kDrawTextQuad(u32 codepoint, kVec3 pos, kVec4 color, float scale = 1);
void    kDrawTextQuad(u32 codepoint, kVec2 pos, kVec4 color, float scale = 1);
void    kDrawTextQuadRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale = 1);
void    kDrawTextQuadRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale = 1);
void    kDrawTextQuadCentered(u32 codepoint, kVec3 pos, kVec4 color, float scale = 1);
void    kDrawTextQuadCentered(u32 codepoint, kVec2 pos, kVec4 color, float scale = 1);
void    kDrawTextQuadCenteredRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale = 1);
void    kDrawTextQuadCenteredRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale = 1);

kVec2   kCalculateText(kString text, const kFont *font, float scale = 1);
kVec2   kCalculateText(kString text, float scale = 1);
void    kDrawText(kString text, kVec3 pos, const kFont *font, kVec4 color = kVec4(1), float scale = 1);
void    kDrawText(kString text, kVec2 pos, const kFont *font, kVec4 color = kVec4(1), float scale = 1);
void    kDrawText(kString text, kVec3 pos, kVec4 color = kVec4(1), float scale = 1);
void    kDrawText(kString text, kVec2 pos, kVec4 color = kVec4(1), float scale = 1);
