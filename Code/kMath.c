#pragma once
#include "kMath.h"
#include <math.h>

float kSgn(float val)             { return (float)(((float)(0) < val) - (val < (float)(0))); }
float kAbsolute(float x)          { return fabsf(x); }
float kSin(float x)               { return sinf(kTurnToRad * (x)); }
float kCos(float x)               { return cosf(kTurnToRad * (x)); }
float kTan(float x)               { return tanf(kTurnToRad * (x)); }
float kArcSin(float x)            { return kRadToTurn * (asinf(x)); }
float kArcCos(float x)            { return kRadToTurn * (acosf(x)); }
float kArcTan2(float y, float x)  { return kRadToTurn * (atan2f(y, x)); }
float kSquareRoot(float x)        { return sqrtf(x); }
float kPow(float x, float y)      { return powf(x, y); }
float kCopySign(float x, float y) { return copysignf(x, y); }
float kMod(float x, float y)      { return fmodf(x, y); }
float kSquare(float x)            { return (x * x); }
float kFloor(float x)             { return floorf(x); }
float kRound(float x)             { return roundf(x); }
float kCeil(float x)              { return ceilf(x); }

float kWrap(float min, float a, float max) {
	float range = max - min;
	float offset = a - min;
	float result = (offset - (kFloor(offset / range) * range) + min);
	return result;
}

kVec2 kArm(float angle)        { return (kVec2){ kCos(angle), kSin(angle) }; }
kVec2 kArmInverse(float angle) { return (kVec2){ kSin(angle), kCos(angle) }; }

// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
bool kAlmostEqual(float a, float b) {
	float diff = kAbsolute(a - b);
	a = kAbsolute(a);
	b = kAbsolute(b);

	float larger = (b > a) ? b : a;

	if (diff <= larger * FLT_EPSILON)
		return true;
	return false;
}

#define kVec2UnaryOp(a, ch)     { .x = ch (a).x, .y = ch (a).x }
#define kVec3UnaryOp(a, ch)     { .x = ch (a).x, .y = ch (a).y, .z = ch (a).z }
#define kVec4UnaryOp(a, ch)     { .x = ch (a).x, .y = ch (a).y, .z = ch (a).z, .w = ch (a).w }

#define kVec2BinaryOp(a, b, ch) { .x = (a).x ch (b).x, .y = (a).x ch (b).x }
#define kVec3BinaryOp(a, b, ch) { .x = (a).x ch (b).x, .y = (a).y ch (b).y, .z = (a).z ch (b).z }
#define kVec4BinaryOp(a, b, ch) { .x = (a).x ch (b).x, .y = (a).y ch (b).y, .z = (a).z ch (b).z, .w = (a).w ch (b).w }

kVec2 kVec2Neg(kVec2 a)              { kVec2 r = kVec2UnaryOp(a, -); return r; }
kVec3 kVec3Neg(kVec3 a)              { kVec3 r = kVec3UnaryOp(a, -); return r; }
kVec4 kVec4Neg(kVec4 a)              { kVec4 r = kVec4UnaryOp(a, -); return r; }

kVec2 kVec2Add(kVec2 a, kVec2 b)     { kVec2 r = kVec2BinaryOp(a, b, +); return r; }
kVec2 kVec2Sub(kVec2 a, kVec2 b)     { kVec2 r = kVec2BinaryOp(a, b, -); return r; }
kVec2 kVec2Mul(kVec2 a, kVec2 b)     { kVec2 r = kVec2BinaryOp(a, b, *); return r; }
kVec2 kVec2Div(kVec2 a, kVec2 b)     { kVec2 r = kVec2BinaryOp(a, b, /); return r; }

kVec2 kVec2AddScaled(kVec2 a, float f, kVec2 b) { kVec2 s = { f * b.x, f * b.y }; return kVec2Add(a, s); }
kVec2 kVec2SubScaled(kVec2 a, float f, kVec2 b) { kVec2 s = { f * b.x, f * b.y }; return kVec2Sub(a, s); }

kVec3 kVec3Add(kVec3 a, kVec3 b)     { kVec3 r = kVec3BinaryOp(a, b, +); return r; }
kVec3 kVec3Sub(kVec3 a, kVec3 b)     { kVec3 r = kVec3BinaryOp(a, b, -); return r; }
kVec3 kVec3Mul(kVec3 a, kVec3 b)     { kVec3 r = kVec3BinaryOp(a, b, *); return r; }
kVec3 kVec3Div(kVec3 a, kVec3 b)     { kVec3 r = kVec3BinaryOp(a, b, /); return r; }

kVec3 kVec3AddScaled(kVec3 a, float f, kVec3 b) { kVec3 s = { f * b.x, f * b.y, f * b.z }; return kVec3Add(a, s); }
kVec3 kVec3SubScaled(kVec3 a, float f, kVec3 b) { kVec3 s = { f * b.x, f * b.y, f * b.z }; return kVec3Sub(a, s); }

kVec4 kVec4Add(kVec4 a, kVec4 b)     { kVec4 r = kVec4BinaryOp(a, b, +); return r; }
kVec4 kVec4Sub(kVec4 a, kVec4 b)     { kVec4 r = kVec4BinaryOp(a, b, -); return r; }
kVec4 kVec4Mul(kVec4 a, kVec4 b)     { kVec4 r = kVec4BinaryOp(a, b, *); return r; }
kVec4 kVec4Div(kVec4 a, kVec4 b)     { kVec4 r = kVec4BinaryOp(a, b, /); return r; }

kVec4 kVec4AddScaled(kVec4 a, float f, kVec4 b) { kVec4 s = { f * b.x, f * b.y, f * b.z, f * b.w }; return kVec4Add(a, s); }
kVec4 kVec4SubScaled(kVec4 a, float f, kVec4 b) { kVec4 s = { f * b.x, f * b.y, f * b.z, f * b.w }; return kVec4Sub(a, s); }

kVec2i kVec2iNeg(kVec2i a)           { kVec2i r = kVec2UnaryOp(a, -); return r; }
kVec3i kVec3iNeg(kVec3i a)           { kVec3i r = kVec3UnaryOp(a, -); return r; }
kVec4i kVec4iNeg(kVec4i a)           { kVec4i r = kVec4UnaryOp(a, -); return r; }

kVec2i kVec2iAdd(kVec2i a, kVec2i b) { kVec2i r = kVec2BinaryOp(a, b, +); return r; }
kVec2i kVec2iSub(kVec2i a, kVec2i b) { kVec2i r = kVec2BinaryOp(a, b, -); return r; }
kVec2i kVec2iMul(kVec2i a, kVec2i b) { kVec2i r = kVec2BinaryOp(a, b, *); return r; }
kVec2i kVec2iDiv(kVec2i a, kVec2i b) { kVec2i r = kVec2BinaryOp(a, b, /); return r; }

kVec2i kVec2iAddScaled(kVec2i a, int f, kVec2i b) { kVec2i s = { f * b.x, f * b.y }; return kVec2iAdd(a, s); }
kVec2i kVec2iSubScaled(kVec2i a, int f, kVec2i b) { kVec2i s = { f * b.x, f * b.y }; return kVec2iSub(a, s); }

kVec3i kVec3iAdd(kVec3i a, kVec3i b) { kVec3i r = kVec3BinaryOp(a, b, +); return r; }
kVec3i kVec3iSub(kVec3i a, kVec3i b) { kVec3i r = kVec3BinaryOp(a, b, -); return r; }
kVec3i kVec3iMul(kVec3i a, kVec3i b) { kVec3i r = kVec3BinaryOp(a, b, *); return r; }
kVec3i kVec3iDiv(kVec3i a, kVec3i b) { kVec3i r = kVec3BinaryOp(a, b, /); return r; }

kVec3i kVec3iAddScaled(kVec3i a, int f, kVec3i b) { kVec3i s = { f * b.x, f * b.y, f * b.z }; return kVec3iAdd(a, s); }
kVec3i kVec3iSubScaled(kVec3i a, int f, kVec3i b) { kVec3i s = { f * b.x, f * b.y, f * b.z }; return kVec3iSub(a, s); }

kVec4i kVec4iAdd(kVec4i a, kVec4i b) { kVec4i r = kVec4BinaryOp(a, b, +); return r; }
kVec4i kVec4iSub(kVec4i a, kVec4i b) { kVec4i r = kVec4BinaryOp(a, b, -); return r; }
kVec4i kVec4iMul(kVec4i a, kVec4i b) { kVec4i r = kVec4BinaryOp(a, b, *); return r; }
kVec4i kVec4iDiv(kVec4i a, kVec4i b) { kVec4i r = kVec4BinaryOp(a, b, /); return r; }

