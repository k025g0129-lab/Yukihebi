#pragma once
#include <KamataEngine.h>
#include <vector>
#include "GameMath.h"

using namespace KamataEngine;
class MapChipField;
class Boss;

class Snow {
public:
	//==========================
	// 雪玉の設定
	//==========================

	struct Settings {
		float initialRadius = 0.2f;
		float maxRadius = 0.7f;
		float growthPerDistance = 0.25f;
		float placementGap = 0.05f;

		float baseMass = 1.0f;
		float playerMass = 10.0f; // 放した雪玉をPlayerが押すときの質量比
		float gravity = 9.8f;

		float friction = 0.45f;
		float rollingResistance = 0.2f;
		float rollingDeceleration = 0.4f;
		float rollingMassInfluence = 0.05f; // 半径による質量増加が減速へ与える割合
		float wallRestitution = 0.65f;
		float floorRestitution = 0.0f;
		float bounceThreshold = 0.5f;
		float stopSpeed = 0.08f;

		float contactMargin = 0.0001f;
		float movementStepRadiusRatio = 0.5f;
		int maxMovementSteps = 32;
		int collisionIterations = 4;
		float playerRestitution = 0.0f; // Playerに押されたときの反発
		float squashAmount = 0.35f;
		float squashTime = 0.09f;
		float squashImpulse = 0.4f;


		float snowRestitution = 0.25f; // 雪玉同士の反発
		float playerEffectMinSpeed = 0.35f; // Playerへの接近速度がこれ以上なら粒を出す
		float snowEffectMinSpeed = 0.35f; // 雪玉同士の接近速度がこれ以上なら粒を出す
		float wallEffectMinSpeed = 1.0f; // ステージ外周への衝突速度がこれ以上なら粒を出す
		float maxSizePulseAmount = 0.3f; // 最大サイズ到達時に一瞬膨らむ割合
		float maxSizePulseTime = 0.25f; // 膨らみが戻るまでの秒数
	};

	// 判定側は発生事実だけを通知し、粒の色や数はGameSceneで決める。
	enum class EffectType { kPlayerHit, kSnowHit, kMaxSize, kWallHit, kKick, kCreate };

	struct EffectEvent {
		EffectType type;
		Vector3 position;
		Vector3 normal;
		float radius;
		float impactSpeed;
	};

	void SetSettings(const Settings& settings) { settings_ = settings; }
	void Release();
	void Kick(const Vector3& direction, float speed);

	//==========================
	// コンストラクタ・デストラクタ
	//==========================
	Snow(); // コンストラクタ
	~Snow(); // デストラクタ
	//==========================
	// メンバ関数
	//==========================
	void Initialize(const Vector3& position); // 初期化
	void UpdatePhysics(float deltaTime, MapChipField& map, const Vector3& playerPosition, const Vector3& playerVelocity,
	                   float playerHalfWidth); // 更新
	void Draw(Model* model, const Camera* camera); // 描画
	void DrainEffectEvents(std::vector<EffectEvent>& events); // 発生済みイベントを移して消す
	void NotifyCreated(); // 生成して正面に配置した直後に呼ぶ。生成の演出を通知する
	//==========================
	// 雪玉を持っているときの位置更新
	//==========================
	void UpdateHeld(const Vector3& playerPosition, const Vector3& playerVelocity, float playerYaw,
	                float playerHalfWidth, float floorY, float deltaTime, MapChipField* map); // Space中の押し位置と転がりを更新する
	//==========================
	// セッター
	//	=========================
	void SetGrowthPerDistance(float value) {
		settings_.growthPerDistance = value;
	}

	void SetMaxRadius(float value) {
		settings_.maxRadius = value;
	}

	//==========================
	// ゲッター
	//==========================
	const WorldTransform& GetWorldTransform() const { return worldTransform_; } // ワールド変換行列を取得する
	float GetMass() const; // 半径に応じた質量

	const Vector3& GetPos() const { return position_; };
	const Vector3& GetVelocity() const { return velocity_; }
	const float& GetRadius() const { return radius_; };
	bool IsHeld() const { return state_ == State::kPushed; }
	bool IsSizeMax() const;
	float GetMaxRadius() const { return settings_.maxRadius; }
	//=====================
	// 接触判定
	//=====================


	/** プレイヤーと雪玉の接触判定 */
	void PlayerToSnowCollision(const Vector3& playerposition, const Vector3& playerVelocity,
	                           float playerHalfWidth);

	/** 雪玉と雪玉の接触判定 */
	void SnowToSnowCollision(Snow& other);

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
	float pulseTimer_ = 0.0f; // 最大サイズ到達時の膨らみが残る時間
	Vector3 rotationSpeed_; // 雪玉の回転速度
	Vector3 rotationAcceleration_; // 雪玉の回転加速度

	float radius_ = 0.2f; // 雪玉の半径
	enum class State { kPushed, kFree, kStopped };

	void ResolveMap(MapChipField& map);
	void ConstrainToStage(const MapChipField& map);
	void PrepareDraw();
	void UpdatePulse(float deltaTime);
	void NotifyMaxSizeReached(float previousRadius);
	// outward は雪玉から壁へ向かう向き。impactSpeed は壁へ向かっていた速さ。
	void NotifyWallHit(const Vector3& outward, float impactSpeed);
	Settings settings_{};
	State state_ = State::kStopped;
	bool grounded_ = false;
	std::vector<EffectEvent> pendingEffectEvents_;
};
