#include "Snow.h"
#include <algorithm>
#include <cmath>
#include "MapChipField.h"
Snow::Snow() = default;

Snow::~Snow() {
}

void Snow::Initialize(const Vector3& position) {
	//===========================
	// 初期化
	//===========================

	// ワールド変換の初期化
	worldTransform_.Initialize();

	// プレイヤーの位置から生成
	position_ = position;
	// 雪玉の初期位置をプレイヤーの正面方向に置く
	position_.z += 1.5f;

	// 雪玉の大きさ
	radius_ = settings_.initialRadius;
	size_ = {radius_, radius_, radius_};

	// 初期値
	velocity_ = {};
	angularVelocity_ = {};
	squashNormal_ = {};
	squashTimer_ = 0.0f;
	grounded_ = false;
	state_ = State::kPushed;
	acceleration_ = {0.0f, -0.1f, 0.0f};
	rotation_ = {0.0f, 0.0f, 0.0f};
	rotationSpeed_ = {0.0f, 0.0f, 0.0f};
	rotationAcceleration_ = {0.0f, 0.0f, 0.0f};

	// 描画用の座標を設定
	worldTransform_.translation_ = position_;
	worldTransform_.scale_ = size_;
	worldTransform_.rotation_ = rotation_;

	// ワールド変換行列を更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_,
	                                             worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}


void Snow::Draw(Model* model, const Camera* camera) {
	//===========================
	// 描画
	//===========================
	model->Draw(worldTransform_, *camera);
}

//===========================
// 雪玉を持っているときの位置更新
//===========================
void Snow::UpdateHeld(const Vector3& playerPosition, float playerYaw, float movedDistance, float playerHalfWidth,
                      float floorY) {
	// 移動距離に応じて雪玉の半径を成長させる。最大半径を超えないように制限する。
	radius_ = (std::min)(radius_ + movedDistance * growthPerDistance_, maxRadius_);
	// 移動距離に応じた成長量を加算する。最大半径を超えないように制限する。

	// Playerの表示上の正面方向（+Zを正面とする）
	const Vector3 forward = {std::sin(playerYaw), 0.0f, std::cos(playerYaw)}; // プレイヤーの正面方向の単位ベクトルを計算する。

	// 雪玉の位置をプレイヤーの正面方向に置く。プレイヤーと雪玉の間に隙間を設ける。
	position_ = playerPosition + forward * (playerHalfWidth + radius_ + placementGap_);
	// 雪玉の位置をプレイヤーの正面方向に置く。プレイヤーと雪玉の間に隙間を設ける。
	position_.y = floorY + radius_; // 雪玉の位置のY座標を床の上に置く。雪玉の半径分だけ上に置く。

	//===========================
	// ワールド変換行列を更新
	//===========================
	worldTransform_.translation_ = position_; // 雪玉の位置を更新する
	worldTransform_.scale_ = {radius_, radius_, radius_}; // 雪玉の大きさを更新する
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_,
	                                             worldTransform_.translation_); // ワールド変換行列を更新する
	worldTransform_.TransferMatrix();
}

void Snow::PlayerToSnowCollsion(const Vector3& playerposition, float playerHalfWidth) {
	if (state_ == State::kPushed) {
		return; // 持って転がしている雪玉には、この接触処理を重ねない
	}
	const Vector3 minimum = {
		+playerposition.x - playerHalfWidth, +playerposition.y - playerHalfWidth, +playerposition.z - playerHalfWidth
	};
	const Vector3 maximum = {
		+playerposition.x + playerHalfWidth, +playerposition.y + playerHalfWidth, +playerposition.z + playerHalfWidth
	};
	const Vector3 nearest = {
		+std::clamp(position_.x, minimum.x, maximum.x), +std::clamp(position_.y, minimum.y, maximum.y),
		+std::clamp(position_.z, minimum.z, maximum.z)
	};
	const Vector3 difference = position_ - nearest;
	const float distanceSquared = LengthSquared(difference);
	if (distanceSquared >= radius_ * radius_) {
		return;
	}
	Vector3 normal{};
	float penetration = 0.0f;
	if (distanceSquared > 0.00000001f) {
		const float distance = std::sqrt(distanceSquared);
		normal = difference / distance;
		penetration = radius_ - distance;
	}
	else {
		// 球の中心がPlayerの箱の中なら、近い方の水平面へ出す。
		const float dx = position_.x - playerposition.x;
		const float dz = position_.z - playerposition.z;
		if (std::abs(dx) > std::abs(dz)) {
			normal = {dx >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f};
			penetration = playerHalfWidth + radius_ - std::abs(dx);
		}
		else {
			normal = {0.0f, 0.0f, dz >= 0.0f ? 1.0f : -1.0f};
			penetration = playerHalfWidth + radius_ - std::abs(dz);
		}
	}
	position_ += normal * (penetration + settings_.contactMargin);
	const float outwardSpeed = Dot(velocity_, normal);
	if (outwardSpeed < settings_.playerContactSpeed) {
		velocity_ += normal * +(settings_.playerContactSpeed - outwardSpeed);
	}
	state_ = State::kFree;
}

