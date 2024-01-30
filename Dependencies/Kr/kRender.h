#pragma once
#include "kRenderCommand.h"

typedef struct kGlyph
{
	kRect  Rect;
	kVec2i Bearing;
	kVec2i Size;
	kVec2i Advance;
} kGlyph;

typedef struct kFont
{
	kTexture Atlas;
	u16     *CodepointMap;
	kGlyph  *Glyphs;
	kGlyph  *Fallback;
	u32      MinCodepoint;
	u32      MaxCodepoint;
	u32      GlyphsCount;
	i16      Height;
	i16      Ascent;
	i16      Descent;
	i16      Linegap;
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

typedef enum kEmbeddedMesh
{
	kEmbeddedMesh_Cube,
	kEmbeddedMesh_Count
} kEmbeddedMesh;

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

static constexpr kRenderSpec kDefaultRenderSpec = {.Vertices = 524288,
                                                   .Indices  = 524288 * 6,
                                                   .Commands = 512,
                                                   .Scenes   = 64};

//
//
//

void kCreateRenderContext(const kRenderSpec &spec, kMesh meshes[kEmbeddedMesh_Count],
                          kTexture textures[kTextureType_Count], kFont *font);
void kDestroyRenderContext(void);
void kGetRenderMemoryUsage(kRenderMemory *usage);
void kGetRenderMemoryCaps(kRenderMemory *usage);

void kResetFrame(void);
void kGetFrameData(kRenderFrame *frame);

//
//
//

kCameraView kPerspectiveView(float fov, float aspect_ratio, float near, float far, const kMat4 &transform);
kCameraView kPerspectiveView(float fov, float aspect_ratio, float near, float far);

kCameraView KOrthographicView(float left, float right, float top, float bottom, float near = -1.0f, float far = 1.0f);
kCameraView kOrthographicView(float aspect_ratio, float height);

//
//
//

struct kRender2D
{
	static void kBeginRenderPass(kRenderPass pass);
	static void kEndRenderPass(void);

	static void kBeginScene(const kCameraView &view, const kViewport &viewport);
	static void kEndScene(void);

	static void kLineThickness(float thickness);

	static void kSetBlendMode(kBlendMode mode);
	static void kBeginBlendMode(kBlendMode mode);
	static void kEndBlendMode(void);

	static void kSetTextureFilter(kTextureFilter filter);

	static void kSetTexture(kTexture texture, kTextureType idx);
	static void kPushTexture(kTexture texture, kTextureType idx);
	static void kPopTexture(kTextureType idx);

	static void kSetOutLineStyle(kVec3 color, float width);
	static void kPushOutLineStyle(kVec3 color, float width);
	static void kPopOutLineStyle(void);

	static void kSetRect(kRect rect);
	static void kSetRectEx(float x, float y, float w, float h);
	static void kPushRect(kRect rect);
	static void kPushRectEx(float x, float y, float w, float h);
	static void kPopRect(void);

	static void kSetTransform(const kMat3 &transform);
	static void kPushTransform(const kMat3 &transform);
	static void kPushTransform(kVec2 pos, float angle);
	static void kPopTransform(void);

	static void kDrawTriangle(kVec3 va, kVec3 vb, kVec3 vc, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 ca, kVec4 cb, kVec4 cc);
	static void kDrawTriangle(kVec3 a, kVec3 b, kVec3 c, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 col);
	static void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec2 ta, kVec2 tb, kVec2 tc, kVec4 col);
	static void kDrawTriangle(kVec3 a, kVec3 b, kVec3 c, kVec4 color);
	static void kDrawTriangle(kVec2 a, kVec2 b, kVec2 c, kVec4 color);

	static void kDrawQuad(kVec3 va, kVec3 vb, kVec3 vc, kVec3 vd, kVec2 ta, kVec2 tb, kVec2 tc, kVec2 td, kVec4 color);
	static void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	                      kVec4 color);
	static void kDrawQuad(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec4 color);
	static void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);
	static void kDrawQuad(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kRect rect, kVec4 color);
	static void kDrawQuad(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kRect rect, kVec4 color);

	static void kDrawRect(kVec3 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
	static void kDrawRect(kVec2 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
	static void kDrawRect(kVec3 pos, kVec2 dim, kVec4 color);
	static void kDrawRect(kVec2 pos, kVec2 dim, kVec4 color);
	static void kDrawRect(kVec3 pos, kVec2 dim, kRect rect, kVec4 color);
	static void kDrawRect(kVec2 pos, kVec2 dim, kRect rect, kVec4 color);

	static void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	                             kVec4 color);
	static void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d,
	                             kVec4 color);
	static void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kVec4 color);
	static void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);
	static void kDrawRectRotated(kVec3 pos, kVec2 dim, float angle, kRect rect, kVec4 color);
	static void kDrawRectRotated(kVec2 pos, kVec2 dim, float angle, kRect rect, kVec4 color);

	static void kDrawRectCentered(kVec3 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
	static void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c, kVec2 uv_d, kVec4 color);
	static void kDrawRectCentered(kVec3 pos, kVec2 dim, kVec4 color);
	static void kDrawRectCentered(kVec2 pos, kVec2 dim, kVec4 color);
	static void kDrawRectCentered(kVec3 pos, kVec2 dim, kRect rect, kVec4 color);
	static void kDrawRectCentered(kVec2 pos, kVec2 dim, kRect rect, kVec4 color);

	static void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c,
	                                     kVec2 uv_d, kVec4 color);
	static void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec2 uv_a, kVec2 uv_b, kVec2 uv_c,
	                                     kVec2 uv_d, kVec4 color);
	static void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kVec4 color);
	static void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kVec4 color);
	static void kDrawRectCenteredRotated(kVec3 pos, kVec2 dim, float angle, kRect rect, kVec4 color);
	static void kDrawRectCenteredRotated(kVec2 pos, kVec2 dim, float angle, kRect rect, kVec4 color);

	static void kDrawEllipse(kVec3 pos, float radius_a, float radius_b, kVec4 color);
	static void kDrawEllipse(kVec2 pos, float radius_a, float radius_b, kVec4 color);

	static void kDrawCircle(kVec3 pos, float radius, kVec4 color);
	static void kDrawCircle(kVec2 pos, float radius, kVec4 color);

	static void kDrawPie(kVec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color);
	static void kDrawPie(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color);
	static void kDrawPie(kVec3 pos, float radius, float theta_a, float theta_b, kVec4 color);
	static void kDrawPie(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color);

	static void kDrawPiePart(kVec3 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max,
	                         float theta_a, float theta_b, kVec4 color);
	static void kDrawPiePart(kVec2 pos, float radius_a_min, float radius_b_min, float radius_a_max, float radius_b_max,
	                         float theta_a, float theta_b, kVec4 color);
	static void kDrawPiePart(kVec3 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color);
	static void kDrawPiePart(kVec2 pos, float radius_min, float radius_max, float theta_a, float theta_b, kVec4 color);

	static void kDrawLine(kVec3 a, kVec3 b, kVec4 color);
	static void kDrawLine(kVec2 a, kVec2 b, kVec4 color);

	static void kPathTo(kVec2 a);
	static void kArcTo(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b);
	static void kBezierQuadraticTo(kVec2 a, kVec2 b, kVec2 c);
	static void kBezierCubicTo(kVec2 a, kVec2 b, kVec2 c, kVec2 d);

	static void kDrawPathStroked(kVec4 color, bool closed = false, float z = 1.0f);
	static void kDrawPathFilled(kVec4 color, float z = 1.0f);

	static void kDrawBezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec4 color, float z = 0);
	static void kDrawBezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color, float z = 0);

	static void kDrawPolygon(const kVec2 *vertices, u32 count, float z, kVec4 color);
	static void kDrawPolygon(const kVec2 *vertices, u32 count, kVec4 color);

	static void kDrawTriangleOutline(kVec3 a, kVec3 b, kVec3 c, kVec4 color);
	static void kDrawTriangleOutline(kVec2 a, kVec2 b, kVec2 c, kVec4 color);

	static void kDrawQuadOutline(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec4 color);
	static void kDrawQuadOutline(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec4 color);

	static void kDrawRectOutline(kVec3 pos, kVec2 dim, kVec4 color);
	static void kDrawRectOutline(kVec2 pos, kVec2 dim, kVec4 color);

	static void kDrawRectCenteredOutline(kVec3 pos, kVec2 dim, kVec4 color);
	static void kDrawRectCenteredOutline(kVec2 pos, kVec2 dim, kVec4 color);

	static void kDrawEllipseOutline(kVec3 pos, float radius_a, float radius_b, kVec4 color);
	static void kDrawEllipseOutline(kVec2 pos, float radius_a, float radius_b, kVec4 color);

	static void kDrawCircleOutline(kVec3 pos, float radius, kVec4 color);
	static void kDrawCircleOutline(kVec2 pos, float radius, kVec4 color);

	static void kDrawArcOutline(kVec3 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color,
	                            bool closed);
	static void kDrawArcOutline(kVec2 pos, float radius_a, float radius_b, float theta_a, float theta_b, kVec4 color,
	                            bool closed);
	static void kDrawArcOutline(kVec3 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed);
	static void kDrawArcOutline(kVec2 pos, float radius, float theta_a, float theta_b, kVec4 color, bool closed);

	static void kDrawPolygonOutline(const kVec2 *vertices, u32 count, float z, kVec4 color);
	static void kDrawPolygonOutline(const kVec2 *vertices, u32 count, kVec4 color);

	static void kDrawTexture(kTexture texture, kVec3 pos, kVec2 dim, kVec4 color = kVec4(1));
	static void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color = kVec4(1));
	static void kDrawTextureCentered(kTexture texture, kVec3 pos, kVec2 dim, kVec4 color = kVec4(1));
	static void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kVec4 color = kVec4(1));
	static void kDrawTexture(kTexture texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
	static void kDrawTexture(kTexture texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
	static void kDrawTextureCentered(kTexture texture, kVec3 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
	static void kDrawTextureCentered(kTexture texture, kVec2 pos, kVec2 dim, kRect rect, kVec4 color = kVec4(1));
	static void kDrawTextureMasked(kTexture texture, kTexture mask, kVec3 pos, kVec2 dim, kVec4 color);
	static void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec3 pos, kVec2 dim, kVec4 color);
	static void kDrawTextureMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color);
	static void kDrawTextureCenteredMasked(kTexture texture, kTexture mask, kVec2 pos, kVec2 dim, kVec4 color);

	static void kDrawRoundedRect(kVec3 pos, kVec2 dim, kVec4 color, float radius = 1.0f);
	static void kDrawRoundedRect(kVec2 pos, kVec2 dim, kVec4 color, float radius = 1.0f);

	static void kDrawRoundedRectOutline(kVec3 pos, kVec2 dim, kVec4 color, float radius = 1.0f);
	static void kDrawRoundedRectOutline(kVec2 pos, kVec2 dim, kVec4 color, float radius = 1.0f);

	static kGlyph *kFindFontGlyph(const kFont *font, u32 codepoint);
	static kVec2   kAdjectCursorToBaseline(const kFont *font, kVec2 cursor, float scale);
	static bool    kCalculateGlyphMetrics(const kFont *font, u32 codepoint, float scale, kVec2 *cursor, kVec2 *rpos,
	                                      kVec2 *rsize, kRect *rect);

	static void    kDrawTextQuad(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuad(u32 codepoint, const kFont *font, kVec2 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuadRotated(u32 codepoint, const kFont *font, kVec3 pos, float angle, kVec4 color,
	                                    float scale = 1);
	static void    kDrawTextQuadRotated(u32 codepoint, const kFont *font, kVec2 pos, float angle, kVec4 color,
	                                    float scale = 1);
	static void    kDrawTextQuadCentered(u32 codepoint, const kFont *font, kVec3 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuadCentered(u32 codepoint, const kFont *font, kVec2 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuadCenteredRotated(u32 codepoint, const kFont *font, kVec3 pos, float angle, kVec4 color,
	                                            float scale = 1);
	static void    kDrawTextQuadCenteredRotated(u32 codepoint, const kFont *font, kVec2 pos, float angle, kVec4 color,
	                                            float scale = 1);

	static void    kDrawTextQuad(u32 codepoint, kVec3 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuad(u32 codepoint, kVec2 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuadRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale = 1);
	static void    kDrawTextQuadRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale = 1);
	static void    kDrawTextQuadCentered(u32 codepoint, kVec3 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuadCentered(u32 codepoint, kVec2 pos, kVec4 color, float scale = 1);
	static void    kDrawTextQuadCenteredRotated(u32 codepoint, kVec3 pos, float angle, kVec4 color, float scale = 1);
	static void    kDrawTextQuadCenteredRotated(u32 codepoint, kVec2 pos, float angle, kVec4 color, float scale = 1);

	static kVec2   kCalculateText(kString text, const kFont *font, float scale = 1);
	static kVec2   kCalculateText(kString text, float scale = 1);
	static void    kDrawText(kString text, kVec3 pos, const kFont *font, kVec4 color = kVec4(1), float scale = 1);
	static void    kDrawText(kString text, kVec2 pos, const kFont *font, kVec4 color = kVec4(1), float scale = 1);
	static void    kDrawText(kString text, kVec3 pos, kVec4 color = kVec4(1), float scale = 1);
	static void    kDrawText(kString text, kVec2 pos, kVec4 color = kVec4(1), float scale = 1);
};

struct kRender3D
{
	static void kBeginScene(const kCameraView &view, const kViewport &viewport);
	static void kEndScene(void);

	static void kPushSkeleton(const kSkeleton &skeleton);
	static void kPopSkeleton(void);

	static void kDrawMesh(kMesh mesh, kTexture diffuse, const kMat4 &transform, kVec4 color = kVec4(1));
	static void kDrawMesh(kMesh mesh, const kMat4 &transform, kVec4 color = kVec4(1));
	static void kDrawEmbeddedMesh(kEmbeddedMesh mesh, kTexture diffuse, const kMat4 &transform, kVec4 color = kVec4(1));
	static void kDrawCube(kTexture diffuse, const kMat4 &transform, kVec4 color = kVec4(1));
	static void kDrawEmbeddedMesh(kEmbeddedMesh mesh, const kMat4 &transform, kVec4 color = kVec4(1));
	static void kDrawCube(const kMat4 &transform, kVec4 color = kVec4(1));
};
