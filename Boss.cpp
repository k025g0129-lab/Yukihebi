#include "Boss.h"
#include "Player.h"

Boss::Boss() = default;

Boss::~Boss() = default;

void Boss::Initialize(Model* model, Model* tailModel, const Camera* camera, Player* player) {

	behavior_ = Behavior::kTrackingWalk;
	behaviorRequest_ = Behavior::kUnknown;

	hp_ = kMaxHp;
	velocity_ = {};
	selfCollisionAgeSeconds_ = 0.0f;
	selfHitCooldownRemaining_ = 0.0f;
	damageFlashRemaining_ = 0.0f;
	pulseRemaining_ = 0.0f;
	pulseDuration_ = 0.0f;
	pulseAmount_ = 0.0f;
	removedTailPositions_.clear();

	camera_ = camera;
	model_ = model;
	worldTransform_.Initialize();
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransform_.translation_ = {0.0f, 0.5f, 0.0f};
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_,
	                                             worldTransform_.translation_);
	worldTransform_.TransferMatrix();
	color_.Initialize();
	// 通常時は元の見た目のまま。被ダメージ時だけ ApplyVisual で白くする。
	color_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

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
	damageFlashRemaining_ = (std::max)(0.0f, damageFlashRemaining_ - deltaTime);
	pulseRemaining_ = (std::max)(0.0f, pulseRemaining_ - deltaTime);

	//動き
	BehaviorRequest();
	BehaviorUpdate();
	velocity_ = (worldTransform_.translation_ - previousPosition) / deltaTime;

	//口
	mouth.SetPos(GetMouthPos());
	mouth.Update();


	ApplyVisual();
	CheckSelfCollision();
}

void Boss::Draw() {
	mouth.Draw();
	model_->Draw(worldTransform_, *camera_, &color_);

	for (std::unique_ptr<BossTail>& tail : bossTails_) {
		tail->Draw();
	}
}

void Boss::BehaviorRequest() {

	if (behaviorRequest_ != Behavior::kUnknown) {
		behavior_ = behaviorRequest_;
		
		switch (behavior_) {
		case Behavior::kTrackingWalk:
			TrackingWalkInitialize();
			break;
		case Behavior::kDeath:
			break;
		case Behavior::kDamageReaction:
			break;
		case Behavior::kRush:
			RushInitialize();
			break;
		case Behavior::kSweeping:
			SweepingInitialize();
			break;

		default:
			break;
		}
	
		behaviorRequest_ = Behavior::kUnknown;
	}

}

void Boss::BehaviorUpdate() {

	switch (behavior_) {
	case Behavior::kTrackingWalk:
		TrackingWalkUpdate();
		break;
	case Behavior::kDeath:
		break;
	case Behavior::kDamageReaction:
		break;
	case Behavior::kRush:
		RushUpdate();
		break;
	case Behavior::kSweeping:
		SweepingUpdate();
		break;

	default:
		break;
	}


}

void Boss::FaceTowardPlayer() {

	//顔をプレイヤーの方向に徐々に向けさせる

	Vector3 playerPos = player_->GetVector3();
	float turnAngle = 0.0f;

	float dx = playerPos.x - worldTransform_.translation_.x;
	float dz = playerPos.z - worldTransform_.translation_.z;

	if (dx * dx + dz * dz <= 0.01f) {
		return;
	}

	float targetAngle = std::atan2(dx, dz);
	turnAngle = targetAngle - worldTransform_.rotation_.y;
	turnAngle = std::atan2(std::sinf(turnAngle), std::cosf(turnAngle));
	turnAngle = std::clamp(turnAngle, -kMaxTurnAngle, kMaxTurnAngle);

	worldTransform_.rotation_.y += turnAngle;
}


void Boss::NotFaceTowardPlayer() {

	// 顔をプレイヤーの逆方向に徐々に向けさせる

	Vector3 playerPos = player_->GetVector3();
	float turnAngle = 0.0f;

	float dx = playerPos.x - worldTransform_.translation_.x;
	float dz = playerPos.z - worldTransform_.translation_.z;

	if (dx * dx + dz * dz <= 0.01f) {

		return;
	}

	float targetAngle = std::atan2( -dx, -dz);
	turnAngle = targetAngle - worldTransform_.rotation_.y;
	turnAngle = std::atan2(std::sinf(turnAngle), std::cosf(turnAngle));
	turnAngle = std::clamp(turnAngle, -kMaxTurnAngle /2.0f, kMaxTurnAngle /2.0f);

	worldTransform_.rotation_.y += turnAngle;


}

