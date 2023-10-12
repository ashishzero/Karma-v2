#include "kMath.h"

float kWrap(float min, float a, float max)
{
	float range  = max - min;
	float offset = a - min;
	float result = (offset - (kFloor(offset / range) * range) + min);
	return result;
}

kVec2 kArm(float angle) { return kVec2(kCos(angle), kSin(angle)); }
kVec2 kArmInverse(float angle) { return kVec2(kSin(angle), kCos(angle)); }

// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
bool kAlmostEqual(float a, float b, float delta)
{
	float diff   = kAbsolute(a - b);
	a            = kAbsolute(a);
	b            = kAbsolute(b);

	float larger = (b > a) ? b : a;

	if (diff <= larger * delta) return true;
	return false;
}

bool kAlmostEqual(kVec2 a, kVec2 b, float delta)
{
	return kAlmostEqual(a.x, b.x, delta) && kAlmostEqual(a.y, b.y, delta);
}

bool kAlmostEqual(kVec3 a, kVec3 b, float delta)
{
	return kAlmostEqual(a.x, b.x, delta) && kAlmostEqual(a.y, b.y, delta) && kAlmostEqual(a.z, b.z, delta);
}

bool kAlmostEqual(kVec4 a, kVec4 b, float delta)
{
	return kAlmostEqual(a.x, b.x, delta) && kAlmostEqual(a.y, b.y, delta) && kAlmostEqual(a.z, b.z, delta) &&
	       kAlmostEqual(a.w, b.w, delta);
}

bool  kIsNull(float a) { return kAlmostEqual(a, 0.0f); }
bool  kIsNull(kVec2 a) { return kAlmostEqual(a, kVec2(0.0f)); }
bool  kIsNull(kVec3 a) { return kAlmostEqual(a, kVec3(0.0f)); }
bool  kIsNull(kVec4 a) { return kAlmostEqual(a, kVec4(0.0f)); }
bool  kIsNull(int32_t a) { return a == 0; }
bool  kIsNull(kVec2i a) { return a.x == 0 && a.y == 0; }
bool  kIsNull(kVec3i a) { return a.x == 0 && a.y == 0 && a.z == 0; }
bool  kIsNull(kVec4i a) { return a.x == 0 && a.y == 0 && a.z == 0 && a.w == 0; }

kVec2 kNormalizeZ(kVec2 v)
{
	kVec2 res(0);
	float len = kLength(v);
	if (len != 0) res = v / len;
	return res;
}

kVec3 kNormalizeZ(kVec3 v)
{
	kVec3 res(0);
	float len = kLength(v);
	if (len != 0) res = v / len;
	return res;
}

kVec4 kNormalizeZ(kVec4 v)
{
	kVec4 res(0);
	float len = kLength(v);
	if (len != 0) res = v * (1.0f / len);
	return res;
}

kVec2 kNormalize(kVec2 v)
{
	float len = kLength(v);
	kAssert(len != 0);
	kVec2 res = v / len;
	return res;
}

kVec3 kNormalize(kVec3 v)
{
	float len = kLength(v);
	kAssert(len != 0);
	kVec3 res = v / len;
	return res;
}

kVec4 kNormalize(kVec4 v)
{
	float len = kLength(v);
	kAssert(len != 0);
	kVec4 res = v * (1.0f / len);
	return res;
}

kVec2 kPerpendicularVector(kVec2 a, kVec2 b)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	return kNormalizeZ(kVec2(-dy, dx));
}

float kAngleBetween(kVec2 a, kVec2 b)
{
	float dot = kClamp(-1.0f, 1.0f, kDotProduct(a, b));
	return kArcCos(dot);
}

float kAngleBetween(kVec3 a, kVec3 b)
{
	float dot = kClamp(-1.0f, 1.0f, kDotProduct(a, b));
	return kArcCos(dot);
}

float kAngleBetweenNormalized(kVec2 a, kVec2 b)
{
	a = kNormalize(a);
	b = kNormalize(b);
	return kAngleBetween(a, b);
}

float kAngleBetweenNormalized(kVec3 a, kVec3 b)
{
	a = kNormalize(a);
	b = kNormalize(b);
	return kAngleBetween(a, b);
}

float kSignedAngleBetween(kVec2 a, kVec2 b)
{
	float dot   = kClamp(-1.0f, 1.0f, kDotProduct(a, b));
	float angle = kArcCos(dot);
	float cross = a.x * b.y - a.y * b.x;
	if (cross < 0)
	{
		angle = -angle;
	}
	return angle;
}

float kSignedAngleBetween(kVec3 a, kVec3 b, kVec3 n)
{
	float dot   = kClamp(-1.0f, 1.0f, kDotProduct(a, b));
	float angle = kArcCos(dot);
	kVec3 cross = kCrossProduct(a, b);
	if (kDotProduct(n, cross) < 0)
	{
		angle = -angle;
	}
	return angle;
}

float kSignedAngleBetweenNormalized(kVec2 a, kVec2 b)
{
	a = kNormalize(a);
	b = kNormalize(b);
	return kSignedAngleBetween(a, b);
}

float kSignedAngleBetweenNormalized(kVec3 a, kVec3 b, kVec3 n)
{
	a = kNormalize(a);
	b = kNormalize(b);
	n = kNormalize(n);
	return kSignedAngleBetween(a, b, n);
}

float kDeterminant(const kMat2 &mat) { return mat.m2[0][0] * mat.m2[1][1] - mat.m2[0][1] * mat.m2[1][0]; }

kMat2 kInverse(const kMat2 &mat)
{
	float det = mat.m2[0][0] * mat.m2[1][1] - mat.m2[0][1] * mat.m2[1][0];
	det /= det;
	kMat2 res;
	res.m2[0][0] = mat.m2[1][1];
	res.m2[0][1] = -mat.m2[0][1];
	res.m2[1][0] = -mat.m2[1][0];
	res.m2[1][1] = mat.m2[0][0];
	res.m[0] *= det;
	res.m[1] *= det;
	res.m[2] *= det;
	res.m[3] *= det;
	return res;
}

kMat2 kTranspose(const kMat2 &m)
{
	kMat2 t;
	t.m2[0][0] = m.m2[0][0];
	t.m2[0][1] = m.m2[1][0];
	t.m2[1][0] = m.m2[0][1];
	t.m2[1][1] = m.m2[1][1];
	return t;
}

float kDeterminant(const kMat3 &mat)
{
	float det = mat.m2[0][0] * (mat.m2[1][1] * mat.m2[2][2] - mat.m2[2][1] * mat.m2[1][2]) +
	            mat.m2[0][1] * (mat.m2[1][2] * mat.m2[2][0] - mat.m2[1][0] * mat.m2[2][2]) +
	            mat.m2[0][2] * (mat.m2[1][0] * mat.m2[2][1] - mat.m2[2][0] * mat.m2[1][1]);
	return det;
}

