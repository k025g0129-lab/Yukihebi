#include "Player.h"

#include <algorithm>
#include <cmath>
#include "GameMath.h"
#include "MapChipField.h"

Player::Player() = default;
Player::~Player() = default;


void Player::Initialize(Model* moder, Model* snowModel, const Camera* camera) {
	//==========================
	// 初期化
	//=========================

	// モデルとカメラを設定
	camera_ = camera;
	model_ = moder;
	snowModel_ = snowModel;

	// ワールド変換の初期化
	worldTransform_.Initialize();

	// 初期の大きさを設定
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};

	// 初期の位置を設定
	worldTransform_.translation_ = {0.0f, 0.5f, 0.0f};

	// 初期の回転を設定
	worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};

	// Player.obj は各軸 -1～+1。表示サイズと判定サイズを一致させる。
	mapCollider_.SetSize(worldTransform_.scale_.x * 2.0f, worldTransform_.scale_.y * 2.0f);

	// SetMapChipField() で地形情報を設定するまで、衝突判定は行わない。
	verticalSpeed_ = 0.0f;

	// worldTransform_ の行列を更新
	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// ワールド変換行列をシェーダに転送
	worldTransform_.TransferMatrix();
	// 色の初期化
	color_.Initialize();
	color_.SetColor({1.0f, 0.25f, 0.15f, 1.0f});

	// 雪玉の初期化
	snows_.clear();
	snow_ = nullptr;
}

