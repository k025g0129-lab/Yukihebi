#include "CameraController.h"

#include <algorithm>
#include <cmath>


#include "GameMath.h"
#include "Player.h"

// カメラの初期化
void CameraController::Initialize() {
	camera_->Initialize();
	followPosition_ = camera_->translation_;
	playerFocusY_ = followPosition_.y;
	movableArea_ = {
		.left = -0.0f,
		.top = 0.0f,
		.right = 50.0f,
		.bottom = -50.0f,
	};
}

// カメラの更新
void CameraController::Update() {
	const Vector3 targetPosition = target_->GetWorldTransform().translation_;
	const Vector3 playerPosition = target_->GetWorldTransform().translation_;
	const Vector3 targetVelocity = target_->GetVelocity();

	constexpr float velocityBias = kVelocityBias * 0.15f;
	targetPosition_.x = playerPosition.x + targetOffset_.x + targetVelocity.x * velocityBias;
	targetPosition_.y = playerPosition.y + targetOffset_.y;
	targetPosition_.z = playerPosition.z + targetOffset_.z + targetVelocity.z * velocityBias;

	targetPosition_.z = targetPosition.z + targetOffset_.z + targetVelocity.z * velocityBias;
	followPosition_.x = std::lerp(followPosition_.x, targetPosition_.x, kInterpolationRate);
	followPosition_.y = std::lerp(followPosition_.y, targetPosition_.y, kInterpolationRate);
	followPosition_.z = std::lerp(followPosition_.z, targetPosition_.z, kInterpolationRate);

	followPosition_.x = std::clamp(followPosition_.x, targetPosition_.x + margin.left,
	                               targetPosition_.x + margin.right);
	followPosition_.y = std::clamp(followPosition_.y, targetPosition_.y + margin.bottom,
	                               targetPosition_.y + margin.top);

	ApplyCameraOffsets(followPosition_);
}

void CameraController::UpdateShakeOnly(const Vector3& basePosition) {
	ApplyCameraOffsets(basePosition);
}

void CameraController::ApplyCameraOffsets(const Vector3& basePosition) {
	// 寄りは高さだけを下げる。下げた高さで範囲制限するので、画面端はマップの外へ出ない。
	const Vector3 zoomOffset{0.0f, -zoomPunch_, 0.0f};
	// 反動と揺れも含めた最終位置を制限し、画面端がマップの外へ出ないようにする。
	camera_->translation_ = ClampToMovableArea(basePosition + impactKickOffset_ + zoomOffset + Shake());
	impactKickOffset_ *= impactKickRetention_;
	if (LengthSquared(impactKickOffset_) < 0.0001f) {
		impactKickOffset_ = {};
	}
	zoomPunch_ *= zoomPunchRetention_;
	if (zoomPunch_ < 0.01f) {
		zoomPunch_ = 0.0f;
	}
	camera_->UpdateMatrix();
	camera_->TransferMatrix();
}

// カメラのリセット
void CameraController::Reset() {
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	followPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x;
	playerFocusY_ = targetWorldTransform.translation_.y + targetOffset_.y;
	followPosition_.y = playerFocusY_;
	followPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z;

	camera_->translation_ = followPosition_;

	shakeAmplitude_ = 0.0f;
	shakeDurationFrames_ = 0.0f;
	shakeRemainingFrames_ = 0.0f;
	impactKickOffset_ = {};
	zoomPunch_ = 0.0f;
	camera_->UpdateMatrix();

	camera_->TransferMatrix();
}

Vector3 CameraController::ClampToMovableArea(const Vector3& position) const {
	Vector3 clamped = position;

	const float height = position.y; // 地面Y=0からの高さ
	const float angle = camera_->rotation_.x;
	const float tanHalfY = std::tan(camera_->fovAngleY * 0.5f);
	const float tanHalfX = tanHalfY * camera_->aspectRatio;
	const float sinAngle = std::sin(angle);
	const float cosAngle = std::cos(angle);

	// 画面上端が地面と交わらない角度では、この範囲制限は使えない。
	const float farDenominator = sinAngle - tanHalfY * cosAngle;
	if (height <= 0.0f || farDenominator <= 0.0f) {
		return clamped;
	}

	const float nearDenominator = sinAngle + tanHalfY * cosAngle;
	const float nearZ = height * (cosAngle - tanHalfY * sinAngle) / nearDenominator;
	const float farZ = height * (cosAngle + tanHalfY * sinAngle) / farDenominator;
	const float halfWidth = height * tanHalfX / farDenominator;

	const float minX = movableArea_.left + halfWidth;
	const float maxX = movableArea_.right - halfWidth;
	const float minZ = movableArea_.bottom - nearZ;
	const float maxZ = movableArea_.top - farZ;

	clamped.x = (minX <= maxX) ? std::clamp(position.x, minX, maxX) : (movableArea_.left + movableArea_.right) * 0.5f;
	clamped.z = (minZ <= maxZ) ? std::clamp(position.z, minZ, maxZ) : (minZ + maxZ) * 0.5f;

	return clamped;
}