kVec4i kVec4iAddScaled(kVec4i a, int f, kVec4i b) { kVec4i s = { f * b.x, f * b.y, f * b.z, f * b.w }; return kVec4iAdd(a, s); }
kVec4i kVec4iSubScaled(kVec4i a, int f, kVec4i b) { kVec4i s = { f * b.x, f * b.y, f * b.z, f * b.w }; return kVec4iSub(a, s); }

kVec2 kComplexProduct(kVec2 a, kVec2 b)  { kVec2 r = { .x = a.x * b.x - a.y * b.y, .y = a.x * b.y + a.y * b.x }; return r; }
kVec2 kComplexConjugate(kVec2 a)         { kVec2 r = { .x = a.x, .y = -a.y }; return r; }

float kVec2DotProduct(kVec2 a, kVec2 b)  { return a.x * b.x + a.y * b.y; }
float kVec3DotProduct(kVec3 a, kVec3 b)  { return a.x * b.x + a.y * b.y + a.z * b.z; }
float kVec4DotProduct(kVec4 a, kVec4 b)  { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

float kVec2LengthSq(kVec2 v)             { return kDotProduct(v, v); }
float kVec3LengthSq(kVec3 v)             { return kDotProduct(v, v); }
float kVec4LengthSq(kVec4 v)             { return kDotProduct(v, v); }
float kVec2Length(kVec2 v)               { return kSquareRoot(kDotProduct(v, v)); }
float kVec3Length(kVec3 v)               { return kSquareRoot(kDotProduct(v, v)); }
float kVec4Length(kVec4 v)               { return kSquareRoot(kDotProduct(v, v)); }
float kVec1Distance(float a, float b)    { return b - a; }
float kVec2Distance(kVec2 a, kVec2 b)    { return kLength(kSub(b, a)); }
float kVec3Distance(kVec3 a, kVec3 b)    { return kLength(kSub(b, a)); }
float kVec4Distance(kVec4 a, kVec4 b)    { return kLength(kSub(b, a)); }

kVec2 kVec2NormalizeZ(kVec2 v) {
	kVec2  res = {0};
	float len  = kLength(v);
	if (len != 0)
		res = kDiv(v, kVec2Factor(len));
	return res;
}

kVec3 kVec3NormalizeZ(kVec3 v) {
	kVec3  res = {0};
	float len  = kLength(v);
	if (len != 0)
		res = kDiv(v, kVec3Factor(len));
	return res;
}

kVec4 kVec4NormalizeZ(kVec4 v) {
	kVec4  res = {0};
	float len  = kLength(v);
	if (len != 0)
		res = kDiv(v, kVec4Factor(len));
	return res;
}

kVec2 kVec2Normalize(kVec2 v) {
	float len = kLength(v);
	kAssert(len != 0);
	kVec2 res = kDiv(v, kVec2Factor(len));
	return res;
}

kVec3 kVec3Normalize(kVec3 v) {
	float len = kLength(v);
	kAssert(len != 0);
	kVec3 res = kDiv(v, kVec3Factor(len));
	return res;
}

kVec4 kVec4Normalize(kVec4 v) {
	float len = kLength(v);
	kAssert(len != 0);
	kVec4 res = kDiv(v, kVec4Factor(len));
	return res;
}

kVec3 kCrossProduct(kVec3 a, kVec3 b) {
	kVec3 r;
	r.x = (a.y * b.z) - (a.z * b.y);
	r.y = (a.z * b.x) - (a.x * b.z);
	r.z = (a.x * b.y) - (a.y * b.x);
	return r;
}

kVec3 kTripleProduct(kVec3 a, kVec3 b, kVec3 c) {
	kVec3 d = kCrossProduct(a, b);
	kVec3 r = kCrossProduct(d, c);
	return r;
}

kVec3 kOrthoNormalBasisRH(kVec3 *a, kVec3 *b) {
	*a = kNormalizeZ(*a);
	kVec3 c = kCrossProduct(*a, *b);
	if (kLengthSq(c) == 0.0f) return c;
	c = kNormalizeZ(c);
	*b = kCrossProduct(c, *a);
	return c;
}

kVec3 kOrthoNormalBasisLH(kVec3 *a, kVec3 *b) {
	*a = kNormalizeZ(*a);
	kVec3 c = kCrossProduct(*b, *a);
	if (kLengthSq(c) == 0.0f) return c;
	c = kNormalizeZ(c);
	*b = kCrossProduct(*a, c);
	return c;
}

float kVec2AngleBetween(kVec2 a, kVec2 b) {
	float dot = kClamp(-1.0f, 1.0f, kDotProduct(a, b));
	return kArcCos(dot);
}

float kVec3AngleBetween(kVec3 a, kVec3 b) {
	float dot = kClamp(-1.0f, 1.0f, kDotProduct(a, b));
	return kArcCos(dot);
}

float kVec2AngleBetweenNormalized(kVec2 a, kVec2 b) {
	a = kNormalize(a);
	b = kNormalize(b);
	return kVec2AngleBetween(a, b);
}

float kVec3AngleBetweenNormalized(kVec3 a, kVec3 b) {
	a = kNormalize(a);
	b = kNormalize(b);
	return kVec3AngleBetween(a, b);
}

float kMat2Determinant(const kMat2 mat) {
	return mat.m2[0][0] * mat.m2[1][1] - mat.m2[0][1] * mat.m2[1][0];
}

kMat2 kMat2Inverse(const kMat2 *mat) {
	float det = mat->m2[0][0] * mat->m2[1][1] - mat->m2[0][1] * mat->m2[1][0];
	det /= det;
	kMat2 res;
	res.m2[0][0] = mat->m2[1][1];
	res.m2[0][1] = -mat->m2[0][1];
	res.m2[1][0] = -mat->m2[1][0];
	res.m2[1][1] = mat->m2[0][0];
	res.m[0] *= det;
	res.m[1] *= det;
	res.m[2] *= det;
	res.m[3] *= det;
	return res;
}

kMat2 kMat2Transpose(const kMat2 *m) {
	kMat2 t;
	t.m2[0][0] = m->m2[0][0];
	t.m2[0][1] = m->m2[1][0];
	t.m2[1][0] = m->m2[0][1];
	t.m2[1][1] = m->m2[1][1];
	return t;
}

float kMat3Determinant(const kMat3 *mat) {
	float det = mat->m2[0][0] * (mat->m2[1][1] * mat->m2[2][2] - mat->m2[2][1] * mat->m2[1][2]) +
		mat->m2[0][1] * (mat->m2[1][2] * mat->m2[2][0] - mat->m2[1][0] * mat->m2[2][2]) +
		mat->m2[0][2] * (mat->m2[1][0] * mat->m2[2][1] - mat->m2[2][0] * mat->m2[1][1]);
	return det;
}

kMat3 kMat3Inverse(const kMat3 *mat) {
	kMat3 result;
	result.m2[0][0] = mat->m2[1][1] * mat->m2[2][2] - mat->m2[2][1] * mat->m2[1][2];
	result.m2[0][1] = mat->m2[0][2] * mat->m2[2][1] - mat->m2[0][1] * mat->m2[2][2];
	result.m2[0][2] = mat->m2[0][1] * mat->m2[1][2] - mat->m2[0][2] * mat->m2[1][1];
	result.m2[1][0] = mat->m2[1][2] * mat->m2[2][0] - mat->m2[1][0] * mat->m2[2][2];
	result.m2[1][1] = mat->m2[0][0] * mat->m2[2][2] - mat->m2[0][2] * mat->m2[2][0];
	result.m2[1][2] = mat->m2[1][0] * mat->m2[0][2] - mat->m2[0][0] * mat->m2[1][2];
	result.m2[2][0] = mat->m2[1][0] * mat->m2[2][1] - mat->m2[2][0] * mat->m2[1][1];
	result.m2[2][1] = mat->m2[2][0] * mat->m2[0][1] - mat->m2[0][0] * mat->m2[2][1];
	result.m2[2][2] = mat->m2[0][0] * mat->m2[1][1] - mat->m2[1][0] * mat->m2[0][1];

	float det = mat->m2[0][0] * result.m2[0][0] + mat->m2[0][1] * result.m2[1][0] + mat->m2[0][2] * result.m2[2][0];
	det /= det;
	kVec3 factor = kVec3Factor(det);
	for (int i = 0; i < 3; i++)
		result.rows[i] = kMul(result.rows[i], factor);
	return result;
}

kMat3 kMat3Transpose(const kMat3 *m) {
	kMat3 res;
	res.rows[0] = (kVec3){ .x = m->m2[0][0], .y = m->m2[1][0], .z = m->m2[2][0] };
	res.rows[1] = (kVec3){ .x = m->m2[0][1], .y = m->m2[1][1], .z = m->m2[2][1] };
	res.rows[2] = (kVec3){ .x = m->m2[0][2], .y = m->m2[1][2], .z = m->m2[2][2] };
	return res;
}

float kMat4Determinant(const kMat4 *mat) {
	float m0 = mat->m[5] * mat->m[10] * mat->m[15] - mat->m[5] * mat->m[11] * mat->m[14] - mat->m[9] * mat->m[6] * mat->m[15] +
		mat->m[9] * mat->m[7] * mat->m[14] + mat->m[13] * mat->m[6] * mat->m[11] - mat->m[13] * mat->m[7] * mat->m[10];
	float m4 = -mat->m[4] * mat->m[10] * mat->m[15] + mat->m[4] * mat->m[11] * mat->m[14] + mat->m[8] * mat->m[6] * mat->m[15] -
		mat->m[8] * mat->m[7] * mat->m[14] - mat->m[12] * mat->m[6] * mat->m[11] + mat->m[12] * mat->m[7] * mat->m[10];
	float m8 = mat->m[4] * mat->m[9] * mat->m[15] - mat->m[4] * mat->m[11] * mat->m[13] - mat->m[8] * mat->m[5] * mat->m[15] +
		mat->m[8] * mat->m[7] * mat->m[13] + mat->m[12] * mat->m[5] * mat->m[11] - mat->m[12] * mat->m[7] * mat->m[9];
	float m12 = -mat->m[4] * mat->m[9] * mat->m[14] + mat->m[4] * mat->m[10] * mat->m[13] + mat->m[8] * mat->m[5] * mat->m[14] -
		mat->m[8] * mat->m[6] * mat->m[13] - mat->m[12] * mat->m[5] * mat->m[10] + mat->m[12] * mat->m[6] * mat->m[9];
	float det = mat->m[0] * m0 + mat->m[1] * m4 + mat->m[2] * m8 + mat->m[3] * m12;
	return det;
}

kMat4 kMat4Inverse(const kMat4 *mat) {
	kMat4 result;
	result.m[0] = mat->m[5] * mat->m[10] * mat->m[15] - mat->m[5] * mat->m[11] * mat->m[14] -
		mat->m[9] * mat->m[6] * mat->m[15] + mat->m[9] * mat->m[7] * mat->m[14] + mat->m[13] * mat->m[6] * mat->m[11] -
		mat->m[13] * mat->m[7] * mat->m[10];
	result.m[4] = -mat->m[4] * mat->m[10] * mat->m[15] + mat->m[4] * mat->m[11] * mat->m[14] +
		mat->m[8] * mat->m[6] * mat->m[15] - mat->m[8] * mat->m[7] * mat->m[14] - mat->m[12] * mat->m[6] * mat->m[11] +
		mat->m[12] * mat->m[7] * mat->m[10];
	result.m[8] = mat->m[4] * mat->m[9] * mat->m[15] - mat->m[4] * mat->m[11] * mat->m[13] - mat->m[8] * mat->m[5] * mat->m[15] +
		mat->m[8] * mat->m[7] * mat->m[13] + mat->m[12] * mat->m[5] * mat->m[11] - mat->m[12] * mat->m[7] * mat->m[9];
	result.m[12] = -mat->m[4] * mat->m[9] * mat->m[14] + mat->m[4] * mat->m[10] * mat->m[13] +
		mat->m[8] * mat->m[5] * mat->m[14] - mat->m[8] * mat->m[6] * mat->m[13] -
		mat->m[12] * mat->m[5] * mat->m[10] + mat->m[12] * mat->m[6] * mat->m[9];
	result.m[1] = -mat->m[1] * mat->m[10] * mat->m[15] + mat->m[1] * mat->m[11] * mat->m[14] +
		mat->m[9] * mat->m[2] * mat->m[15] - mat->m[9] * mat->m[3] * mat->m[14] - mat->m[13] * mat->m[2] * mat->m[11] +
		mat->m[13] * mat->m[3] * mat->m[10];
	result.m[5] = mat->m[0] * mat->m[10] * mat->m[15] - mat->m[0] * mat->m[11] * mat->m[14] -
		mat->m[8] * mat->m[2] * mat->m[15] + mat->m[8] * mat->m[3] * mat->m[14] + mat->m[12] * mat->m[2] * mat->m[11] -
		mat->m[12] * mat->m[3] * mat->m[10];
	result.m[9] = -mat->m[0] * mat->m[9] * mat->m[15] + mat->m[0] * mat->m[11] * mat->m[13] +
		mat->m[8] * mat->m[1] * mat->m[15] - mat->m[8] * mat->m[3] * mat->m[13] - mat->m[12] * mat->m[1] * mat->m[11] +
		mat->m[12] * mat->m[3] * mat->m[9];
	result.m[13] = mat->m[0] * mat->m[9] * mat->m[14] - mat->m[0] * mat->m[10] * mat->m[13] -
		mat->m[8] * mat->m[1] * mat->m[14] + mat->m[8] * mat->m[2] * mat->m[13] +
		mat->m[12] * mat->m[1] * mat->m[10] - mat->m[12] * mat->m[2] * mat->m[9];
	result.m[2] = mat->m[1] * mat->m[6] * mat->m[15] - mat->m[1] * mat->m[7] * mat->m[14] - mat->m[5] * mat->m[2] * mat->m[15] +
		mat->m[5] * mat->m[3] * mat->m[14] + mat->m[13] * mat->m[2] * mat->m[7] - mat->m[13] * mat->m[3] * mat->m[6];
	result.m[6] = -mat->m[0] * mat->m[6] * mat->m[15] + mat->m[0] * mat->m[7] * mat->m[14] + mat->m[4] * mat->m[2] * mat->m[15] -
		mat->m[4] * mat->m[3] * mat->m[14] - mat->m[12] * mat->m[2] * mat->m[7] + mat->m[12] * mat->m[3] * mat->m[6];
	result.m[10] = mat->m[0] * mat->m[5] * mat->m[15] - mat->m[0] * mat->m[7] * mat->m[13] - mat->m[4] * mat->m[1] * mat->m[15] +
		mat->m[4] * mat->m[3] * mat->m[13] + mat->m[12] * mat->m[1] * mat->m[7] - mat->m[12] * mat->m[3] * mat->m[5];
	result.m[14] = -mat->m[0] * mat->m[5] * mat->m[14] + mat->m[0] * mat->m[6] * mat->m[13] +
		mat->m[4] * mat->m[1] * mat->m[14] - mat->m[4] * mat->m[2] * mat->m[13] - mat->m[12] * mat->m[1] * mat->m[6] +
		mat->m[12] * mat->m[2] * mat->m[5];
	result.m[3] = -mat->m[1] * mat->m[6] * mat->m[11] + mat->m[1] * mat->m[7] * mat->m[10] + mat->m[5] * mat->m[2] * mat->m[11] -
		mat->m[5] * mat->m[3] * mat->m[10] - mat->m[9] * mat->m[2] * mat->m[7] + mat->m[9] * mat->m[3] * mat->m[6];
	result.m[7] = mat->m[0] * mat->m[6] * mat->m[11] - mat->m[0] * mat->m[7] * mat->m[10] - mat->m[4] * mat->m[2] * mat->m[11] +
		mat->m[4] * mat->m[3] * mat->m[10] + mat->m[8] * mat->m[2] * mat->m[7] - mat->m[8] * mat->m[3] * mat->m[6];
	result.m[11] = -mat->m[0] * mat->m[5] * mat->m[11] + mat->m[0] * mat->m[7] * mat->m[9] + mat->m[4] * mat->m[1] * mat->m[11] -
		mat->m[4] * mat->m[3] * mat->m[9] - mat->m[8] * mat->m[1] * mat->m[7] + mat->m[8] * mat->m[3] * mat->m[5];
	result.m[15] = mat->m[0] * mat->m[5] * mat->m[10] - mat->m[0] * mat->m[6] * mat->m[9] - mat->m[4] * mat->m[1] * mat->m[10] +
		mat->m[4] * mat->m[2] * mat->m[9] + mat->m[8] * mat->m[1] * mat->m[6] - mat->m[8] * mat->m[2] * mat->m[5];
	float det = mat->m[0] * result.m[0] + mat->m[1] * result.m[4] + mat->m[2] * result.m[8] + mat->m[3] * result.m[12];
	det = 1.0f / det;
	kVec4 factor = kVec4Factor(det);
	for (int i = 0; i < 4; i++)
		result.rows[i] = kMul(result.rows[i], factor);
	return result;
}

kMat4 kMat4Transpose(const kMat4 *m) {
	kMat4 res;
	res.rows[0] = (kVec4){ .x = m->m2[0][0], .y = m->m2[1][0], .z = m->m2[2][0], .w = m->m2[3][0] };
	res.rows[1] = (kVec4){ .x = m->m2[0][1], .y = m->m2[1][1], .z = m->m2[2][1], .w = m->m2[3][1] };
	res.rows[2] = (kVec4){ .x = m->m2[0][2], .y = m->m2[1][2], .z = m->m2[2][2], .w = m->m2[3][2] };
	res.rows[3] = (kVec4){ .x = m->m2[0][3], .y = m->m2[1][3], .z = m->m2[2][3], .w = m->m2[3][3] };
	return res;
}

kMat2 kMat2Mul(const kMat2 *left, const kMat2 *right) {
	kMat2 res;
	kMat2 transposed = kTranspose(right);
	res.m2[0][0] = kDotProduct(left->rows[0], transposed.rows[0]);
	res.m2[0][1] = kDotProduct(left->rows[0], transposed.rows[1]);
	res.m2[1][0] = kDotProduct(left->rows[1], transposed.rows[0]);
	res.m2[1][1] = kDotProduct(left->rows[1], transposed.rows[1]);
	return res;
}

kVec2 kMatVec2Mul(const kMat2 *mat, kVec2 vec) {
	kVec2 res;
	res.m[0] = kDotProduct(vec, mat->rows[0]);
	res.m[1] = kDotProduct(vec, mat->rows[1]);
	return res;
}

kVec2 kVecMat2Mul(kVec2 vec, const kMat2 *mat) {
	kMat2 transposed = kTranspose(mat);
	return kMatVec2Mul(&transposed, vec);
}

kMat3 kMat3Mul(const kMat3 *left, const kMat3 *right) {
	kMat3 res;
	kMat3 tranposed = kTranspose(right);
	res.m2[0][0] = kDotProduct(left->rows[0], tranposed.rows[0]);
	res.m2[0][1] = kDotProduct(left->rows[0], tranposed.rows[1]);
	res.m2[0][2] = kDotProduct(left->rows[0], tranposed.rows[2]);
	res.m2[1][0] = kDotProduct(left->rows[1], tranposed.rows[0]);
	res.m2[1][1] = kDotProduct(left->rows[1], tranposed.rows[1]);
	res.m2[1][2] = kDotProduct(left->rows[1], tranposed.rows[2]);
	res.m2[2][0] = kDotProduct(left->rows[2], tranposed.rows[0]);
	res.m2[2][1] = kDotProduct(left->rows[2], tranposed.rows[1]);
	res.m2[2][2] = kDotProduct(left->rows[2], tranposed.rows[2]);
	return res;
}

kVec3 kMatVec3Mul(const kMat3 *mat, kVec3 vec) {
	kVec3 res;
	res.m[0] = kDotProduct(vec, mat->rows[0]);
	res.m[1] = kDotProduct(vec, mat->rows[1]);
	res.m[2] = kDotProduct(vec, mat->rows[2]);
	return res;
}

kVec3 kVecMat3Mul(kVec3 vec, const kMat3 *mat) {
	kMat3 transposed = kTranspose(mat);
	return kMatVec3Mul(&transposed, vec);
}

kMat4 kMat4Mul(const kMat4 *left, const kMat4 *right) {
	kMat4 res;
	kMat4 tranposed = kTranspose(right);
	res.m2[0][0] = kDotProduct(left->rows[0], tranposed.rows[0]);
	res.m2[0][1] = kDotProduct(left->rows[0], tranposed.rows[1]);
	res.m2[0][2] = kDotProduct(left->rows[0], tranposed.rows[2]);
	res.m2[0][3] = kDotProduct(left->rows[0], tranposed.rows[3]);
	res.m2[1][0] = kDotProduct(left->rows[1], tranposed.rows[0]);
	res.m2[1][1] = kDotProduct(left->rows[1], tranposed.rows[1]);
	res.m2[1][2] = kDotProduct(left->rows[1], tranposed.rows[2]);
	res.m2[1][3] = kDotProduct(left->rows[1], tranposed.rows[3]);
	res.m2[2][0] = kDotProduct(left->rows[2], tranposed.rows[0]);
	res.m2[2][1] = kDotProduct(left->rows[2], tranposed.rows[1]);
	res.m2[2][2] = kDotProduct(left->rows[2], tranposed.rows[2]);
	res.m2[2][3] = kDotProduct(left->rows[2], tranposed.rows[3]);
	res.m2[3][0] = kDotProduct(left->rows[3], tranposed.rows[0]);
	res.m2[3][1] = kDotProduct(left->rows[3], tranposed.rows[1]);
	res.m2[3][2] = kDotProduct(left->rows[3], tranposed.rows[2]);
	res.m2[3][3] = kDotProduct(left->rows[3], tranposed.rows[3]);
	return res;
}

kVec4 kMatVec4Mul(const kMat4 *mat, kVec4 vec) {
	kVec4 res;
	res.m[0] = kDotProduct(vec, mat->rows[0]);
	res.m[1] = kDotProduct(vec, mat->rows[1]);
	res.m[2] = kDotProduct(vec, mat->rows[2]);
	res.m[3] = kDotProduct(vec, mat->rows[3]);
	return res;
}

kVec4 kVecMat4Mul(kVec4 vec, const kMat4 *mat) {
	kMat4 transposed = kTranspose(mat);
	return kMatVec4Mul(&transposed, vec);
}

kQuat kQuatNeg(kQuat a)          { kQuat r = kVec4UnaryOp(a, +); return r; }
kQuat kQuatAdd(kQuat a, kQuat b) { kQuat r = kVec4BinaryOp(a, b, +); return r; }
kQuat kQuatSub(kQuat a, kQuat b) { kQuat r = kVec4BinaryOp(a, b, -); return r; }

float kQuatDotProduct(kQuat q1, kQuat q2) { return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w; }
float kQuatLengthSq(kQuat q)              { return kDotProduct(q, q); }
float kQuatLength(kQuat q)                { return kSquareRoot(kLengthSq(q)); }

kQuat kQuatNormalize(kQuat q) {
	float len = kLength(q);
	kAssert(len != 0);
	kVec4 v = kDiv(q.vector, kVec4Factor(len));
	kQuat r = { .vector = v };
	return r;
}

kQuat kQuatConjugate(kQuat q) {
	kQuat r = { .x = -q.x, .y = -q.y, .z = -q.z, .w = q.w };
	return r;
}

kQuat kQuatMul(kQuat q1, kQuat q2) {
	float a = q1.w;
	float b = q1.x;
	float c = q1.y;
	float d = q1.z;
	float e = q2.w;
	float f = q2.x;
	float g = q2.y;
	float h = q2.z;
	kQuat  res;
	res.w = a * e - b * f - c * g - d * h;
	res.x = a * f + b * e + c * h - d * g;
	res.y = a * g - b * h + c * e + d * f;
	res.z = a * h + b * g - c * f + d * e;
	return res;
}

kVec3 kQuatRotate(kQuat q, kVec3 v) {
	kQuat p  = { .x = v.x, .y = v.y, .z = v.z, .w = 0 };
	kQuat qc = kConjugate(q);
	kQuat rc = kMul(q, p);
	kQuat rv = kMul(rc, qc);
	kVec3 r  = { .x = rv.x, .y = rv.y, .z = rv.z };
	return r;
}

kVec3 kRightVector(kQuat q) {
	kVec3 right;
	right.x = 1 - 2 * (q.y * q.y + q.z * q.z);
	right.y = 2 * (q.x * q.y + q.z * q.w);
	right.z = 2 * (q.x * q.z - q.y * q.w);
	return kNormalize(right);
}

kVec3 kUpVector(kQuat q) {
	kVec3 forward;
	forward.x = 2 * (q.x * q.y - q.z * q.w);
	forward.y = 1 - 2 * (q.x * q.x + q.z * q.z);
	forward.z = 2 * (q.y * q.z + q.x * q.w);
	return kNormalize(forward);
}

kVec3 kForwardVector(kQuat q) {
	kVec3 up;
	up.x = 2 * (q.x * q.z + q.y * q.w);
	up.y = 2 * (q.y * q.z - q.x * q.w);
	up.z = 1 - 2 * (q.x * q.x + q.y * q.y);
	return kNormalize(up);
}

kMat2 kMat3ToMat2(const kMat3 *mat) {
	kMat2 result;
	result.rows[0] = (kVec2){ .x = mat->rows[0].x, .y = mat->rows[0].y };
	result.rows[1] = (kVec2){ .x = mat->rows[1].x, .y = mat->rows[1].y };
	return result;
}

kMat3 kMat2ToMat3(const kMat2 *mat) {
	kMat3 result;
	result.rows[0] = (kVec3){ .xy = mat->rows[0], ._end = 0 };
	result.rows[1] = (kVec3){ .xy = mat->rows[1], ._end = 0 };
	result.rows[2] = (kVec3){ .x = 0, .y = 0, .z = 1};
	return result;
}

kMat3 kMat4ToMat3(const kMat4 *mat) {
	kMat3 result;
	result.rows[0] = (kVec3){ .x = mat->rows[0].x, .y = mat->rows[0].y, .z = mat->rows[0].z };
	result.rows[1] = (kVec3){ .x = mat->rows[1].x, .y = mat->rows[1].y, .z = mat->rows[1].z };
	result.rows[2] = (kVec3){ .x = mat->rows[2].x, .y = mat->rows[2].y, .z = mat->rows[2].z };
	return result;
}

kMat4 kMat3ToMat4(const kMat3 *mat) {
	kMat4 result;
	result.rows[0] = (kVec4){ .xyz = mat->rows[0], ._end = 0 };
	result.rows[1] = (kVec4){ .xyz = mat->rows[1], ._end = 0 };
	result.rows[2] = (kVec4){ .xyz = mat->rows[2], ._end = 0 };
	result.rows[3] = (kVec4){ .x = 0, .y = 0, .z = 0, .w = 1 };
	return result;
}

void kQuatToAngleAxis(kQuat q, float *angle, kVec3 *axis) {
	float len = kSquareRoot(q.x * q.x + q.y * q.y + q.z * q.z);
	if (len) {
		*angle = 2.0f * kArcTan2(len, q.w);
		len = 1.0f / len;
		axis->x = q.x * len;
		axis->y = q.y * len;
		axis->z = q.z * len;
	} else {
		// degenerate case, unit quaternion
		*angle = 0;
		*axis  = (kVec3){ .x = 0, .y = 0, .z = 1 };
	}
}

kMat4 kQuatToMat4(kQuat q) {
	float i = q.x;
	float j = q.y;
	float k = q.z;
	float r = q.w;

	float ii = i * i;
	float jj = j * j;
	float kk = k * k;

	float ij = i * j;
	float jk = j * k;
	float kr = k * r;
	float jr = j * r;
	float ir = i * r;
	float ik = i * k;

	kMat4  m;

	m.m2[0][0] = 1 - 2 * (jj + kk);
	m.m2[0][1] = 2 * (ij - kr);
	m.m2[0][2] = 2 * (ik + jr);
	m.m2[0][3] = 0;

	m.m2[1][0] = 2 * (ij + kr);
	m.m2[1][1] = 1 - 2 * (ii + kk);
	m.m2[1][2] = 2 * (jk - ir);
	m.m2[1][3] = 0;

	m.m2[2][0] = 2 * (ik - jr);
	m.m2[2][1] = 2 * (jk + ir);
	m.m2[2][2] = 1 - 2 * (ii + jj);
	m.m2[2][3] = 0;

	m.m2[3][0] = 0;
	m.m2[3][1] = 0;
	m.m2[3][2] = 0;
	m.m2[3][3] = 1;

	return m;
}

kVec3 kQuatToEulerAngles(kQuat q) {
	kVec3  angles;
	float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
	angles.z = kArcTan2(sinr_cosp, cosr_cosp);
	float sinp = 2.0f * (q.w * q.y - q.z * q.x);
	if (kAbsolute(sinp) >= 1.0f) {
		// use 90 degrees if out of range
		angles.x = kCopySign(K_PI / 2, sinp);
	} else {
		angles.x = kArcSin(sinp);
	}
	float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	angles.y = kArcTan2(siny_cosp, cosy_cosp);
	return angles;
}

kQuat kAngleAxisToQuat(kVec3 axis, float angle) {
	float r = kCos(angle * 0.5f);
	float s = kSin(angle * 0.5f);
	float i = s * axis.x;
	float j = s * axis.y;
	float k = s * axis.z;
	kQuat q = { .x = i, .y = j, .z = k, .w = r };
	return q;
}

kQuat kAngleAxisNormalizedToQuat(kVec3 axis, float angle) {
	return kAngleAxisToQuat(kNormalize(axis), angle);
}

kQuat kMat4ToQuat(const kMat4 *m) {
	kQuat  q;
	float trace = m->m2[0][0] + m->m2[1][1] + m->m2[2][2];
	if (trace > 0.0f) {
		float s = 0.5f / kSquareRoot(trace + 1.0f);
		q.w = 0.25f / s;
		q.x = (m->m2[2][1] - m->m2[1][2]) * s;
		q.y = (m->m2[0][2] - m->m2[2][0]) * s;
		q.z = (m->m2[1][0] - m->m2[0][1]) * s;
	} else {
		if (m->m2[0][0] > m->m2[1][1] && m->m2[0][0] > m->m2[2][2]) {
			float s = 2.0f * kSquareRoot(1.0f + m->m2[0][0] - m->m2[1][1] - m->m2[2][2]);
			q.w = (m->m2[2][1] - m->m2[1][2]) / s;
			q.x = 0.25f * s;
			q.y = (m->m2[0][1] + m->m2[1][0]) / s;
			q.z = (m->m2[0][2] + m->m2[2][0]) / s;
		} else if (m->m2[1][1] > m->m2[2][2]) {
			float s = 2.0f * kSquareRoot(1.0f + m->m2[1][1] - m->m2[0][0] - m->m2[2][2]);
			q.w = (m->m2[0][2] - m->m2[2][0]) / s;
			q.x = (m->m2[0][1] + m->m2[1][0]) / s;
			q.y = 0.25f * s;
			q.z = (m->m2[1][2] + m->m2[2][1]) / s;
		} else {
			float s = 2.0f * kSquareRoot(1.0f + m->m2[2][2] - m->m2[0][0] - m->m2[1][1]);
			q.w = (m->m2[1][0] - m->m2[0][1]) / s;
			q.x = (m->m2[0][2] + m->m2[2][0]) / s;
			q.y = (m->m2[1][2] + m->m2[2][1]) / s;
			q.z = 0.25f * s;
		}
	}
	return kNormalize(q);
}

kQuat kEulerAnglesToQuat(float pitch, float yaw, float roll) {
	float cy = kCos(roll * 0.5f);
	float sy = kSin(roll * 0.5f);
	float cp = kCos(yaw * 0.5f);
	float sp = kSin(yaw * 0.5f);
	float cr = kCos(pitch * 0.5f);
	float sr = kSin(pitch * 0.5f);

	kQuat  q;
	q.w = cy * cp * cr + sy * sp * sr;
	q.x = cy * cp * sr - sy * sp * cr;
	q.y = sy * cp * sr + cy * sp * cr;
	q.z = sy * cp * cr - cy * sp * sr;
	return q;
}

kMat2 kMat2Identity(void) { 
	kMat2 identity = {
		.m = {
			1, 0,
			0, 1
		}
	};
	return identity;
}

kMat2 kMat2Diagonal(float x, float y) {
	kMat2 m;
	m.rows[0] = (kVec2){ x, 0.0f };
	m.rows[1] = (kVec2){ 0.0f, y };
	return m;
}

kMat2 kMat2Rotation(kVec2 arm) {
	float c = arm.x;
	float s = arm.y;
	kMat2  mat;
	mat.rows[0] = (kVec2){ .x = c, .y = -s };
	mat.rows[1] = (kVec2){ .x = s, .y = c };
	return mat;
}

kMat2 kMat2RotationAngle(float angle) {
	return kMat2Rotation(kArm(angle));
}

kMat3 kMat3Identity(void) {
	kMat3 identity = {
		.m = {
			1, 0, 0,
			0, 1, 0,
			0, 0, 1
		}
	};
	return identity;
}

kMat3 kMat3Diagonal(float s1, float s2, float s3) {
	kMat3 m;
	m.rows[0] = (kVec3){ .x = s1, .y = 0.0f, .z = 0.0f };
	m.rows[1] = (kVec3){ .x = 0.0f, .y = s2, .z = 0.0f };
	m.rows[2] = (kVec3){ .x = 0.0f, .y = 0.0f, .z = s3 };
	return m;
}

kMat3 kMat3Scale(kVec2 scale) {
	return kMat3Diagonal(scale.x, scale.y, 1.0f);
}

kMat3 kMat3Translation(kVec2 t) {
	kMat3 m;
	m.rows[0] = (kVec3){ .x = 1.0f, .y = 0.0f, .z = t.x };
	m.rows[1] = (kVec3){ .x = 0.0f, .y = 1.0f, .z = t.y };
	m.rows[2] = (kVec3){ .x = 0.0f, .y = 0.0f, .z = 1.0f };
	return m;
}

kMat3 kMat3Rotation(kVec2 arm) {
	kMat3  m;
	float c = arm.x;
	float s = arm.y;
	m.rows[0] = (kVec3){ .x = c, .y = -s, .z = 0.0f };
	m.rows[1] = (kVec3){ .x = s, .y = c, .z = 0.0f };
	m.rows[2] = (kVec3){ .x = 0.0f, .y = 0.0f, .z = 1.0f };
	return m;
}

kMat3 kMat3RotationAngle(float angle) {
	return kMat3Rotation(kArm(angle));
}

kMat3 kMat3LookAt(kVec2 from, kVec2 to, kVec2 forward) {
	kVec2  dir = kNormalize(kSub(to, from));
	float cos_theta = kDotProduct(dir, forward);
	float sin_theta = kSquareRoot(1.0f - cos_theta * cos_theta);

	kMat3  m;
	m.rows[0] = (kVec3){ .x = cos_theta, .y = -sin_theta, .z = from.x };
	m.rows[1] = (kVec3){ .x = sin_theta, .y = cos_theta, .z = from.y };
	m.rows[2] = (kVec3){ .x = 0.0f, .y = 0.0f, .z = 1.0f };
	return m;
}

kMat4 kIdentity(void) {
	kMat4 identity = {
		.m = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		}
	};
	return identity;
}