kMat3 kInverse(const kMat3 &mat)
{
	kMat3 result;
	result.m2[0][0] = mat.m2[1][1] * mat.m2[2][2] - mat.m2[2][1] * mat.m2[1][2];
	result.m2[0][1] = mat.m2[0][2] * mat.m2[2][1] - mat.m2[0][1] * mat.m2[2][2];
	result.m2[0][2] = mat.m2[0][1] * mat.m2[1][2] - mat.m2[0][2] * mat.m2[1][1];
	result.m2[1][0] = mat.m2[1][2] * mat.m2[2][0] - mat.m2[1][0] * mat.m2[2][2];
	result.m2[1][1] = mat.m2[0][0] * mat.m2[2][2] - mat.m2[0][2] * mat.m2[2][0];
	result.m2[1][2] = mat.m2[1][0] * mat.m2[0][2] - mat.m2[0][0] * mat.m2[1][2];
	result.m2[2][0] = mat.m2[1][0] * mat.m2[2][1] - mat.m2[2][0] * mat.m2[1][1];
	result.m2[2][1] = mat.m2[2][0] * mat.m2[0][1] - mat.m2[0][0] * mat.m2[2][1];
	result.m2[2][2] = mat.m2[0][0] * mat.m2[1][1] - mat.m2[1][0] * mat.m2[0][1];

	float det       = mat.m2[0][0] * result.m2[0][0] + mat.m2[0][1] * result.m2[1][0] + mat.m2[0][2] * result.m2[2][0];
	det /= det;
	for (int i = 0; i < 3; i++)
		result.rows[i] = result.rows[i] * det;
	return result;
}

kMat3 kTranspose(const kMat3 &m)
{
	kMat3 res;
	res.rows[0] = kVec3(m.m2[0][0], m.m2[1][0], m.m2[2][0]);
	res.rows[1] = kVec3(m.m2[0][1], m.m2[1][1], m.m2[2][1]);
	res.rows[2] = kVec3(m.m2[0][2], m.m2[1][2], m.m2[2][2]);
	return res;
}

float kDeterminant(const kMat4 &mat)
{
	float m0 = mat.m[5] * mat.m[10] * mat.m[15] - mat.m[5] * mat.m[11] * mat.m[14] - mat.m[9] * mat.m[6] * mat.m[15] +
	           mat.m[9] * mat.m[7] * mat.m[14] + mat.m[13] * mat.m[6] * mat.m[11] - mat.m[13] * mat.m[7] * mat.m[10];

	float m4 = -mat.m[4] * mat.m[10] * mat.m[15] + mat.m[4] * mat.m[11] * mat.m[14] + mat.m[8] * mat.m[6] * mat.m[15] -
	           mat.m[8] * mat.m[7] * mat.m[14] - mat.m[12] * mat.m[6] * mat.m[11] + mat.m[12] * mat.m[7] * mat.m[10];

	float m8 = mat.m[4] * mat.m[9] * mat.m[15] - mat.m[4] * mat.m[11] * mat.m[13] - mat.m[8] * mat.m[5] * mat.m[15] +
	           mat.m[8] * mat.m[7] * mat.m[13] + mat.m[12] * mat.m[5] * mat.m[11] - mat.m[12] * mat.m[7] * mat.m[9];

	float m12 = -mat.m[4] * mat.m[9] * mat.m[14] + mat.m[4] * mat.m[10] * mat.m[13] + mat.m[8] * mat.m[5] * mat.m[14] -
	            mat.m[8] * mat.m[6] * mat.m[13] - mat.m[12] * mat.m[5] * mat.m[10] + mat.m[12] * mat.m[6] * mat.m[9];

	float det = mat.m[0] * m0 + mat.m[1] * m4 + mat.m[2] * m8 + mat.m[3] * m12;
	return det;
}

kMat4 kInverse(const kMat4 &mat)
{
	kMat4 result;

	result.m[0] = mat.m[5] * mat.m[10] * mat.m[15] - mat.m[5] * mat.m[11] * mat.m[14] -
	              mat.m[9] * mat.m[6] * mat.m[15] + mat.m[9] * mat.m[7] * mat.m[14] + mat.m[13] * mat.m[6] * mat.m[11] -
	              mat.m[13] * mat.m[7] * mat.m[10];

	result.m[4] = -mat.m[4] * mat.m[10] * mat.m[15] + mat.m[4] * mat.m[11] * mat.m[14] +
	              mat.m[8] * mat.m[6] * mat.m[15] - mat.m[8] * mat.m[7] * mat.m[14] - mat.m[12] * mat.m[6] * mat.m[11] +
	              mat.m[12] * mat.m[7] * mat.m[10];

	result.m[8] = mat.m[4] * mat.m[9] * mat.m[15] - mat.m[4] * mat.m[11] * mat.m[13] - mat.m[8] * mat.m[5] * mat.m[15] +
	              mat.m[8] * mat.m[7] * mat.m[13] + mat.m[12] * mat.m[5] * mat.m[11] - mat.m[12] * mat.m[7] * mat.m[9];

	result.m[12] = -mat.m[4] * mat.m[9] * mat.m[14] + mat.m[4] * mat.m[10] * mat.m[13] +
	               mat.m[8] * mat.m[5] * mat.m[14] - mat.m[8] * mat.m[6] * mat.m[13] -
	               mat.m[12] * mat.m[5] * mat.m[10] + mat.m[12] * mat.m[6] * mat.m[9];

	result.m[1] = -mat.m[1] * mat.m[10] * mat.m[15] + mat.m[1] * mat.m[11] * mat.m[14] +
	              mat.m[9] * mat.m[2] * mat.m[15] - mat.m[9] * mat.m[3] * mat.m[14] - mat.m[13] * mat.m[2] * mat.m[11] +
	              mat.m[13] * mat.m[3] * mat.m[10];

	result.m[5] = mat.m[0] * mat.m[10] * mat.m[15] - mat.m[0] * mat.m[11] * mat.m[14] -
	              mat.m[8] * mat.m[2] * mat.m[15] + mat.m[8] * mat.m[3] * mat.m[14] + mat.m[12] * mat.m[2] * mat.m[11] -
	              mat.m[12] * mat.m[3] * mat.m[10];

	result.m[9] = -mat.m[0] * mat.m[9] * mat.m[15] + mat.m[0] * mat.m[11] * mat.m[13] +
	              mat.m[8] * mat.m[1] * mat.m[15] - mat.m[8] * mat.m[3] * mat.m[13] - mat.m[12] * mat.m[1] * mat.m[11] +
	              mat.m[12] * mat.m[3] * mat.m[9];

	result.m[13] = mat.m[0] * mat.m[9] * mat.m[14] - mat.m[0] * mat.m[10] * mat.m[13] -
	               mat.m[8] * mat.m[1] * mat.m[14] + mat.m[8] * mat.m[2] * mat.m[13] +
	               mat.m[12] * mat.m[1] * mat.m[10] - mat.m[12] * mat.m[2] * mat.m[9];

	result.m[2] = mat.m[1] * mat.m[6] * mat.m[15] - mat.m[1] * mat.m[7] * mat.m[14] - mat.m[5] * mat.m[2] * mat.m[15] +
	              mat.m[5] * mat.m[3] * mat.m[14] + mat.m[13] * mat.m[2] * mat.m[7] - mat.m[13] * mat.m[3] * mat.m[6];

	result.m[6] = -mat.m[0] * mat.m[6] * mat.m[15] + mat.m[0] * mat.m[7] * mat.m[14] + mat.m[4] * mat.m[2] * mat.m[15] -
	              mat.m[4] * mat.m[3] * mat.m[14] - mat.m[12] * mat.m[2] * mat.m[7] + mat.m[12] * mat.m[3] * mat.m[6];

	result.m[10] = mat.m[0] * mat.m[5] * mat.m[15] - mat.m[0] * mat.m[7] * mat.m[13] - mat.m[4] * mat.m[1] * mat.m[15] +
	               mat.m[4] * mat.m[3] * mat.m[13] + mat.m[12] * mat.m[1] * mat.m[7] - mat.m[12] * mat.m[3] * mat.m[5];

	result.m[14] = -mat.m[0] * mat.m[5] * mat.m[14] + mat.m[0] * mat.m[6] * mat.m[13] +
	               mat.m[4] * mat.m[1] * mat.m[14] - mat.m[4] * mat.m[2] * mat.m[13] - mat.m[12] * mat.m[1] * mat.m[6] +
	               mat.m[12] * mat.m[2] * mat.m[5];

	result.m[3] = -mat.m[1] * mat.m[6] * mat.m[11] + mat.m[1] * mat.m[7] * mat.m[10] + mat.m[5] * mat.m[2] * mat.m[11] -
	              mat.m[5] * mat.m[3] * mat.m[10] - mat.m[9] * mat.m[2] * mat.m[7] + mat.m[9] * mat.m[3] * mat.m[6];

	result.m[7] = mat.m[0] * mat.m[6] * mat.m[11] - mat.m[0] * mat.m[7] * mat.m[10] - mat.m[4] * mat.m[2] * mat.m[11] +
	              mat.m[4] * mat.m[3] * mat.m[10] + mat.m[8] * mat.m[2] * mat.m[7] - mat.m[8] * mat.m[3] * mat.m[6];

	result.m[11] = -mat.m[0] * mat.m[5] * mat.m[11] + mat.m[0] * mat.m[7] * mat.m[9] + mat.m[4] * mat.m[1] * mat.m[11] -
	               mat.m[4] * mat.m[3] * mat.m[9] - mat.m[8] * mat.m[1] * mat.m[7] + mat.m[8] * mat.m[3] * mat.m[5];

	result.m[15] = mat.m[0] * mat.m[5] * mat.m[10] - mat.m[0] * mat.m[6] * mat.m[9] - mat.m[4] * mat.m[1] * mat.m[10] +
	               mat.m[4] * mat.m[2] * mat.m[9] + mat.m[8] * mat.m[1] * mat.m[6] - mat.m[8] * mat.m[2] * mat.m[5];

	float det = mat.m[0] * result.m[0] + mat.m[1] * result.m[4] + mat.m[2] * result.m[8] + mat.m[3] * result.m[12];
	det       = 1.0f / det;
	for (int i = 0; i < 4; i++)
		result.rows[i] = result.rows[i] * det;
	return result;
}

