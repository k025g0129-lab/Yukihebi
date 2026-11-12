#pragma once

#include <KamataEngine.h>

#include <cstdint>

using namespace KamataEngine;

//========================================
// 算術ユーティリティ
//========================================

// Vector3 / Matrix4x4 は数値を入れる箱だけで、演算子も関数も持たない。
// ゲーム側で必要になるぶんだけをここに置く。

//========================================
// 定数
//========================================

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = kPi * 2.0f;
constexpr float kHalfPi = kPi * 0.5f;

// 設定を度数で書きたい場合の入口。回転行列・sin/cosへ渡す値はラジアンへそろえる。
constexpr float ToRadian(float degree) { return degree * (kPi / 180.0f); }
constexpr float ToDegree(float radian) { return radian * (180.0f / kPi); }

//========================================
// フレーム換算
//========================================

constexpr float kFramesPerSecond = 60.0f;

// 60FPSを前提とする旧来のフレーム換算用。現在の物理は秒×deltaTimeを使うので混ぜない。
constexpr float PerFrame(float perSecond) { return perSecond / kFramesPerSecond; }

constexpr float PerFrameSquared(float perSecondSquared) {
	return perSecondSquared / (kFramesPerSecond * kFramesPerSecond);
}

constexpr float ToFrames(float seconds) { return seconds * kFramesPerSecond; }

//========================================
// Vector3 演算子
//========================================

Vector3 operator-(const Vector3& value);
Vector3 operator+(const Vector3& left, const Vector3& right);
Vector3 operator-(const Vector3& left, const Vector3& right);
Vector3 operator*(const Vector3& left, float right);
Vector3 operator*(float left, const Vector3& right);
Vector3 operator/(const Vector3& left, float right);

Vector3& operator+=(Vector3& left, const Vector3& right);
Vector3& operator-=(Vector3& left, const Vector3& right);
Vector3& operator*=(Vector3& left, float right);
Vector3& operator/=(Vector3& left, float right);

//========================================
// Vector3 関数
//========================================

float Dot(const Vector3& left, const Vector3& right);
Vector3 Cross(const Vector3& left, const Vector3& right);
float LengthSquared(const Vector3& value);
float Length(const Vector3& value);

// 長さが 0 に近いときは 0 ベクトルを返す。0 除算でNaNを作らないため。
Vector3 Normalize(const Vector3& value);

//========================================
// 補間
//========================================

// start + (end-start)×t の線形補間。tを0～1に制限する処理は含まない。
float Lerp(float start, float end, float t);
Vector3 Lerp(const Vector3& start, const Vector3& end, float t);
Vector4 Lerp(const Vector4& start, const Vector4& end, float t);

//========================================
// Matrix4x4
//========================================

// 行優先・行ベクトル（v' = v * M）。シェーダー側も row_major で揃えてある。
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
Matrix4x4 MakeRotateXMatrix(float radian);
Matrix4x4 MakeRotateYMatrix(float radian);
Matrix4x4 MakeRotateZMatrix(float radian);
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 operator*(const Matrix4x4& left, const Matrix4x4& right);

// Scale → RotateX → RotateY → RotateZ → Translate の順で合成する。
// WorldTransform は行列を作らないので、これを matWorld_ へ入れて TransferMatrix() を呼ぶ。
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

// 平行移動を含まない回転。逆変換は回転行列の転置で求める。
// 点の平行移動には対応しない。Stageがpivotを引いて戻す操作と組み合わせる。
Vector3 RotateVector(const Vector3& value, const Matrix4x4& rotation);
Vector3 InverseRotateVector(const Vector3& value, const Matrix4x4& rotation);
// 1回にmaximumDeltaだけ目標へ近づけ、目標を飛び越えない。角度の傾斜・復帰に使用。
float MoveTowards(float current, float target, float maximumDelta);

//========================================
// カメラ補助
//========================================

// from から to を見るための回転角（X:ピッチ, Y:ヨー, Z:0）を返す。
// Camera は注視点を取れないので、角度へ直して rotation_ に入れる。
Vector3 LookRotationEuler(const Vector3& from, const Vector3& to);

//========================================
// 乱数
//========================================

namespace Random {

float Float();                       // [0, 1)
float Float(float minimum, float maximum);
int32_t Int(int32_t minimum, int32_t maximum); // 両端を含む

Vector3 Direction();                 // 一様な単位ベクトル
Vector3 InsideSphere(float radius);  // 半径 radius の球の内部

} // namespace Random
