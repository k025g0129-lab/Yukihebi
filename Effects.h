#pragma once

#include "GameEvent.h"
#include "GameSettings.h"
#include <KamataEngine.h>
#include <array>
#include <memory>

using namespace KamataEngine;

// 出来事から粒を出して動かすだけのクラス。ゲームの状態は読まないし書かない。
// cubeモデルを白テクスチャ＋ObjectColorで描く。Playerのキュー・狙いの点と同じ手法。
class Effects {
public:
	// エンジン初期化後に呼ぶ。Rによる再開始では出ている粒だけ消す。
	void Initialize();
	// 生成済みの粒モデルに、ステージと同じ光源を結び付ける。
	void SetLightGroup(const LightGroup* lightGroup);
	// 出来事に応じた粒を出す。種類ごとの色・数・速さはこのクラスに閉じる。
	void Spawn(const GameEvent& event, const GameSettings& settings);
	// 球の移動区間に沿って粒を置く。固定物理更新ごとに呼ぶ。
	void EmitMovement(const Vector3& previousPosition, const Vector3& position,
	                  float deltaTime, const GameSettings& settings);
	// 球を開始位置へ戻したとき、前の移動区間の余りを持ち越さない。
	void ResetMovement();
	// 描画フレームごとに進める。ヒットストップ中は0を渡して止める。
	void Update(float deltaTime);
	// Model::PreDraw（加算・デプス読み取り専用）とPostDrawの間で呼ぶ。
	void Draw(const Camera& camera) const;

private:
	struct Particle {
		Vector3 position{};
		Vector3 velocity{};
		Vector4 color{};
		// 1秒あたりに落ちる速さの増加。0で重力を受けない。
		float gravity = 0.0f;
		// 出したときの1辺の半分。残り寿命の比率で小さくする。
		float size = 0.1f;
		// 残り秒数。0以下が空き。
		float life = 0.0f;
		// 出したときの寿命。濃さと大きさの比率に使う。
		float lifeTime = 1.0f;
	};
	// 同時に出せる粒の数。足りなければ新しい粒を出さない。
	// 出ている粒を途中で消すより、増えないほうが見た目が安定する。
	static constexpr size_t kMaximumParticles = 192;
	void Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float size, float lifeTime, float gravity);
	// axisを中心にangleラジアンまで開いた、ばらけた単位ベクトル。
	static Vector3 ScatterDirection(const Vector3& axis, float angle);
	std::unique_ptr<Model> model_;
	uint32_t texture_ = 0;
	std::array<Particle, kMaximumParticles> particles_{};
	// 粒ごとの描画リソース。Initializeで作り切り、毎フレーム値だけ入れ替える。
	// particles_の添字とは対応しない。生きている粒を先頭から詰めて並べる。
	std::array<WorldTransform, kMaximumParticles> transforms_{};
	std::array<ObjectColor, kMaximumParticles> colors_{};
	size_t drawCount_ = 0;
	float movementDistance_ = 0.0f;
};