kMat4 kTranspose(const kMat4 &m)
{
	kMat4 res;
	res.rows[0] = kVec4(m.m2[0][0], m.m2[1][0], m.m2[2][0], m.m2[3][0]);
	res.rows[1] = kVec4(m.m2[0][1], m.m2[1][1], m.m2[2][1], m.m2[3][1]);
	res.rows[2] = kVec4(m.m2[0][2], m.m2[1][2], m.m2[2][2], m.m2[3][2]);
	res.rows[3] = kVec4(m.m2[0][3], m.m2[1][3], m.m2[2][3], m.m2[3][3]);
	return res;
}

kMat2 operator-(kMat2 &mat)
{
	kMat2 r;
	r.rows[0] = -mat.rows[0];
	r.rows[1] = -mat.rows[1];
	return r;
}

kMat3 operator-(kMat3 &mat)
{
	kMat3 r;
	r.rows[0] = -mat.rows[0];
	r.rows[1] = -mat.rows[1];
	r.rows[2] = -mat.rows[2];
	return r;
}

kMat4 operator-(kMat4 &mat)
{
	kMat4 r;
	r.rows[0] = -mat.rows[0];
	r.rows[1] = -mat.rows[1];
	r.rows[2] = -mat.rows[2];
	r.rows[3] = -mat.rows[3];
	return r;
}

kMat2 operator+(const kMat2 &Left, const kMat2 &Right)
{
	kMat2 r;
	r.rows[0] = Left.rows[0] + Right.rows[0];
	r.rows[1] = Left.rows[1] + Right.rows[1];
	return r;
}

kMat2 operator-(const kMat2 &Left, const kMat2 &Right)
{
	kMat2 r;
	r.rows[0] = Left.rows[0] - Right.rows[0];
	r.rows[1] = Left.rows[1] - Right.rows[1];
	return r;
}

kMat3 operator+(const kMat3 &Left, const kMat3 &Right)
{
	kMat3 r;
	r.rows[0] = Left.rows[0] + Right.rows[0];
	r.rows[1] = Left.rows[1] + Right.rows[1];
	r.rows[2] = Left.rows[2] + Right.rows[2];
	return r;
}

kMat3 operator-(const kMat3 &Left, const kMat3 &Right)
{
	kMat3 r;
	r.rows[0] = Left.rows[0] - Right.rows[0];
	r.rows[1] = Left.rows[1] - Right.rows[1];
	r.rows[2] = Left.rows[2] - Right.rows[2];
	return r;
}

kMat4 operator+(const kMat4 &Left, const kMat4 &Right)
{
	kMat4 r;
	r.rows[0] = Left.rows[0] + Right.rows[0];
	r.rows[1] = Left.rows[1] + Right.rows[1];
	r.rows[2] = Left.rows[2] + Right.rows[2];
	r.rows[3] = Left.rows[3] + Right.rows[3];
	return r;
}

kMat4 operator-(const kMat4 &Left, const kMat4 &Right)
{
	kMat4 r;
	r.rows[0] = Left.rows[0] - Right.rows[0];
	r.rows[1] = Left.rows[1] - Right.rows[1];
	r.rows[2] = Left.rows[2] - Right.rows[2];
	r.rows[3] = Left.rows[3] - Right.rows[3];
	return r;
}

kMat2 &operator-=(kMat2 &mat, const kMat2 &other)
{
	mat = mat - other;
	return mat;
}
kMat3 &operator-=(kMat3 &mat, const kMat3 &other)
{
	mat = mat - other;
	return mat;
}
kMat4 &operator-=(kMat4 &mat, const kMat4 &other)
{
	mat = mat - other;
	return mat;
}
kMat2 &operator+=(kMat2 &mat, const kMat2 &other)
{
	mat = mat + other;
	return mat;
}
kMat3 &operator+=(kMat3 &mat, const kMat3 &other)
{
	mat = mat + other;
	return mat;
}
kMat4 &operator+=(kMat4 &mat, const kMat4 &other)
{
	mat = mat + other;
	return mat;
}