void Boss::TrackingWalkInitialize() { 
	trackingWalkT = 0.0f;

}

void Boss::TrackingWalkUpdate() {
	trackingWalkT += 1.0f/60.0f;

	//顔の向きを徐々にプレイヤーに向ける
	FaceTowardPlayer();

	//その方向に移動
	worldTransform_.translation_.x += std::sinf(worldTransform_.rotation_.y) * kMaxWalkSpeed;
	worldTransform_.translation_.z += std::cosf(worldTransform_.rotation_.y) * kMaxWalkSpeed;

	//尻尾
	TailsTrackingUpdate();

	if (trackingWalkT >kMaxTrackingWalkTime ) {
		behaviorRequest_ = Behavior::kSweeping;
	}

}

void Boss::RushInitialize() { 
	rushT = 0; 
	rushAttack = RushAttack::FaceTowardCharge;

}
void Boss::RushUpdate() {

	//突撃内容を変える,以下は旧

	rushT += 1.0f / 60.0f;

	switch (rushAttack) {
	case FaceTowardCharge:
		FaceTowardPlayer();

		if (rushT > kMaxFaceTowardChargeTime) {
			rushAttack = NotTowardCharge;
			rushT = 0.0f;
		}

		break;
	case NotTowardCharge:
		if (rushT > kMaxRushChargeTime) {
			rushAttack = Rush;
			rushT = 0.0f;
		}
		break;
	case Rush:
		// その方向に移動
		worldTransform_.translation_.x += std::sinf(worldTransform_.rotation_.y) * kMaxWalkSpeed;
		worldTransform_.translation_.z += std::cosf(worldTransform_.rotation_.y) * kMaxWalkSpeed;

		TailsTrackingUpdate();

		if (rushT > kMaxRushTime) {
			rushAttack = FaceTowardCharge;
			rushT = 0.0f;
			behaviorRequest_ = Behavior::kTrackingWalk;
		}
		break;
	}
}

void Boss::SweepingInitialize() { 
	sweepingAttack = SweepingAttack::FaceTowardFanCharge;
	sweepingT = 0.0f;

}

void Boss::SweepingUpdate() { 
	sweepingT += 1.0f / 60.0f;

	switch (sweepingAttack) {
	case FaceTowardFanCharge:
		FaceTowardPlayer();
		TailsFanUpdate();

		if (sweepingT > kMaxSweepingChargeTime) {
			sweepingStartAngle = worldTransform_.rotation_.y;
			sweepingT = 0.0f;
			sweepingAttack = SweepingAttack::Attack;
		}

		break;
	case Attack:
		SweepingAttackUpdate();
		break;

	}

}

void Boss::TailsFanUpdate() {
	float lerpT = 0.08f;


	//次への向き
	float toNextAngle = worldTransform_.rotation_.y - 3.14f + kTailsFanAngle;

	//一個前の座標(頭から尻尾1)
	Vector3 ordPrevPos = worldTransform_.translation_;
	Vector3 targetPrevPos = worldTransform_.translation_;

	for (std::unique_ptr<BossTail>& tail : bossTails_) {

		//現在の尻尾の位置
		Vector3 ordPos = tail->GetPos();

		float dx = ordPos.x - ordPrevPos.x;
		float dz = ordPos.z - ordPrevPos.z;

		//現在の角度
		float currentAngle = toNextAngle;

		//保険
		if (dx * dx + dz * dz > 0.001f){
			currentAngle = std::atan2(dx,dz);
		}

		//角度の差
		float angleDiff = toNextAngle - currentAngle;
		angleDiff = std::atan2(std::sinf(angleDiff), std::cosf(angleDiff));

		currentAngle += angleDiff * lerpT;

		//計算
		Vector3 pos =targetPrevPos;
		pos.x += std::sinf(currentAngle) * tailInterval;
		pos.z += std::cosf(currentAngle) * tailInterval;


		// その足跡の位置に尻尾を配置
		tail->SetPos(pos);

		// 尻尾を更新
		tail->Update();

		ordPrevPos = ordPos;
		targetPrevPos = pos;

		toNextAngle += kTailsFanAngle;
	}



}

