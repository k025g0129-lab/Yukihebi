#include "Snow.h"
#include <algorithm>
#include <cmath>
#include "MapChipField.h"
Snow::Snow() = default;

Snow::~Snow() {
}

float Snow::GetMass() const {
	const float initialRadius = (std::max)(settings_.initialRadius, 0.001f);
	const float ratio = (std::max)(radius_, 0.001f) / initialRadius;
	return (std::max)(settings_.baseMass, 0.001f) * ratio * ratio * ratio;
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
	pulseTimer_ = 0.0f;
	grounded_ = false;
	state_ = State::kPushed;
	pendingEffectEvents_.clear();
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

void Snow::DrainEffectEvents(std::vector<EffectEvent>& events) {
	for (const EffectEvent& event : pendingEffectEvents_) {
		events.push_back(event);
	}
	pendingEffectEvents_.clear();
}

void Snow::NotifyMaxSizeReached(float previousRadius) {
	// 初めて上限に達した瞬間だけ通知する。上限に達した後の毎フレームは通知しない。
	if (previousRadius < settings_.maxRadius && radius_ >= settings_.maxRadius) {
		pendingEffectEvents_.push_back({EffectType::kMaxSize, position_, {0.0f, 1.0f, 0.0f}, radius_, 0.0f});
		// 完成した瞬間に一瞬膨らませる。
		pulseTimer_ = settings_.maxSizePulseTime;
	}
}

void Snow::NotifyCreated() {
	pendingEffectEvents_.push_back({EffectType::kCreate, position_, {0.0f, 1.0f, 0.0f}, radius_, 0.0f});
}

void Snow::NotifyWallHit(const Vector3& outward, float impactSpeed) {
	// 押している間は壁沿いに滑らせるだけなので演出しない。
	if (state_ == State::kPushed || impactSpeed < settings_.wallEffectMinSpeed) {
		return;
	}
	// 接点は壁側の表面。法線はステージの内側へ向ける。
	pendingEffectEvents_.push_back({EffectType::kWallHit, position_ + outward * radius_, -outward,
	                                radius_, impactSpeed});
	squashNormal_ = outward;
	squashTimer_ = settings_.squashTime;
}

void Snow::UpdatePulse(float deltaTime) {
	pulseTimer_ = (std::max)(0.0f, pulseTimer_ - deltaTime);
}

//===========================
// 雪玉を持っているときの位置更新
//===========================
void Snow::UpdateHeld(const Vector3& playerPosition, const Vector3& playerVelocity, float playerYaw,
                      float playerHalfWidth, float floorY, float deltaTime, MapChipField* map) {
	const Vector3 previousPosition = position_;
	// Playerの表示上の正面方向（+Zを正面とする）
	const Vector3 forward = {std::sin(playerYaw), 0.0f, std::cos(playerYaw)}; // プレイヤーの正面方向の単位ベクトルを計算する。

	// 雪玉の位置をプレイヤーの正面方向に置く。プレイヤーと雪玉の間に隙間を設ける。
	position_ = playerPosition + forward * (playerHalfWidth + radius_ + settings_.placementGap);
	position_.y = floorY + radius_; // 雪玉の位置のY座標を床の上に置く。雪玉の半径分だけ上に置く。

	// 押している間は慣性で正面から離れない。壁・床との重なりは配置後に解消する。
	velocity_ = {};
	grounded_ = false;
	if (map != nullptr) {
		ResolveMap(*map);
		ConstrainToStage(*map);
	}

	const Vector3 moved = {position_.x - previousPosition.x, 0.0f, position_.z - previousPosition.z};
	const Vector3 playerHorizontal = {playerVelocity.x, 0.0f, playerVelocity.z};
	const float playerSpeed = Length(playerHorizontal);
	if (deltaTime > 0.0f && playerSpeed > 0.0f) {
		// 方向転換だけで雪玉が円弧を描いても、押す速度や成長量には数えない。
		const float pushedDistance = std::clamp(Dot(moved, playerHorizontal) / playerSpeed,
		                                        0.0f, playerSpeed * deltaTime);
		velocity_ = playerHorizontal * (pushedDistance / (playerSpeed * deltaTime));
		if (grounded_) {
			const float previousRadius = radius_;
			radius_ = (std::min)(radius_ + pushedDistance * settings_.growthPerDistance, settings_.maxRadius);
			position_.y += radius_ - previousRadius;
			NotifyMaxSizeReached(previousRadius);
		}
	}
	if (map != nullptr) {
		ConstrainToStage(*map);
	}
	angularVelocity_ = Cross(Vector3{0.0f, 1.0f, 0.0f}, velocity_) / (std::max)(radius_, 0.001f);
	rotation_ += angularVelocity_ * deltaTime;
	UpdatePulse(deltaTime);
	PrepareDraw();
}

bool Snow::IsSizeMax() const { 
		
	if (radius_ >= settings_.maxRadius) {
		return true;
	
	} else {
		return false;
	
	}

	//return false; 
}

void Snow::PlayerToSnowCollision(const Vector3& playerposition, const Vector3& playerVelocity, float playerHalfWidth) {
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
	// Playerとの質量比を使い、大きい雪玉ほど接触で速度が変わりにくくする。
	const float relativeNormalSpeed = Dot(velocity_ - playerVelocity, normal);
	if (-relativeNormalSpeed >= settings_.playerEffectMinSpeed) {
		pendingEffectEvents_.push_back({EffectType::kPlayerHit, position_ - normal * radius_, normal,
		                                radius_, -relativeNormalSpeed});
	}
	if (relativeNormalSpeed < 0.0f) {
		const float inverseMass = 1.0f / GetMass();
		const float inversePlayerMass = 1.0f / (std::max)(settings_.playerMass, 0.001f);
		const float restitution = std::clamp(settings_.playerRestitution, 0.0f, 1.0f);
		const float impulse = -(1.0f + restitution) * relativeNormalSpeed / (inverseMass + inversePlayerMass);
		velocity_ += normal * (impulse * inverseMass);
	}
	state_ = State::kFree;
	PrepareDraw();
}

void Snow::SnowToSnowCollision(Snow& other) {
	if (state_ == State::kPushed && other.state_ == State::kPushed) {
		return;
	}
	const Vector3 difference = position_ - other.position_;
	const float distanceSquared = LengthSquared(difference);
	const float radiusSum = radius_ + other.radius_;
	if (distanceSquared >= radiusSum * radiusSum) {
		return;
	}
	Vector3 normal{};
	float penetration = 0.0f;
	if (distanceSquared > 0.00000001f) {
		const float distance = std::sqrt(distanceSquared);
		normal = difference / distance;
		penetration = radiusSum - distance;
	}
	else {
		// 球の中心が重なっている場合は、適当に押し出す。
		normal = {1.0f, 0.0f, 0.0f};
		penetration = radiusSum;
	}

	const float inverseMass = state_ == State::kPushed ? 0.0f : 1.0f / GetMass();
	const float otherInverseMass = other.state_ == State::kPushed ? 0.0f : 1.0f / other.GetMass();
	const float inverseMassSum = inverseMass + otherInverseMass;
	// 大きい玉ほど押し出されにくい。めり込み補正と衝突速度に同じ質量比を使う。
	const float correction = (penetration + settings_.contactMargin) / inverseMassSum;
	position_ += normal * (correction * inverseMass);
	other.position_ -= normal * (correction * otherInverseMass);

	const float relativeNormalSpeed = Dot(velocity_ - other.velocity_, normal);
	if (-relativeNormalSpeed >= settings_.snowEffectMinSpeed) {
		pendingEffectEvents_.push_back({EffectType::kSnowHit, position_ - normal * radius_, normal,
		                                (std::max)(radius_, other.radius_), -relativeNormalSpeed});
	}
	if (relativeNormalSpeed < 0.0f) {
		const float restitution = std::clamp((std::min)(settings_.snowRestitution, other.settings_.snowRestitution),
		                                     0.0f, 1.0f);
		const float impulse = -(1.0f + restitution) * relativeNormalSpeed / inverseMassSum;
		velocity_ += normal * (impulse * inverseMass);
		other.velocity_ -= normal * (impulse * otherInverseMass);
		if (state_ == State::kStopped) {
			state_ = State::kFree;
		}
		if (other.state_ == State::kStopped) {
			other.state_ = State::kFree;
		}
	}

	PrepareDraw();
	other.PrepareDraw();
}

void Snow::Release() {
	if (state_ == State::kPushed) {
		state_ = State::kFree;
		// Space中に得た速度と回転をそのまま引き継ぐ。
	}
}

void Snow::Kick(const Vector3& direction, float speed) {
	if (state_ == State::kPushed || speed <= 0.0f) {
		return;
	}
	const Vector3 horizontal = {direction.x, 0.0f, direction.z};
	if (LengthSquared(horizontal) <= 0.000001f) {
		return;
	}
	const Vector3 kickDirection = Normalize(horizontal);
	velocity_ = kickDirection * speed;
	angularVelocity_ = Cross(Vector3{0.0f, 1.0f, 0.0f}, velocity_) / (std::max)(radius_, 0.001f);
	state_ = State::kFree;
	// 蹴られた面（Player側）を接点とし、法線はPlayer側へ向ける。
	pendingEffectEvents_.push_back({EffectType::kKick, position_ - kickDirection * radius_, -kickDirection,
	                                radius_, speed});
	PrepareDraw();
}

void Snow::UpdatePhysics(float deltaTime, MapChipField& map, const Vector3& playerPosition,
                         const Vector3& playerVelocity,
                         float playerHalfWidth) {
	if (deltaTime <= 0.0f) {
		return;
	}
	if (state_ == State::kPushed) {
		return; // Space中はPlayer::Updateで配置・回転済み。自由物体として積分しない。
	}
	const bool pulsing = pulseTimer_ > 0.0f;
	const bool squashing = squashTimer_ > 0.0f;
	UpdatePulse(deltaTime);
	// 止まった玉でもつぶれが固まらないよう、早期 return より前に減らす。
	squashTimer_ = (std::max)(0.0f, squashTimer_ - deltaTime);
	PlayerToSnowCollision(playerPosition, playerVelocity, playerHalfWidth);

	if (state_ == State::kStopped) {
		if (pulsing || squashing) {
			PrepareDraw(); // 止まっていても膨らみ・つぶれは戻す
		}
		return;
	}

	const float maximumTravel = (std::max)(radius_ * settings_.movementStepRadiusRatio, 0.001f);
	const float travel = (Length(velocity_) + settings_.gravity * deltaTime) * deltaTime;
	const int steps = std::clamp(static_cast<int>(std::ceil(travel / maximumTravel)), 1, settings_.maxMovementSteps);
	const float stepTime = deltaTime / static_cast<float>(steps);

	for (int step = 0; step < steps; ++step) {
		const Vector3 previousPosition = position_;

		velocity_.y -= settings_.gravity * stepTime;
		position_ += velocity_ * stepTime;

		grounded_ = false;
		ResolveMap(map);
		ConstrainToStage(map);

		if (grounded_) {
			const float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
			const float damping = std::exp(-settings_.rollingResistance * stepTime);
			const float massRatio = GetMass() / (std::max)(settings_.baseMass, 0.001f);
			const float massLoad = 1.0f + ((std::max)(massRatio, 1.0f) - 1.0f) * settings_.rollingMassInfluence;
			const float slowed = (
				std::max)(0.0f, speed * damping - settings_.rollingDeceleration / massLoad * stepTime);
			const float ratio = speed > 0.0f ? slowed / speed : 0.0f;

			velocity_.x *= ratio;
			velocity_.z *= ratio;
			angularVelocity_ *= ratio;
		}

		rotation_ += angularVelocity_ * stepTime;

		// 離した後も接地して実際に進んだ距離だけ成長する。
		if (grounded_) {
			const Vector3 moved = {position_.x - previousPosition.x, 0.0f, position_.z - previousPosition.z};
			const float previousRadius = radius_;
			radius_ = (std::min)(radius_ + Length(moved) * settings_.growthPerDistance, settings_.maxRadius);

			if (radius_ > previousRadius) {
				position_.y += radius_ - previousRadius;
				ResolveMap(map);
				NotifyMaxSizeReached(previousRadius);
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

	ConstrainToStage(map);
	PrepareDraw();
}


void Snow::ConstrainToStage(const MapChipField& map) {
	const float halfWidth = map.GetLoadedBlockHorizontal() * MapChipField::kBlockWindth * 0.5f;
	const float halfDepth = map.GetLoadedBlockVirtical() * MapChipField::kBlockHeight * 0.5f;
	const float maxX = (std::max)(0.0f, halfWidth - radius_);
	const float maxZ = (std::max)(0.0f, halfDepth - radius_);
	const float x = std::clamp(position_.x, -maxX, maxX);
	const float z = std::clamp(position_.z, -maxZ, maxZ);
	if (x != position_.x) {
		const float outward = position_.x > x ? 1.0f : -1.0f;
		const float impactSpeed = velocity_.x * outward;
		position_.x = x;
		velocity_.x = 0.0f;
		NotifyWallHit({outward, 0.0f, 0.0f}, impactSpeed);
	}
	if (z != position_.z) {
		const float outward = position_.z > z ? 1.0f : -1.0f;
		const float impactSpeed = velocity_.z * outward;
		position_.z = z;
		velocity_.z = 0.0f;
		NotifyWallHit({0.0f, 0.0f, outward}, impactSpeed);
	}
}

void Snow::ResolveMap(MapChipField& map) {
	bool touchedGround = false;
	const int columns = static_cast<int>(map.GetLoadedBlockHorizontal());
	const int rows = static_cast<int>(map.GetLoadedBlockVirtical());
	// GetTopDownPositionByIndex の逆算に使う。マスの中心は x = column*w - centerX, z = centerZ - row*h。
	const float centerX = (static_cast<float>(columns) - 1.0f) * MapChipField::kBlockWindth * 0.5f;
	const float centerZ = (static_cast<float>(rows) - 1.0f) * MapChipField::kBlockHeight * 0.5f;

	for (int iteration = 0; iteration < settings_.collisionIterations; ++iteration) {
		bool found = false;
		float deepest = -1.0f;
		Vector3 contactNormal{};

		// 全マスを見ると玉の数×ステップ数×マス数で重くなる。球が触れうるのは
		// 中心が「半径＋マスの半分」以内にあるマスだけなので、その範囲に絞る。
		const float reachX = radius_ + MapChipField::kBlockWindth * 0.5f;
		const float reachZ = radius_ + MapChipField::kBlockHeight * 0.5f;
		const int minColumn = (std::max)(
			0, static_cast<int>(std::floor((position_.x - reachX + centerX) / MapChipField::kBlockWindth)));
		const int maxColumn = (std::min)(
			columns - 1, static_cast<int>(std::ceil((position_.x + reachX + centerX) / MapChipField::kBlockWindth)));
		// z が大きいほど row は小さい。
		const int minRow = (std::max)(
			0, static_cast<int>(std::floor((centerZ - (position_.z + reachZ)) / MapChipField::kBlockHeight)));
		const int maxRow = (std::min)(
			rows - 1, static_cast<int>(std::ceil((centerZ - (position_.z - reachZ)) / MapChipField::kBlockHeight)));

		for (int row = minRow; row <= maxRow; ++row) {
			for (int column = minColumn; column <= maxColumn; ++column) {
				const uint32_t xIndex = static_cast<uint32_t>(column);
				const uint32_t yIndex = static_cast<uint32_t>(row);
				if (map.GetMapChipTypeByIndex(xIndex, yIndex) != MapChipType::kBlock) {
					continue;
				}

				const Vector3 chip = map.GetTopDownPositionByIndex(xIndex, yIndex);
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

		const float mass = GetMass();
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

		// 質量込みの衝撃で判定すると、重い玉は床に乗っているだけで毎ステップつぶれる。当たった速さで判定する。
		if (-normalSpeed >= settings_.squashMinSpeed) {
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

	// 最大サイズ到達時の膨らみ。見た目だけ大きくし、判定の半径は変えない。
	float pulse = 1.0f;
	if (pulseTimer_ > 0.0f && settings_.maxSizePulseTime > 0.0f) {
		const float ratio = pulseTimer_ / settings_.maxSizePulseTime;
		pulse += settings_.maxSizePulseAmount * ratio * ratio;
	}
	const float drawRadius = radius_ * pulse;

	worldTransform_.scale_ = {radius_, radius_, radius_};
	worldTransform_.rotation_ = rotation_;
	worldTransform_.translation_ = position_;
	worldTransform_.matWorld_ = MakeAffineMatrix({drawRadius, drawRadius, drawRadius}, rotation_, {0, 0, 0}) *
		MakeScaleMatrix(squash)
		* MakeTranslateMatrix(position_);
	worldTransform_.TransferMatrix();
}
