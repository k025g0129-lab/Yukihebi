#pragma once
#include <KamataEngine.h>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <vector>
#include "GameMath.h"


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

	bool ApplyDamage(int damage) {
		if (damage <= 0 || IsDead()) {
			return false;
		}
		hp_ = (std::max)(0, hp_ - damage);
		return true;
	}


	//セッター
	const WorldTransform& GetWorldTransform() { return worldTransform_; };
	const Vector3 GetMouthPos();

	std::size_t GetTailCount() const { return bossTails_.size(); }
	Vector3 GetTailPos(std::size_t index) const { return bossTails_[index]->GetPos(); }
	float GetMouthRadius() const { return kMouthRadius; }
	float GetTailRadius() const { return kTailRadius; }
	// Playerとの接触・押し出しに使う頭の半径。モデルの見た目（約0.75）に合わせる。
	float GetHeadBodyRadius() const { return kHeadBodyRadius; }

	//演出
	// 頭と尻尾を一瞬白く光らせる。ヒットストップ中も光って見えるよう、すぐ反映する。
	void RequestDamageFlash();
	// 頭の見た目を一瞬膨らませる。判定の大きさは変えない。
	void RequestPulse(float amount, float seconds);
	// 自分に当たって切れた尻尾の位置を、切れた順（当たった節が先頭）で移して消す。
	void DrainRemovedTailPositions(std::vector<Vector3>& positions);

private:
	void CheckSelfCollision();
	// フラッシュと膨らみを色と行列へ反映する。
	void ApplyVisual();

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

	//演出
	float damageFlashRemaining_ = 0.0f;
	float pulseRemaining_ = 0.0f;
	float pulseDuration_ = 0.0f;
	float pulseAmount_ = 0.0f;
	std::vector<Vector3> removedTailPositions_;
	static constexpr float kDamageFlashSeconds = 0.15f;
	// シェーダーは色を乗算するので、1より大きい値で白く飛ばす。
	static constexpr Vector4 kFlashColor = {3.0f, 3.0f, 3.0f, 1.0f};

	static constexpr int kMaxHp = 5;
	static constexpr float kMinSelfImpactSpeed = 1.0f;
	static constexpr float kDamagePerImpactSpeed = 0.5f;
	static constexpr float kSelfHitCooldownSeconds = 0.6f;
	static constexpr float kInitialSelfCollisionDelaySeconds = 1.5f;
	static constexpr std::size_t kIgnoredFrontSegments = 2;

	// 追跡・突進の値
	static constexpr float kMaxWalkSpeed = 0.03f;
	static constexpr float kMaxTurnAngle = 0.05f;
	static constexpr float kMaxFaceTowardChargeTime = 5.0f;
	static constexpr float kMaxRushChargeTime = 1.0f;
	static constexpr float kMaxRushTime = 3.0f;
	RushAttack rushAttack;
	float rushT = 0.0f;


	//尻尾
	Model* tailModel_ = nullptr;
	std::vector<std::unique_ptr<BossTail>> bossTails_;
	BossTail* bossTail_;
	static constexpr int kDefaultMaxTail = 5;
	static constexpr int kMaxTail = 30;
	static constexpr int kTailInterval = 30;
	static constexpr int kMaxFootprint = kMaxTail * kTailInterval + 1;
	std::list<Footprint> footprints_;
	static constexpr float kTailRadius = 0.5f;
	static constexpr float kHeadBodyRadius = 0.75f;

	//口
	BossTail mouth;
	static constexpr float kMouthRange = 1.0f;
	static constexpr float kMouthRadius = 0.5f;
};