kMat2 operator*(const kMat2 &Left, const kMat2 &Right)
{
	kMat2 res;
	kMat2 tras   = kTranspose(Right);

	res.m2[0][0] = kDotProduct(Left.rows[0], tras.rows[0]);
	res.m2[0][1] = kDotProduct(Left.rows[0], tras.rows[1]);
	res.m2[1][0] = kDotProduct(Left.rows[1], tras.rows[0]);
	res.m2[1][1] = kDotProduct(Left.rows[1], tras.rows[1]);

	return res;
}

kVec2 operator*(const kMat2 &mat, kVec2 vec)
{
	kVec2 res;
	res.m[0] = kDotProduct(vec, mat.rows[0]);
	res.m[1] = kDotProduct(vec, mat.rows[1]);
	return res;
}

kVec2 operator*(kVec2 vec, const kMat2 &mat)
{
	kVec2 res;
	res.m[0] = kDotProduct(vec, kVec2(mat.m2[0][0], mat.m2[1][0]));
	res.m[1] = kDotProduct(vec, kVec2(mat.m2[0][1], mat.m2[1][1]));
	return res;
}

kMat3 operator*(const kMat3 &Left, const kMat3 &Right)
{
	kMat3 res;
	kMat3 tras   = kTranspose(Right);

	res.m2[0][0] = kDotProduct(Left.rows[0], tras.rows[0]);
	res.m2[0][1] = kDotProduct(Left.rows[0], tras.rows[1]);
	res.m2[0][2] = kDotProduct(Left.rows[0], tras.rows[2]);

	res.m2[1][0] = kDotProduct(Left.rows[1], tras.rows[0]);
	res.m2[1][1] = kDotProduct(Left.rows[1], tras.rows[1]);
	res.m2[1][2] = kDotProduct(Left.rows[1], tras.rows[2]);

	res.m2[2][0] = kDotProduct(Left.rows[2], tras.rows[0]);
	res.m2[2][1] = kDotProduct(Left.rows[2], tras.rows[1]);
	res.m2[2][2] = kDotProduct(Left.rows[2], tras.rows[2]);

	return res;
}

kVec3 operator*(const kMat3 &mat, kVec3 vec)
{
	kVec3 res;
	res.m[0] = kDotProduct(vec, mat.rows[0]);
	res.m[1] = kDotProduct(vec, mat.rows[1]);
	res.m[2] = kDotProduct(vec, mat.rows[2]);
	return res;
}

kMat4 operator*(const kMat4 &Left, const kMat4 &Right)
{
	kMat4 res;
	kMat4 tras   = kTranspose(Right);

	res.m2[0][0] = kDotProduct(Left.rows[0], tras.rows[0]);
	res.m2[0][1] = kDotProduct(Left.rows[0], tras.rows[1]);
	res.m2[0][2] = kDotProduct(Left.rows[0], tras.rows[2]);
	res.m2[0][3] = kDotProduct(Left.rows[0], tras.rows[3]);

	res.m2[1][0] = kDotProduct(Left.rows[1], tras.rows[0]);
	res.m2[1][1] = kDotProduct(Left.rows[1], tras.rows[1]);
	res.m2[1][2] = kDotProduct(Left.rows[1], tras.rows[2]);
	res.m2[1][3] = kDotProduct(Left.rows[1], tras.rows[3]);

	res.m2[2][0] = kDotProduct(Left.rows[2], tras.rows[0]);
	res.m2[2][1] = kDotProduct(Left.rows[2], tras.rows[1]);
	res.m2[2][2] = kDotProduct(Left.rows[2], tras.rows[2]);
	res.m2[2][3] = kDotProduct(Left.rows[2], tras.rows[3]);

	res.m2[3][0] = kDotProduct(Left.rows[3], tras.rows[0]);
	res.m2[3][1] = kDotProduct(Left.rows[3], tras.rows[1]);
	res.m2[3][2] = kDotProduct(Left.rows[3], tras.rows[2]);
	res.m2[3][3] = kDotProduct(Left.rows[3], tras.rows[3]);

	return res;
}

kVec4 operator*(const kMat4 &mat, kVec4 vec)
{
	kVec4 res;
	res.m[0] = kDotProduct(vec, mat.rows[0]);
	res.m[1] = kDotProduct(vec, mat.rows[1]);
	res.m[2] = kDotProduct(vec, mat.rows[2]);
	res.m[3] = kDotProduct(vec, mat.rows[3]);
	return res;
}

kMat2 &operator*=(kMat2 &t, kMat2 &o)
{
	t = t * o;
	return t;
}

kMat3 &operator*=(kMat3 &t, kMat3 &o)
{
	t = t * o;
	return t;
}

kMat4 &operator*=(kMat4 &t, kMat4 &o)
{
	t = t * o;
	return t;
}

kQuat  operator-(kQuat q) { return kQuat(-q.x, -q.y, -q.z, -q.w); }
kQuat  operator-(kQuat r1, kQuat r2) { return kQuat(r1.x - r2.x, r1.y - r2.y, r1.z - r2.z, r1.w - r2.w); }
kQuat  operator+(kQuat r1, kQuat r2) { return kQuat(r1.x + r2.x, r1.y + r2.y, r1.z + r2.z, r1.w + r2.w); }
kQuat  operator*(kQuat q, float s) { return kQuat(q.x * s, q.y * s, q.z * s, q.w * s); }
kQuat  operator*(float s, kQuat q) { return kQuat(q.x * s, q.y * s, q.z * s, q.w * s); }

kQuat &operator-=(kQuat &q, kQuat other)
{
	q = q + other;
	return q;
}
kQuat &operator+=(kQuat &q, kQuat other)
{
	q = q - other;
	return q;
}

float kDotProduct(kQuat q1, kQuat q2) { return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w; }
float kLength(kQuat q) { return kSquareRoot(kDotProduct(q, q)); }

kQuat kNormalize(kQuat q)
{
	float len = kLength(q);
	kAssert(len != 0);
	return q * (1.0f / len);
}

kQuat kConjugate(kQuat q) { return kQuat(-q.x, -q.y, -q.z, q.w); }

kQuat operator*(kQuat q1, kQuat q2)
{
	float a = q1.w;
	float b = q1.x;
	float c = q1.y;
	float d = q1.z;

	float e = q2.w;
	float f = q2.x;
	float g = q2.y;
	float h = q2.z;

	kQuat res;
	res.w = a * e - b * f - c * g - d * h;
	res.x = a * f + b * e + c * h - d * g;
	res.y = a * g - b * h + c * e + d * f;
	res.z = a * h + b * g - c * f + d * e;
	return res;
}

kVec3 operator*(kQuat q, kVec3 v)
{
	kQuat p   = kQuat(v.x, v.y, v.z, 0);
	kQuat qc  = kConjugate(q);
	kQuat res = (q * p * qc);
	return kVec3(res.x, res.y, res.z);
}

kVec3 kRotate(kQuat q, kVec3 v) { return q * v; }

//
//
//

kVec3 kRightDirection(const kMat4 &m)
{
	kVec3 v;
	v.x = m.m2[0][0];
	v.y = m.m2[1][0];
	v.z = m.m2[2][0];
	return v;
}