void Player::Update() {
	//==========================
	// 入力処理
	//==========================
	const Input* input = Input::GetInstance();
	Vector3 direction{};


	//=========================
	// 移動処理
	//=========================


	if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
		direction.x -= 1.0f; // XZ平面の移動方向を決定する。左に移動する場合は、X軸方向に-1.0fを加算する。
	}
	if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
		direction.x += 1.0f; // XZ平面の移動方向を決定する。右に移動する場合は、X軸方向に+1.0fを加算する。
	}
	if (input->PushKey(DIK_W) || input->PushKey(DIK_UP)) {
		direction.z += 1.0f; // XZ平面の移動方向を決定する。前に移動する場合は、Z軸方向に+1.0fを加算する。
	}
	if (input->PushKey(DIK_S) || input->PushKey(DIK_DOWN)) {
		direction.z -= 1.0f; // XZ平面の移動方向を決定する。後ろに移動する場合は、Z軸方向に-1.0fを加算する。
	}

	// 斜め移動だけ速くならないよう、入力方向を正規化する。
	const float lengthSquared = direction.x * direction.x + direction.z * direction.z; // XZ平面の長さの二乗を計算する。

	// ==========================
	// 移動処理
	// ==========================
	Vector3 movement{}; // XZ平面の移動量を計算する。Y方向の移動量は、重力処理で加算する。

	// 方向ベクトルがゼロでない場合のみ、正規化して移動量を計算する。
	if (lengthSquared > 0.0f) {
		// 正規化して向きベクトルを求める
		const float inverseLength = 1.0f / std::sqrt(lengthSquared); // XZ平面の長さの逆数を計算する。

		// 向きベクトルを保持しておく
		facing_.x = direction.x * inverseLength; // XZ平面の向きベクトル
		facing_.y = 0.0f; // Y成分は常に0
		facing_.z = direction.z * inverseLength; // XZ平面の向きベクトル

		// 移動量を計算する
		movement.x = facing_.x * moveSpeed_; // XZ平面の移動量
		movement.z = facing_.z * moveSpeed_; // XZ平面の移動量

		// プレイヤーの向きを、facing_ の方向に回転させる。
		const float targetYaw = std::atan2(facing_.x, facing_.z); // facing_ の方向の角度を求める
		const float currentYaw = worldTransform_.rotation_.y; // 現在の角度を取得

		// -π～πの最短回転角を求める
		const float difference = std::atan2(std::sin(targetYaw - currentYaw), std::cos(targetYaw - currentYaw));
		// -π～πの最短回転角を求める


		constexpr float turnSpeed = ToRadian(6.0f); // 1フレームに最大6度
		worldTransform_.rotation_.y += std::clamp(difference, -turnSpeed, turnSpeed); // 回転量を制限する
	}

	// ==========================
	// 重力処理
	// ==========================
	verticalSpeed_ = (std::max)(verticalSpeed_ - gravityPerFrame_, -maxFallSpeed_);
	// Y方向の速度を更新する。重力加速度を加算する。最大落下速度を超えないように制限する。

	// y方向の移動量を加算する
	movement.y = verticalSpeed_; // Y方向の移動量を加算する。重力処理で計算したY方向の速度を加算する。

	// 床ブロックの上面・側面・下面との接触を MapCollider に解決させる。
	const MapCollider::Result collision = mapCollider_.Check(worldTransform_.translation_, movement);
	// 接触判定を行う。接触判定の結果、移動量が修正される。
	worldTransform_.translation_ += collision.movement; // 接触判定後の移動量を加算する
	// 接触判定の結果、床に接している場合は、y方向の速度を0にする。
	if ((collision.landing && verticalSpeed_ < 0.0f) ||
		(collision.ceiling && verticalSpeed_ > 0.0f)) {
		// 接触している場合は、y方向の速度を0にする。
		verticalSpeed_ = 0.0f;
	}

	// ==========================
	// 雪玉の生成・更新
	// ==========================

	// 雪玉を生成する。すでに雪玉を持っている場合は、雪玉の位置を更新する。
	if (input->TriggerKey(DIK_SPACE) && snow_ == nullptr) {
		// 雪玉を生成する。snows_ に追加する。
		snows_.emplace_back();
		// 生成した雪玉を保持する。snows_ の最後の要素を取得する。
		snow_ = &snows_.back();
		// 雪玉の初期化。プレイヤーの位置から生成する。
		snow_->Initialize(worldTransform_.translation_);
		snow_->UpdateHeld(worldTransform_.translation_, worldTransform_.rotation_.y, 0.0f, worldTransform_.scale_.x,
		                  MapChipField::kTopDownFloorTop);
	}

	// 雪玉を持っている場合は、雪玉の位置を更新する。プレイヤーの正面方向に置く。
	if (snow_ != nullptr) {
		//=========================
		// 雪玉の位置を更新する
		//=========================
		const bool holding = input->PushKey(DIK_SPACE); // 雪玉を持っているかどうか


		// 壁で止められた分は成長に数えない
		const Vector3 actualMove = {collision.movement.x / 10, 0.0f, collision.movement.z / 10};
		// XZ平面の移動量を計算する。Y方向の移動量は無視する。

		// 雪玉の位置を更新する。プレイヤーの正面方向に置く。
		snow_->UpdateHeld(
			worldTransform_.translation_, worldTransform_.rotation_.y, // 移動距離を成長量に加算する。XZ平面の移動量を計算する。
			holding ? Length(actualMove) : 0.0f, worldTransform_.scale_.x,
			MapChipField::kTopDownFloorTop);

		//=========================
		// 雪玉を置く
		//=========================
		if (!holding) {
			snow_->Release();
			snow_ = nullptr; // 置く。snows_の要素は消さない
		}
	}


	//
	// ワールド変換行列を更新
	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix(); // ワールド変換行列をシェーダに転送
}

void Player::Draw() {
	//==========================
	// 描画処理
	//==========================
	model_->Draw(worldTransform_, *camera_);
	// snows_ の描画
	for (Snow& snow : snows_) {
		//==========================
		// 雪玉の描画処理
		//==========================
		snow.Draw(snowModel_, camera_);
	}
}

void Player::SetMapChipField(MapChipField* field) {
	mapCollider_.SetMapChipField(field);
	mapCollider_.SetTopDownMode(true);
}

void Player::UpdateSnowPhysics(float deltaTime, MapChipField& map) {
	for (Snow& snow : snows_) {
		snow.UpdatePhysics(deltaTime, map, worldTransform_.translation_, worldTransform_.rotation_.y,
		                   worldTransform_.scale_.x);
	}
}
