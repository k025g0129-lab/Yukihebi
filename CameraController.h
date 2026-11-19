#pragma once

#include "KamataEngine.h"
using namespace KamataEngine;

class Player;
class Enemy;
class Bomb;
class Core;

class CameraController {
public:
	struct MapImpactShakeSettings {
		// 衝突速度はワールド単位/秒、振幅はワールド単位、時間はフレーム数。
		float minImpactSpeed = 4.0f;
		float amplitudePerSpeed = 0.015f;
		float maxAmplitude = 0.3f;
		float durationFrames = 10.0f;
	};

	// <summary>
	// 初期化
	// </summary>
	void Initialize();

	// <summary>
	// 更新
	// </summary>
	void Update();
	// 結果画面では追従せず、本編と同じ Shake だけ固定カメラへ適用する。
	void UpdateShakeOnly(const Vector3& basePosition);
	const Vector3& GetFollowPosition() const { return followPosition_; }

	// 目標プレイヤー
	Player* target_ = nullptr;
	void SetTarget(Player* target) { target_ = target; }


	void Reset();

	void RequestShake(float amplitude, float durationFrames);
	void RequestImpactKick(const Vector3& surfaceNormal, float amplitude, float retention);
	void RequestMapImpactShake(float impactSpeed);
	void SetMapImpactShakeSettings(const MapImpactShakeSettings& settings);
	void SetPlayerVerticalFocusMargin(float verticalMargin);
	// 追従対象とカメラの座標の差
	Vector3 targetOffset_ = {0, 30.0f, -5.0f};
	void SetCamera(Camera* camera) { camera_ = camera; }

	struct Rect {
		float left = 0.0f;
		float top = 1.0f;
		float right = 10.0f;
		float bottom = 0.0f;
	};

	Rect movableArea_ = {0, 100, 0, 100};

	void SetMovableArea(const Rect& area) { movableArea_ = area; }

	// カメラの目標座標
	Vector3 targetPosition_ = {};

	static constexpr float kInterpolationRate = 0.1f;

	static constexpr float kVelocityBias = 0.5f;
	WorldTransform worldTransform_;
	static constexpr Rect margin = {
		.left = -10.0f,
		.top = 10.0f,
		.right = 10.0f,
		.bottom = -10.0f,
	};
	const Camera* GetCamera() const { return camera_; }

private:
	// マップの四辺から、画面全体が収まるカメラ中心の範囲を求める。
	Vector3 ClampToMovableArea(const Vector3& position) const;
	Vector3 Shake();
	Vector3 followPosition_{};
	// ジャンプ中に小さな上下動を追わないための仮想の注視点。
	float playerFocusY_ = 0.0f;
	float playerVerticalFocusMargin_ = 3.0f;
	float shakeAmplitude_ = 0.0f;
	float shakeDurationFrames_ = 0.0f;
	float shakeRemainingFrames_ = 0.0f;
	Vector3 impactKickOffset_{};
	float impactKickRetention_ = 0.72f;
	MapImpactShakeSettings mapImpactShakeSettings_{};
	// カメラ
	Camera* camera_ = nullptr;
};