kMat4 kDiagonal(float x, float y, float z, float w) {
	kMat4 m;
	m.rows[0] = (kVec4){ .x = x, .y = 0.0f, .z = 0.0f, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = y, .z = 0.0f, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = z, .w = 0.0f };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = w };
	return m;
}

kMat4 Scale(kVec3 s) {
	return kDiagonal(s.x, s.y, s.z, 1.0f);
}

kMat4 kTranslation(kVec3 t) {
	kMat4 m;
	m.rows[0] = (kVec4){ .x = 1.0f, .y = 0.0f, .z = 0.0f, .w = t.x };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = 1.0f, .z = 0.0f, .w = t.y };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 1.0f, .w = t.z };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kRotationX(kVec2 arm) {
	float c = arm.x;
	float s = arm.y;
	kMat4 m;
	m.rows[0] = (kVec4){ .x = 1.0f, .y = 0.0f, .z = 0.0f, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = c, .z = -s, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = s, .z = c, .w = 0.0f };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kRotationAngleX(float angle) {
	kVec2 arm = kArm(angle);
	return kRotationX(arm);
}

kMat4 kRotationY(kVec2 arm) {
	float c = arm.x;
	float s = arm.y;
	kMat4 m;
	m.rows[0] = (kVec4){ .x = c, .y = 0.0f, .z = s, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = 1.0f, .z = 0.0f, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = -s, .y = 0.0f, .z = c, .w = 0.0f };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kRotationAngleY(float angle) {
	kVec2 arm = kArm(angle);
	return kRotationY(arm);
}

kMat4 kRotationZ(kVec2 arm) {
	float c = arm.x;
	float s = arm.y;
	kMat4 m;
	m.rows[0] = (kVec4){ .x = c, .y = -s, .z = 0.0f, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = s, .y = c, .z = 0.0f, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 0.0f };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kRotationAngleZ(float angle) {
	kVec2 arm = kArm(angle);
	return kRotationZ(arm);
}

kMat4 kRotation(kVec3 axis, kVec2 arm) {
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;

	float c = arm.x;
	float s = arm.y;

	float x2 = x * x;
	float xy = x * y;
	float zx = x * z;
	float y2 = y * y;
	float yz = y * z;
	float z2 = z * z;

	kMat4  m;
	m.rows[0] = (kVec4){ .x = c + x2 * (1 - c), .y = xy * (1 - c) - z * s, .z = zx * (1 - c) + y * s, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = xy * (1 - c) + z * s, .y = c + y2 * (1 - c), .z = yz * (1 - c) - x * s, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = zx * (1 - c) - y * s, .y = yz * (1 - c) + x * s, .z = c + z2 * (1 - c), .w = 0.0f };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kRotationAngle(kVec3 axis, float angle) {
	return kRotation(axis, kArm(angle));
}

kMat4 kLookAt(kVec3 from, kVec3 to, kVec3 world_up) {
	kVec3 forward = kNormalize(kSub(from, to));
	kVec3 right   = kNormalize(kCrossProduct(world_up, forward));
	kVec3 up      = kCrossProduct(right, forward);

	kMat4 look;
	look.rows[0] = (kVec4){ .x = right.x, .y = up.x, .z = forward.x, .w = -from.x };
	look.rows[1] = (kVec4){ .x = right.y, .y = up.y, .z = forward.y, .w = -from.y };
	look.rows[2] = (kVec4){ .x = right.z, .y = up.z, .z = forward.z, .w = -from.z };
	look.rows[3] = (kVec4){ .x = 0, .y = 0, .z = 0, .w = 1 };

	return look;
}

kMat4 kLookTowards(kVec3 dir, kVec3 world_up) {
	kVec3 forward = dir;
	kVec3 right   = kNormalize(kCrossProduct(world_up, forward));
	kVec3 up      = kCrossProduct(right, forward);

	kMat4 look;
	look.rows[0] = (kVec4){ .x = right.x, .y = up.x, .z = forward.x, .w = 0 };
	look.rows[1] = (kVec4){ .x = right.y, .y = up.y, .z = forward.y, .w = 0 };
	look.rows[2] = (kVec4){ .x = right.z, .y = up.z, .z = forward.z, .w = 0 };
	look.rows[3] = (kVec4){ .x = 0, .y = 0, .z = 0, .w = 1 };

	return look;
}

kMat4 kOrthographicRH(float l, float r, float t, float b, float n, float f) {
	float iwidth  = 1 / (r - l);
	float iheight = 1 / (t - b);
	float range   = 1 / (n - f);

	kMat4  m;
	m.rows[0] = (kVec4){ .x = 2 * iwidth, .y = 0.0f, .z = 0.0f, .w = -(l + r) * iwidth };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = 2 * iheight, .z = 0.0f, .w = -(t + b) * iheight };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = range, .w = -n * range };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kOrthographicLH(float l, float r, float t, float b, float n, float f) {
	float iwidth  = 1 / (r - l);
	float iheight = 1 / (t - b);
	float range   = 1 / (f - n);

	kMat4  m;
	m.rows[0] = (kVec4){ .x = 2 * iwidth, .y = 0.0f, .z = 0.0f, .w = -(l + r) * iwidth };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = 2 * iheight, .z = 0.0f, .w = -(t + b) * iheight };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = range, .w = -n * range };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
	return m;
}

kMat4 kPerspectiveRH(float fov, float aspect_ratio, float n, float f) {
	float height = 1.0f / kTan(fov * 0.5f);
	float width  = height / aspect_ratio;
	float range  = 1 / (n - f);

	kMat4 m;
	m.rows[0] = (kVec4){ .x = width, .y = 0.0f, .z = 0.0f, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = height, .z = 0.0f, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = f * range, .w = -1.0f * f * n * range };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = -1.0f, .w = 0.0f };
	return m;
}

