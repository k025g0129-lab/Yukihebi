#include "Boss.h"
#include "Player.h"

Boss::Boss() = default;

Boss::~Boss() = default;

void Boss::Initialize(Model* model, Model* tailModel, const Camera* camera, Player* player) {

	behavior = Behavior::kUnknown;
	behaviorRequest = Behavior::kUnknown;

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
	for (int i = 0; i < kDefaultMaxTail; i++) {
		AddTail();
	}

	mouth.Initialize(tailModel_, camera_, GetMouthPos());
	mouth.SetColor(Vector4{1.0f, 0.0f, 0.0f, 1.0f});
}

void Boss::Update() {

	//動き
	TrackingWalkUpdate();
	//RushUpdate();

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
		if (rushT >kMaxRushChargeTime) {
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

void Boss::AddTail() {

	if (bossTails_.size() >= kMaxTail) {
		return;
	
	}

	auto it = footprints_.begin();

	int index = (int(bossTails_.size()) + 1) * 10;

	std::advance(it, index);

	bossTails_.push_back(std::make_unique<BossTail>());
	bossTails_.back()->Initialize(tailModel_,camera_,it->pos);

}

const Vector3 Boss::GetMouthPos() { 
	Vector3 re = worldTransform_.translation_;
	float angle = worldTransform_.rotation_.y;

	re.x += std::sinf(angle) * kMouthRange;
	re.z += std::cosf(angle) * kMouthRange;

	
	
	return re; 
}

void Boss::IsCollisionMouthToSnow(Snow& snow) {


	(void)snow;
	AddTail();

}
