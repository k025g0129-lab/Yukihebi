#include "Boss.h"
#include "Player.h"

Boss::Boss() = default;

Boss::~Boss() = default;

void Boss::Initialize(Model* model, Model* tailModel, const Camera* camera, Player* player) {

	behavior = Behavior::kUnknown;
	behaviorRequest = Behavior::kUnknown;
	hp_ = kMaxHp;
	velocity_ = {};
	selfCollisionAgeSeconds_ = 0.0f;
	selfHitCooldownRemaining_ = 0.0f;

	camera_ = camera;
	model_ = model;
	worldTransform_.Initialize();
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransform_.translation_ = {0.0f, 0.5f, 0.0f};
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
	color_.Initialize();
	color_.SetColor({1.0f, 0.25f, 0.15f, 1.0f});

	player_ = player;

	// 足跡
	footprints_.clear();
	for (int i = 0; i < kMaxFootprint; i++) {
		Footprint footprint;

		footprint.pos = worldTransform_.translation_;
		footprint.rot = worldTransform_.rotation_;

		footprints_.push_back(footprint);
	}

	// 尻尾
	tailModel_ = tailModel;

	bossTails_.clear();
	for (int i = 0; i < kDefaultMaxTail; ++i) {
		AddTail();
	}

	mouth.Initialize(tailModel_, camera_, GetMouthPos());
	mouth.SetColor(Vector4{1.0f, 0.0f, 0.0f, 1.0f});
}

void Boss::Update(float deltaTime) {
	if (deltaTime <= 0.0f || IsDead()) {
		return;
	}
	const Vector3 previousPosition = worldTransform_.translation_;
	selfCollisionAgeSeconds_ += deltaTime;
	selfHitCooldownRemaining_ = (std::max)(0.0f, selfHitCooldownRemaining_ - deltaTime);

	//動き
	TrackingWalkUpdate();
	velocity_ = (worldTransform_.translation_ - previousPosition) / deltaTime;

	//口
	mouth.SetPos(GetMouthPos());
	mouth.Update();

	//尻尾
	Footprint footprint;
	footprint.pos = worldTransform_.translation_;
	footprint.rot = worldTransform_.rotation_;

	footprints_.push_front(footprint);

	if (footprints_.size() > kMaxFootprint) {
		footprints_.pop_back();
	}
	auto it = footprints_.begin();

	for (std::unique_ptr < BossTail>& tail : bossTails_) {

		// kTailIntervalフレーム分、過去の足跡へ移動
		std::advance(it, kTailInterval);

		// その足跡の位置に尻尾を配置
		tail->SetPos(it->pos);

		// 尻尾を更新
		tail->Update();
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
	CheckSelfCollision();
}

void Boss::Draw() {
	
	mouth.Draw();
	model_->Draw(worldTransform_, *camera_); 
	
	for (std::unique_ptr<BossTail>&tail : bossTails_) {
		tail->Draw();
	
	}

}

void Boss::FaceTowardPlayer() { 

	//顔をプレイヤーの方向に徐々に向けさせる

	Vector3 playerPos = player_->GetVector3();
	float turnAngle = 0.0f;

	float dx = playerPos.x - worldTransform_.translation_.x;
	float dz = playerPos.z - worldTransform_.translation_.z;

	if (dx * dx + dz * dz  <= 0.01f) {

		return;
	}

	float targetAngle = std::atan2(dx, dz);
	turnAngle = targetAngle - worldTransform_.rotation_.y; 
	turnAngle = std::atan2(std::sinf(turnAngle), std::cosf(turnAngle));
	turnAngle = std::clamp(turnAngle, -kMaxTurnAngle, kMaxTurnAngle);

	worldTransform_.rotation_.y += turnAngle;

}

void Boss::TrackingWalkUpdate() {

	//顔の向きを徐々にプレイヤーに向ける
	FaceTowardPlayer();
	
	//その方向に移動
	worldTransform_.translation_.x += std::sinf(worldTransform_.rotation_.y) * kMaxWalkSpeed;
	worldTransform_.translation_.z += std::cosf(worldTransform_.rotation_.y) * kMaxWalkSpeed;


}

void Boss::RushUpdate() {
	rushT += 1.0f / 60.0f;

	switch (rushAttack) {
	case FaceTowardCharge:
		FaceTowardPlayer();

		if (rushT > kMaxFaceTowardChargeTime) {
			rushAttack = RushAttack::NotTowardCharge;
			rushT = 0.0f;
		}

		break;
	case NotTowardCharge:
		if (rushT > kMaxRushChargeTime) {
			rushAttack = RushAttack::Rush;
			rushT = 0.0f;
		}
		break;
	case Rush:
		// その方向に移動
		worldTransform_.translation_.x += std::sinf(worldTransform_.rotation_.y) * kMaxWalkSpeed;
		worldTransform_.translation_.z += std::cosf(worldTransform_.rotation_.y) * kMaxWalkSpeed;

		if (rushT > kMaxRushTime) {
			rushAttack = RushAttack::FaceTowardCharge;
			rushT = 0.0f;
		}
		break;

	}
	
}

bool Boss::AddTail() {

	if (bossTails_.size() >= kMaxTail) {
		return false;
	
	}

	auto it = footprints_.begin();

	int index = (int(bossTails_.size()) + 1) * kTailInterval;

	std::advance(it, index);

	bossTails_.push_back(std::make_unique<BossTail>());
	bossTails_.back()->Initialize(tailModel_,camera_,it->pos);
	return true;
}

bool Boss::TryEatSnow() {
	return !IsDead() && AddTail();
}

void Boss::CheckSelfCollision() {
	if (selfCollisionAgeSeconds_ < kInitialSelfCollisionDelaySeconds ||
	    selfHitCooldownRemaining_ > 0.0f) {
		return;
	}
	const Vector3 headPosition = worldTransform_.translation_;
	const float headRadius = worldTransform_.scale_.x;
	for (std::size_t index = kIgnoredFrontSegments; index < bossTails_.size(); ++index) {
		const Vector3 offset = headPosition - bossTails_[index]->GetPos();
		const float radiusSum = headRadius + kTailRadius;
		const float distanceSquared = LengthSquared(offset);
		if (distanceSquared > radiusSum * radiusSum) {
			continue;
		}
		// 接触面へ向かう速度を使い、かすめた接触や離れている接触では傷つかない。
		const float impactSpeed = distanceSquared > 0.000001f
			? (std::max)(0.0f, -Dot(velocity_, offset / std::sqrt(distanceSquared)))
			: Length(velocity_);
		if (impactSpeed < kMinSelfImpactSpeed) {
			continue;
		}
		const int damage = (std::max)(1, static_cast<int>(std::ceil(impactSpeed * kDamagePerImpactSpeed)));
		hp_ = (std::max)(0, hp_ - damage);
		// 当たった雪玉を含め、それより後ろの胴体を消す。
		bossTails_.erase(bossTails_.begin() + static_cast<std::ptrdiff_t>(index), bossTails_.end());
		selfHitCooldownRemaining_ = kSelfHitCooldownSeconds;
		return;
	}
}

const Vector3 Boss::GetMouthPos() { 
	Vector3 re = worldTransform_.translation_;
	float angle = worldTransform_.rotation_.y;

	re.x += std::sinf(angle) * kMouthRange;
	re.z += std::cosf(angle) * kMouthRange;

	
	
	return re; 
}

