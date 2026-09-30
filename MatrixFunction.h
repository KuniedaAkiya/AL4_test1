#pragma once
#include "KamataEngine.h"

// これ使うとKamataEngine::を省略できる
using namespace KamataEngine;

// AABB（軸並行境界ボックス）
struct AABB {
	Vector3 min;
	Vector3 max;
};

// ===== 行列生成関数 =====

// 行列の積（m1 * m2）
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian);

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian);

// アフィン変換行列の作成関数
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m);

// 透視投影行列（プロジェクション行列）
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

// ===== 補間・当たり判定関数 =====

// 線形補間（イージング付き）
float Lerp(float start, float end, float t, float tmax);

// 線形補間（Vector3版）
Vector3 Lerp(const Vector3& start, const Vector3& end, float interpolationRate);

// AABB同士の交差判定
bool IsCollisionAABB(const AABB& aabb1, const AABB& aabb2);

// イーズイン (始まりが緩やかで、終わりにかけて急加速する)
float EaseIn(float start, float end, float t);

// イーズアウト (始まりが急で、終わりにかけて緩やかに減速する)
float EaseOut(float start, float end, float t);

// Vector3 + Vector3
inline Vector3 operator+(const Vector3& a, const Vector3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

// Vector3 - Vector3 (差分もよく使うのでついでに)
inline Vector3 operator-(const Vector3& a, const Vector3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

// Vector3 * スカラー (拡大縮小もよく使うのでついでに)
inline Vector3 operator*(const Vector3& v, float scalar) { return {v.x * scalar, v.y * scalar, v.z * scalar}; }

// スカラー * Vector3 (2.0f * velocity のような書き方もできるように)
inline Vector3 operator*(float scalar, const Vector3& v) { return v * scalar; }

// Vector3 / スカラー (中間座標を求めるときなどによく使う)
inline Vector3 operator/(const Vector3& v, float scalar) { return {v.x / scalar, v.y / scalar, v.z / scalar}; }

// Vector3 *= スカラー (velocity_ *= 0.9f のような減衰処理に使う)
inline Vector3& operator*=(Vector3& v, float scalar) {
	v.x *= scalar;
	v.y *= scalar;
	v.z *= scalar;
	return v;
}

// Vector3 /= スカラー (ついでに)
inline Vector3& operator/=(Vector3& v, float scalar) {
	v.x /= scalar;
	v.y /= scalar;
	v.z /= scalar;
	return v;
}