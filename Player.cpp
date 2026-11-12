#include "Player.h"

#include "GameMath.h"

#include <cmath>

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize(const Camera* camera) {
	camera_ = camera;
	model_.reset(Model::CreateSphere());
	worldTransform_.Initialize();
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransform_.translation_ = {0.0f, 0.5f, 0.0f};
	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
	color_.Initialize();
	color_.SetColor({1.0f, 0.25f, 0.15f, 1.0f});
}

void Player::Update() {
	const Input* input = Input::GetInstance();
	Vector3 direction{};

	if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
		direction.x -= 1.0f;
	}
	if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
		direction.x += 1.0f;
	}
	if (input->PushKey(DIK_W) || input->PushKey(DIK_UP)) {
		direction.z += 1.0f;
	}
	if (input->PushKey(DIK_S) || input->PushKey(DIK_DOWN)) {
		direction.z -= 1.0f;
	}

	// 斜め移動だけ速くならないよう、入力方向を正規化する。
	const float lengthSquared = direction.x * direction.x + direction.z * direction.z;
	if (lengthSquared > 0.0f) {
		const float step = moveSpeed_ / std::sqrt(lengthSquared);
		worldTransform_.translation_.x += direction.x * step;
		worldTransform_.translation_.z += direction.z * step;
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() {
	model_->Draw(worldTransform_, *camera_, &color_);
}
