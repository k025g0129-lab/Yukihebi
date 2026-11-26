#pragma once

#include <list>
#include <vector>
#include "Snow.h"
#include "XInput.h"

#include "MapCollider.h"

class MapChipField;


class Snow;

class Player {
public:
	//==========================
	// コンストラクタ・デストラクタ
	//==========================
	Player(); // コンストラクタ
	~Player(); // デストラクタ

	//==========================
	// メンバ関数
	// ==========================
	void Initialize(Model* model, Model* snowModel, const Camera* camera); // 初期化
	void Update(float deltaTime); // 秒単位の経過時間で更新
	void UpdateSnowPhysics(float deltaTime, MapChipField& map); // 雪玉の物理更新
	void DrainSnowEffectEvents(std::vector<Snow::EffectEvent>& events); // 雪玉からの演出通知を集める
	void Draw(); // 描画
	//==========================
	// セッター
	//=========================

	void SetMoveSpeed(float speed) { moveSpeed_ = speed * 60.0f; } // 従来の1フレームあたりの移動量を設定する
	void SetMoveSpeedPerSecond(float speed) { moveSpeed_ = speed; }
	void SetMoveAcceleration(float acceleration) { moveAcceleration_ = acceleration; }
	void SetMoveDeceleration(float deceleration) { moveDeceleration_ = deceleration; }
	void SetTurnSpeed(float radiansPerSecond) { turnSpeed_ = radiansPerSecond; }
	void SetPushLoadFactor(float factor) { pushLoadFactor_ = factor > 0.0f ? factor : 0.0f; } // 雪玉の質量による移動速度・加速度への影響
	void SetMapChipField(MapChipField* field); // MapChipField を設定する。MapCollider に地形情報を渡す
	void SetGravityPerFrame(float gravity) { gravity_ = gravity * 60.0f * 60.0f; } // 従来のフレーム単位の値を換算する
	void SetSpaceRiseSpeed(float speed) { spaceRiseSpeed_ = speed; } // スペースキーで上昇する速度を設定する
	void SetMaxFallSpeed(float speed) { maxFallSpeed_ = speed * 60.0f; } // 従来のフレーム単位の値を換算する
	void SetMaxHp(int hp) {
		maxHp_ = hp > 0 ? hp : 1;
		hp_ = maxHp_;
	}

	void SetDamageInvincibilitySeconds(float seconds) { damageInvincibilitySeconds_ = seconds > 0.0f ? seconds : 0.0f; }
	void SetKickSpeed(float speed) { kickSpeed_ = speed > 0.0f ? speed : 0.0f; }
	void SetKickReach(float reach) { kickReach_ = reach > 0.0f ? reach : 0.0f; }
	void SetKickKey(BYTE key) { kickKey_ = key; }
	void SetSnowDespawnY(float y) { snowDespawnY_ = y; }
	void SetSpawnPosition(const Vector3& position) { spawnPosition_ = position; }
	bool ApplyDamage(int damage);
	void ApplyKnockback(const Vector3& direction, float speed);
	// 水平面で球の外へ押し出し、球へ向かう速度を消す。重なっていたら true を返す。
	bool PushOutFromSphere(const Vector3& center, float radius);

	Vector3 GetRotate() { return worldTransform_.rotation_; };
	Vector3 GetVector3() { return worldTransform_.translation_; };
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }
	const Vector3& GetVelocity() const { return velocity_; }
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	float GetCollisionRadius() const { return worldTransform_.scale_.x; }
	bool IsDead() const { return hp_ <= 0; }
	std::list<Snow>& GetSnows() { return snows_; };
	std::list<Snow>::iterator RemoveSnow(std::list<Snow>::iterator it);

private:
	// マップの外周を見えない壁として扱い、場外へ出ないようにする。
	void ClampToStage();
	void UpdateMatrix();

	//==========================
	// メンバ変数
	//==========================
	MapCollider mapCollider_{}; // MapCollider を使って、床ブロックの上面・側面・下面との接触を解決する
	MapChipField* mapChipField_ = nullptr; // 押している雪玉の接地・壁判定に使用する
	const Camera* camera_ = nullptr; // カメラの参照を保持する
	WorldTransform worldTransform_{}; // ワールド変換行列を保持する
	ObjectColor color_{}; // 色を保持する
	Model* model_ = nullptr; // プレイヤーのモデルを保持する
	Model* snowModel_ = nullptr; // 雪玉のモデルを保持する
	float moveSpeed_ = 6.0f; // 毎秒の最高移動速度
	float moveAcceleration_ = 30.0f; // 毎秒の水平加速度
	float moveDeceleration_ = 36.0f; // 入力を離したときの減速度
	float turnSpeed_ = ToRadian(360.0f); // 毎秒の最大回転角
	float pushLoadFactor_ = 0.02f; // 雪玉の質量1あたりの負荷
	float gravity_ = 9.8f; // 毎秒の重力加速度。雪玉と同じ単位・初期値にする
	float spaceRiseSpeed_ = 1.0f; // スペースキーで上昇する速度
	float maxFallSpeed_ = 60.0f; // 毎秒の最大落下速度
	float verticalSpeed_ = 0.0f; // 毎秒のY方向速度
	Vector3 facing_ = {0.0f, 0.0f, 1.0f}; // 最後に向いていた方向
	Vector3 velocity_ = {}; // 毎秒のプレイヤー速度
	Snow* snow_ = nullptr; // プレイヤーが持っている雪玉のポインタ。nullptr の場合は、雪玉を持っていない
	std::list<Snow> snows_; // 生成した雪玉。場外へ落ちた要素は物理更新時に取り除く
	int maxHp_ = 5;
	int hp_ = 5;
	float damageInvincibilitySeconds_ = 0.75f;
	float damageInvincibilityRemaining_ = 0.0f;
	float kickSpeed_ = 8.0f;
	float kickReach_ = 0.5f; // プレイヤーと雪玉の表面間の許容距離
	BYTE kickKey_ = DIK_E;
	float snowDespawnY_ = -5.0f;
	Vector3 spawnPosition_ = {0.0f, 0.5f, -4.0f};
	// snows_ の要素があるかどうかを保持するフラグ。snows_ の要素がある場合は true、ない場合は false
	bool snowsHave = false;
	size_t numberOfJoysticks = 0; // 接続されているジョイスティックの数	
};