void Boss::SweepingAttackUpdate() {

	//イージングT
	float t = std::clamp(sweepingT / kMaxSweepingAttackTime, 0.0f, 1.0f);
	t = Ease::EaseInOutBack(t);

	//頭
	worldTransform_.rotation_.y = sweepingStartAngle + 3.14f * 2.0f * t;

	//尻尾
	float fanAngle = kTailsFanAngle * (1.0f - 2.0f * t);
	float nextFanAngle = worldTransform_.rotation_.y - 3.14f + fanAngle;

	Vector3 pos = worldTransform_.translation_;

	for (std::unique_ptr<BossTail>& tail : bossTails_) {

		pos.x += std::sinf(nextFanAngle) * tailInterval;
		pos.z += std::cosf(nextFanAngle) * tailInterval;

		// その足跡の位置に尻尾を配置
		tail->SetPos(pos);

		// 尻尾を更新
		tail->Update();

		nextFanAngle += fanAngle;
	}

}

bool Boss::AddTail() {
	if (bossTails_.size() >= kMaxTail) {
		return false;
	}

	auto it = footprints_.begin();

	int index = (static_cast<int>(bossTails_.size()) + 1) * kTailInterval;

	std::advance(it, index);

	bossTails_.push_back(std::make_unique<BossTail>());
	bossTails_.back()->Initialize(tailModel_, camera_, it->pos);
	return true;
}

bool Boss::TryEatSnow() {
	return !IsDead() && AddTail(); }

void Boss::TailsTrackingUpdate() {

	// 尻尾
	Footprint footprint;
	footprint.pos = worldTransform_.translation_;
	footprint.rot = worldTransform_.rotation_;

	footprints_.push_front(footprint);

	if (footprints_.size() > kMaxFootprint) {
		footprints_.pop_back();
	}
	auto it = footprints_.begin();

	for (std::unique_ptr<BossTail>& tail : bossTails_) {

		// kTailIntervalフレーム分、過去の足跡へ移動
		std::advance(it, kTailInterval);

		// その足跡の位置に尻尾を配置
		tail->SetPos(it->pos);

		// 尻尾を更新
		tail->Update();
	}


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

		// 当たった雪玉を含め、それより後ろの胴体を消す。消える位置は演出用に残す。
		for (std::size_t removed = index; removed < bossTails_.size(); ++removed) {
			removedTailPositions_.push_back(bossTails_[removed]->GetPos());
		}

		bossTails_.erase(bossTails_.begin() + static_cast<std::ptrdiff_t>(index), bossTails_.end());
		selfHitCooldownRemaining_ = kSelfHitCooldownSeconds;
		return;
	}
}

void Boss::RequestDamageFlash() {
	damageFlashRemaining_ = kDamageFlashSeconds;
	ApplyVisual();
}

void Boss::RequestPulse(float amount, float seconds) {
	if (amount <= 0.0f || seconds <= 0.0f) {
		return;
	}
	pulseAmount_ = amount;
	pulseDuration_ = seconds;
	pulseRemaining_ = seconds;
	ApplyVisual();
}

void Boss::DrainRemovedTailPositions(std::vector<Vector3>& positions) {
	positions.insert(positions.end(), removedTailPositions_.begin(), removedTailPositions_.end());
	removedTailPositions_.clear();
}

void Boss::ApplyVisual() {
	const Vector4 color = damageFlashRemaining_ > 0.0f ? kFlashColor : Vector4{1.0f, 1.0f, 1.0f, 1.0f};
	color_.SetColor(color);
	for (std::unique_ptr<BossTail>& tail : bossTails_) {
		tail->SetColor(color);
	}

	// 膨らみは見た目の行列だけに掛ける。scale_ は当たり判定の半径なので変えない。
	float pulse = 1.0f;
	if (pulseRemaining_ > 0.0f && pulseDuration_ > 0.0f) {
		const float ratio = pulseRemaining_ / pulseDuration_;
		pulse += pulseAmount_ * ratio * ratio;
	}
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_ * pulse, worldTransform_.rotation_,
	                                             worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

const Vector3 Boss::GetMouthPos() {
	Vector3 re = worldTransform_.translation_;
	float angle = worldTransform_.rotation_.y;

	re.x += std::sinf(angle) * kMouthRange;
	re.z += std::cosf(angle) * kMouthRange;


	return re;
}
