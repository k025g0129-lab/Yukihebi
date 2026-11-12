#pragma once
#include <KamataEngine.h>
#include "GameMath.h"
using namespace KamataEngine;
class MapChipField;

class Snow {
public:
	//==========================
	// 雪玉の設定
	//==========================

	struct Settings {
		float initialRadius = 0.2f;
		float maxRadius = 1.0f;
		float growthPerDistance = 0.25f;
		float placementGap = 0.05f;

		float baseMass = 1.0f;
		float gravity = 9.8f;
		float pushGain = 12.0f;
		float pushResponse = 14.0f;
		float maxPushSpeed = 8.0f;
		float maxPushForce = 120.0f;

		float friction = 0.45f;
		float rollingResistance = 0.6f;
		float rollingDeceleration = 1.2f;
		float wallRestitution = 0.65f;
		float floorRestitution = 0.0f;
		float bounceThreshold = 0.5f;
		float stopSpeed = 0.08f;

		float contactMargin = 0.0001f;
		float movementStepRadiusRatio = 0.5f;
		int maxMovementSteps = 32;
		int collisionIterations = 4;
		float playerContactSpeed = 2.0f;
		float squashAmount = 0.35f;
		float squashTime = 0.09f;
		float squashImpulse = 0.4f;
	};

	void SetSettings(const Settings& settings) { settings_ = settings; }
	void Release();

	//==========================
	// コンストラクタ・デストラクタ
	//==========================
	Snow(); // コンストラクタ
	~Snow(); // デストラクタ
	//==========================
	// メンバ関数
	//==========================
	void Initialize(const Vector3& position); // 初期化
	void UpdatePhysics(float deltaTime, MapChipField& map, const Vector3& playerPosition, float playerYaw,
	                   float playerHalfWidth); // 更新
	void Draw(Model* model, const Camera* camera); // 描画
	//==========================
	// 雪玉を持っているときの位置更新
	//==========================
	void UpdateHeld(const Vector3& playerPosition, float playerYaw, float movedDistance, float playerHalfWidth,
	                float floorY); // プレイヤーが雪玉を持っているときの位置更新。プレイヤーの正面方向に置く
	//==========================
	// セッター
	//	=========================
	void SetGrowthPerDistance(float value) {
		growthPerDistance_ = value;
		settings_.growthPerDistance = value;
	}

	void SetMaxRadius(float value) {
		maxRadius_ = value;
		settings_.maxRadius = value;
	}

	//==========================
	// ゲッター
	//==========================
	const WorldTransform& GetWorldTransform() const { return worldTransform_; } // ワールド変換行列を取得する

	//==========================
	// プレイヤーとの接触判定
	//==========================
	void PlayerToSnowCollsion(const Vector3& playerposition, float playerHalfWidth);
	// プレイヤーと雪玉の接触判定。プレイヤーが雪玉に接触した場合、雪玉を持つようにする

private:
	//==========================
	// メンバ変数
	//==========================
	WorldTransform worldTransform_; // ワールド変換行列を保持する

	Vector3 position_; // 雪玉の位置
	Vector3 velocity_; // 雪玉の速度
	Vector3 acceleration_; // 雪玉の加速度
	Vector3 size_; // 雪玉の大きさ
	Vector3 rotation_; // 雪玉の回転角
	Vector3 angularVelocity_{}; // 接触の摩擦から得る角速度
	Vector3 squashNormal_{}; // 強く当たった面の法線
	float squashTimer_ = 0.0f; // 衝突時のつぶれが残る時間
	Vector3 rotationSpeed_; // 雪玉の回転速度
	Vector3 rotationAcceleration_; // 雪玉の回転加速度

	float radius_ = 0.2f; // 雪玉の半径
	float growthPerDistance_ = 0.25f; // 移動距離に応じた成長量
	float maxRadius_ = 1.0f; // 雪玉の最大半径
	float placementGap_ = 0.05f; // プレイヤーと雪玉の間の隙間
	enum class State { kPushed, kFree, kStopped };

	void ResolveMap(MapChipField& map);
	void PrepareDraw();
	Settings settings_{};
	State state_ = State::kStopped;
	bool grounded_ = false;
};
