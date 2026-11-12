#pragma once
#include "GameMath.h"

class Stage;

// 表示階層だけを所有する。ボールの存在階層を書き換えない。
class CameraController {
public:
	// 最初は球の所属階層を表示する。以後は手動切替と実際の階層移動を区別する。
	void Initialize(const GameSettings& settings, const Stage& stage, int ballLayer, const Vector3& ballPosition);
	// 【現在は未使用】スペースはショットに使うため、表示階層は球に自動追従する。
	// 表示階層を順に巡回するだけで、球の座標・速度・階層は変えない。
	void SwitchLayer(int layerCount);
	void FollowBallLayer(int ballLayer);
	// 小さいマップは中央を、広いマップは球の周辺を注視して見下ろす。
	void Update(float deltaTime, const Stage& stage, const Vector3& ballPosition);
	// 打った向きへ注視点を一時的にずらす。Updateで元の位置へ戻る。
	void AddKick(const Vector3& direction, float distance);
	// 当たりの強さ(0～1)で注視点を揺らす。より強い揺れが残っている間は上書きしない。
	void AddShake(float strength);
	// ゴール時に、ヒットストップと一緒に残るキック・揺れを消す。
	void ClearImpact();
	int GetDisplayLayer() const { return displayLayer_; }
	const Camera& GetCamera() const { return camera_; }

private:
	Vector3 CalculateTarget(const Stage& stage, const Vector3& ballPosition) const;
	GameSettings settings_{};
	Camera camera_{};
	// 補間中の注視点のワールド位置。大きいマップだけ球のXZ位置に追従する。
	Vector3 target_{};
	int displayLayer_ = 0;
	// 球の実際の階層移動を検出するための前回値。手動表示切替では変更しない。
	int previousBallLayer_ = 0;
	bool initialized_ = false;
	// 注視点に足す一時的なずれ。cameraKickReturnSpeedで0へ戻る。
	Vector3 kick_{};
	// 揺れの残り強さ(0～1)。毎秒cameraShakeDecayずつ減る。
	float shake_ = 0.0f;
};
