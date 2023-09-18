#pragma once
#include "kCommon.h"

#define kFormatVec2(v)  "{%f, %f}"
#define kFormatVec3(v)  "{%f, %f, %f}"
#define kFormatVec4(v)  "{%f, %f, %f, %f}"

#define kFormatVec2i(v) "{%d, %d}"
#define kFormatVec3i(v) "{%d, %d, %d}"
#define kFormatVec4i(v) "{%d, %d, %d, %d}"

#define kExpandVec2(v)  (v).x, (v).y
#define kExpandVec3(v)  (v).x, (v).y, (v).z
#define kExpandVec4(v)  (v).x, (v).y, (v).z, (v).w

#define kVec2Factor(a)  (kVec2){ .x = a, .y = a }
#define kVec3Factor(a)  (kVec3){ .x = a, .y = a, .z = a }
#define kVec4Factor(a)  (kVec4){ .x = a, .y = a, .z = a, .w = a }

#define kVec2iFactor(a) (kVec2i){ .x = a, .y = a }
#define kVec3iFactor(a) (kVec3i){ .x = a, .y = a, .z = a }
#define kVec4iFactor(a) (kVec4i){ .x = a, .y = a, .z = a, .w = a }

float kSgn(float val);
float kAbsolute(float x);
float kSin(float x);
float kCos(float x);
float kTan(float x);
float kArcSin(float x);
float kArcCos(float x);
float kArcTan2(float y, float x);
float kSquareRoot(float x);
float kPow(float x, float y);
float kCopySign(float x, float y);
float kMod(float x, float y);
float kSquare(float x);
float kFloor(float x);
float kRound(float x);
float kCeil(float x);

float kWrap(float min, float a, float max);
kVec2 kArm(float angle);
kVec2 kArmInverse(float angle);
bool  kAlmostEqual(float a, float b);

kVec2 kVec2Neg(kVec2 a);
kVec3 kVec3Neg(kVec3 a);
kVec4 kVec4Neg(kVec4 a);

kVec2 kVec2Add(kVec2 a, kVec2 b);
kVec2 kVec2Sub(kVec2 a, kVec2 b);
kVec2 kVec2Mul(kVec2 a, kVec2 b);
kVec2 kVec2Div(kVec2 a, kVec2 b);

kVec3 kVec3Add(kVec3 a, kVec3 b);
kVec3 kVec3Sub(kVec3 a, kVec3 b);
kVec3 kVec3Mul(kVec3 a, kVec3 b);
kVec3 kVec3Div(kVec3 a, kVec3 b);

kVec4 kVec4Add(kVec4 a, kVec4 b);
kVec4 kVec4Sub(kVec4 a, kVec4 b);
kVec4 kVec4Mul(kVec4 a, kVec4 b);
kVec4 kVec4Div(kVec4 a, kVec4 b);

kVec2i kVec2iNeg(kVec2i a);
kVec3i kVec3iNeg(kVec3i a);
kVec4i kVec4iNeg(kVec4i a);

kVec2i kVec2iAdd(kVec2i a, kVec2i b);
kVec2i kVec2iSub(kVec2i a, kVec2i b);
kVec2i kVec2iMul(kVec2i a, kVec2i b);
kVec2i kVec2iDiv(kVec2i a, kVec2i b);

kVec3i kVec3iAdd(kVec3i a, kVec3i b);
kVec3i kVec3iSub(kVec3i a, kVec3i b);
kVec3i kVec3iMul(kVec3i a, kVec3i b);
kVec3i kVec3iDiv(kVec3i a, kVec3i b);

kVec4i kVec4iAdd(kVec4i a, kVec4i b);
kVec4i kVec4iSub(kVec4i a, kVec4i b);
kVec4i kVec4iMul(kVec4i a, kVec4i b);
kVec4i kVec4iDiv(kVec4i a, kVec4i b);

kVec2 kComplexProduct(kVec2 a, kVec2 b);
kVec2 kComplexConjugate(kVec2 a);

float kVec2DotProduct(kVec2 a, kVec2 b);
float kVec3DotProduct(kVec3 a, kVec3 b);
float kVec4DotProduct(kVec4 a, kVec4 b);

float kVec2LengthSq(kVec2 v);
float kVec3LengthSq(kVec3 v);
float kVec4LengthSq(kVec4 v);

float kVec2Length(kVec2 v);
float kVec3Length(kVec3 v);
float kVec4Length(kVec4 v);