kVec3 kUpDirection(const kMat4 &m)
{
	kVec3 v;
	v.x = m.m2[0][2];
	v.y = m.m2[1][2];
	v.z = m.m2[2][2];
	return v;
}

kVec3 kForwardDirection(const kMat4 &m)
{
	kVec3 v;
	v.x = m.m2[0][1];
	v.y = m.m2[1][1];
	v.z = m.m2[2][1];
	return v;
}

kVec3 kRightDirection(kQuat q)
{
	kVec3 Right;
	Right.x = 1 - 2 * (q.y * q.y + q.z * q.z);
	Right.y = 2 * (q.x * q.y + q.z * q.w);
	Right.z = 2 * (q.x * q.z - q.y * q.w);
	return kNormalize(Right);
}

kVec3 kUpDirection(kQuat q)
{
	kVec3 forward;
	forward.x = 2 * (q.x * q.y - q.z * q.w);
	forward.y = 1 - 2 * (q.x * q.x + q.z * q.z);
	forward.z = 2 * (q.y * q.z + q.x * q.w);
	return kNormalize(forward);
}

kVec3 kForwardDirection(kQuat q)
{
	kVec3 up;
	up.x = 2 * (q.x * q.z + q.y * q.w);
	up.y = 2 * (q.y * q.z - q.x * q.w);
	up.z = 1 - 2 * (q.x * q.x + q.y * q.y);
	return kNormalize(up);
}

//
//
//

kMat2 kMat3ToMat2(const kMat3 &mat)
{
	kMat2 result;
	result.rows[0] = kVec2(mat.rows[0].x, mat.rows[0].y);
	result.rows[1] = kVec2(mat.rows[1].x, mat.rows[1].y);
	return result;
}

kMat3 kMat4ToMat3(const kMat2 &mat)
{
	kMat3 result;
	result.rows[0] = kVec3(mat.rows[0], 0);
	result.rows[1] = kVec3(mat.rows[1], 0);
	result.rows[2] = kVec3(0, 0, 1);
	return result;
}

kMat3 kMat4ToMat3(const kMat4 &mat)
{
	kMat3 result;
	result.rows[0] = kVec3(mat.rows[0].x, mat.rows[0].y, mat.rows[0].z);
	result.rows[1] = kVec3(mat.rows[1].x, mat.rows[1].y, mat.rows[1].z);
	result.rows[2] = kVec3(mat.rows[2].x, mat.rows[2].y, mat.rows[2].z);
	return result;
}

kMat4 kMat3ToMat4(const kMat3 &mat)
{
	kMat4 result;
	result.rows[0] = kVec4(mat.rows[0], 0);
	result.rows[1] = kVec4(mat.rows[1], 0);
	result.rows[2] = kVec4(mat.rows[2], 0);
	result.rows[3] = kVec4(0, 0, 0, 1);
	return result;
}

void kQuatToAngleAxis(kQuat q, float *angle, kVec3 *axis)
{
	float len = kSquareRoot(q.x * q.x + q.y * q.y + q.z * q.z);
	if (len)
	{
		*angle  = 2.0f * kArcTan2(len, q.w);
		len     = 1.0f / len;
		axis->x = q.x * len;
		axis->y = q.y * len;
		axis->z = q.z * len;
	}
	else
	{
		// degenerate case, unit quaternion
		*angle = 0;
		*axis  = kVec3(0, 0, 1);
	}
}

