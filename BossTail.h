#pragma once
#include "GameMath.h"
#include <KamataEngine.h>
#include <algorithm>
#include <memory>

#include <cmath>

class Boss;

class BossTail {

public:
	BossTail();
	~BossTail();

	void Initialize(Model* model, const Camera* camera);
	void Update();
	void Draw();



private:

	
	// 基本変数設定
	const Camera* camera_ = nullptr;
	WorldTransform worldTransform_{};
	ObjectColor color_{};
	Model* model_ = nullptr;

	Boss* boss;



};