float kVec1Distance(float a, float b);
float kVec2Distance(kVec2 a, kVec2 b);
float kVec3Distance(kVec3 a, kVec3 b);
float kVec4Distance(kVec4 a, kVec4 b);

kVec2 kVec2NormalizeZ(kVec2 v);
kVec3 kVec3NormalizeZ(kVec3 v);
kVec4 kVec4NormalizeZ(kVec4 v);
kVec2 kVec2Normalize(kVec2 v);
kVec3 kVec3Normalize(kVec3 v);
kVec4 kVec4Normalize(kVec4 v);

kVec3 kCrossProduct(kVec3 a, kVec3 b);
kVec3 kTripleProduct(kVec3 a, kVec3 b, kVec3 c);

kVec3 kOrthoNormalBasisRH(kVec3 *a, kVec3 *b);
kVec3 kOrthoNormalBasisLH(kVec3 *a, kVec3 *b);

float kVec2AngleBetween(kVec2 a, kVec2 b);
float kVec3AngleBetween(kVec3 a, kVec3 b);
float kVec2AngleBetweenNormalized(kVec2 a, kVec2 b);
float kVec3AngleBetweenNormalized(kVec3 a, kVec3 b);

float kMat2Determinant(const kMat2 mat);
kMat2 kMat2Inverse(const kMat2 *mat);
kMat2 kMat2Transpose(const kMat2 *m);
float kMat3Determinant(const kMat3 *mat);
kMat3 kMat3Inverse(const kMat3 *mat);
kMat3 kMat3Transpose(const kMat3 *m);
float kMat4Determinant(const kMat4 *mat);
kMat4 kMat4Inverse(const kMat4 *mat);
kMat4 kMat4Transpose(const kMat4 *m);

kMat2 kMat2Mul(const kMat2 *left, const kMat2 *right);
kVec2 kMatVec2Mul(const kMat2 *mat, kVec2 vec);
kVec2 kVecMat2Mul(kVec2 vec, const kMat2 *mat);
kMat3 kMat3Mul(const kMat3 *left, const kMat3 *right);
kVec3 kMatVec3Mul(const kMat3 *mat, kVec3 vec);
kVec3 kVecMat3Mul(kVec3 vec, const kMat3 *mat);
kMat4 kMat4Mul(const kMat4 *left, const kMat4 *right);
kVec4 kMatVec4Mul(const kMat4 *mat, kVec4 vec);
kVec4 kVecMat4Mul(kVec4 vec, const kMat4 *mat);

kQuat kQuatNeg(kQuat a);
kQuat kQuatAdd(kQuat a, kQuat b);
kQuat kQuatSub(kQuat a, kQuat b);

float kQuatDotProduct(kQuat q1, kQuat q2);
float kQuatLengthSq(kQuat q);
float kQuatLength(kQuat q);
kQuat kQuatNormalize(kQuat q);
kQuat kQuatConjugate(kQuat q);
kQuat kQuatMul(kQuat q1, kQuat q2);
kVec3 kQuatRotate(kQuat q, kVec3 v);

kVec3 kRightVector(kQuat q);
kVec3 kUpVector(kQuat q);
kVec3 kForwardVector(kQuat q);

kMat2 kMat3ToMat2(const kMat3 *mat);
kMat3 kMat2ToMat3(const kMat2 *mat);
kMat3 kMat4ToMat3(const kMat4 *mat);
kMat4 kMat3ToMat4(const kMat3 *mat);

void  kQuatToAngleAxis(kQuat q, float *angle, kVec3 *axis);
kMat4 kQuatToMat4(kQuat q);
kVec3 kQuatToEulerAngles(kQuat q);
kQuat kAngleAxisToQuat(kVec3 axis, float angle);
kQuat kAngleAxisNormalizedToQuat(kVec3 axis, float angle);
kQuat kMat4ToQuat(const kMat4 *m);
kQuat kEulerAnglesToQuat(float pitch, float yaw, float roll);