void CameraController::RequestShake(float amplitude, float durationFrames) {
	if (amplitude <= 0.0f || durationFrames <= 0.0f) {
		return;
	}
	const float remainingRatio = shakeDurationFrames_ > 0.0f ? shakeRemainingFrames_ / shakeDurationFrames_ : 0.0f;
	const float currentAmplitude = shakeAmplitude_ * remainingRatio * remainingRatio;
	if (amplitude < currentAmplitude) {
		return;
	}
	shakeAmplitude_ = amplitude;
	shakeDurationFrames_ = durationFrames;
	shakeRemainingFrames_ = durationFrames;
}

Vector3 CameraController::Shake() {
	if (shakeRemainingFrames_ <= 0.0f || shakeDurationFrames_ <= 0.0f) {
		return {};
	}

	// 上から見た画面の横と縦に相当するX/Z方向を揺らす。
	// 2乗で減衰させ、最初の一発を強く、余韻を短くする。
	const float remainingRatio = shakeRemainingFrames_ / shakeDurationFrames_;
	const float strength = shakeAmplitude_ * remainingRatio * remainingRatio;
	shakeRemainingFrames_ = (std::max)(0.0f, shakeRemainingFrames_ - 1.0f);
	return {Random::Float(-strength, strength), 0.0f, Random::Float(-strength, strength)};
}

void CameraController::RequestImpactKick(
	const Vector3& surfaceNormal, float amplitude, float retention) {
	// 真上から見下ろすカメラなので、画面の縦横に当たるX/Z平面で反動を付ける。
	const Vector3 planarNormal{surfaceNormal.x, 0.0f, surfaceNormal.z};
	if (LengthSquared(planarNormal) <= 0.0001f || amplitude <= 0.0f) {
		return;
	}
	impactKickOffset_ = -Normalize(planarNormal) * amplitude;
	impactKickRetention_ = std::clamp(retention, 0.0f, 0.99f);
}

void CameraController::RequestZoomPunch(float amount, float retention) {
	if (amount <= 0.0f) {
		return;
	}
	// 強い方を優先する。弱い寄りで強い寄りを打ち消さない。
	if (amount >= zoomPunch_) {
		zoomPunch_ = amount;
		zoomPunchRetention_ = std::clamp(retention, 0.0f, 0.99f);
	}
}

void CameraController::SetMapImpactShakeSettings(const MapImpactShakeSettings& settings) {
	mapImpactShakeSettings_ = settings;
	mapImpactShakeSettings_.minImpactSpeed = (std::max)(0.0f, settings.minImpactSpeed);
	mapImpactShakeSettings_.amplitudePerSpeed = (std::max)(0.0f, settings.amplitudePerSpeed);
	mapImpactShakeSettings_.maxAmplitude = (std::max)(0.0f, settings.maxAmplitude);
	mapImpactShakeSettings_.durationFrames = (std::max)(0.0f, settings.durationFrames);
}

void CameraController::SetPlayerVerticalFocusMargin(float verticalMargin) {
	playerVerticalFocusMargin_ = (std::max)(0.0f, verticalMargin);
}

void CameraController::RequestMapImpactShake(float impactSpeed) {
	if (impactSpeed <= mapImpactShakeSettings_.minImpactSpeed) {
		return;
	}
	const float amplitude = std::clamp(
		(impactSpeed - mapImpactShakeSettings_.minImpactSpeed) * mapImpactShakeSettings_.amplitudePerSpeed,
		0.0f, mapImpactShakeSettings_.maxAmplitude);
	RequestShake(amplitude, mapImpactShakeSettings_.durationFrames);
}
