#pragma once
#include "kCommon.h"

#include <math.h>

#define kVec2Arg(v)   (v).x, (v).y
#define kVec3Arg(v)   (v).x, (v).y, (v).z
#define kBivec3Arg(v) (v).xy, (v).xz, (v).yz
#define kVec4Arg(v)   (v).x, (v).y, (v).z, (v).w
#define kQuatArg(q)   (q).x, (q).y, (q).z, (q).w

#define kMat2Arg(m)   kVec2Arg((m).rows[0]), kVec2Arg((m).rows[1])
#define kMat3Arg(m)   kVec3Arg((m).rows[0]), kVec3Arg((m).rows[1]), kVec3Arg((m).rows[2])
#define kMat4Arg(m)   kVec4Arg((m).rows[0]), kVec4Arg((m).rows[1]), kVec4Arg((m).rows[2]), kVec4Arg((m).rows[3])

inproc float kSgn(float val)
{
	return (float)((float(0) < val) - (val < float(0)));
}
inproc kVec2 kSgn(kVec2 v)
{
	return kVec2(kSgn(v.x), kSgn(v.y));
}
inproc kVec3 kSgn(kVec3 v)
{
	return kVec3(kSgn(v.x), kSgn(v.y), kSgn(v.z));
}
inproc kVec4 kSgn(kVec4 v)
{
	return kVec4(kSgn(v.x), kSgn(v.y), kSgn(v.z), kSgn(v.w));
}

inproc float kAbsolute(float x)
{
	return fabsf(x);
}
inproc float kSin(float x)
{
	return sinf(kTurnToRad * (x));
}
inproc float kCos(float x)
{
	return cosf(kTurnToRad * (x));
}
inproc float kTan(float x)
{
	return tanf(kTurnToRad * (x));
}
inproc float kArcSin(float x)
{
	return kRadToTurn * (asinf(x));
}
inproc float kArcCos(float x)
{
	return kRadToTurn * (acosf(x));
}
inproc float kArcTan2(float y, float x)
{
	return kRadToTurn * (atan2f(y, x));
}
inproc float kSquareRoot(float x)
{
	return sqrtf(x);
}
inproc float kPow(float x, float y)
{
	return powf(x, y);
}
inproc float kCopySign(float x, float y)
{
	return copysignf(x, y);
}
inproc float kMod(float x, float y)
{
	return fmodf(x, y);
}
inproc float kSquare(float x)
{
	return (x * x);
}
inproc float kFloor(float x)
{
	return floorf(x);
}
inproc float kRound(float x)
{
	return roundf(x);
}
inproc float kCeil(float x)
{
	return ceilf(x);
}

//
//
//

float kWrap(float min, float a, float max);
kVec2 kArm(float angle);
kVec2 kArmInverse(float angle);

bool  kAlmostEqual(float a, float b, float delta = REAL_EPSILON);
bool  kAlmostEqual(kVec2 a, kVec2 b, float delta = REAL_EPSILON);
bool  kAlmostEqual(kVec3 a, kVec3 b, float delta = REAL_EPSILON);
bool  kAlmostEqual(kVec4 a, kVec4 b, float delta = REAL_EPSILON);
bool  kAlmostEqual(kBivec3 a, kBivec3 b, float delta = REAL_EPSILON);

bool  kIsNull(float a);
bool  kIsNull(kVec2 a);
bool  kIsNull(kVec3 a);
bool  kIsNull(kVec4 a);
bool  kIsNull(kBivec3 a);
bool  kIsNull(int32_t a);
bool  kIsNull(kVec2i a);
bool  kIsNull(kVec3i a);
bool  kIsNull(kVec4i a);

