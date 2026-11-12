#pragma once
#include <KamataEngine.h>
#include <memory>

using namespace KamataEngine;

class Player {
public:
	Player();
	~Player();

	void Initialize(Model* model, const Camera* camera);
	void Update();
	void Draw();
	void SetMoveSpeed(float speed) { moveSpeed_ = speed; }

private:
	const Camera* camera_ = nullptr;
	WorldTransform worldTransform_{};
	ObjectColor color_{};
	Model* model_ = nullptr;
	float moveSpeed_ = 0.1f; // 1 フレームあたりの移動量
};
