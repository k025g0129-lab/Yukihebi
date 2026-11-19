#pragma once

#include <KamataEngine.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "GameMath.h"
using namespace KamataEngine;

// 呼び出し元が指定した粒を生成・更新・描画する。演出の種類や発生位置は判断しない。
// cubeモデルを白テクスチャ＋ObjectColorで描く。
class Effects {
public:
	// 粒の初期値。位置や速度、色などは呼び出し元が決める。
	struct ParticleDesc {
		Vector3 position;
		Vector3 velocity;
		Vector4 color;
		float size;
		float lifeTime;
		float gravity;
	};

	// エンジン初期化後に呼ぶ。再初期化時は描画リソースを再利用して粒だけ消す。
	void Initialize();
	// 生成済みの粒モデルに光源を結び付ける。
	void SetLightGroup(const LightGroup* lightGroup);
	// 空きがあれば粒を生成する。サイズや寿命が不正、または満杯ならfalseを返す。
	bool Spawn(const ParticleDesc& desc);
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
	std::unique_ptr<Model> model_;
	uint32_t texture_ = 0;
	std::array<Particle, kMaximumParticles> particles_{};
	// 粒ごとの描画リソース。Initializeで作り切り、毎フレーム値だけ入れ替える。
	// particles_の添字とは対応しない。生きている粒を先頭から詰めて並べる。
	std::array<WorldTransform, kMaximumParticles> transforms_{};
	std::array<ObjectColor, kMaximumParticles> colors_{};
	size_t drawCount_ = 0;
};