kMat2 kMat2Identity(void);
kMat2 kMat2Diagonal(float x, float y);
kMat2 kMat2Rotation(kVec2 arm);
kMat2 kMat2RotationAngle(float angle);
kMat3 kMat3Identity(void);
kMat3 kMat3Diagonal(float s1, float s2, float s3);
kMat3 kMat3Scale(kVec2 scale);
kMat3 kMat3Translation(kVec2 t);
kMat3 kMat3Rotation(kVec2 arm);
kMat3 kMat3RotationAngle(float angle);
kMat3 kMat3LookAt(kVec2 from, kVec2 to, kVec2 forward);
kMat4 kIdentity(void);
kMat4 kDiagonal(float x, float y, float z, float w);
kMat4 Scale(kVec3 s);
kMat4 kTranslation(kVec3 t);
kMat4 kRotationX(kVec2 arm);
kMat4 kRotationAngleX(float angle);
kMat4 kRotationY(kVec2 arm);
kMat4 kRotationAngleY(float angle);
kMat4 kRotationZ(kVec2 arm);
kMat4 kRotationAngleZ(float angle);
kMat4 kRotation(kVec3 axis, kVec2 arm);
kMat4 kRotationAngle(kVec3 axis, float angle);
kMat4 kLookAt(kVec3 from, kVec3 to, kVec3 world_up);
kMat4 kLookTowards(kVec3 dir, kVec3 world_up);
kMat4 kOrthographicRH(float l, float r, float t, float b, float n, float f);
kMat4 kOrthographicLH(float l, float r, float t, float b, float n, float f);
kMat4 kPerspectiveRH(float fov, float aspect_ratio, float n, float f);
kMat4 kPerspectiveLH(float fov, float aspect_ratio, float n, float f);

kQuat kQuatIdentity(void);
kQuat kQuatBetweenVectors(kVec3 from, kVec3 to);
kQuat kQuatBetween(kQuat a, kQuat b);
kQuat kQuatLookAt(kVec3 from, kVec3 to, kVec3 world_forward);

kVec2 kLerpWeight(float t);
float kVec1Lerp(float from, float to, float t);
kVec2 kVec2Lerp(kVec2 from, kVec2 to, float t);
kVec3 kVec3Lerp(kVec3 from, kVec3 to, float t);
kVec4 kVec4Lerp(kVec4 from, kVec4 to, float t);
kQuat kQuatLerp(kQuat from, kQuat to, float t);
kVec3 kBezierQuadraticWeight(float t);
float kVec1BezierQuadratic(float a, float b, float c, float t);
kVec2 kVec2BezierQuadratic(kVec2 a, kVec2 b, kVec2 c, float t);
kVec3 kVec3BezierQuadratic(kVec3 a, kVec3 b, kVec3 c, float t);
kVec4 kVec4BezierQuadratic(kVec4 a, kVec4 b, kVec4 c, float t);
kQuat kQuatBezierQuadratic(kQuat a, kQuat b, kQuat c, float t);
kVec4 kBezierCubicWeight(float t);
float kVec1BezierCubic(float a, float b, float c, float d, float t);
kVec2 kVec2BezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, float t);
kVec3 kVec3BezierCubic(kVec3 a, kVec3 b, kVec3 c, kVec3 d, float t);
kVec4 kVec4BezierCubic(kVec4 a, kVec4 b, kVec4 c, kVec4 d, float t);
kQuat kQuatBezierCubic(kQuat a, kQuat b, kQuat c, kQuat d, float t);
void  kBuildVec1BezierQuadratic(float a, float b, float c, float *points, int segments);
void  kBuildVec2BezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec2 *points, int segments);
void  kBuildVec3BezierQuadratic(kVec3 a, kVec3 b, kVec3 c, kVec3 *points, int segments);
void  kBuildVec4BezierQuadratic(kVec4 a, kVec4 b, kVec4 c, kVec4 *points, int segments);
void  kBuildQuatBezierQuadratic(kQuat a, kQuat b, kQuat c, kQuat *points, int segments);
void  kBuildVec1BezierCubic(float a, float b, float c, float d, float *points, int segments);
void  kBuildVec2BezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec2 *points, int segments);
void  kBuildVec3BezierCubic(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec3 *points, int segments);
void  kBuildVec4BezierCubic(kVec4 a, kVec4 b, kVec4 c, kVec4 d, kVec4 *points, int segments);
void  kBuildQuatBezierCubic(kQuat a, kQuat b, kQuat c, kQuat d, kQuat *points, int segments);

kQuat kSlerp(kQuat from, kQuat to, float t);

float kVec1Step(float edge, float val);
kVec2 kVec2Step(kVec2 edge, kVec2 val);
kVec3 kVec3Step(kVec3 edge, kVec3 val);
kVec4 kVec4Step(kVec4 edge, kVec4 val);
kQuat kQuatStep(kQuat edge, kQuat val);

float kInverseSmoothStep(float x);

