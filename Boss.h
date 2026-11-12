#pragma once
#include <KamataEngine.h>
#include <memory>
#include "GameMath.h"
#include <algorithm>

#include <cmath>

class Player;

using namespace KamataEngine;


enum class Behavior {
	kTrackingWalk,
	kDeath,
	kDamageReaction,
	kRush,
	k,
	kUnknown,
};

enum RushAttack {
	FaceTowardCharge,
	NotTowardCharge,
	Rush,
};



class Boss {
public:
	Boss();
	~Boss();

	void Initialize(Model* model, const Camera* camera,Player* player);
	void Update();
	void Draw();

	void FaceTowardPlayer(); 
	void TrackingWalkUpdate();
	void DushMove();

	void RushUpdate();

	const WorldTransform& GetWorldTransform() { return worldTransform_; };


private:

	//状態
	Behavior behavior;
	Behavior behaviorRequest;

	//基本変数設定
	const Camera* camera_ = nullptr;
	WorldTransform worldTransform_{};
	ObjectColor color_{};
	Model* model_ = nullptr;

	Player* player_;

	//追従
	static inline const float kMaxWalkSpeed = 0.03f;
	static inline const float kMaxTurnAngle = 0.05f;

	//突進
	static inline const float kMaxFaceTowardChargeTime = 5.0f;
	static inline const float kMaxRushChargeTime = 1.0f;
	RushAttack rushAttack;
	float rushT = 0.0f;


};