void Snow::Release() {
	if (state_ == State::kPushed) {
		state_ = State::kFree;
	}
}

void Snow::UpdatePhysics(float deltaTime, MapChipField& map, const Vector3& playerPosition, float playerYaw,
                         float playerHalfWidth) {
	if (deltaTime <= 0.0f) {
		return;
	}
	PlayerToSnowCollsion(playerPosition, playerHalfWidth);
	if (state_ == State::kStopped) {
		return;
	}
	squashTimer_ = (std::max)(0.0f, squashTimer_ - deltaTime);

	const float maximumTravel = (std::max)(radius_ * settings_.movementStepRadiusRatio, 0.001f);
	const float travel = (Length(velocity_) + settings_.gravity * deltaTime) * deltaTime;
	const int steps = std::clamp(static_cast<int>(std::ceil(travel / maximumTravel)), 1, settings_.maxMovementSteps);
	const float stepTime = deltaTime / static_cast<float>(steps);

	for (int step = 0; step < steps; ++step) {
		const Vector3 previousPosition = position_;

		if (state_ == State::kPushed) {
			const Vector3 forward = {std::sin(playerYaw), 0.0f, std::cos(playerYaw)};
			const Vector3 target = playerPosition + forward * (playerHalfWidth + radius_ + settings_.placementGap);

			Vector3 error = target - position_;
			error.y = 0.0f;

			Vector3 desiredVelocity = error * settings_.pushGain;
			const float desiredSpeed = Length(desiredVelocity);
			if (desiredSpeed > settings_.maxPushSpeed) {
				desiredVelocity *= settings_.maxPushSpeed / desiredSpeed;
			}

			const float radiusRatio = radius_ / settings_.initialRadius;
			const float mass = settings_.baseMass * radiusRatio * radiusRatio * radiusRatio;

			const Vector3 currentHorizontal = {velocity_.x, 0.0f, velocity_.z};
			Vector3 acceleration = (desiredVelocity - currentHorizontal) * settings_.pushResponse;

			const float accelerationLength = Length(acceleration);
			const float maximumAcceleration = settings_.maxPushForce / mass;
			if (accelerationLength > maximumAcceleration) {
				acceleration *= maximumAcceleration / accelerationLength;
			}

			velocity_.x += acceleration.x * stepTime;
			velocity_.z += acceleration.z * stepTime;
		}

		velocity_.y -= settings_.gravity * stepTime;
		position_ += velocity_ * stepTime;

		grounded_ = false;
		ResolveMap(map);

		if (grounded_) {
			const float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
			const float damping = std::exp(-settings_.rollingResistance * stepTime);
			const float slowed = (std::max)(0.0f, speed * damping - settings_.rollingDeceleration * stepTime);
			const float ratio = speed > 0.0f ? slowed / speed : 0.0f;

			velocity_.x *= ratio;
			velocity_.z *= ratio;
			angularVelocity_ *= ratio;
		}

		rotation_ += angularVelocity_ * stepTime;

		// 押している間、衝突解決後に実際に転がった距離だけ成長する。
		if (state_ == State::kPushed && grounded_) {
			const Vector3 moved = {position_.x - previousPosition.x, 0.0f, position_.z - previousPosition.z};
			const float previousRadius = radius_;
			radius_ = (std::min)(radius_ + Length(moved) * settings_.growthPerDistance, settings_.maxRadius);

			if (radius_ > previousRadius) {
				position_.y += radius_ - previousRadius;
				ResolveMap(map);
			}
		}

		if (state_ == State::kFree && grounded_) {
			const float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
			if (speed < settings_.stopSpeed) {
				velocity_ = {};
				angularVelocity_ = {};
				state_ = State::kStopped;
				break;
			}
		}
	}

	PrepareDraw();
}


