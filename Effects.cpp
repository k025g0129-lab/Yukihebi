#include "Effects.h"

void Effects::Initialize() {
	// 再初期化では描画リソースを作り直さず、粒の状態だけ消す。
	if (!model_) {
		model_.reset(Model::CreateFromOBJ("cube"));
		texture_ = TextureManager::Load("white1x1.png");
		for (WorldTransform& transform : transforms_) {
			transform.Initialize();
		}
		for (ObjectColor& color : colors_) {
			color.Initialize();
		}
	}
	for (Particle& particle : particles_) {
		particle.life = 0.0f;
	}
	drawCount_ = 0;
}

void Effects::SetLightGroup(const LightGroup* lightGroup) {
	if (model_) { model_->SetLightGroup(lightGroup); }
}

bool Effects::Spawn(const ParticleDesc& desc) {
	if (desc.size <= 0.0f || desc.lifeTime <= 0.0f) {
		return false;
	}
	for (Particle& particle : particles_) {
		if (particle.life > 0.0f) {
			continue;
		}
		particle = {desc.position, desc.velocity, desc.color, desc.gravity,
		            desc.size, desc.lifeTime, desc.lifeTime};
		return true;
	}
	return false;
}

void Effects::Update(float deltaTime) {
	// 生きている粒を先頭から詰め直す。deltaTime=0なら値は変わらず、その場で止まる。
	drawCount_ = 0;
	for (Particle& particle : particles_) {
		if (particle.life <= 0.0f) {
			continue;
		}
		particle.life -= deltaTime;
		if (particle.life <= 0.0f) {
			continue;
		}
		particle.velocity.y -= particle.gravity * deltaTime;
		particle.position += particle.velocity * deltaTime;
		if (drawCount_ >= kMaximumParticles) {
			break;
		}
		// 残り寿命で小さく薄くする。加算合成なのでαを落とすと消えていく。
		const float ratio = particle.life / particle.lifeTime;
		const float size = particle.size * ratio;
		WorldTransform& transform = transforms_[drawCount_];
		transform.scale_ = {size, size, size};
		transform.translation_ = particle.position;
		transform.matWorld_ = MakeScaleMatrix(transform.scale_) * MakeTranslateMatrix(particle.position);
		transform.TransferMatrix();
		colors_[drawCount_].SetColor({particle.color.x, particle.color.y, particle.color.z, particle.color.w * ratio});
		++drawCount_;
	}
}

void Effects::Draw(const Camera& camera) const {
	if (!model_) {
		return;
	}
	for (size_t index = 0; index < drawCount_; ++index) {
		model_->Draw(transforms_[index], camera, texture_, &colors_[index]);
	}
}