kMat4 kQuatToMat4(kQuat q)
{
	float i  = q.x;
	float j  = q.y;
	float k  = q.z;
	float r  = q.w;

	float ii = i * i;
	float jj = j * j;
	float kk = k * k;

	float ij = i * j;
	float jk = j * k;
	float kr = k * r;
	float jr = j * r;
	float ir = i * r;
	float ik = i * k;

	kMat4 m;

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

kVec3 kQuatToEulerAngles(kQuat q)
{
	kVec3 angles;

	float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
	angles.z        = kArcTan2(sinr_cosp, cosr_cosp);

	float sinp      = 2.0f * (q.w * q.y - q.z * q.x);
	if (kAbsolute(sinp) >= 1.0f)
	{
		// use 90 degrees if out of range
		angles.x = kCopySign(K_PI / 2, sinp);
	}
	else
	{
		angles.x = kArcSin(sinp);
	}

	float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	angles.y        = kArcTan2(siny_cosp, cosy_cosp);

	return angles;
}

kQuat kAngleAxisToQuat(kVec3 axis, float angle)
{
	float r = kCos(angle * 0.5f);
	float s = kSin(angle * 0.5f);
	float i = s * axis.x;
	float j = s * axis.y;
	float k = s * axis.z;
	return kQuat(i, j, k, r);
}

kQuat kAngleAxisNormalizedToQuat(kVec3 axis, float angle) { return kAngleAxisToQuat(kNormalize(axis), angle); }

kQuat kMat4ToQuat(const kMat4 &m)
{
	kQuat q;
	float trace = m.m2[0][0] + m.m2[1][1] + m.m2[2][2];
	if (trace > 0.0f)
	{
		float s = 0.5f / kSquareRoot(trace + 1.0f);
		q.w     = 0.25f / s;
		q.x     = (m.m2[2][1] - m.m2[1][2]) * s;
		q.y     = (m.m2[0][2] - m.m2[2][0]) * s;
		q.z     = (m.m2[1][0] - m.m2[0][1]) * s;
	}
	else
	{
		if (m.m2[0][0] > m.m2[1][1] && m.m2[0][0] > m.m2[2][2])
		{
			float s = 2.0f * kSquareRoot(1.0f + m.m2[0][0] - m.m2[1][1] - m.m2[2][2]);
			q.w     = (m.m2[2][1] - m.m2[1][2]) / s;
			q.x     = 0.25f * s;
			q.y     = (m.m2[0][1] + m.m2[1][0]) / s;
			q.z     = (m.m2[0][2] + m.m2[2][0]) / s;
		}
		else if (m.m2[1][1] > m.m2[2][2])
		{
			float s = 2.0f * kSquareRoot(1.0f + m.m2[1][1] - m.m2[0][0] - m.m2[2][2]);
			q.w     = (m.m2[0][2] - m.m2[2][0]) / s;
			q.x     = (m.m2[0][1] + m.m2[1][0]) / s;
			q.y     = 0.25f * s;
			q.z     = (m.m2[1][2] + m.m2[2][1]) / s;
		}
		else
		{
			float s = 2.0f * kSquareRoot(1.0f + m.m2[2][2] - m.m2[0][0] - m.m2[1][1]);
			q.w     = (m.m2[1][0] - m.m2[0][1]) / s;
			q.x     = (m.m2[0][2] + m.m2[2][0]) / s;
			q.y     = (m.m2[1][2] + m.m2[2][1]) / s;
			q.z     = 0.25f * s;
		}
	}
	return kNormalize(q);
}

kQuat kMat4NomalizedToQuat(const kMat4 &m)
{
	kMat4 nm;
	nm.rows[0] = kVec4(kNormalize(m.rows[0].xyz), m.rows[0].w);
	nm.rows[1] = kVec4(kNormalize(m.rows[1].xyz), m.rows[1].w);
	nm.rows[2] = kVec4(kNormalize(m.rows[2].xyz), m.rows[2].w);
	nm.rows[3] = kVec4(kNormalize(m.rows[3].xyz), m.rows[3].w);
	return kMat4ToQuat(nm);
}

kQuat kEulerAnglesToQuat(float pitch, float yaw, float roll)
{
	float cy = kCos(roll * 0.5f);
	float sy = kSin(roll * 0.5f);
	float cp = kCos(yaw * 0.5f);
	float sp = kSin(yaw * 0.5f);
	float cr = kCos(pitch * 0.5f);
	float sr = kSin(pitch * 0.5f);

	kQuat q;
	q.w = cy * cp * cr + sy * sp * sr;
	q.x = cy * cp * sr - sy * sp * cr;
	q.y = sy * cp * sr + cy * sp * cr;
	q.z = sy * cp * cr - cy * sp * sr;
	return q;
}

kQuat kEulerAnglesToQuat(kVec3 euler) { return kEulerAnglesToQuat(euler.x, euler.y, euler.z); }

//
//
//

kMat2 kIdentity2x2() { return kMat2(1); }

kMat2 kDiagonal2x2(float x, float y)
{
	kMat2 m;
	m.rows[0] = kVec2(x, 0.0f);
	m.rows[1] = kVec2(0.0f, y);
	return m;
}

kMat2 kDiagonal2x2(kVec2 s)
{
	kMat2 m;
	m.rows[0] = kVec2(s.x, 0.0f);
	m.rows[1] = kVec2(0.0f, s.y);
	return m;
}

kMat3 kIdentity3x3() { return kMat3(1); }

kMat2 kRotation2x2(kVec2 arm)
{
	float c = arm.x;
	float s = arm.y;

	kMat2 mat;
	mat.rows[0] = kVec2(c, -s);
	mat.rows[1] = kVec2(s, c);
	return mat;
}

kMat2 kRotation2x2(float angle)
{
	float c = kCos(angle);
	float s = kSin(angle);

	kMat2 mat;
	mat.rows[0] = kVec2(c, -s);
	mat.rows[1] = kVec2(s, c);
	return mat;
}

kMat3 kDiagonal3x3(float S_1, float S_2, float S_3)
{
	kMat3 m;
	m.rows[0] = kVec3(S_1, 0.0f, 0.0f);
	m.rows[1] = kVec3(0.0f, S_2, 0.0f);
	m.rows[2] = kVec3(0.0f, 0.0f, S_3);
	return m;
}

kMat3 kDiagonal3x3(kVec3 s) { return kDiagonal3x3(s.x, s.y, s.z); }

kMat3 kScale3x3(float x, float y) { return kDiagonal3x3(x, y, 1.0f); }

kMat3 kScale3x3(kVec2 s) { return kDiagonal3x3(s.x, s.y, 1.0f); }

kMat3 kTranslation3x3(float T_x, float T_y)
{
	kMat3 m;
	m.rows[0] = kVec3(1.0f, 0.0f, T_x);
	m.rows[1] = kVec3(0.0f, 1.0f, T_y);
	m.rows[2] = kVec3(0.0f, 0.0f, 1.0f);
	return m;
}

kMat3 kTranslation3x3(kVec2 t) { return kTranslation3x3(t.x, t.y); }

kMat3 kRotation3x3(kVec2 arm)
{
	kMat3 m;
	float c   = arm.x;
	float s   = arm.y;

	m.rows[0] = kVec3(c, -s, 0.0f);
	m.rows[1] = kVec3(s, c, 0.0f);
	m.rows[2] = kVec3(0.0f, 0.0f, 1.0f);
	return m;
}

kMat3 kRotation3x3(float angle)
{
	kMat3 m;
	float c   = kCos(angle);
	float s   = kSin(angle);

	m.rows[0] = kVec3(c, -s, 0.0f);
	m.rows[1] = kVec3(s, c, 0.0f);
	m.rows[2] = kVec3(0.0f, 0.0f, 1.0f);
	return m;
}

kMat3 kLookAt3x3(kVec2 from, kVec2 to, kVec2 forward)
{
	kVec2 dir       = kNormalize(to - from);
	float cos_theta = kDotProduct(dir, forward);
	float sin_theta = kSquareRoot(1.0f - cos_theta * cos_theta);

	kMat3 m;
	m.rows[0] = kVec3(cos_theta, -sin_theta, from.x);
	m.rows[1] = kVec3(sin_theta, cos_theta, from.y);
	m.rows[2] = kVec3(0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kIdentity() { return kMat4(1); }

kMat4 kDiagonal(float x, float y, float z, float w)
{
	kMat4 m;
	m.rows[0] = kVec4(x, 0.0f, 0.0f, 0.0f);
	m.rows[1] = kVec4(0.0f, y, 0.0f, 0.0f);
	m.rows[2] = kVec4(0.0f, 0.0f, z, 0.0f);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, w);
	return m;
}

kMat4 kScale(float S_1, float S_2, float S_3) { return kDiagonal(S_1, S_2, S_3, 1.0f); }

kMat4 kScale(kVec3 s) { return kDiagonal(s.x, s.y, s.z, 1.0f); }

kMat4 kTranslation(float T_x, float T_y, float T_z)
{
	kMat4 m;
	m.rows[0] = kVec4(1.0f, 0.0f, 0.0f, T_x);
	m.rows[1] = kVec4(0.0f, 1.0f, 0.0f, T_y);
	m.rows[2] = kVec4(0.0f, 0.0f, 1.0f, T_z);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kTranslation(kVec3 t) { return kTranslation(t.x, t.y, t.z); }

kMat4 kRotationX(float c, float s)
{
	kMat4 m;
	m.rows[0] = kVec4(1.0f, 0.0f, 0.0f, 0.0f);
	m.rows[1] = kVec4(0.0f, c, -s, 0.0f);
	m.rows[2] = kVec4(0.0f, s, c, 0.0f);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kRotationX(float angle)
{
	kVec2 arm = kArm(angle);
	return kRotationX(arm.x, arm.y);
}

kMat4 kRotationY(float c, float s)
{
	kMat4 m;
	m.rows[0] = kVec4(c, 0.0f, s, 0.0f);
	m.rows[1] = kVec4(0.0f, 1.0f, 0.0f, 0.0f);
	m.rows[2] = kVec4(-s, 0.0f, c, 0.0f);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kRotationY(float angle)
{
	kVec2 arm = kArm(angle);
	return kRotationY(arm.x, arm.y);
}

kMat4 kRotationZ(float c, float s)
{
	kMat4 m;
	m.rows[0] = kVec4(c, -s, 0.0f, 0.0f);
	m.rows[1] = kVec4(s, c, 0.0f, 0.0f);
	m.rows[2] = kVec4(0.0f, 0.0f, 1.0f, 0.0f);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kRotationZ(float angle)
{
	kVec2 arm = kArm(angle);
	return kRotationZ(arm.x, arm.y);
}

kMat4 kRotation(float x, float y, float z, kVec2 arm)
{
	float c  = arm.x;
	float s  = arm.y;

	float x2 = x * x;
	float xy = x * y;
	float zx = x * z;
	float y2 = y * y;
	float yz = y * z;
	float z2 = z * z;

	kMat4 m;
	m.rows[0] = kVec4(c + x2 * (1 - c), xy * (1 - c) - z * s, zx * (1 - c) + y * s, 0.0f);
	m.rows[1] = kVec4(xy * (1 - c) + z * s, c + y2 * (1 - c), yz * (1 - c) - x * s, 0.0f);
	m.rows[2] = kVec4(zx * (1 - c) - y * s, yz * (1 - c) + x * s, c + z2 * (1 - c), 0.0f);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kRotation(float x, float y, float z, float angle) { return kRotation(x, y, z, kArm(angle)); }

kMat4 kRotation(kVec3 axis, kVec2 arm) { return kRotation(axis.x, axis.y, axis.z, arm); }

kMat4 kRotation(kVec3 axis, float angle) { return kRotation(axis.x, axis.y, axis.z, angle); }

kMat4 kLookAt(kVec3 from, kVec3 to, kVec3 world_up)
{
	kVec3 forward = kNormalize(from - to);
	kVec3 Right   = kNormalize(kCrossProduct(world_up, forward));
	kVec3 up      = kCrossProduct(Right, forward);

	kMat4 lookat;
	lookat.rows[0] = kVec4(Right.x, up.x, forward.x, -from.x);
	lookat.rows[1] = kVec4(Right.y, up.y, forward.y, -from.y);
	lookat.rows[2] = kVec4(Right.z, up.z, forward.z, -from.z);
	lookat.rows[3] = kVec4(0, 0, 0, 1);

	return lookat;
}

kMat4 kLookAtDirection(kVec3 dir, kVec3 world_up)
{
	kAssert(!kIsNull(dir));

	kVec3 forward = dir;
	kVec3 Right   = kNormalize(kCrossProduct(world_up, forward));
	kVec3 up      = kCrossProduct(Right, forward);

	kMat4 lookat;
	lookat.rows[0] = kVec4(Right.x, up.x, forward.x, 0);
	lookat.rows[1] = kVec4(Right.y, up.y, forward.y, 0);
	lookat.rows[2] = kVec4(Right.z, up.z, forward.z, 0);
	lookat.rows[3] = kVec4(0, 0, 0, 1);

	return lookat;
}

kMat4 kOrthographicRH(float l, float r, float t, float b, float n, float f)
{
	float iwidth  = 1 / (r - l);
	float iheight = 1 / (t - b);
	float range   = 1 / (n - f);

	kMat4 m;
	m.rows[0] = kVec4(2 * iwidth, 0.0f, 0.0f, -(l + r) * iwidth);
	m.rows[1] = kVec4(0.0f, 2 * iheight, 0.0f, -(t + b) * iheight);
	m.rows[2] = kVec4(0.0f, 0.0f, range, -n * range);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kOrthographicLH(float l, float r, float t, float b, float n, float f)
{
	float iwidth  = 1 / (r - l);
	float iheight = 1 / (t - b);
	float range   = 1 / (f - n);

	kMat4 m;
	m.rows[0] = kVec4(2 * iwidth, 0.0f, 0.0f, -(l + r) * iwidth);
	m.rows[1] = kVec4(0.0f, 2 * iheight, 0.0f, -(t + b) * iheight);
	m.rows[2] = kVec4(0.0f, 0.0f, range, -n * range);
	m.rows[3] = kVec4(0.0f, 0.0f, 0.0f, 1.0f);
	return m;
}

kMat4 kPerspectiveRH(float fov, float aspect_ratio, float n, float f)
{
	float height = 1.0f / kTan(fov * 0.5f);
	float width  = height / aspect_ratio;
	float range  = 1 / (n - f);

	kMat4 m;
	m.rows[0] = kVec4(width, 0.0f, 0.0f, 0.0f);
	m.rows[1] = kVec4(0.0f, height, 0.0f, 0.0f);
	m.rows[2] = kVec4(0.0f, 0.0f, f * range, -1.0f * f * n * range);
	m.rows[3] = kVec4(0.0f, 0.0f, -1.0f, 0.0f);
	return m;
}

kMat4 kPerspectiveLH(float fov, float aspect_ratio, float n, float f)
{
	float height = 1.0f / kTan(fov * 0.5f);
	float width  = height / aspect_ratio;
	float range  = 1 / (f - n);

	kMat4 m;
	m.rows[0] = kVec4(width, 0.0f, 0.0f, 0.0f);
	m.rows[1] = kVec4(0.0f, height, 0.0f, 0.0f);
	m.rows[2] = kVec4(0.0f, 0.0f, f * range, -1.0f * f * n * range);
	m.rows[3] = kVec4(0.0f, 0.0f, 1.0f, 0.0f);
	return m;
}

kQuat kQuatIdentity() { return kQuat(0, 0, 0, 1); }

kQuat kQuatBetween(kVec3 from, kVec3 to)
{
	kQuat q;
	float w = 1.0f + kDotProduct(from, to);

	if (w)
	{
		q = kQuat(kVec4(kCrossProduct(from, to), w));
	}
	else
	{
		kVec3 xyz = kAbsolute(from.x) > kAbsolute(from.z) ? kVec3(-from.y, from.x, 0.f) : kVec3(0.f, -from.z, from.y);
		q         = kQuat(kVec3Arg(xyz), w);
	}

	return kNormalize(q);
}

kQuat kQuatBetween(kQuat a, kQuat b)
{
	kQuat t = kConjugate(a);
	t       = t * (1.0f / kDotProduct(t, t));
	return t * b;
}

kQuat kQuatLookAt(kVec3 from, kVec3 to, kVec3 world_forward)
{
	kVec3 dir = to - from;
	return kQuatBetween(world_forward, dir);
}

//
//
//

kVec2 kSlerp(kVec2 from, kVec2 to, float t) { return kSlerp(from, to, kAngleBetween(from, to), t); }

kVec3 kSlerp(kVec3 from, kVec3 to, float t) { return kSlerp(from, to, kAngleBetween(from, to), t); }

kQuat kSlerp(kQuat from, kQuat to, float t)
{
	float dot = kClamp(-1.0f, 1.0f, kDotProduct(from, to));

	// use shorter path
	if (dot < 0.0f)
	{
		to  = -to;
		dot = -dot;
	}

	if (dot > 0.9999f)
	{
		kQuat result = kLerp(from, to, t);
		return kNormalize(result);
	}

	float theta_0     = kArcCos(dot);
	float theta       = theta_0 * t;
	float sin_theta   = kSin(theta);
	float sin_theta_0 = kSin(theta_0);

	float s0          = kCos(theta) - dot * sin_theta / sin_theta_0;
	float s1          = sin_theta / sin_theta_0;

	return (s0 * from) + (s1 * to);
}

float kStep(float edge, float val) { return val < edge ? 0.0f : 1.0f; }

kVec2 kStep(kVec2 edge, kVec2 val)
{
	kVec2 res;
	res.x = kStep(edge.x, val.x);
	res.y = kStep(edge.y, val.y);
	return res;
}

kVec3 kStep(kVec3 edge, kVec3 val)
{
	kVec3 res;
	res.x = kStep(edge.x, val.x);
	res.y = kStep(edge.y, val.y);
	res.z = kStep(edge.z, val.z);
	return res;
}

kVec4 kStep(kVec4 edge, kVec4 val)
{
	kVec4 res;
	res.x = kStep(edge.x, val.x);
	res.y = kStep(edge.y, val.y);
	res.z = kStep(edge.z, val.z);
	res.w = kStep(edge.w, val.w);
	return res;
}

kQuat kStep(kQuat edge, kQuat val)
{
	kQuat res;
	res.x = kStep(edge.x, val.x);
	res.y = kStep(edge.y, val.y);
	res.z = kStep(edge.z, val.z);
	res.w = kStep(edge.w, val.w);
	return res;
}

float kInverseSmoothStep(float x) { return 0.5f - kSin(kArcSin(1.0f - 2.0f * x) / 3.0f); }

float kMoveTowards(float from, float to, float factor)
{
	if (factor)
	{
		float direction = to - from;
		float distance  = kAbsolute(direction);

		if (distance < factor)
		{
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec2 kMoveTowards(kVec2 from, kVec2 to, float factor)
{
	if (factor)
	{
		kVec2 direction = to - from;
		float distance  = kLength(direction);

		if (distance < factor)
		{
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec3 kMoveTowards(kVec3 from, kVec3 to, float factor)
{
	if (factor)
	{
		kVec3 direction = to - from;
		float distance  = kLength(direction);

		if (distance < factor)
		{
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec4 kMoveTowards(kVec4 from, kVec4 to, float factor)
{
	if (factor)
	{
		kVec4 direction = to - from;
		float distance  = kLength(direction);

		if (distance < factor)
		{
			return to;
		}

		float t = factor / distance;

		return kLerp(from, to, t);
	}

	return from;
}

kVec2 kRotateAround(kVec2 point, kVec2 center, float angle)
{
	float c = kCos(angle);
	float s = kSin(angle);
	kVec2 res;
	kVec2 p = point - center;
	res.x   = p.x * c - p.y * s;
	res.y   = p.x * s + p.y * c;
	res += center;
	return res;
}

kQuat kRotateTowards(kQuat from, kQuat to, float max_angle)
{
	if (max_angle)
	{
		float dot = kClamp(-1.0f, 1.0f, kDotProduct(from, to));

		// use shorter path
		if (dot < 0.0f)
		{
			to  = -to;
			dot = -dot;
		}

		float theta_0 = kArcCos(dot);

		if (theta_0 < max_angle)
		{
			return to;
		}

		float t           = max_angle / theta_0;

		theta_0           = max_angle;
		float theta       = theta_0 * t;
		float sin_theta   = kSin(theta);
		float sin_theta_0 = kSin(theta_0);

		float s0          = kCos(theta) - dot * sin_theta / sin_theta_0;
		float s1          = sin_theta / sin_theta_0;

		return (s0 * from) + (s1 * to);
	}
	else
	{
		return from;
	}
}

kVec2 kReflect(kVec2 d, kVec2 n)
{
	float c = kDotProduct(kNormalizeZ(d), n);
	float s = kSquareRoot(10.f - kSquare(c));
	kVec2 r;
	r.x = d.x * c - d.y * s;
	r.y = d.x * s + d.y * c;
	return r;
}

//
//
//

// https://en.wikipedia.org/wiki/SRGB#Specification_of_the_transformation
kVec3 kLinearToSrgb(kVec3 col)
{
	const kMat3 Transforms{kVec3(0.4142f, 0.3576f, 0.1805f), kVec3(0.2126f, 0.7152f, 0.0722f),
	                       kVec3(0.0193f, 0.1192f, 0.9505f)};

	kVec3       res = Transforms * col;
	return res;
}

kVec4 kLinearToSrgb(kVec4 col)
{
	kVec4 res = kVec4(kLinearToSrgb(col.xyz), col.w);
	return res;
}

kVec3 kLinearToSrgb(kVec3 col, float gamma)
{
	float igamma = 1.0f / gamma;
	kVec3 res;
	res.x = kPow(col.x, igamma);
	res.y = kPow(col.y, igamma);
	res.z = kPow(col.z, igamma);
	return res;
}

kVec4 kLinearToSrgb(kVec4 col, float gamma)
{
	kVec4 res = kVec4(kLinearToSrgb(col.xyz, gamma), col.w);
	return res;
}

kVec3 kSrgbToLinear(kVec3 col)
{
	const kMat3 Transforms{kVec3(+3.2406f, -1.5372f, -0.4986f), kVec3(-0.9689f, +1.8758f, +0.0415f),
	                       kVec3(+0.0557f, -0.2040f, +1.0570f)};
	kVec3       res = Transforms * col;
	return res;
}

kVec4 kSrgbToLinear(kVec4 col)
{
	kVec4 res = kVec4(kSrgbToLinear(col.xyz), col.w);
	return res;
}

kVec3 kSrgbToLinear(kVec3 col, float gamma)
{
	kVec3 res;
	res.x = kPow(col.x, gamma);
	res.y = kPow(col.y, gamma);
	res.z = kPow(col.z, gamma);
	return res;
}

kVec4 kSrgbToLinear(kVec4 col, float gamma)
{
	kVec4 res = kVec4(kSrgbToLinear(col.xyz, gamma), col.w);
	return res;
}

// http://en.wikipedia.org/wiki/HSL_and_HSV
kVec3 kHsvToRgb(kVec3 col)
{
	kVec3 res;

	float h = col.x;
	float s = col.y;
	float v = col.z;

	if (s == 0.0f)
	{
		// gray
		res.x = res.y = res.z = v;
		return res;
	}

	h       = kMod(h, 1.0f) / (60.0f / 360.0f);
	int   i = (int)h;
	float f = h - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (i)
	{
		case 0: res = kVec3(v, t, p); break;
		case 1: res = kVec3(q, v, p); break;
		case 2: res = kVec3(p, v, t); break;
		case 3: res = kVec3(p, q, v); break;
		case 4: res = kVec3(t, p, v); break;
		case 5:
		default: res = kVec3(v, p, q); break;
	}

	return res;
}

// http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
kVec3 kRgbToHsv(kVec3 c)
{
	float r = c.x;
	float g = c.y;
	float b = c.z;

	float k = 0.f;
	if (g < b)
	{
		auto t = b;
		b      = g;
		g      = t;
		k      = -1.f;
	}
	if (r < g)
	{
		auto t = g;
		g      = r;
		r      = t;
		k      = -2.f / 6.f - k;
	}

	kVec3 res;
	float chroma = r - (g < b ? g : b);
	res.x        = kAbsolute(k + (g - b) / (6.f * chroma + 1e-20f));
	res.y        = chroma / (r + 1e-20f);
	res.z        = r;
	return res;
}

kVec4 kHsvToRgb(kVec4 c) { return kVec4(kHsvToRgb(c.xyz), c.w); }

kVec4 kRgbToHsv(kVec4 c) { return kVec4(kRgbToHsv(c.xyz), c.w); }
