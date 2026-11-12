#pragma once
#include <KamataEngine.h>
#include <memory>
#include "GameMath.h"
#include <algorithm>

#include <cmath>

class Player;

using namespace KamataEngine;

class Boss {
public:
	Boss();
	~Boss();

	void Initialize(Model* model, const Camera* camera,Player* player);
	void Update();
	void Draw();

	void FaceTowardPlayer(); 
	void TrackingWalk();


private:

	//基本変数設定
	const Camera* camera_ = nullptr;
	WorldTransform worldTransform_{};
	ObjectColor color_{};
	Model* model_ = nullptr;

	Player* player_;

	//旋回
	static inline const float kMaxWalkSpeed = 0.03f;
	static inline const float kMaxTurnAngle = 0.1f;



};
