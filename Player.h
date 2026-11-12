#pragma once

#include <list>
#include "Snow.h"

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
	void Update(); // 更新
	void UpdateSnowPhysics(float deltaTime, MapChipField& map); // 雪玉の物理更新
	void Draw(); // 描画
	//==========================
	// セッター
	//=========================

	void SetMoveSpeed(float speed) { moveSpeed_ = speed; } // 1フレームあたりの移動量を設定する
	void SetMapChipField(MapChipField* field); // MapChipField を設定する。MapCollider に地形情報を渡す
	void SetGravityPerFrame(float gravity) { gravityPerFrame_ = gravity; } // 1フレームあたりの重力加速度を設定する
	void SetSpaceRiseSpeed(float speed) { spaceRiseSpeed_ = speed; } // スペースキーで上昇する速度を設定する
	void SetMaxFallSpeed(float speed) { maxFallSpeed_ = speed; } // 最大落下速度を設定する

private:
	//==========================
	// メンバ変数
	//==========================
	MapCollider mapCollider_{}; // MapCollider を使って、床ブロックの上面・側面・下面との接触を解決する
	const Camera* camera_ = nullptr; // カメラの参照を保持する
	WorldTransform worldTransform_{}; // ワールド変換行列を保持する
	ObjectColor color_{}; // 色を保持する
	Model* model_ = nullptr; // プレイヤーのモデルを保持する
	Model* snowModel_ = nullptr; // 雪玉のモデルを保持する
	float moveSpeed_ = 0.1f; // 1 フレームあたりの移動量
	float gravityPerFrame_ = 0.02f; // 1 フレームあたりの重力加速度
	float spaceRiseSpeed_ = 1.0f; // スペースキーで上昇する速度
	float maxFallSpeed_ = 1.0f; // 最大落下速度
	float verticalSpeed_ = 0.0f; // Y方向の速度
	Vector3 facing_ = {0.0f, 0.0f, 1.0f}; // 最後に向いていた方向
	Snow* snow_ = nullptr; // プレイヤーが持っている雪玉のポインタ。nullptr の場合は、雪玉を持っていない
	std::list<Snow> snows_; // プレイヤーが生成した雪玉のリスト。雪玉を置いた場合でも、snows_ の要素は消さない
	// snows_ の要素があるかどうかを保持するフラグ。snows_ の要素がある場合は true、ない場合は false
	bool snowsHave = false;
};