kMat4 kPerspectiveLH(float fov, float aspect_ratio, float n, float f) {
	float height = 1.0f / kTan(fov * 0.5f);
	float width  = height / aspect_ratio;
	float range  = 1 / (f - n);

	kMat4  m;
	m.rows[0] = (kVec4){ .x = width, .y = 0.0f, .z = 0.0f, .w = 0.0f };
	m.rows[1] = (kVec4){ .x = 0.0f, .y = height, .z = 0.0f, .w = 0.0f };
	m.rows[2] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = f * range, .w = -1.0f * f * n * range };
	m.rows[3] = (kVec4){ .x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 0.0f };
	return m;
}

kQuat kQuatIdentity(void) {
	kQuat q = { .x = 0, .y = 0, .z = 0, .w = 1 };
	return q;
}

kQuat kQuatBetweenVectors(kVec3 from, kVec3 to) {
	kQuat q;
	float w = 1.0f + kDotProduct(from, to);

	if (w) {
		kVec4 v = { .xyz = kCrossProduct(from, to), ._end = w };
		q = (kQuat){ .vector = v };
	} else {
		kVec3 xyz;
		if (kAbsolute(from.x) > kAbsolute(from.z)) {
			xyz = (kVec3){ .x = -from.y, .y = from.x, .z = 0 };
		} else {
			xyz = (kVec3){ .x = 0.f, .y = -from.z, .z = from.y };
		}
		kVec4 v = { .xyz = xyz, .w = w };
		q = (kQuat) { .vector = v };
	}

	return kNormalize(q);
}

