#include "ParticleSystem.h"

#include <algorithm>

#include "GameMath.h"
#include "ModelUnits.h"

namespace {

// Initialize した ParticleSystem。出す側から引くために持つ。
ParticleSystem* g_instance = nullptr;

// 球は面が多いほど重い。Particle は小さく出るので粗くてよい。
const uint32_t kSphereDivisionVertical = 6u;
const uint32_t kSphereDivisionHorizontal = 8u;

} // namespace

ParticleSystem* ParticleSystem::GetInstance() {
	return g_instance;
}

ParticleSystem::~ParticleSystem() {
	if (g_instance == this) {
		g_instance = nullptr;
	}

	// Model は Create が new して返すだけなので、消す責任はここにある。
	delete model_;
	model_ = nullptr;
}

//==============================
// 初期化
//==============================

void ParticleSystem::Initialize() {
	if (isInitialized_) {
		return;
	}

	model_ = Model::CreateSphere(kSphereDivisionVertical, kSphereDivisionHorizontal);
	whiteTextureHandle_ = TextureManager::Load("white1x1.png");

	// 定数バッファはここで全部ぶんまとめて作る。描画中に作らないため。
	for (size_t index = 0; index < kMaxParticles; ++index) {
		transforms_[index].Initialize();
		objectColors_[index].Initialize();
	}

	aliveCount_ = 0;
	isInitialized_ = true;
	g_instance = this;
}

//==============================
// 発生
//==============================

void ParticleSystem::Emit(
	const Vector3& position,
	const Vector3& velocity,
	const Vector4& startColor,
	const Vector4& endColor,
	float startScale,
	float endScale,
	float lifetimeFrames,
	const Vector3& gravity) {
	if (!isInitialized_) {
		return;
	}

	// いっぱいなら何もしない。
	if (aliveCount_ >= kMaxParticles) {
		return;
	}

	// 0 で割らないよう、最低 1 フレームは生かす。
	float lifetime = lifetimeFrames;
	if (lifetime < 1.0f) {
		lifetime = 1.0f;
	}

	Particle& particle = particles_[aliveCount_];
	particle.origin = position;
	particle.velocity = velocity;
	particle.gravity = gravity;
	particle.startColor = startColor;
	particle.endColor = endColor;
	particle.startScale = startScale;
	particle.endScale = endScale;
	particle.lifetimeFrames = lifetime;
	particle.ageFrames = 0.0f;

	++aliveCount_;
}

//==============================
// 更新
//==============================

void ParticleSystem::Update() {
	size_t index = 0;
	while (index < aliveCount_) {
		particles_[index].ageFrames += 1.0f;

		if (particles_[index].ageFrames < particles_[index].lifetimeFrames) {
			++index;
			continue;
		}

		// 死んだものは最後の生存Particleと入れ替えて、生存ぶんを前へ詰めたままにする。
		// 入れ替えた側はまだ見ていないので、添字は進めない。
		--aliveCount_;
		particles_[index] = particles_[aliveCount_];
	}
}

//==============================
// 描画
//==============================

void ParticleSystem::Draw(const Camera& camera) {
	if (!isInitialized_ || aliveCount_ == 0) {
		return;
	}

	// 裏面も描き、αで重ね、深度は読むだけにする。
	// 深度を書くと、後ろのParticleが先に描かれたParticleに隠されて穴が開く。
	Model::PreDraw(
		Model::CullingMode::kNone,
		Model::BlendMode::kNormal,
		Model::DepthTestMode::kReadOnly);

	for (size_t index = 0; index < aliveCount_; ++index) {
		const Particle& particle = particles_[index];

		// 寿命のうち何割すすんだか。0.0 で生まれたとき、1.0 で消えるとき。
		float progress = particle.ageFrames / particle.lifetimeFrames;
		progress = std::clamp(progress, 0.0f, 1.0f);

		// 等加速度運動。毎フレーム速度を足し込むのではなく、経過フレーム数から直接求める。
		const float age = particle.ageFrames;
		const Vector3 position =
			particle.origin +
			particle.velocity * age +
			particle.gravity * (0.5f * age * age);

		const float size = Lerp(particle.startScale, particle.endScale, progress);
		const float modelScale = size * kSphereModelScale;

		WorldTransform& transform = transforms_[index];
		transform.scale_ = {modelScale, modelScale, modelScale};
		transform.rotation_ = {0.0f, 0.0f, 0.0f};
		transform.translation_ = position;
		transform.matWorld_ =
			MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();

		const Vector4 color = Lerp(particle.startColor, particle.endColor, progress);
		objectColors_[index].SetColor(color);

		model_->Draw(transform, camera, whiteTextureHandle_, &objectColors_[index]);
	}

	Model::PostDraw();
}

//==============================
// その他
//==============================

void ParticleSystem::Clear() {
	aliveCount_ = 0;
}
