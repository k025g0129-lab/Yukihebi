#include "BossTail.h"

void BossTail::Initialize(Model* model, const Camera* camera, const Vector3 pos) {

	camera_ = camera;
	model_ = model;
	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;
	color_.Initialize();
	color_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
}

void BossTail::Update() {

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void BossTail::Draw() { 
	model_->Draw(worldTransform_, *camera_, &color_); }