float kVec1MoveTowards(float from, float to, float factor);
kVec2 kVec2MoveTowards(kVec2 from, kVec2 to, float factor);
kVec3 kVec3MoveTowards(kVec3 from, kVec3 to, float factor);
kVec4 kVec4MoveTowards(kVec4 from, kVec4 to, float factor);
kVec2 kRotateAround(kVec2 point, kVec2 center, float angle);
kQuat kRotateTowards(kQuat from, kQuat to, float max_angle);
kVec2 kReflect(kVec2 d, kVec2 n);

void  kUnpackRGBA(u32 c, u8 channels[4]);
u32   kPackRGBA(u8 r, u8 g, u8 b, u8 a);
u32   kColor4ToUint(kVec4 v);
u32   kColor3ToUint(kVec3 v);
kVec4 kUintToColor4(u32 c);
kVec3 kUintToColor3(u32 c);

kVec3 kLinearToSRGB(kVec3 Col, float gamma);
kVec3 kSRGBToLinear(kVec3 Col, float gamma);
kVec3 kHSVToRGB(kVec3 col);
kVec3 kRGBToHSV(kVec3 c);

//
//
//

#define kNeg(A)           _Generic((A),        \
                            kVec2:  kVec2Neg,  \
                            kVec3:  kVec3Neg,  \
                            kVec4:  kVec4Neg,  \
                            kVec2i: kVec2iNeg, \
                            kVec3i: kVec3iNeg, \
                            kVec4i: kVec4iNeg, \
                            kQuat:  kQuatNeg   \
                            ) (A)

#define kAdd(A, B)        _Generic((A),       \
                           kVec2:  kVec2Add,  \
                           kVec3:  kVec3Add,  \
                           kVec4:  kVec4Add,  \
                           kVec2i: kVec2iAdd, \
                           kVec3i: kVec3iAdd, \
                           kVec4i: kVec4iAdd, \
                           kQuat:  kQuatAdd   \
                           ) (A, B)

#define kSub(A, B)        _Generic((A),       \
                           kVec2:  kVec2Sub,  \
                           kVec3:  kVec3Sub,  \
                           kVec4:  kVec4Sub,  \
                           kVec2i: kVec2iSub, \
                           kVec3i: kVec3iSub, \
                           kVec4i: kVec4iSub, \
                           kQuat:  kQuatSub   \
                           ) (A, B)

#define kMul(A, B)        _Generic((A),       \
                           kVec2:  kVec2Mul,  \
                           kVec3:  kVec3Mul,  \
                           kVec4:  kVec4Mul,  \
                           kVec2i: kVec2iMul, \
                           kVec3i: kVec3iMul, \
                           kVec4i: kVec4iMul, \
                           kMat2:  kMat2Mul,  \
                           kMat3:  kMat3Mul,  \
                           kMat4:  kMat4Mul,  \
                           kQuat:  kQuatMul   \
                           ) (A, B)

#define kMatVecMul(A, B)  _Generic((A),          \
                           kMat2:  kMatVec2Mul,  \
                           kMat3:  kMatVec3Mul,  \
                           kMat4:  kMatVec4Mul   \
                           ) (A, B)

#define kVecMatMul(A, B)  _Generic((B),          \
                           kMat2:  kVecMat2Mul,  \
                           kMat3:  kVecMat3Mul,  \
                           kMat4:  kVecMat4Mul   \
                           ) (A, B)

#define kDiv(A, B)        _Generic((A),       \
                           kVec2:  kVec2Div,  \
                           kVec3:  kVec3Div,  \
                           kVec4:  kVec4Div,  \
                           kVec2i: kVec2iDiv, \
                           kVec3i: kVec3iDiv, \
                           kVec4i: kVec4iDiv  \
                           ) (A, B)

#define kDotProduct(A, B) _Generic((A),          \
                           kVec2:  kVec2DotProduct, \
                           kVec3:  kVec3DotProduct, \
                           kVec4:  kVec4DotProduct,  \
                           kQuat:  kQuatDotProduct  \
                           ) (A, B)

#define kLengthSq(A)     _Generic((A),            \
                           kVec2:  kVec2LengthSq, \
                           kVec3:  kVec3LengthSq, \
                           kVec4:  kVec4LengthSq,  \
                           kQuat:  kQuatLengthSq  \
                           ) (A)

#define kLength(A)       _Generic((A),          \
                           kVec2:  kVec2Length, \
                           kVec3:  kVec3Length, \
                           kVec4:  kVec4Length,  \
                           kQuat:  kQuatLength  \
                           ) (A)

