#include "CameraController.h"
#include "Stage.h"

#include <algorithm>
#include <cmath>

void CameraController::Initialize(const GameSettings& settings, const Stage& stage, int ballLayer,
	                              const Vector3& ballPosition) {
	settings_ = settings;
	// Rで再開始するときは、CameraのGPUリソースを作り直さず設定と注視点を戻す。
	if (!initialized_) {
		camera_.Initialize();
		initialized_ = true;
	}
	displayLayer_ = ballLayer;
	previousBallLayer_ = ballLayer;
	// 前のプレイで残った揺れ・ずれを持ち越さない。
	kick_ = {};
	shake_ = 0.0f;
	// 開始地点が端にある大きいマップでも、初回から球を画面に入れる。
	target_ = CalculateTarget(stage, ballPosition);
	Update(0.0f, stage, ballPosition);
}

void CameraController::SwitchLayer(int layerCount) {
	// 内部階層は0始まり。最後の階層の次は剰余で0へ戻す。
	if (layerCount > 0) { displayLayer_ = (displayLayer_ + 1) % layerCount; }
}

void CameraController::FollowBallLayer(int ballLayer) {
	// ボールが実際に別階層へ移った瞬間だけ追従する。
	// 毎フレーム上書きすると、Spaceで別階層を確認できなくなる。
	if (ballLayer != previousBallLayer_) { displayLayer_ = ballLayer; }
	previousBallLayer_ = ballLayer;
}

void CameraController::AddKick(const Vector3& direction, float distance) {
	kick_ += Normalize(direction) * distance;
}

void CameraController::AddShake(float strength) {
	// 弱い当たりが続いても、強い揺れの途中で弱めない。
	shake_ = (std::max)(shake_, std::clamp(strength, 0.0f, 1.0f));
}

void CameraController::ClearImpact() {
	kick_ = {};
	shake_ = 0.0f;
}

Vector3 CameraController::CalculateTarget(const Stage& stage, const Vector3& ballPosition) const {
	const auto& map = stage.GetMap();
	const float halfWidth = static_cast<float>(map.GetNumBlockHorizontal()) * settings_.cellSize * 0.5f;
	const float halfDepth = static_cast<float>(map.GetNumBlockDepth()) * settings_.cellSize * 0.5f;
	const float framedHalfSize = settings_.cameraMaximumFramingCells * settings_.cellSize * 0.5f;
	const float maxOffsetX = (std::max)(0.0f, halfWidth - framedHalfSize);
	const float maxOffsetZ = (std::max)(0.0f, halfDepth - framedHalfSize);
	const Vector3 ballLocal = stage.WorldToLocal(ballPosition);
	return stage.LocalToWorld({
		std::clamp(ballLocal.x, -maxOffsetX, maxOffsetX),
		map.GetFloorHeight(displayLayer_) + settings_.cellSize * 0.5f,
		std::clamp(ballLocal.z, -maxOffsetZ, maxOffsetZ)
	});
}

void CameraController::Update(float deltaTime, const Stage& stage, const Vector3& ballPosition) {
	const auto& map = stage.GetMap();
	const Vector3 target = CalculateTarget(stage, ballPosition);
	// 指数補間の係数1-exp(-速さ×秒)を使い、描画FPSが違っても追従の速さをそろえやすくする。
	target_ = Lerp(target_, target, 1.0f - std::exp(-settings_.cameraFollowSpeed * deltaTime));
	// 広いマップは表示範囲を設定マス数で制限し、球が小さくなりすぎるのを防ぐ。
	const float framedHalfSize = settings_.cameraMaximumFramingCells * settings_.cellSize * 0.5f;
	const float halfWidth = (std::min)(static_cast<float>(map.GetNumBlockHorizontal()) * settings_.cellSize * 0.5f,
	                                   framedHalfSize);
	const float halfDepth = (std::min)(static_cast<float>(map.GetNumBlockDepth()) * settings_.cellSize * 0.5f,
	                                   framedHalfSize);
	// 透視投影の関係 tan(縦画角/2)=半高さ/距離を利用し、幅と奥行きを収める距離を計算する。
	const float tangent = std::tan(camera_.fovAngleY * 0.5f);
	const float widthDistance = halfWidth / (tangent * camera_.aspectRatio);
	const float depthDistance = halfDepth / (tangent * std::sin(settings_.cameraPitch));
	// 表示範囲の幅・奥行きから距離を選び、周辺が見える余白を足す。
	const float distance = (std::max)(widthDistance, depthDistance) * settings_.cameraPadding + settings_.cellSize;
	// キックは指数で戻し、揺れは一定量ずつ減らす。描画FPSが変わっても速さをそろえる。
	kick_ = Lerp(kick_, {0.0f, 0.0f, 0.0f}, 1.0f - std::exp(-settings_.cameraKickReturnSpeed * deltaTime));
	shake_ = (std::max)(0.0f, shake_ - settings_.cameraShakeDecay * deltaTime);
	// 揺れは注視点ごと動かす。残り強さに比例した半径の球内へ、毎フレーム別のずれを足す。
	const Vector3 shakeOffset = Random::InsideSphere(shake_ * settings_.cameraShakeRadius);
	// 箱の回転には合わせず、カメラは一定の俯角で見る。90度なら真上、0度なら水平。
	camera_.rotation_ = {settings_.cameraPitch, 0.0f, 0.0f};
	camera_.translation_ = target_ + kick_ + shakeOffset + Vector3{
		0.0f, std::sin(settings_.cameraPitch) * distance, -std::cos(settings_.cameraPitch) * distance
	};
	// translation_とrotation_を変えた後にカメラ行列を更新し、次の描画に反映する。
	camera_.UpdateMatrix();
}