kQuat kQuatBetween(kQuat a, kQuat b) {
	kQuat t   = kConjugate(a);
	float dot = kDotProduct(t, t);
	t.vector  = kDiv(t.vector, kVec4Factor(dot));
	kQuat r   = kMul(t, b);
	return r;
}

kQuat kQuatLookAt(kVec3 from, kVec3 to, kVec3 world_forward) {
	kVec3 dir = kSub(to, from);
	return kQuatBetweenVectors(world_forward, dir);
}

kVec2 kLerpWeight(float t) {
	kVec2 w = { .x = 1.0f - t, .y = t };
	return w;
}

float kVec1Lerp(float from, float to, float t) {
	kVec2 w = kLerpWeight(t);
	return w.x * from + w.y * to;
}

kVec2 kVec2Lerp(kVec2 from, kVec2 to, float t) {
	kVec2 r;
	kVec2 w = kLerpWeight(t);
	r.x = w.x * from.x + w.y * to.x;
	r.y = w.x * from.y + w.y * to.y;
	return r;
}

kVec3 kVec3Lerp(kVec3 from, kVec3 to, float t) {
	kVec3 r;
	kVec2 w = kLerpWeight(t);
	r.x = w.x * from.x + w.y * to.x;
	r.y = w.x * from.y + w.y * to.y;
	r.z = w.x * from.z + w.y * to.z;
	return r;
}

