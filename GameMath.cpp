#include "GameMath.h"

#include <algorithm>
#include <cmath>
#include <random>

//========================================
// Vector3 演算子
//========================================

Vector3 operator-(const Vector3& value) { return {-value.x, -value.y, -value.z}; }

Vector3 operator+(const Vector3& left, const Vector3& right) {
	return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vector3 operator-(const Vector3& left, const Vector3& right) {
	return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vector3 operator*(const Vector3& left, float right) {
	return {left.x * right, left.y * right, left.z * right};
}

Vector3 operator*(float left, const Vector3& right) { return right * left; }

Vector3 operator/(const Vector3& left, float right) {
	return {left.x / right, left.y / right, left.z / right};
}

Vector3& operator+=(Vector3& left, const Vector3& right) {
	left = left + right;
	return left;
}

Vector3& operator-=(Vector3& left, const Vector3& right) {
	left = left - right;
	return left;
}

Vector3& operator*=(Vector3& left, float right) {
	left = left * right;
	return left;
}

Vector3& operator/=(Vector3& left, float right) {
	left = left / right;
	return left;
}

//========================================
// Vector3 関数
//========================================

float Dot(const Vector3& left, const Vector3& right) {
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3 Cross(const Vector3& left, const Vector3& right) {
	return {
		left.y * right.z - left.z * right.y,
		left.z * right.x - left.x * right.z,
		left.x * right.y - left.y * right.x,
	};
}


float LengthSquared(const Vector3& value) { return Dot(value, value); }

float Length(const Vector3& value) { return std::sqrt(LengthSquared(value)); }

Vector3 Normalize(const Vector3& value) {
	const float lengthSquared = LengthSquared(value);
	if (lengthSquared <= 1.0e-12f) {
		return {0.0f, 0.0f, 0.0f};
	}
	return value / std::sqrt(lengthSquared);
}

//========================================
// 補間
//========================================

float Lerp(float start, float end, float t) { return start + (end - start) * t; }

Vector3 Lerp(const Vector3& start, const Vector3& end, float t) {
	return {
		Lerp(start.x, end.x, t),
		Lerp(start.y, end.y, t),
		Lerp(start.z, end.z, t),
	};
}

Vector4 Lerp(const Vector4& start, const Vector4& end, float t) {
	return {
		Lerp(start.x, end.x, t),
		Lerp(start.y, end.y, t),
		Lerp(start.z, end.z, t),
		Lerp(start.w, end.w, t),
	};
}

//========================================
// Matrix4x4
//========================================

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	return {
		{
			{scale.x, 0.0f, 0.0f, 0.0f},
			{0.0f, scale.y, 0.0f, 0.0f},
			{0.0f, 0.0f, scale.z, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
		}
	};
}

Matrix4x4 MakeRotateXMatrix(float radian) {
	const float sinValue = std::sin(radian);
	const float cosValue = std::cos(radian);
	return {
		{
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, cosValue, sinValue, 0.0f},
			{0.0f, -sinValue, cosValue, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
		}
	};
}

Matrix4x4 MakeRotateYMatrix(float radian) {
	const float sinValue = std::sin(radian);
	const float cosValue = std::cos(radian);
	return {
		{
			{cosValue, 0.0f, -sinValue, 0.0f},
			{0.0f, 1.0f, 0.0f, 0.0f},
			{sinValue, 0.0f, cosValue, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
		}
	};
}

Matrix4x4 MakeRotateZMatrix(float radian) {
	const float sinValue = std::sin(radian);
	const float cosValue = std::cos(radian);
	return {
		{
			{cosValue, sinValue, 0.0f, 0.0f},
			{-sinValue, cosValue, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f},
		}
	};
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	return {
		{
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f},
			{translate.x, translate.y, translate.z, 1.0f},
		}
	};
}

Matrix4x4 operator*(const Matrix4x4& left, const Matrix4x4& right) {
	Matrix4x4 result{};
	for (int32_t row = 0; row < 4; ++row) {
		for (int32_t column = 0; column < 4; ++column) {
			float sum = 0.0f;
			for (int32_t index = 0; index < 4; ++index) {
				sum += left.m[row][index] * right.m[index][column];
			}
			result.m[row][column] = sum;
		}
	}
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	const Matrix4x4 rotateMatrix =
		MakeRotateXMatrix(rotate.x) * MakeRotateYMatrix(rotate.y) * MakeRotateZMatrix(rotate.z);
	return MakeScaleMatrix(scale) * rotateMatrix * MakeTranslateMatrix(translate);
}

// 行ベクトル規約に合わせて回転行列の3×3部分だけを掛ける。平行移動成分は使わない。
Vector3 RotateVector(const Vector3& value, const Matrix4x4& rotation) {
	return {
		value.x * rotation.m[0][0] + value.y * rotation.m[1][0] + value.z * rotation.m[2][0],
		value.x * rotation.m[0][1] + value.y * rotation.m[1][1] + value.z * rotation.m[2][1],
		value.x * rotation.m[0][2] + value.y * rotation.m[1][2] + value.z * rotation.m[2][2]
	};
}

// 純粋な回転行列は直交行列なので、逆行列は転置で求まる。
// 拡大縮小やせん断を含む行列では、この関数を一般の逆変換として使わない。
Vector3 InverseRotateVector(const Vector3& value, const Matrix4x4& rotation) {
	return {
		value.x * rotation.m[0][0] + value.y * rotation.m[0][1] + value.z * rotation.m[0][2],
		value.x * rotation.m[1][0] + value.y * rotation.m[1][1] + value.z * rotation.m[1][2],
		value.x * rotation.m[2][0] + value.y * rotation.m[2][1] + value.z * rotation.m[2][2]
	};
}

// 差を±maximumDeltaへ制限して足す。呼び出し元は角速度×秒の非負の上限を渡す。
float MoveTowards(float current, float target, float maximumDelta) {
	return current + std::clamp(target - current, -maximumDelta, maximumDelta);
}

//========================================
// カメラ補助
//========================================

Vector3 LookRotationEuler(const Vector3& from, const Vector3& to) {
	const Vector3 forward = Normalize(to - from);
	if (LengthSquared(forward) <= 1.0e-12f) {
		return {0.0f, 0.0f, 0.0f};
	}

	// Camera は rotation_ から前方向を (cosP*sinY, -sinP, cosP*cosY) として組み立てる。
	// その式を逆に解いてピッチとヨーを取り出す。
	const float pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
	const float yaw = std::atan2(forward.x, forward.z);
	return {pitch, yaw, 0.0f};
}

//========================================
// 乱数
//========================================

namespace Random {
	namespace {
		// 実行のたびに違う並びにする。Particleの散り方が毎回同じにならないように。
		std::mt19937& Engine() {
			static std::mt19937 engine{std::random_device{}()};
			return engine;
		}
	} // namespace

	float Float() {
		std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
		return distribution(Engine());
	}

	float Float(float minimum, float maximum) {
		if (maximum <= minimum) {
			return minimum;
		}
		std::uniform_real_distribution<float> distribution(minimum, maximum);
		return distribution(Engine());
	}

	int32_t Int(int32_t minimum, int32_t maximum) {
		if (maximum <= minimum) {
			return minimum;
		}
		std::uniform_int_distribution<int32_t> distribution(minimum, maximum);
		return distribution(Engine());
	}

	Vector3 Direction() {
		// 立方体内の点を球の内側に入るまで引き直す。角へ偏らせないため。
		for (;;) {
			const Vector3 value{Float(-1.0f, 1.0f), Float(-1.0f, 1.0f), Float(-1.0f, 1.0f)};
			const float lengthSquared = LengthSquared(value);
			if (lengthSquared > 1.0e-6f && lengthSquared <= 1.0f) {
				return Normalize(value);
			}
		}
	}

	Vector3 InsideSphere(float radius) {
		const float safeRadius = (std::max)(radius, 0.0f);
		// 立方根を掛けて、体積あたりの密度を一様にする。
		return Direction() * (safeRadius * std::cbrt(Float()));
	}
} // namespace Random
