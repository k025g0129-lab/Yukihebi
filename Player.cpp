#include "Player.h"

#include <algorithm>
#include <cmath>
#include "GameMath.h"
#include "MapChipField.h"
XINPUT_STATE Xstate;
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
	// BOSSの初期位置(原点)と重ならない位置から開始する。
	worldTransform_.translation_ = spawnPosition_;

	// 初期の回転を設定
	worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};

	// Player.obj は各軸 -1～+1。表示サイズと判定サイズを一致させる。
	mapCollider_.SetSize(worldTransform_.scale_.x * 2.0f, worldTransform_.scale_.y * 2.0f);

	// SetMapChipField() で地形情報を設定するまで、衝突判定は行わない。
	verticalSpeed_ = 0.0f;
	velocity_ = {};
	hp_ = maxHp_;
	damageInvincibilityRemaining_ = 0.0f;

	// worldTransform_ の行列を更新
	worldTransform_.matWorld_ = MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// ワールド変換行列をシェーダに転送
	worldTransform_.TransferMatrix();
	// 色の初期化
	color_.Initialize();
	color_.SetColor({1.0f, 0.25f, 0.15f, 1.0f});
	numberOfJoysticks = Input::GetInstance()->GetNumberOfJoysticks();
	// 雪玉の初期化
	snows_.clear();
	snow_ = nullptr;
}