kVec4 kVec4Lerp(kVec4 from, kVec4 to, float t) {
	kVec4 r;
	kVec2 w = kLerpWeight(t);
	r.x = w.x * from.x + w.y * to.x;
	r.y = w.x * from.y + w.y * to.y;
	r.z = w.x * from.z + w.y * to.z;
	r.w = w.x * from.w + w.y * to.w;
	return r;
}

kQuat kQuatLerp(kQuat from, kQuat to, float t) {
	kQuat r;
	r.vector = kVec4Lerp(from.vector, to.vector, t);
	return r;
}

kVec3 kBezierQuadraticWeight(float t) {
	float mt = 1 - t;
	float w1 = mt * mt;
	float w2 = 2 * mt * t;
	float w3 = t * t;
	return (kVec3) { .x = w1, .y = w2, .z = w3 };
}

float kVec1BezierQuadratic(float a, float b, float c, float t) {
	kVec3 w = kBezierQuadraticWeight(t);
	return w.x * a + w.y * b + w.z * c;
}

kVec2 kVec2BezierQuadratic(kVec2 a, kVec2 b, kVec2 c, float t) {
	kVec2 r;
	kVec3 w = kBezierQuadraticWeight(t);
	r.x = w.x * a.x + w.y * b.x + w.z * c.x;
	r.y = w.x * a.y + w.y * b.y + w.z * c.y;
	return r;
}