template <typename Item>
bool operator==(kVec2T<Item> a, kVec2T<Item> b)
{
	return a.x == b.x && a.y == b.y;
}
template <typename Item>
bool operator!=(kVec2T<Item> a, kVec2T<Item> b)
{
	return a.x != b.x || a.y != b.y;
}
template <typename Item>
bool operator==(kVec3T<Item> a, kVec3T<Item> b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
template <typename Item>
bool operator!=(kVec3T<Item> a, kVec3T<Item> b)
{
	return a.x != b.x || a.y != b.y || a.z != b.z;
}
template <typename Item>
bool operator==(kVec4T<Item> a, kVec4T<Item> b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
template <typename Item>
bool operator!=(kVec4T<Item> a, kVec4T<Item> b)
{
	return a.x != b.x || a.y != b.y && a.z != b.z || a.w != b.w;
}

inproc kVec2 kRound(kVec2 v)
{
	return kVec2(kRound(v.x), kRound(v.y));
}
inproc kVec3 kRound(kVec3 v)
{
	return kVec3(kRound(v.x), kRound(v.y), kRound(v.z));
}
inproc kVec4 kRound(kVec4 v)
{
	return kVec4(kRound(v.x), kRound(v.y), kRound(v.z), kRound(v.w));
}
inproc kBivec3 kRound(kBivec3 v)
{
	return kBivec3(kRound(v.yz), kRound(v.zx), kRound(v.xy));
}

template <typename Item>
kVec2T<Item> kMin(kVec2T<Item> a, kVec2T<Item> b)
{
	return kVec2T<Item>{kMin(a.x, b.x), kMin(a.y, b.y)};
}
template <typename Item>
kVec2T<Item> kMax(kVec2T<Item> a, kVec2T<Item> b)
{
	return kVec2T<Item>{kMax(a.x, b.x), kMax(a.y, b.y)};
}
template <typename Item>
kVec3T<Item> kMin(kVec3T<Item> a, kVec3T<Item> b)
{
	return kVec3T<Item>{kMin(a.x, b.x), kMin(a.y, b.y), kMin(a.z, b.z)};
}
template <typename Item>
kVec3T<Item> kMax(kVec3T<Item> a, kVec3T<Item> b)
{
	return kVec3T<Item>{kMax(a.x, b.x), kMax(a.y, b.y), kMax(a.z, b.z)};
}
template <typename Item>
kVec4T<Item> kMin(kVec4T<Item> a, kVec4T<Item> b)
{
	return kVec4T<Item>{kMin(a.x, b.x), kMin(a.y, b.y), kMin(a.z, b.z), kMin(a.w, b.w)};
}
template <typename Item>
kVec4T<Item> kMax(kVec4T<Item> a, kVec4T<Item> b)
{
	return kVec4T<Item>{kMax(a.x, b.x), kMax(a.y, b.y), kMax(a.z, b.z), kMax(a.w, b.w)};
}
template <typename Item>
kBivec3T<Item> kMin(kBivec3T<Item> a, kBivec3T<Item> b)
{
	return kBivec3T<Item>{kMin(a.yz, b.yz), kMin(a.zx, b.zx), kMin(a.xy, b.xy)};
}
template <typename Item>
kBivec3T<Item> kMax(kBivec3T<Item> a, kBivec3T<Item> b)
{
	return kBivec3T<Item>{kMax(a.yz, b.yz), kMax(a.zx, b.zx), kMax(a.xy, b.xy)};
}

template <typename Item>
bool kIsInRange(kVec2T<Item> a, kVec2T<Item> b, kVec2T<Item> v)
{
	return kIsInRange(a.x, b.x, v.x) && kIsInRange(a.y, b.y, v.y);
}
template <typename Item>
bool kIsInRange(kVec3T<Item> a, kVec3T<Item> b, kVec3T<Item> v)
{
	return kIsInRange(a.x, b.x, v.x) && kIsInRange(a.y, b.y, v.y) && kIsInRange(a.z, b.z, v.z);
}
template <typename Item>
bool kIsInRange(kVec4T<Item> a, kVec4T<Item> b, kVec4T<Item> v)
{
	return kIsInRange(a.x, b.x, v.x) && kIsInRange(a.y, b.y, v.y) && kIsInRange(a.z, b.z, v.z) &&
	       kIsInRange(a.w, b.w, v.w);
}

template <typename Item>
kVec2T<Item> operator+(kVec2T<Item> a, kVec2T<Item> b)
{
	return kVec2T<Item>(a.x + b.x, a.y + b.y);
}
template <typename Item>
kVec3T<Item> operator+(kVec3T<Item> a, kVec3T<Item> b)
{
	return kVec3T<Item>(a.x + b.x, a.y + b.y, a.z + b.z);
}
template <typename Item>
kVec4T<Item> operator+(kVec4T<Item> a, kVec4T<Item> b)
{
	return kVec4T<Item>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
template <typename Item>
kBivec3T<Item> operator+(kBivec3T<Item> a, kBivec3T<Item> b)
{
	return kBivec3T<Item>(a.yz + b.yz, a.zx + b.zx, a.xy + b.xy);
}
template <typename Item>
kVec2T<Item> operator-(kVec2T<Item> a, kVec2T<Item> b)
{
	return kVec2T<Item>(a.x - b.x, a.y - b.y);
}
template <typename Item>
kVec3T<Item> operator-(kVec3T<Item> a, kVec3T<Item> b)
{
	return kVec3T<Item>(a.x - b.x, a.y - b.y, a.z - b.z);
}
template <typename Item>
kVec4T<Item> operator-(kVec4T<Item> a, kVec4T<Item> b)
{
	return kVec4T<Item>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
template <typename Item>
kBivec3T<Item> operator-(kBivec3T<Item> a, kBivec3T<Item> b)
{
	return kBivec3T<Item>(a.yz - b.yz, a.zx - b.zx, a.xy - b.xy);
}
template <typename Item>
kVec2T<Item> operator*(Item s, kVec2T<Item> v)
{
	return kVec2T<Item>(s * v.x, s * v.y);
}
template <typename Item>
kVec2T<Item> operator*(kVec2T<Item> v, Item s)
{
	return s * v;
}
template <typename Item>
kVec3T<Item> operator*(Item s, kVec3T<Item> v)
{
	return kVec3T<Item>(s * v.x, s * v.y, s * v.z);
}
template <typename Item>
kVec3T<Item> operator*(kVec3T<Item> v, Item s)
{
	return s * v;
}
template <typename Item>
kVec4T<Item> operator*(Item s, kVec4T<Item> v)
{
	return kVec4T<Item>(s * v.x, s * v.y, s * v.z, s * v.w);
}
template <typename Item>
kVec4T<Item> operator*(kVec4T<Item> v, Item s)
{
	return s * v;
}
template <typename Item>
kBivec3T<Item> operator*(Item s, kBivec3T<Item> v)
{
	return kBivec3T<Item>(s * v.yz, s * v.zx, s * v.xy);
}
template <typename Item>
kBivec3T<Item> operator*(kBivec3T<Item> v, Item s)
{
	return s * v;
}
template <typename Item>
kVec2T<Item> operator/(kVec2T<Item> v, Item s)
{
	return kVec2T<Item>(v.x / s, v.y / s);
}
template <typename Item>
kVec3T<Item> operator/(kVec3T<Item> v, Item s)
{
	return kVec3T<Item>(v.x / s, v.y / s, v.z / s);
}
template <typename Item>
kVec4T<Item> operator/(kVec4T<Item> v, Item s)
{
	return kVec4T<Item>(v.x / s, v.y / s, v.z / s, v.w / s);
}
template <typename Item>
kBivec3T<Item> operator/(kBivec3T<Item> v, Item s)
{
	return kBivec3T<Item>(v.yz / s, v.zx / s, v.xy / s);
}
template <typename Item>
kVec2T<Item> operator*(kVec2T<Item> l, kVec2T<Item> r)
{
	return kVec2T<Item>(l.x * r.x, l.y * r.y);
}
template <typename Item>
kVec3T<Item> operator*(kVec3T<Item> l, kVec3T<Item> r)
{
	return kVec3T<Item>(l.x * r.x, l.y * r.y, l.z * r.z);
}
template <typename Item>
kVec4T<Item> operator*(kVec4T<Item> l, kVec4T<Item> r)
{
	return kVec4T<Item>(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w);
}
template <typename Item>
kBivec3T<Item> operator*(kBivec3T<Item> l, kBivec3T<Item> r)
{
	return kVec3T<Item>(l.yz * r.yz, l.zx * r.zx, l.xy * r.xy);
}
template <typename Item>
kVec2T<Item> operator/(kVec2T<Item> l, kVec2T<Item> r)
{
	return kVec2T<Item>(l.x / r.x, l.y / r.y);
}
template <typename Item>
kVec3T<Item> operator/(kVec3T<Item> l, kVec3T<Item> r)
{
	return kVec3T<Item>(l.x / r.x, l.y / r.y, l.z / r.z);
}
template <typename Item>
kVec4T<Item> operator/(kVec4T<Item> l, kVec4T<Item> r)
{
	return kVec4T<Item>(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
}
template <typename Item>
kVec2T<Item> operator-(const kVec2T<Item> &v)
{
	return kVec2T<Item>(-v.x, -v.y);
}
template <typename Item>
kVec3T<Item> operator-(const kVec3T<Item> &v)
{
	return kVec3T<Item>(-v.x, -v.y, -v.z);
}
template <typename Item>
kVec4T<Item> operator-(const kVec4T<Item> &v)
{
	return kVec4T<Item>(-v.x, -v.y, -v.z, -v.w);
}
template <typename Item>
kBivec3T<Item> operator-(const kBivec3T<Item> &v)
{
	return kBivec3T<Item>(-v.yz, -v.zx, -v.xy);
}

template <typename Item>
kVec2T<Item> &operator+=(kVec2T<Item> &a, kVec2T<Item> b)
{
	a = a + b;
	return a;
}
template <typename Item>
kVec3T<Item> &operator+=(kVec3T<Item> &a, kVec3T<Item> b)
{
	a = a + b;
	return a;
}
template <typename Item>
kVec4T<Item> &operator+=(kVec4T<Item> &a, kVec4T<Item> b)
{
	a = a + b;
	return a;
}
template <typename Item>
kBivec3T<Item> &operator+=(kBivec3T<Item> &a, kBivec3T<Item> b)
{
	a = a + b;
	return a;
}
template <typename Item>
kVec2T<Item> &operator-=(kVec2T<Item> &a, kVec2T<Item> b)
{
	a = a - b;
	return a;
}
template <typename Item>
kVec3T<Item> &operator-=(kVec3T<Item> &a, kVec3T<Item> b)
{
	a = a - b;
	return a;
}
template <typename Item>
kVec4T<Item> &operator-=(kVec4T<Item> &a, kVec4T<Item> b)
{
	a = a - b;
	return a;
}
template <typename Item>
kBivec3T<Item> &operator-=(kBivec3T<Item> &a, kBivec3T<Item> b)
{
	a = a - b;
	return a;
}
template <typename Item>
kVec2T<Item> &operator*=(kVec2T<Item> &t, Item s)
{
	t = t * s;
	return t;
}
template <typename Item>
kVec3T<Item> &operator*=(kVec3T<Item> &t, Item s)
{
	t = t * s;
	return t;
}
template <typename Item>
kVec4T<Item> &operator*=(kVec4T<Item> &t, Item s)
{
	t = t * s;
	return t;
}
template <typename Item>
kBivec3T<Item> &operator*=(kBivec3T<Item> &t, Item s)
{
	t = t * s;
	return t;
}
template <typename Item>
kVec2T<Item> &operator/=(kVec2T<Item> &t, Item s)
{
	t = t / s;
	return t;
}
template <typename Item>
kVec3T<Item> &operator/=(kVec3T<Item> &t, Item s)
{
	t = t / s;
	return t;
}
template <typename Item>
kVec4T<Item> &operator/=(kVec4T<Item> &t, Item s)
{
	t = t / s;
	return t;
}
template <typename Item>
kBivec3T<Item> &operator/=(kBivec3T<Item> &t, Item s)
{
	t = t / s;
	return t;
}
template <typename Item>
kVec2T<Item> &operator*=(kVec2T<Item> &t, kVec2T<Item> s)
{
	t = t * s;
	return t;
}
template <typename Item>
kVec3T<Item> &operator*=(kVec3T<Item> &t, kVec3T<Item> s)
{
	t = t * s;
	return t;
}
template <typename Item>
kVec4T<Item> &operator*=(kVec4T<Item> &t, kVec4T<Item> s)
{
	t = t * s;
	return t;
}
template <typename Item>
kBivec3T<Item> &operator*=(kBivec3T<Item> &t, kBivec3T<Item> s)
{
	t = t * s;
	return t;
}
template <typename Item>
kVec2T<Item> &operator/=(kVec2T<Item> &t, kVec2T<Item> s)
{
	t = t / s;
	return t;
}
template <typename Item>
kVec3T<Item> &operator/=(kVec3T<Item> &t, kVec3T<Item> s)
{
	t = t / s;
	return t;
}
template <typename Item>
kVec4T<Item> &operator/=(kVec4T<Item> &t, kVec4T<Item> s)
{
	t = t / s;
	return t;
}

template <typename Item>
kVec2T<Item> kComplexProduct(kVec2T<Item> a, kVec2T<Item> b)
{
	return kVec2T<Item>(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}
template <typename Item>
kVec2T<Item> kComplexConjugate(kVec2T<Item> a)
{
	return kVec2T<Item>(a.x, -a.y);
}

template <typename Item>
Item kDotProduct(kVec2T<Item> a, kVec2T<Item> b)
{
	return a.x * b.x + a.y * b.y;
}
template <typename Item>
Item kDotProduct(kVec3T<Item> a, kVec3T<Item> b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
template <typename Item>
Item kDotProduct(kVec4T<Item> a, kVec4T<Item> b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
template <typename Item>
Item kDotProduct(kBivec3T<Item> a, kBivec3T<Item> b)
{
	return a.yz * b.yz + a.zx * b.zx + a.xy * b.xy;
}
template <typename Item>
Item kDeterminant(kVec2T<Item> a, kVec2T<Item> b)
{
	return (a.x * b.y) - (a.y * b.x);
}

template <typename Item>
float kCrossProduct(kVec2T<Item> a, kVec2T<Item> b)
{
	Item z = (a.x * b.y) - (a.y * b.x);
	return z;
}

template <typename Item>
float kWedgeProduct(kVec2T<Item> a, kVec2T<Item> b)
{
	Item z = (a.x * b.y) - (a.y * b.x);
	return z;
}

template <typename Item>
kVec3T<Item> kCrossProduct(kVec3T<Item> a, kVec3T<Item> b)
{
	kVec3T<Item> res;
	res.x = (a.y * b.z) - (a.z * b.y);
	res.y = (a.z * b.x) - (a.x * b.z);
	res.z = (a.x * b.y) - (a.y * b.x);
	return res;
}

template <typename Item>
kBivec3T<Item> kWedgeProduct(kVec3T<Item> a, kVec3T<Item> b)
{
	kBivec3T<Item> res;
	res.yz = (a.y * b.z) - (a.z * b.y);
	res.zx = (a.z * b.x) - (a.x * b.z);
	res.xy = (a.x * b.y) - (a.y * b.x);
	return res;
}

template <typename Item>
kVec2T<Item> kTripleProduct(kVec2T<Item> a, kVec2T<Item> b, kVec2T<Item> c)
{
	Item         det = kDeterminant(a, b);
	kVec2T<Item> res;
	res.x = -c.y * det;
	res.y = c.x * det;
	return res;
}

template <typename Item>
kVec3T<Item> kTripleProduct(kVec3T<Item> a, kVec3T<Item> b, kVec3T<Item> c)
{
	return kCrossProduct(kCrossProduct(a, b), c);
}

template <typename Item>
kVec3T<Item> kOrthoNormalBasisRH(kVec3T<Item> *a, kVec3T<Item> *b)
{
	*a             = kNormalizeZ(*a);
	kVec3T<Item> c = kCrossProduct(*a, *b);
	if (kLengthSq(c) == 0.0f)
		return;
	c  = kNormalizeZ(c);
	*b = kCrossProduct(c, *a);
	return c;
}

template <typename Item>
kVec3T<Item> kOrthoNormalBasisLH(kVec3T<Item> *a, kVec3T<Item> *b)
{
	*a             = kNormalizeZ(*a);
	kVec3T<Item> c = kCrossProduct(*b, *a);
	if (kLengthSq(c) == 0.0f)
		return;
	c  = kNormalizeZ(c);
	*b = kCrossProduct(*a, c);
	return c;
}

template <typename Item>
Item kLengthSq(kVec2T<Item> v)
{
	return kDotProduct(v, v);
}
template <typename Item>
Item kLengthSq(kVec3T<Item> v)
{
	return kDotProduct(v, v);
}
template <typename Item>
Item kLengthSq(kVec4T<Item> v)
{
	return kDotProduct(v, v);
}
template <typename Item>
Item kLength(kVec2T<Item> v)
{
	return kSquareRoot(kDotProduct(v, v));
}
template <typename Item>
Item kLength(kVec3T<Item> v)
{
	return kSquareRoot(kDotProduct(v, v));
}
template <typename Item>
Item kLength(kVec4T<Item> v)
{
	return kSquareRoot(kDotProduct(v, v));
}
template <typename Item>
Item kLength(kBivec3T<Item> v)
{
	return kSquareRoot(kDotProduct(v, v));
}
template <typename Item>
Item kDistance(Item a, Item b)
{
	return b - a;
}
template <typename Item>
Item kDistance(kVec2T<Item> a, kVec2T<Item> b)
{
	return kLength(b - a);
}
template <typename Item>
Item kDistance(kVec3T<Item> a, kVec3T<Item> b)
{
	return kLength(b - a);
}
template <typename Item>
Item kDistance(kVec4T<Item> a, kVec4T<Item> b)
{
	return kLength(b - a);
}

kVec2    kNormalizeZ(kVec2 v);
kVec3    kNormalizeZ(kVec3 v);
kVec4    kNormalizeZ(kVec4 v);
kVec2    kNormalize(kVec2 v);
kVec3    kNormalize(kVec3 v);
kVec4    kNormalize(kVec4 v);
kBivec3  kNormalize(kBivec3 v);
kVec2    kPerpendicularVector(kVec2 a, kVec2 b);

float    kAngleBetween(kVec2 a, kVec2 b);
float    kAngleBetween(kVec3 a, kVec3 b);
float    kAngleBetweenNormalized(kVec2 a, kVec2 b);
float    kAngleBetweenNormalized(kVec3 a, kVec3 b);

float    kSignedAngleBetween(kVec2 a, kVec2 b);
float    kSignedAngleBetween(kVec3 a, kVec3 b, kVec3 n);
float    kSignedAngleBetweenNormalized(kVec2 a, kVec2 b);
float    kSignedAngleBetweenNormalized(kVec3 a, kVec3 b, kVec3 n);

float    kDeterminant(const kMat2 &mat);
kMat2    kInverse(const kMat2 &mat);
kMat2    kTranspose(const kMat2 &m);
float    kDeterminant(const kMat3 &mat);
kMat3    kInverse(const kMat3 &mat);
kMat3    kTranspose(const kMat3 &m);
float    kDeterminant(const kMat4 &mat);
kMat4    kInverse(const kMat4 &mat);
kMat4    kTranspose(const kMat4 &m);

kMat2    operator-(const kMat2 &mat);
kMat3    operator-(const kMat3 &mat);
kMat4    operator-(const kMat4 &mat);
kMat2    operator+(const kMat2 &Left, const kMat2 &Right);
kMat2    operator-(const kMat2 &Left, const kMat2 &Right);
kMat3    operator+(const kMat3 &Left, const kMat3 &Right);
kMat3    operator-(const kMat3 &Left, const kMat3 &Right);
kMat4    operator+(const kMat4 &Left, const kMat4 &Right);
kMat4    operator-(const kMat4 &Left, const kMat4 &Right);

kMat2   &operator-=(kMat2 &mat, const kMat2 &other);
kMat3   &operator-=(kMat3 &mat, const kMat3 &other);
kMat4   &operator-=(kMat4 &mat, const kMat4 &other);
kMat2   &operator+=(kMat2 &mat, const kMat2 &other);
kMat3   &operator+=(kMat3 &mat, const kMat3 &other);
kMat4   &operator+=(kMat4 &mat, const kMat4 &other);

kMat2    operator*(const kMat2 &Left, const kMat2 &Right);
kVec2    operator*(const kMat2 &mat, kVec2 vec);
kVec2    operator*(kVec2 vec, const kMat2 &mat);
kMat3    operator*(const kMat3 &Left, const kMat3 &Right);
kVec3    operator*(const kMat3 &mat, kVec3 vec);
kMat4    operator*(const kMat4 &Left, const kMat4 &Right);
kVec4    operator*(const kMat4 &mat, kVec4 vec);
kMat2   &operator*=(kMat2 &t, kMat2 &o);
kMat3   &operator*=(kMat3 &t, kMat3 &o);
kMat4   &operator*=(kMat4 &t, kMat4 &o);

kRotor3  operator-(kRotor3 q);
kRotor3  operator-(kRotor3 r1, kRotor3 r2);
kRotor3  operator+(kRotor3 r1, kRotor3 r2);
kRotor3  operator*(kRotor3 q, float s);
kRotor3  operator*(float s, kRotor3 q);

kRotor3 &operator-=(kRotor3 &q, kRotor3 other);
kRotor3 &operator+=(kRotor3 &q, kRotor3 other);

float    kDotProduct(kRotor3 q1, kRotor3 q2);
float    kLength(kRotor3 q);
kRotor3  kNormalize(kRotor3 q);
kRotor3  kReverse(kRotor3 q);
kRotor3  operator*(kRotor3 q1, kRotor3 q2);
kVec3    operator*(kRotor3 q, kVec3 v);
kVec3    kRotate(kRotor3 q, kVec3 v);

//
//
//

kVec3 kRightDirection(const kMat4 &m);
kVec3 kUpDirection(const kMat4 &m);
kVec3 kForwardDirection(const kMat4 &m);

kVec3 kRightDirection(kRotor3 q);
kVec3 kUpDirection(kRotor3 q);
kVec3 kForwardDirection(kRotor3 q);

//
//
//

kMat2   kMat3ToMat2(const kMat3 &mat);
kMat3   kMat4ToMat3(const kMat2 &mat);
kMat3   kMat4ToMat3(const kMat4 &mat);
kMat4   kMat3ToMat4(const kMat3 &mat);

void    kRotor3ToAngleAxis(kRotor3 q, float *angle, kVec3 *axis);
kMat4   kRotor3ToMat4(kRotor3 q);
kVec3   kRotor3ToEulerAngles(kRotor3 q);

kRotor3 kQuaternionToRotor3(float x, float y, float z, float w);
kRotor3 kQuaternionToRotor3(kVec4 vec);
kRotor3 kPlaneToRotor3(kBivec3 plane, float angle);
kRotor3 kAngleAxisToRotor3(kVec3 axis, float angle);
kRotor3 kPlaneNormalizedToRotor3(kBivec3 plane, float angle);
kRotor3 kAngleAxisNormalizedToRotor3(kVec3 axis, float angle);
kRotor3 kMat4ToRotor3(const kMat4 &m);
kRotor3 kMat4NomalizedToRotor3(const kMat4 &m);
kRotor3 kEulerAnglesToRotor3(float pitch, float yaw, float roll);
kRotor3 kEulerAnglesToRotor3(kVec3 euler);

//
//
//

kMat2   kIdentity2x2();
kMat2   kDiagonal2x2(float x, float y);
kMat2   kDiagonal2x2(kVec2 s);
kMat2   kRotation2x2(kVec2 arm);
kMat2   kRotation2x2(float angle);

kMat3   kIdentity3x3();
kMat3   kDiagonal3x3(float S_1, float S_2, float S_3);
kMat3   kDiagonal3x3(kVec3 s);
kMat3   kScale3x3(float x, float y);
kMat3   kScale3x3(kVec2 s);
kMat3   kTranslation3x3(float T_x, float T_y);
kMat3   kTranslation3x3(kVec2 t);
kMat3   kRotation3x3(kVec2 arm);
kMat3   kRotation3x3(float angle);
kMat3   kLookAt3x3(kVec2 from, kVec2 to, kVec2 forward);

kMat4   kIdentity();
kMat4   kDiagonal(float x, float y, float z, float w);

kMat4   kScale(float S_1, float S_2, float S_3);
kMat4   kScale(kVec3 s);
kMat4   kTranslation(float T_x, float T_y, float T_z);
kMat4   kTranslation(kVec3 t);
kMat4   kRotationX(float c, float s);
kMat4   kRotationX(float angle);
kMat4   kRotationY(float c, float s);
kMat4   kRotationY(float angle);
kMat4   kRotationZ(float c, float s);
kMat4   kRotationZ(float angle);
kMat4   kRotation(float x, float y, float z, kVec2 arm);
kMat4   kRotation(float x, float y, float z, float angle);
kMat4   kRotation(kVec3 axis, kVec2 arm);
kMat4   kRotation(kVec3 axis, float angle);
kMat4   kLookAt(kVec3 from, kVec3 to, kVec3 world_up);
kMat4   kLookAtDirection(kVec3 dir, kVec3 world_up);
kMat4   kOrthographicRH(float l, float r, float t, float b, float n, float f);
kMat4   kOrthographicLH(float l, float r, float t, float b, float n, float f);
kMat4   kPerspectiveRH(float fov, float aspect_ratio, float n, float f);
kMat4   kPerspectiveLH(float fov, float aspect_ratio, float n, float f);

kRotor3 kRotor3Between(kVec3 from, kVec3 to);
kRotor3 kRotor3Between(kRotor3 a, kRotor3 b);
kRotor3 kRotor3LookAt(kVec3 from, kVec3 to, kVec3 world_forward);

//
//
//

float kEaseInSine(float x);
float kEaseOutSine(float x);
float kEaseInOutSine(float x);
float kEaseInQuad(float x);
float kEaseOutQuad(float x);
float kEaseInOutQuad(float x);
float kEaseInCubic(float x);
float kEaseOutCubic(float x);
float kEaseInOutCubic(float x);
float kEaseInQuart(float x);
float kEaseOutQuart(float x);
float kEaseInOutQuart(float x);
float kEaseInQuint(float x);
float kEaseOutQuint(float x);
float kEaseInOutQuint(float x);
float kEaseInExpo(float x);
float kEaseOutExpo(float x);
float kEaseInOutExpo(float x);
float kEaseInCirc(float x);
float kEaseOutCirc(float x);
float kEaseInOutCirc(float x);
float kEaseInBack(float x);
float kEaseOutBack(float x);
float kEaseInOutBack(float x);
float kEaseInElastic(float x);
float kEaseOutElastic(float x);
float kEaseInOutElastic(float x);
float kEaseInBounce(float x);
float kEaseOutBounce(float x);
float kEaseInOutBounce(float x);

//
//
//

template <typename type>
type kLerp(type from, type to, float t)
{
	return (1 - t) * from + t * to;
}

template <typename type>
type kBezierQuadratic(type a, type b, type c, float t)
{
	float mt = 1 - t;
	float w1 = mt * mt;
	float w2 = 2 * mt * t;
	float w3 = t * t;
	return w1 * a + w2 * b + w3 * c;
}

template <typename type>
type kBezierCubic(type a, type b, type c, type d, float t)
{
	float mt = 1.0f - t;
	float w1 = mt * mt * mt;
	float w2 = 3 * mt * mt * t;
	float w3 = 3 * mt * t * t;
	float w4 = t * t * t;
	return w1 * a + w2 * b + w3 * c + w4 * d;
}

template <typename type>
void kBuildBezierQuadratic(type a, type b, type c, type *points, int segments)
{
	for (int seg_index = 0; seg_index <= segments; ++seg_index)
	{
		float t           = (float)seg_index / (float)segments;
		auto  np          = kBezierQuadratic(a, b, c, t);
		points[seg_index] = np;
	}
}

template <typename type>
void kBuildBezierCubic(type a, type b, type c, type d, type *points, int segments)
{
	for (int seg_index = 0; seg_index <= segments; ++seg_index)
	{
		float t           = (float)seg_index / (float)segments;
		auto  np          = kBezierCubic(a, b, c, d, t);
		points[seg_index] = np;
	}
}

template <typename type>
type kSlerp(type from, type to, float angle, float t)
{
	float s   = kSin(angle);
	float ts  = kSin(angle * t);
	float mts = kSin(angle * (1 - t));
	return (mts * from + ts * to) * (1.0f / s);
}

kVec2   kSlerp(kVec2 from, kVec2 to, float t);
kVec3   kSlerp(kVec3 from, kVec3 to, float t);
kRotor3 kSlerp(kRotor3 from, kRotor3 to, float t);
float   kStep(float edge, float val);
kVec2   kStep(kVec2 edge, kVec2 val);
kVec3   kStep(kVec3 edge, kVec3 val);
kVec4   kStep(kVec4 edge, kVec4 val);
kRotor3 kStep(kRotor3 edge, kRotor3 val);

template <typename Item>
float kSmoothStepZ(Item a, Item b, Item v)
{
	float div_distance = kDistance(a, b);
	if (div_distance)
	{
		auto x = kClamp(0.0f, 1.0f, kDistance(a, v) / div_distance);
		return x * x * (3 - 2 * x);
	}
	return 1;
}

template <typename Item>
float kSmoothStep(Item a, Item b, Item v)
{
	auto x = kClamp(0.0f, 1.0f, kDistance(a, v) / kDistance(a, b));
	return x * x * (3 - 2 * x);
}

float kInverseSmoothStep(float x);

template <typename Item>
Item kMapRange(Item in_a, Item in_b, Item out_a, Item out_b, Item v)
{
	return (out_b - out_a) / (in_b - in_a) * (v - in_a) + out_a;
}
template <typename Item>
Item kMap01(Item in_a, Item in_b, Item v)
{
	return kMapRange(in_a, in_b, Item(0), Item(1), v);
}

float   kMoveTowards(float from, float to, float factor);
kVec2   kMoveTowards(kVec2 from, kVec2 to, float factor);
kVec3   kMoveTowards(kVec3 from, kVec3 to, float factor);
kVec4   kMoveTowards(kVec4 from, kVec4 to, float factor);
kVec2   kRotateAround(kVec2 point, kVec2 center, float angle);
kRotor3 kRotateTowards(kRotor3 from, kRotor3 to, float max_angle);
kVec2   kReflect(kVec2 d, kVec2 n);

//
//
//

constexpr void kUnpackRGBA(u32 c, u8 channels[4])
{
	channels[0] = (c >> 24) & 0xff;
	channels[1] = (c >> 16) & 0xff;
	channels[2] = (c >> 8) & 0xff;
	channels[3] = (c >> 0) & 0xff;
}

constexpr u32 kPackRGBA(u8 r, u8 g, u8 b, u8 a)
{
	#if K_ENDIAN_LITTLE == 1
	return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
	#else
	return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
	#endif
}

constexpr u32 kColor4ToUint(kVec4 v)
{
	u8 r = (u8)kMin((int)(255.0f * v.x), 0xff);
	u8 g = (u8)kMin((int)(255.0f * v.y), 0xff);
	u8 b = (u8)kMin((int)(255.0f * v.z), 0xff);
	u8 a = (u8)kMin((int)(255.0f * v.w), 0xff);
	return kPackRGBA(r, g, b, a);
}

constexpr u32 kColor3ToUint(kVec3 v)
{
	u8 r = static_cast<u8>(255.0f * v.x);
	u8 g = static_cast<u8>(255.0f * v.y);
	u8 b = static_cast<u8>(255.0f * v.z);
	return kPackRGBA(r, g, b, 0xff);
}

constexpr kVec4 kUintToColor4(u32 c)
{
	kVec4 res;
	res.x = (float)((c >> 24) & 0xff) / 255.0f;
	res.y = (float)((c >> 16) & 0xff) / 255.0f;
	res.z = (float)((c >> 8) & 0xff) / 255.0f;
	res.w = (float)((c >> 0) & 0xff) / 255.0f;
	return res;
}

constexpr kVec3 kUintToColor3(u32 c)
{
	kVec3 res;
	res.x = (float)((c >> 24) & 0xff) / 255.0f;
	res.y = (float)((c >> 16) & 0xff) / 255.0f;
	res.z = (float)((c >> 8) & 0xff) / 255.0f;
	return res;
}

kVec3 kLinearToSrgb(kVec3 Col, float gamma = 2.2f);
kVec4 kLinearToSrgb(kVec4 Col, float gamma = 2.2f);
kVec3 kSrgbToLinear(kVec3 Col, float gamma = 2.2f);
kVec4 kSrgbToLinear(kVec4 Col, float gamma = 2.2f);

kVec3 kHsvToRgb(kVec3 c);
kVec3 kRgbToHsv(kVec3 c);
kVec4 kHsvToRgb(kVec4 c);
kVec4 kRgbToHsv(kVec4 c);
