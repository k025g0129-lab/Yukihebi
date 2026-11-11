#pragma once

#include <KamataEngine.h>

#include <array>
#include <cstdint>

#include "GameMath.h"

using namespace KamataEngine;

//========================================
// Particle
//========================================

// Particle 機能は用意されていないので、Model の多重描画で作る。
//
// WorldTransform と ObjectColor は 1 つずつ定数バッファを抱えていて、コピーもできない。
// そのため最大数ぶんを Initialize でまとめて確保し、生きている Particle だけを描く。
// 途中で増やす設計（std::vector へ push_back）は組めない。
//
// 速さ・加速度・寿命はすべて 1 フレームあたりの量で持つ。

class ParticleSystem {
public:
	// Initialize 済みのものを返す。まだ作られていなければ nullptr。
	static ParticleSystem* GetInstance();

	ParticleSystem() = default;
	~ParticleSystem();

	ParticleSystem(const ParticleSystem&) = delete;
	ParticleSystem& operator=(const ParticleSystem&) = delete;

	// エンジンの初期化より後に呼ぶ。定数バッファを確保するため。
	void Initialize();

	// 1フレームぶん進める。
	void Update();

	void Draw(const Camera& camera);

	// Particle を 1 個出す。
	//
	//   position       出る位置
	//   velocity       1フレームあたりの移動量
	//   startColor     生まれたときの色（w がアルファ）
	//   endColor       消えるときの色
	//   startScale     生まれたときの大きさ
	//   endScale       消えるときの大きさ
	//   lifetimeFrames 何フレーム生きるか
	//   gravity        1フレームあたりの加速度。いらなければ省略してよい
	//
	// まとめて出したいときは、使う側でこの関数を回数ぶん呼ぶ。
	// いっぱいで入らなかったときは、何もせずに戻る。
	void Emit(
		const Vector3& position,
		const Vector3& velocity,
		const Vector4& startColor,
		const Vector4& endColor,
		float startScale,
		float endScale,
		float lifetimeFrames,
		const Vector3& gravity = {0.0f, 0.0f, 0.0f});

	// 出ている Particle を全部消す。
	void Clear();

private:
	// 同時に出るのは移動残像が約24個、攻撃の軌跡が約64個、被弾が18個。余裕を見てこの数。
	static const size_t kMaxParticles = 256;

	// Particle 1個ぶんの状態
	struct Particle {
		Vector3 origin{};   // 生まれた位置
		Vector3 velocity{}; // 初速（1フレームあたり）
		Vector3 gravity{};  // 加速度（1フレームあたり）
		Vector4 startColor{};
		Vector4 endColor{};
		float startScale = 0.0f;
		float endScale = 0.0f;
		float lifetimeFrames = 1.0f; // 寿命
		float ageFrames = 0.0f;      // 生まれてから何フレーム経ったか
	};

	std::array<Particle, kMaxParticles> particles_{};

	// Particle と添字で対応する描画用のリソース。中身は毎フレーム書き換える。
	std::array<WorldTransform, kMaxParticles> transforms_{};
	std::array<ObjectColor, kMaxParticles> objectColors_{};

	// 生きている Particle は常に [0, aliveCount_) へ詰めておく。
	size_t aliveCount_ = 0;

	Model* model_ = nullptr;

	// モデル側のテクスチャを無効化して、指定した色だけで出すための 1x1 の白。
	uint32_t whiteTextureHandle_ = 0;

	bool isInitialized_ = false;
};