#define kDistance(A, B) _Generic((A),          \
                           float:  kVec1Distance, \
                           kVec2:  kVec2Distance, \
                           kVec3:  kVec3Distance, \
                           kVec4:  kVec4Distance  \
                           ) (A, B)

#define kNormalizeZ(A) _Generic((A),                \
                           kVec2:  kVec2NormalizeZ, \
                           kVec3:  kVec3NormalizeZ, \
                           kVec4:  kVec4NormalizeZ  \
                           ) (A)

#define kNormalize(A)  _Generic((A),               \
                           kVec2:  kVec2Normalize, \
                           kVec3:  kVec3Normalize, \
                           kVec4:  kVec4Normalize, \
                           kQuat:  kQuatNormalize  \
                           ) (A)

#define kAngleBetween(A, B) _Generic((A),               \
                             kVec2:  kVec2AngleBetween, \
                             kVec3:  kVec3AngleBetween  \
                             ) (A, B)

#define kAngleBetweenNormalized(A, B) _Generic((A),               \
                             kVec2:  kVec2AngleBetweenNormalized, \
                             kVec3:  kVec3AngleBetweenNormalized  \
                             ) (A, B)

#define kDeterminant(A)    _Generic((A),                       \
                             const kMat2 *:  kMat2Determinant, \
                             const kMat3 *:  kMat3Determinant, \
                             const kMat4 *:  kMat4Determinant  \
                             ) (A)

#define kInverse(A)        _Generic((A),                   \
                             const kMat2 *:  kMat2Inverse, \
                             const kMat3 *:  kMat3Inverse, \
                             const kMat4 *:  kMat4Inverse  \
                             ) (A)

#define kTranspose(A)      _Generic((A),                     \
                             const kMat2 *:  kMat2Transpose, \
                             const kMat3 *:  kMat3Transpose, \
                             const kMat4 *:  kMat4Transpose  \
                             ) (A)

#define kConjugate(A)      _Generic((A),               \
                             kVec2: kComplexConjugate, \
                             kQuat: kQuatConjugate     \
                             ) (A)

#define kLerp(A, B, t) _Generic((A),          \
                           float:  kVec1Lerp, \
                           kVec2:  kVec2Lerp, \
                           kVec3:  kVec3Lerp, \
                           kVec4:  kVec4Lerp, \
                           kQuat:  kQuatLerp  \
                           ) (A, B, t)

#define kBezierQuadratic(A, B, C, t) _Generic((A),       \
                           float:  kVec1BezierQuadratic, \
                           kVec2:  kVec2BezierQuadratic, \
                           kVec3:  kVec3BezierQuadratic, \
                           kVec4:  kVec4BezierQuadratic, \
                           kQuat:  kQuatBezierQuadratic  \
                           ) (A, B, C, t)

#define kBezierCubic(A, B, C, D, t) _Generic((A),    \
                           float:  kVec1BezierCubic, \
                           kVec2:  kVec2BezierCubic, \
                           kVec3:  kVec3BezierCubic, \
                           kVec4:  kVec4BezierCubic, \
                           kQuat:  kQuatBezierCubic  \
                           ) (A, B, C, D, t)

#define kBuildBezierQuadratic(A, B, C, points, segments) _Generic((A), \
                           float:  kBuildVec1BezierQuadratic, \
                           kVec2:  kBuildVec2BezierQuadratic, \
                           kVec3:  kBuildVec3BezierQuadratic, \
                           kVec4:  kBuildVec4BezierQuadratic, \
                           kQuat:  kBuildQuatBezierQuadratic  \
                           ) (A, B, C, points, segments)

#define kBuildBezierCubic(A, B, C, D, points, segments) _Generic((A), \
                           float:  kBuildVec1BezierCubic, \
                           kVec2:  kBuildVec2BezierCubic, \
                           kVec3:  kBuildVec3BezierCubic, \
                           kVec4:  kBuildVec4BezierCubic, \
                           kQuat:  kBuildQuatBezierCubic  \
                           ) (A, B, C, D, points, segments)

#define kStep(A, B) _Generic((A),             \
                           float:  kVec1Step, \
                           kVec2:  kVec2Step, \
                           kVec3:  kVec3Step, \
                           kVec4:  kVec4Step, \
                           kQuat:  kQuatStep  \
                           ) (A, B)

#define kMoveTowards(A, B, factor) _Generic((A),     \
                           float:  kVec1MoveTowards, \
                           kVec2:  kVec2MoveTowards, \
                           kVec3:  kVec3MoveTowards, \
                           kVec4:  kVec4MoveTowards  \
                           ) (A, B, factor)
