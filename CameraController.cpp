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
	camera_->rotation_ = {1.3962634015955f, 0.0f, 0.0f};
	movableArea_ = {
		.left = -500.0f,
		.top = 500.0f,
		.right = 500.0f,
		.bottom = -500.0f,
	};
}

// カメラの更新
void CameraController::Update() {
	const Vector3 targetPosition = target_->GetWorldTransform().translation_;
	const Vector3 playerPosition = target_->GetWorldTransform().translation_;
	const Vector3 targetVelocity = target_->GetVelocity();

	const bool followingPlayer = coreTarget_ == nullptr && bombTarget_ == nullptr &&
		projectileTarget_ == nullptr;
	const float velocityBias = followingPlayer ? kVelocityBias : 0.15f;
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

	followPosition_ = ClampToMovableArea(followPosition_);

	// 反動と揺れも含めた最終位置を制限し、画面端がマップの外へ出ないようにする。
	camera_->translation_ = ClampToMovableArea(followPosition_ + impactKickOffset_);
	impactKickOffset_ *= impactKickRetention_;
	if (LengthSquared(impactKickOffset_) < 0.0001f) {
		impactKickOffset_ = {};
	}

	camera_->UpdateMatrix();
}

void CameraController::UpdateShakeOnly(const Vector3& basePosition) {
	camera_->translation_ = basePosition + impactKickOffset_;
	impactKickOffset_ *= impactKickRetention_;
	if (LengthSquared(impactKickOffset_) < 0.0001f) {
		impactKickOffset_ = {};
	}
	camera_->UpdateMatrix();
}

// カメラのリセット
void CameraController::Reset() {
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	followPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x;
	playerFocusY_ = targetWorldTransform.translation_.y + targetOffset_.y;
	followPosition_.y = playerFocusY_;
	followPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z;
	followPosition_ = ClampToMovableArea(followPosition_);
	camera_->translation_ = followPosition_;

	shakeAmplitude_ = 0.0f;
	shakeDurationFrames_ = 0.0f;
	shakeRemainingFrames_ = 0.0f;
	impactKickOffset_ = {};
}

Vector3 CameraController::ClampToMovableArea(const Vector3& position) const {
	Vector3 clamped = position;
	// マップは Z=0 の XY 平面にあり、カメラは回転しない。
	const float viewDepth = std::abs(position.z);
	const float halfHeight = viewDepth * std::tan(camera_->fovAngleY * 0.5f);
	const float halfWidth = halfHeight * camera_->aspectRatio;

	const float minX = movableArea_.left + halfWidth;
	const float maxX = movableArea_.right - halfWidth;
	const float minY = movableArea_.bottom + halfHeight;
	const float maxY = movableArea_.top - halfHeight;

	// 表示範囲がマップより大きい軸は中央に置く（外側の表示は避けられない）。
	clamped.x = minX <= maxX
		            ? std::clamp(position.x, minX, maxX)
		            : (movableArea_.left + movableArea_.right) * 0.5f;
	clamped.z = minY <= maxY
		            ? std::clamp(position.z, minY, maxY)
		            : (movableArea_.bottom + movableArea_.top) * 0.5f;
	return clamped;
}

void CameraController::RequestShake(float amplitude, float durationFrames) {
	if (amplitude <= 0.0f || durationFrames <= 0.0f) {
		return;
	}
	const float currentAmplitude = shakeDurationFrames_ > 0.0f
		                               ? shakeAmplitude_ * shakeRemainingFrames_ / shakeDurationFrames_
		                               : 0.0f;
	if (amplitude < currentAmplitude) {
		return;
	}
	shakeAmplitude_ = amplitude;
	shakeDurationFrames_ = durationFrames;
	shakeRemainingFrames_ = durationFrames;
}

void CameraController::RequestImpactKick(
	const Vector3& surfaceNormal, float amplitude, float retention) {
	const Vector3 planarNormal{surfaceNormal.x, surfaceNormal.y, 0.0f};
	if (LengthSquared(planarNormal) <= 0.0001f || amplitude <= 0.0f) {
		return;
	}
	impactKickOffset_ = -Normalize(planarNormal) * amplitude;
	impactKickRetention_ = std::clamp(retention, 0.0f, 0.99f);
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
