#pragma once
#include "GameMath.h"
#include <KamataEngine.h>
#include <algorithm>
#include <memory>

#include <cmath>

class Boss;

class BossTail {

public:
	//BossTail();
	//~BossTail();

	void Initialize(Model* model, const Camera* camera, const Vector3 pos = (Vector3{0.0f, 0.0f, 0.0f}));
	void Update();
	void Draw();

	void SetPos(const Vector3 pos) { worldTransform_.translation_ = pos; };
	Vector3 GetPos() { return worldTransform_.translation_; };
	void SetColor(const Vector4 color) { color_.SetColor(color); };
	

private:

	// 基本変数設定
	const Camera* camera_ = nullptr;
	WorldTransform worldTransform_{};
	ObjectColor color_{};
	Model* model_ = nullptr;

	Boss* boss;



};
