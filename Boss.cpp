#include "Boss.h"
#include "Player.h"

Boss::Boss() = default;

Boss::~Boss() = default;

void Boss::Initialize(Model* model, const Camera* camera, Player* player) {
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
	

}

void Boss::Update() {

	FaceTowardPlayer();
	TrackingWalk();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Boss::Draw() {
	model_->Draw(worldTransform_, *camera_); 
}

void Boss::FaceTowardPlayer() { 
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

void Boss::TrackingWalk() {

	worldTransform_.translation_.x += std::sinf(worldTransform_.rotation_.y) * kMaxWalkSpeed;
	worldTransform_.translation_.z += std::cosf(worldTransform_.rotation_.y) * kMaxWalkSpeed;


}