void Snow::ResolveMap(MapChipField& map) {
	bool touchedGround = false;

	for (int iteration = 0; iteration < settings_.collisionIterations; ++iteration) {
		bool found = false;
		float deepest = -1.0f;
		Vector3 contactNormal{};

		for (uint32_t row = 0; row < map.GetLoadedBlockVirtical(); ++row) {
			for (uint32_t column = 0; column < map.GetLoadedBlockHorizontal(); ++column) {
				if (map.GetMapChipTypeByIndex(column, row) != MapChipType::kBlock) {
					continue;
				}

				const Vector3 chip = map.GetTopDownPositionByIndex(column, row);
				constexpr Vector3 half = {
					MapChipField::kBlockWindth * 0.5f, MapChipField::kTopDownBlockThickness * 0.5f,
					MapChipField::kBlockHeight * 0.5f
				};
				const Vector3 center = {chip.x, MapChipField::kTopDownFloorTop - half.y, chip.z};
				const Vector3 minimum = center - half;
				const Vector3 maximum = center + half;

				const Vector3 nearest = {
					std::clamp(position_.x, minimum.x, maximum.x), std::clamp(position_.y, minimum.y, maximum.y),
					std::clamp(position_.z, minimum.z, maximum.z)
				};
				const Vector3 difference = position_ - nearest;
				const float distanceSquared = LengthSquared(difference);

				if (distanceSquared > radius_ * radius_) {
					continue;
				}

				Vector3 normal{};
				float penetration = 0.0f;

				if (distanceSquared > 0.00000001f) {
					const float distance = std::sqrt(distanceSquared);
					normal = difference / distance;
					penetration = radius_ - distance;
				}
				else {
					// 球の中心が箱の内部なら、最も近い面へ押し出す。
					const float distances[6] = {
						position_.x - minimum.x, maximum.x - position_.x, position_.y - minimum.y,
						maximum.y - position_.y, position_.z - minimum.z, maximum.z - position_.z
					};
					const Vector3 normals[6] = {
						{-1, 0, 0},
						{1, 0, 0},
						{0, -1, 0},
						{0, 1, 0},
						{0, 0, -1},
						{0, 0, 1}
					};
					int face = 0;
					for (int i = 1; i < 6; ++i) {
						if (distances[i] < distances[face]) {
							face = i;
						}
					}
					normal = normals[face];
					penetration = radius_ + distances[face];
				}

				if (!found || penetration > deepest) {
					found = true;
					deepest = penetration;
					contactNormal = normal;
				}
			}
		}

		if (!found) {
			break;
		}

		const Vector3 normal = contactNormal;
		position_ += normal * (deepest + settings_.contactMargin);

		if (normal.y > 0.6f) {
			touchedGround = true;
		}

		const float normalSpeed = Dot(velocity_, normal);
		if (normalSpeed >= 0.0f) {
			continue;
		}

		const float radiusRatio = radius_ / settings_.initialRadius;
		const float mass = settings_.baseMass * radiusRatio * radiusRatio * radiusRatio;
		const bool floorContact = normal.y > 0.6f;
		const float restitution = -normalSpeed >= settings_.bounceThreshold
			                          ? (floorContact ? settings_.floorRestitution : settings_.wallRestitution)
			                          : 0.0f;
		const float normalImpulse = -(1.0f + restitution) * normalSpeed * mass;

		velocity_ += normal * (normalImpulse / mass);

		// 接触点の滑りを減らす力を、並進と回転に分ける。
		const Vector3 arm = -normal * radius_;
		const Vector3 contactVelocity = velocity_ + Cross(angularVelocity_, arm);
		const Vector3 tangent = contactVelocity - normal * Dot(contactVelocity, normal);
		const float tangentSpeed = Length(tangent);

		if (tangentSpeed > 0.000001f) {
			const float inertia = (2.0f / 5.0f) * mass * radius_ * radius_;
			const float inverseEffectiveMass = 1.0f / mass + radius_ * radius_ / inertia;
			const float frictionImpulse = (std::min)(tangentSpeed / inverseEffectiveMass,
			                                         settings_.friction * normalImpulse);
			const Vector3 impulse = -tangent * (frictionImpulse / tangentSpeed);

			velocity_ += impulse / mass;
			angularVelocity_ += Cross(arm, impulse) / inertia;
		}

		if (normalImpulse >= settings_.squashImpulse) {
			squashNormal_ = normal;
			squashTimer_ = settings_.squashTime;
		}
	}

	grounded_ = grounded_ || touchedGround;
}

void Snow::PrepareDraw() {
	Vector3 squash = {1.0f, 1.0f, 1.0f};

	if (squashTimer_ > 0.0f && settings_.squashTime > 0.0f) {
		const float ratio = squashTimer_ / settings_.squashTime;
		const float amount = settings_.squashAmount * ratio * ratio;
		const float shrink = 1.0f - amount;
		const float grow = 1.0f + amount * 0.5f;

		if (std::abs(squashNormal_.y) > 0.6f) {
			squash = {grow, shrink, grow};
		}
		else if (std::abs(squashNormal_.x) > std::abs(squashNormal_.z)) {
			squash = {shrink, grow, grow};
		}
		else {
			squash = {grow, grow, shrink};
		}
	}

	worldTransform_.scale_ = {radius_, radius_, radius_};
	worldTransform_.rotation_ = rotation_;
	worldTransform_.translation_ = position_;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, rotation_, {0, 0, 0}) * MakeScaleMatrix(squash)
		* MakeTranslateMatrix(position_);
	worldTransform_.TransferMatrix();
}