void Player::Update(float deltaTime) {
	if (deltaTime <= 0.0f) {
		return;
	}
	damageInvincibilityRemaining_ = (std::max)(0.0f, damageInvincibilityRemaining_ - deltaTime);
	if (IsDead()) {
		return;
	}
	Input::GetInstance()->GetJoystickState(0, Xstate);
	//==========================
	// 入力処理
	//==========================
	const Input* input = Input::GetInstance();
	Vector3 direction{};


	//=========================
	// 移動処理
	//=========================

	if (numberOfJoysticks > 0) {
		direction.x = Xstate.Gamepad.sThumbLX / 32768.0f; // XZ平面の移動方向を決定する。左に移動する場合は、X軸方向に-1.0fを加算する。
		direction.z += Xstate.Gamepad.sThumbLY / 32768.0f;
	}
	if ((input->PushKey(DIK_A) || input->PushKey(DIK_LEFT))) {
		direction.x -= 1.0f; // XZ平面の移動方向を決定する。左に移動する場合は、X軸方向に-1.0fを加算する。
	}

	if ((input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT))) {
		direction.x += 1.0f; // XZ平面の移動方向を決定する。右に移動する場合は、X軸方向に+1.0fを加算する。
	}
	if ((input->PushKey(DIK_W) || input->PushKey(DIK_UP))) {
		direction.z += 1.0f; // XZ平面の移動方向を決定する。前に移動する場合は、Z軸方向に+1.0fを加算する。
	}
	if ((input->PushKey(DIK_S) || input->PushKey(DIK_DOWN))) {
		direction.z -= 1.0f; // XZ平面の移動方向を決定する。後ろに移動する場合は、Z軸方向に-1.0fを加算する。
	}


	// 斜め移動だけ速くならないよう、入力方向を正規化する。
	const float lengthSquared = direction.x * direction.x + direction.z * direction.z; // XZ平面の長さの二乗を計算する。

	// ==========================
	// 移動処理
	// ==========================


	// 方向ベクトルがゼロでない場合のみ、正規化して移動量を計算する。
	if (lengthSquared > 0.0f) {
		// 正規化して向きベクトルを求める
		const float inverseLength = 1.0f / std::sqrt(lengthSquared); // XZ平面の長さの逆数を計算する。

		// 向きベクトルを保持しておく
		facing_.x = direction.x * inverseLength; // XZ平面の向きベクトル
		facing_.y = 0.0f; // Y成分は常に0
		facing_.z = direction.z * inverseLength; // XZ平面の向きベクトル
		// プレイヤーの向きを、facing_ の方向に回転させる。
		const float targetYaw = std::atan2(facing_.x, facing_.z); // facing_ の方向の角度を求める
		const float currentYaw = worldTransform_.rotation_.y; // 現在の角度を取得

		// -π～πの最短回転角を求める
		const float difference = std::atan2(std::sin(targetYaw - currentYaw), std::cos(targetYaw - currentYaw));
		// -π～πの最短回転角を求める


		const float maxTurn = turnSpeed_ * deltaTime;
		worldTransform_.rotation_.y += std::clamp(difference, -maxTurn, maxTurn);
	}

	// 雪玉が大きくなるほど、歩行の加速と最高速度に負荷が掛かる。
	const float load = snow_ != nullptr ? snow_->GetMass() * pushLoadFactor_ : 0.0f;
	const float loadRatio = 1.0f / (1.0f + load);
	const Vector3 targetVelocity = lengthSquared > 0.0f
		                               ? facing_ * (moveSpeed_ * loadRatio)
		                               : Vector3{};
	Vector3 horizontal = {velocity_.x, 0.0f, velocity_.z};
	const Vector3 velocityDifference = targetVelocity - horizontal;
	const float differenceLength = Length(velocityDifference);
	const float acceleration = lengthSquared > 0.0f
		                           ? moveAcceleration_ * loadRatio
		                           : moveDeceleration_;
	const float maxChange = acceleration * deltaTime;
	if (differenceLength > 0.0f) {
		horizontal += velocityDifference * ((std::min)(1.0f, maxChange / differenceLength));
	}
	velocity_.x = horizontal.x;
	velocity_.z = horizontal.z;

	// ==========================
	// 重力処理
	// ==========================
	verticalSpeed_ = (std::max)(verticalSpeed_ - gravity_ * deltaTime, -maxFallSpeed_);
	// Y方向の速度を更新する。重力加速度を加算する。最大落下速度を超えないように制限する。

	// y方向の移動量を加算する
	velocity_.y = verticalSpeed_; // Y方向の移動量を加算する。重力処理で計算したY方向の速度を加算する。

	// 床ブロックの上面・側面・下面との接触を MapCollider に解決させる。
	const MapCollider::Result collision = mapCollider_.Check(worldTransform_.translation_, velocity_ * deltaTime);
	// 接触判定を行う。接触判定の結果、移動量が修正される。
	worldTransform_.translation_ += collision.movement; // 接触判定後の移動量を加算する
	// 壁で止まった方向の速度を残すと、解放した雪玉に架空の勢いが伝わる。
	velocity_.x = collision.movement.x / deltaTime;
	velocity_.z = collision.movement.z / deltaTime;
	// 接触判定の結果、床に接している場合は、y方向の速度を0にする。
	if ((collision.landing && verticalSpeed_ < 0.0f) ||
		(collision.ceiling && verticalSpeed_ > 0.0f)) {
		// 接触している場合は、y方向の速度を0にする。
		verticalSpeed_ = 0.0f;
	}
	velocity_.y = verticalSpeed_;

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
		snow_->UpdateHeld(worldTransform_.translation_, velocity_, worldTransform_.rotation_.y,
		                  worldTransform_.scale_.x, MapChipField::kTopDownFloorTop, 0.0f, mapChipField_);
	}

	// Space中だけPlayerの正面へ押し続ける。離した後は雪玉自身の物理で転がる。
	if (snow_ != nullptr) {
		const bool holding = input->PushKey(DIK_SPACE); // 雪玉を持っているかどうか
		if (holding) {
			snow_->UpdateHeld(worldTransform_.translation_, velocity_, worldTransform_.rotation_.y,
			                  worldTransform_.scale_.x, MapChipField::kTopDownFloorTop, deltaTime, mapChipField_);
		}
		else {
			snow_->Release();
			snow_ = nullptr; // 置く。snows_の要素は消さない
		}
	}
	// Eキーで、手から離れた最も近い雪玉を向いている方向へ蹴る。
	if (input->TriggerKey(kickKey_) && snow_ == nullptr) {
		Snow* nearest = nullptr;
		float nearestDistanceSquared = 0.0f;
		for (Snow& snow : snows_) {
			const Vector3 offset = snow.GetPos() - worldTransform_.translation_;
			const float horizontalDistanceSquared = offset.x * offset.x + offset.z * offset.z;
			const float allowedDistance = GetCollisionRadius() + snow.GetRadius() + kickReach_;
			if (std::abs(offset.y) <= allowedDistance &&
			    horizontalDistanceSquared <= allowedDistance * allowedDistance &&
			    (nearest == nullptr || horizontalDistanceSquared < nearestDistanceSquared)) {
				nearest = &snow;
				nearestDistanceSquared = horizontalDistanceSquared;
			}
		}
		if (nearest != nullptr) {
			nearest->Kick(facing_, kickSpeed_);
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
	mapChipField_ = field;
	mapCollider_.SetMapChipField(field);
	mapCollider_.SetTopDownMode(true);
}

void Player::UpdateSnowPhysics(float deltaTime, MapChipField& map) {
	for (auto it = snows_.begin(); it != snows_.end();) {
		it->UpdatePhysics(deltaTime, map, worldTransform_.translation_, velocity_,
		                   worldTransform_.scale_.x);
		if (&*it != snow_ && it->GetPos().y < snowDespawnY_) {
			it = snows_.erase(it);
		} else {
			++it;
		}
	}

	for (auto first = snows_.begin(); first != snows_.end(); ++first) {
		auto second = first;
		++second;
		for (; second != snows_.end(); ++second) {
			first->SnowToSnowCollision(*second);
		}
	}
}

bool Player::ApplyDamage(int damage) {
	if (damage <= 0 || IsDead() || damageInvincibilityRemaining_ > 0.0f) {
		return false;
	}
	hp_ = (std::max)(0, hp_ - damage);
	damageInvincibilityRemaining_ = damageInvincibilitySeconds_;
	return true;
}

std::list<Snow>::iterator Player::RemoveSnow(std::list<Snow>::iterator it) {
	if (it == snows_.end()) {
		return it;
	}
	if (&*it == snow_) {
		snow_ = nullptr;
	}
	return snows_.erase(it);
}

void Player::DrainSnowEffectEvents(std::vector<Snow::EffectEvent>& events) {
	for (Snow& snow : snows_) {
		snow.DrainEffectEvents(events);
	}
}
