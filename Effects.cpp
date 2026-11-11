#include "Effects.h"

#include <algorithm>
#include <cmath>

void Effects::Initialize() {
	// Rによる再開始では描画リソースを作り直さない。Playerのモデル保持と同じ方針。
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
	ResetMovement();
}

void Effects::SetLightGroup(const LightGroup* lightGroup) {
	if (model_) { model_->SetLightGroup(lightGroup); }
}

Vector3 Effects::ScatterDirection(const Vector3& axis, float angle) {
	// 軸へ一様な乱数方向を混ぜる。angle=0で軸そのもの、大きいほど広がる。
	const Vector3 direction = Normalize(axis + Random::Direction() * std::tan(angle));
	// 乱数が軸を打ち消した場合はNormalizeが0を返すため、軸をそのまま使う。
	return LengthSquared(direction) > 0.0f ? direction : axis;
}

void Effects::Spawn(const GameEvent& event, const GameSettings& settings) {
	switch (event.kind) {
	case GameEvent::Kind::kShot: {
		// kShotのstrengthはゲージで決めた0～1の強さ。衝突の力積とは別の尺度。
		const float shotPower = std::clamp(event.strength, 0.0f, 1.0f);
		const Vector3 contact = event.position - event.normal * (settings.ballRadius * 0.9f);
		const float flashSize = Lerp(settings.shotEffectMinimumFlashSize,
		                             settings.shotEffectMaximumFlashSize, shotPower);
		Emit(contact, {}, {1.0f, 0.95f, 0.7f, 1.0f}, flashSize, 0.1f, 0.0f);
		const int count = static_cast<int>(std::round(Lerp(
			static_cast<float>(settings.shotEffectMinimumParticles),
			static_cast<float>(settings.shotEffectMaximumParticles), shotPower)));
		const float baseSpeed = Lerp(settings.shotEffectMinimumSpeed,
		                             settings.shotEffectMaximumSpeed, shotPower);
		const float variation = std::clamp(settings.shotEffectSpeedVariation, 0.0f, 1.0f);
		for (int index = 0; index < count; ++index) {
			const Vector3 direction = ScatterDirection(-event.normal, ToRadian(65.0f));
			const float speed = baseSpeed * Random::Float(1.0f - variation, 1.0f + variation);
			Emit(contact, direction * speed,
			     {1.0f, 0.88f, 0.48f, 1.0f}, 0.065f, Random::Float(0.16f, 0.3f), 2.0f);
		}
		break;
	}
	case GameEvent::Kind::kWallHit: {
		// 壁の法線まわりへ弾ける。強く当たるほど多く、速い。
		const float power = std::clamp(event.strength / settings.strongHitImpulse, 0.0f, 1.0f);
		const int count = 3 + static_cast<int>(power * 14.0f);
		for (int index = 0; index < count; ++index) {
			const Vector3 direction = ScatterDirection(event.normal, ToRadian(65.0f));
			Emit(event.position, direction * Random::Float(1.0f, 2.0f + power * 6.0f), {0.85f, 0.93f, 1.0f, 1.0f}, 0.05f, Random::Float(0.15f, 0.3f), 4.0f);
		}
		break;
	}
	case GameEvent::Kind::kLand: {
		// 着地の土煙。上へ吹き上げず、横へ低く広げる。
		const float power = std::clamp(event.strength / settings.strongHitImpulse, 0.0f, 1.0f);
		const int count = 3 + static_cast<int>(power * 10.0f);
		for (int index = 0; index < count; ++index) {
			const Vector3 direction = ScatterDirection({0.0f, 1.0f, 0.0f}, ToRadian(80.0f));
			Emit(event.position, direction * Random::Float(0.6f, 1.2f + power * 2.5f), {0.8f, 0.76f, 0.68f, 1.0f}, 0.06f, Random::Float(0.2f, 0.35f), 1.5f);
		}
		break;
	}
	case GameEvent::Kind::kCrackBreak:
		// 崩れた床の破片。マス全体から出して下へ落とす。
		for (int index = 0; index < 16; ++index) {
			const Vector3 offset = Random::InsideSphere(settings.cellSize * 0.4f);
			const Vector3 velocity = Random::InsideSphere(2.0f) - Vector3{0.0f, 0.5f, 0.0f};
			Emit(event.position + offset, velocity, settings.crackedFloorColor, 0.09f, Random::Float(0.4f, 0.7f), settings.gravity);
		}
		break;
	}
}

void Effects::EmitMovement(const Vector3& previousPosition, const Vector3& position,
                           float deltaTime, const GameSettings& settings) {
	if (deltaTime <= 0.0f || settings.movementEffectSpacing <= 0.0f ||
	    settings.movementEffectLifeTime <= 0.0f || settings.movementEffectSize <= 0.0f) {
		ResetMovement();
		return;
	}
	const Vector3 movement = position - previousPosition;
	const float distance = Length(movement);
	// 停止中は軌跡を出さず、距離の余りも捨てる。
	if (distance <= 0.0f || distance / deltaTime < settings.movementEffectMinimumSpeed) {
		ResetMovement();
		return;
	}
	const Vector3 direction = movement / distance;
	float next = settings.movementEffectSpacing - movementDistance_;
	while (next <= distance) {
		// サンプル位置を球の後方へずらし、球の内部で粒が隠れないようにする。
		const Vector3 trailPosition = Lerp(previousPosition, position, next / distance) -
			                        direction * (settings.ballRadius * 0.8f) +
			                        Random::InsideSphere(settings.ballRadius * 0.18f);
		Emit(trailPosition, -direction * 0.4f + Random::InsideSphere(0.45f),
		     settings.movementEffectColor, settings.movementEffectSize,
		     settings.movementEffectLifeTime, 0.0f);
		next += settings.movementEffectSpacing;
	}
	movementDistance_ = std::fmod(movementDistance_ + distance, settings.movementEffectSpacing);
}

void Effects::ResetMovement() {
	movementDistance_ = 0.0f;
}

void Effects::Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float size, float lifeTime, float gravity) {
	for (Particle& particle : particles_) {
		if (particle.life > 0.0f) {
			continue;
		}
		particle = {position, velocity, color, gravity, size, lifeTime, lifeTime};
		return;
	}
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
