#pragma once
#include <KamataEngine.h>
#include <memory>
#include "GameMath.h"
#include <algorithm>
#include <list>
#include <iterator>
#include <vector>
#include <cstddef>


#include <cmath>
#include "BossTail.h"


class Player;
class Snow;

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
	void Update(float deltaTime);
	void Draw();

	void FaceTowardPlayer();
	void TrackingWalkUpdate();
	void DushMove();

	//ラッシュ
	void RushUpdate();

	//尻尾
	bool AddTail();
	bool TryEatSnow();
	bool IsDead() const { return hp_ <= 0; }
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return kMaxHp; }


	//セッター
	const WorldTransform& GetWorldTransform() { return worldTransform_; };
	const Vector3 GetMouthPos();

	std::size_t GetTailCount() const { return bossTails_.size(); }
	Vector3 GetTailPos(std::size_t index) const { return bossTails_[index]->GetPos(); }
	float GetMouthRadius() const { return kMouthRadius; }
	float GetTailRadius() const { return kTailRadius; }

private:
	void CheckSelfCollision();

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
	int hp_ = 5;
	Vector3 velocity_{};
	float selfCollisionAgeSeconds_ = 0.0f;
	float selfHitCooldownRemaining_ = 0.0f;

	static inline const int kMaxHp = 5;
	static inline const float kMinSelfImpactSpeed = 1.0f;
	static inline const float kDamagePerImpactSpeed = 0.5f;
	static inline const float kSelfHitCooldownSeconds = 0.6f;
	static inline const float kInitialSelfCollisionDelaySeconds = 1.5f;
	static inline const std::size_t kIgnoredFrontSegments = 2;

	// 既存の追跡・突進の値
	static inline const float kMaxWalkSpeed = 0.03f;
	static inline const float kMaxTurnAngle = 0.05f;
	static inline const float kMaxFaceTowardChargeTime = 5.0f;
	static inline const float kMaxRushChargeTime = 1.0f;
	static inline const float kMaxRushTime = 3.0f;
	RushAttack rushAttack;
	float rushT = 0.0f;


	//尻尾
	Model* tailModel_ = nullptr;
	std::vector<std::unique_ptr<BossTail>> bossTails_;
	BossTail* bossTail_;
	static inline const int kDefaultMaxTail = 0;
	static inline const int kMaxTail = 15;
	static inline const int kTailInterval = 30;
	static inline const int kMaxFootprint = kMaxTail * kTailInterval + 1;
	std::list < Footprint >footprints_; 
	static inline const float kTailRadius = 0.5f;

	//口
	BossTail mouth;
	static inline const float kMouthRange = 1.0f;
	static inline const float kMouthRadius = 0.5f;


};
