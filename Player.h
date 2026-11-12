#pragma once
#include <KamataEngine.h>
#include <memory>

using namespace KamataEngine;

class Player {
public:
	Player();
	~Player();

	void Initialize(const Camera* camera);
	void Update();
	void Draw();
	void SetMoveSpeed(float speed) { moveSpeed_ = speed; }

private:
	const Camera* camera_ = nullptr;
	WorldTransform worldTransform_{};
	ObjectColor color_{};
	std::unique_ptr<Model> model_;
	float moveSpeed_ = 0.1f; // 1 フレームあたりの移動量
};
