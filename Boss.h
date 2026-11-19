#pragma once
#include <KamataEngine.h>
#include <memory>
#include "GameMath.h"
#include <algorithm>
#include <list>
#include <iterator>
#include <vector>



#include <cmath>
#include "BossTail.h"


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

struct Footprint {
	Vector3 pos;
	Vector3 rot;
	
};



class Boss {
public:
	Boss();
	~Boss();

	void Initialize(Model* model, Model* tailModel, const Camera* camera, Player* player);
	void Update();
	void Draw();

	void FaceTowardPlayer(); 
	void TrackingWalkUpdate();
	void DushMove();

	//ラッシュ
	void RushUpdate();

	//尻尾
	void AddTail();

	const WorldTransform& GetWorldTransform() { return worldTransform_; };
	const Vector3 GetMouthPos();


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

	//ステータス
	int hp = 5;

	//追従
	static inline const float kMaxWalkSpeed = 0.03f;
	static inline const float kMaxTurnAngle = 0.05f;

	//突進
	static inline const float kMaxFaceTowardChargeTime = 5.0f;
	static inline const float kMaxRushChargeTime = 1.0f;
	static inline const float kMaxRushTime = 3.0f;
	RushAttack rushAttack;
	float rushT = 0.0f;


	//尻尾
	Model* tailModel_ = nullptr;
	std::vector<std::unique_ptr<BossTail>> bossTails_;
	BossTail* bossTail_;
	static inline const int kDefaultMaxTail = 5;
	static inline const int kMaxTail = 15;
	static inline const int kTailInterval = 30;
	static inline const int kMaxFootprint = kMaxTail * kTailInterval + 1;
	std::list < Footprint >footprints_; 

	//口
	BossTail mouth;
	static inline const float kMouthRange = 1.0f;


};