kVec3 kVec3BezierQuadratic(kVec3 a, kVec3 b, kVec3 c, float t) {
	kVec3 r;
	kVec3 w = kBezierQuadraticWeight(t);
	r.x = w.x * a.x + w.y * b.x + w.z * c.x;
	r.y = w.x * a.y + w.y * b.y + w.z * c.y;
	r.z = w.x * a.z + w.y * b.z + w.z * c.z;
	return r;
}

kVec4 kVec4BezierQuadratic(kVec4 a, kVec4 b, kVec4 c, float t) {
	kVec4 r;
	kVec3 w = kBezierQuadraticWeight(t);
	r.x = w.x * a.x + w.y * b.x + w.z * c.x;
	r.y = w.x * a.y + w.y * b.y + w.z * c.y;
	r.z = w.x * a.z + w.y * b.z + w.z * c.z;
	r.w = w.x * a.w + w.y * b.w + w.z * c.w;
	return r;
}

kQuat kQuatBezierQuadratic(kQuat a, kQuat b, kQuat c, float t) {
	kQuat r;
	r.vector = kVec4BezierQuadratic(a.vector, b.vector, c.vector, t);
	return r;
}

kVec4 kBezierCubicWeight(float t) {
	float mt = 1.0f - t;
	float w1 = mt * mt * mt;
	float w2 = 3 * mt * mt * t;
	float w3 = 3 * mt * t * t;
	float w4 = t * t * t;
	return (kVec4){ .x = w1, .y = w2, .z = w3, .w = w4 };
}

float kVec1BezierCubic(float a, float b, float c, float d, float t) {
	kVec4 w = kBezierCubicWeight(t);
	return w.x * a + w.y * b + w.z * c + w.w * d;
}

kVec2 kVec2BezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, float t) {
	kVec2 r;
	kVec4 w = kBezierCubicWeight(t);
	r.x = w.x * a.x + w.y * b.x + w.z * c.x + w.w * d.x;
	r.y = w.x * a.y + w.y * b.y + w.z * c.y + w.w * d.y;
	return r;
}

kVec3 kVec3BezierCubic(kVec3 a, kVec3 b, kVec3 c, kVec3 d, float t) {
	kVec3 r;
	kVec4 w = kBezierCubicWeight(t);
	r.x = w.x * a.x + w.y * b.x + w.z * c.x + w.w * d.x;
	r.y = w.x * a.y + w.y * b.y + w.z * c.y + w.w * d.y;
	r.z = w.x * a.z + w.y * b.z + w.z * c.z + w.w * d.z;
	return r;
}

kVec4 kVec4BezierCubic(kVec4 a, kVec4 b, kVec4 c, kVec4 d, float t) {
	kVec4 r;
	kVec4 w = kBezierCubicWeight(t);
	r.x = w.x * a.x + w.y * b.x + w.z * c.x + w.w * d.x;
	r.y = w.x * a.y + w.y * b.y + w.z * c.y + w.w * d.y;
	r.z = w.x * a.z + w.y * b.z + w.z * c.z + w.w * d.z;
	r.w = w.x * a.w + w.y * b.w + w.z * c.w + w.w * d.w;
	return r;
}

kQuat kQuatBezierCubic(kQuat a, kQuat b, kQuat c, kQuat d, float t) {
	kQuat r;
	r.vector = kVec4BezierCubic(a.vector, b.vector, c.vector, d.vector, t);
	return r;
}

void kBuildVec1BezierQuadratic(float a, float b, float c, float *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t  = (float)index / (float)segments;
		points[index] = kVec1BezierQuadratic(a, b, c, t);
	}
}

void kBuildVec2BezierQuadratic(kVec2 a, kVec2 b, kVec2 c, kVec2 *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t  = (float)index / (float)segments;
		points[index] = kVec2BezierQuadratic(a, b, c, t);
	}
}

void kBuildVec3BezierQuadratic(kVec3 a, kVec3 b, kVec3 c, kVec3 *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t  = (float)index / (float)segments;
		points[index] = kVec3BezierQuadratic(a, b, c, t);
	}
}

void kBuildVec4BezierQuadratic(kVec4 a, kVec4 b, kVec4 c, kVec4 *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t  = (float)index / (float)segments;
		points[index] = kVec4BezierQuadratic(a, b, c, t);
	}
}

void kBuildQuatBezierQuadratic(kQuat a, kQuat b, kQuat c, kQuat *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t  = (float)index / (float)segments;
		points[index] = kQuatBezierQuadratic(a, b, c, t);
	}
}

void kBuildVec1BezierCubic(float a, float b, float c, float d, float *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t = (float)index / (float)segments;
		points[index] = kVec1BezierCubic(a, b, c, d, t);
	}
}

void kBuildVec2BezierCubic(kVec2 a, kVec2 b, kVec2 c, kVec2 d, kVec2 *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t = (float)index / (float)segments;
		points[index] = kVec2BezierCubic(a, b, c, d, t);
	}
}

void kBuildVec3BezierCubic(kVec3 a, kVec3 b, kVec3 c, kVec3 d, kVec3 *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t = (float)index / (float)segments;
		points[index] = kVec3BezierCubic(a, b, c, d, t);
	}
}

void kBuildVec4BezierCubic(kVec4 a, kVec4 b, kVec4 c, kVec4 d, kVec4 *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t = (float)index / (float)segments;
		points[index] = kVec4BezierCubic(a, b, c, d, t);
	}
}

void kBuildQuatBezierCubic(kQuat a, kQuat b, kQuat c, kQuat d, kQuat *points, int segments) {
	for (int index = 0; index <= segments; ++index) {
		float t = (float)index / (float)segments;
		points[index] = kQuatBezierCubic(a, b, c, d, t);
	}
}

kQuat kSlerp(kQuat from, kQuat to, float t) {
	float dot = kClamp(-1.0f, 1.0f, kDotProduct(from, to));

	// use shorter path
	if (dot < 0.0f) {
		to  = kNeg(to);
		dot = -dot;
	}

	if (dot > 0.9999f) {
		kQuat result = kLerp(from, to, t);
		return kNormalize(result);
	}

	float theta_0 = kArcCos(dot);
	float theta = theta_0 * t;
	float sin_theta = kSin(theta);
	float sin_theta_0 = kSin(theta_0);

	float s0 = kCos(theta) - dot * sin_theta / sin_theta_0;
	float s1 = sin_theta / sin_theta_0;

	kVec4 left  = kMul(from.vector, kVec4Factor(s0));
	kVec4 right = kMul(to.vector, kVec4Factor(s1));

	kQuat r = {
		.vector = kAdd(left, right)
	};

	return r;
}

float kVec1Step(float edge, float val) {
	return val < edge ? 0.0f : 1.0f;
}

kVec2 kVec2Step(kVec2 edge, kVec2 val) {
	kVec2 res;
	res.x = kVec1Step(edge.x, val.x);
	res.y = kVec1Step(edge.y, val.y);
	return res;
}

kVec3 kVec3Step(kVec3 edge, kVec3 val) {
	kVec3 res;
	res.x = kVec1Step(edge.x, val.x);
	res.y = kVec1Step(edge.y, val.y);
	res.z = kVec1Step(edge.z, val.z);
	return res;
}

kVec4 kVec4Step(kVec4 edge, kVec4 val) {
	kVec4 res;
	res.x = kVec1Step(edge.x, val.x);
	res.y = kVec1Step(edge.y, val.y);
	res.z = kVec1Step(edge.z, val.z);
	res.w = kVec1Step(edge.w, val.w);
	return res;
}

kQuat kQuatStep(kQuat edge, kQuat val) {
	kQuat res;
	res.x = kVec1Step(edge.x, val.x);
	res.y = kVec1Step(edge.y, val.y);
	res.z = kVec1Step(edge.z, val.z);
	res.w = kVec1Step(edge.w, val.w);
	return res;
}

float kInverseSmoothStep(float x) {
	return 0.5f - kSin(kArcSin(1.0f - 2.0f * x) / 3.0f);
}

float kVec1MoveTowards(float from, float to, float factor) {
	if (factor) {
		float direction = to - from;
		float distance  = kAbsolute(direction);

		if (distance < factor) {
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec2 kVec2MoveTowards(kVec2 from, kVec2 to, float factor) {
	if (factor) {
		kVec2  direction = kSub(to, from);
		float distance   = kLength(direction);

		if (distance < factor) {
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec3 kVec3MoveTowards(kVec3 from, kVec3 to, float factor) {
	if (factor) {
		kVec3  direction = kSub(to, from);
		float distance   = kLength(direction);

		if (distance < factor) {
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec4 kVec4MoveTowards(kVec4 from, kVec4 to, float factor) {
	if (factor) {
		kVec4  direction = kSub(to, from);
		float distance   = kLength(direction);

		if (distance < factor) {
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec2 kRotateAround(kVec2 point, kVec2 center, float angle) {
	float c = kCos(angle);
	float s = kSin(angle);
	kVec2  res;
	kVec2  p = kSub(point, center);
	res.x = p.x * c - p.y * s;
	res.y = p.x * s + p.y * c;
	res   = kAdd(res, center);
	return res;
}

kQuat kRotateTowards(kQuat from, kQuat to, float max_angle) {
	if (max_angle) {
		float dot = kClamp(-1.0f, 1.0f, kDotProduct(from, to));

		// use shorter path
		if (dot < 0.0f) {
			to  = kNeg(to);
			dot = -dot;
		}

		float theta_0 = kArcCos(dot);

		if (theta_0 < max_angle) {
			return to;
		}

		float t = max_angle / theta_0;

		theta_0 = max_angle;
		float theta = theta_0 * t;
		float sin_theta = kSin(theta);
		float sin_theta_0 = kSin(theta_0);

		float s0 = kCos(theta) - dot * sin_theta / sin_theta_0;
		float s1 = sin_theta / sin_theta_0;

		kVec4 left  = kMul(from.vector, kVec4Factor(s0));
		kVec4 right = kMul(to.vector, kVec4Factor(s1));

		kQuat r = {
			.vector = kAdd(left, right)
		};

		return r;
	}
	else {
		return from;
	}
}

kVec2 kReflect(kVec2 d, kVec2 n) {
	float c = kDotProduct(kNormalizeZ(d), n);
	float s = kSquareRoot(10.f - kSquare(c));
	kVec2  r;
	r.x = d.x * c - d.y * s;
	r.y = d.x * s + d.y * c;
	return r;
}

//
//
//

void kUnpackRGBA(u32 c, u8 channels[4]) {
	channels[0] = (c >> 24) & 0xff;
	channels[1] = (c >> 16) & 0xff;
	channels[2] = (c >> 8) & 0xff;
	channels[3] = (c >> 0) & 0xff;
}

u32 kPackRGBA(u8 r, u8 g, u8 b, u8 a) {
	return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
}

u32 kColor4ToUint(kVec4 v) {
	u8 r = (u8)(255.0f * v.x);
	u8 g = (u8)(255.0f * v.y);
	u8 b = (u8)(255.0f * v.z);
	u8 a = (u8)(255.0f * v.w);
	return kPackRGBA(r, g, b, a);
}

u32 kColor3ToUint(kVec3 v) {
	uint8_t r = (u8)(255.0f * v.x);
	uint8_t g = (u8)(255.0f * v.y);
	uint8_t b = (u8)(255.0f * v.z);
	return kPackRGBA(r, g, b, 0xff);
}

kVec4 kUintToColor4(u32 c) {
	kVec4 res;
	res.x = (float)((c >> 24) & 0xff) / 255.0f;
	res.y = (float)((c >> 16) & 0xff) / 255.0f;
	res.z = (float)((c >> 8) & 0xff) / 255.0f;
	res.w = (float)((c >> 0) & 0xff) / 255.0f;
	return res;
}

kVec3 kUintToColor3(u32 c) {
	kVec3 res;
	res.x = (float)((c >> 24) & 0xff) / 255.0f;
	res.y = (float)((c >> 16) & 0xff) / 255.0f;
	res.z = (float)((c >> 8) & 0xff) / 255.0f;
	return res;
}

kVec3 kLinearToSRGB(kVec3 Col, float gamma) {
	float igamma = 1.0f / gamma;
	kVec3  res;
	res.x = kPow(Col.x, igamma);
	res.y = kPow(Col.y, igamma);
	res.z = kPow(Col.z, igamma);
	return res;
}

kVec3 kSRGBToLinear(kVec3 Col, float gamma) {
	kVec3 res;
	res.x = kPow(Col.x, gamma);
	res.y = kPow(Col.y, gamma);
	res.z = kPow(Col.z, gamma);
	return res;
}

// http://en.wikipedia.org/wiki/HSL_and_HSV
kVec3 kHSVToRGB(kVec3 col) {
	kVec3  res;

	float h = col.x;
	float s = col.y;
	float v = col.z;

	if (s == 0.0f) {
		// gray
		res.x = res.y = res.z = v;
		return res;
	}

	h = kMod(h, 1.0f) / (60.0f / 360.0f);
	int   i = (int)h;
	float f = h - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (i) {
	case 0:
		res = (kVec3){ .x = v, .y = t, .z = p };
		break;
	case 1:
		res = (kVec3){ .x = q, .y = v, .z = p };
		break;
	case 2:
		res = (kVec3){ .x = p, .y = v, .z = t };
		break;
	case 3:
		res = (kVec3){ .x = p, .y = q, .z = v };
		break;
	case 4:
		res = (kVec3){ .x = t, .y = p, .z = v };
		break;
	case 5:
	default:
		res = (kVec3){ .x = v, .y = p, .z = q };
		break;
	}

	return res;
}

// http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
kVec3 kRGBToHSV(kVec3 c) {
	float r = c.x;
	float g = c.y;
	float b = c.z;

	float k = 0.f;
	if (g < b) {
		float t = b;
		b = g;
		g = t;
		k = -1.f;
	}
	if (r < g) {
		float t = g;
		g = r;
		r = t;
		k = -2.f / 6.f - k;
	}

	kVec3  res;
	float chroma = r - (g < b ? g : b);
	res.x = kAbsolute(k + (g - b) / (6.f * chroma + 1e-20f));
	res.y = chroma / (r + 1e-20f);
	res.z = r;
	return res;
}
