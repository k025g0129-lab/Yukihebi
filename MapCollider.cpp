#include "MapCollider.h"
#include "MapChipField.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {
	constexpr float kEpsilon = MapCollider::kEpsilon;

	struct Shape {
		std::array<Vector3, 4> vertices{};
		size_t count = 0;
		uint32_t xIndex = 0;
		uint32_t yIndex = 0;
	};

	struct Axes {
		// 移動する矩形のX・Y軸 + 地形の最大4辺。
		std::array<Vector3, 6> values{};
		size_t count = 2;
	};

	struct HitBatch {
		bool hit = false;
		float time = 1.0f;
		std::vector<MapCollider::Contact> contacts;
	};

	struct Overlap {
		bool hit = false;
		float depth = 0.0f;
		MapCollider::Contact contact{};
	};

	//bool MakeShape(MapChipField& field, uint32_t x, uint32_t y, Shape& shape) {
	//	shape = {};
	//	shape.xIndex = x;
	//	shape.yIndex = y;

	//	if (MapChipField::IsSolid(field.GetMapChipTypeByIndex(x, y))) {
	//		const auto rect = field.GetRectByIndex(x, y);

	//		shape.vertices = {
	//			Vector3{rect.left, rect.bottom, 0.0f},
	//			Vector3{rect.right, rect.bottom, 0.0f},
	//			Vector3{rect.right, rect.top, 0.0f},
	//			Vector3{rect.left, rect.top, 0.0f},
	//		};
	//		shape.count = 4;
	//		return true;
	//	}

	//	MapChipField::SlopeShape slope;
	//	if (!field.TryGetSlopeShape(x, y, slope)) {
	//		return false;
	//	}

	//	std::copy(slope.vertices.begin(), slope.vertices.end(), shape.vertices.begin());

	//	shape.count = 3;
	//	return true;
	//}

	Axes MakeAxes(const Shape& shape) {
		Axes axes;
		axes.values[0] = {1.0f, 0.0f, 0.0f};
		axes.values[1] = {0.0f, 1.0f, 0.0f};

		for (size_t i = 0; i < shape.count; ++i) {
			const Vector3 edge = shape.vertices[(i + 1) % shape.count] - shape.vertices[i];

			axes.values[axes.count++] = Normalize(Vector3{-edge.y, edge.x, 0.0f});
		}

		return axes;
	}

	void Project(const Shape& shape, const Vector3& axis, float& minimum, float& maximum) {
		minimum = Dot(shape.vertices[0], axis);
		maximum = minimum;

		for (size_t i = 1; i < shape.count; ++i) {
			const float value = Dot(shape.vertices[i], axis);
			minimum = (std::min)(minimum, value);
			maximum = (std::max)(maximum, value);
		}
	}

	// 接地面の余白は上下方向にだけ設ける。全方向へ skin を足すと、
	// 斜面と隣のブロック上面がつながる手前で側面へ先に当たってしまう。
	float BoxRadius(const Vector3& half, const Vector3& axis, float skin) {
		return half.x * std::abs(axis.x) + (half.y + skin) * std::abs(axis.y);
	}

	// 開始位置から終了位置までに触れうるマスだけ列挙する。
	// マップ外の負の番号をuint32_tへ変換しない。
	template <class Callback>
	void ForEachShape(MapChipField& field, const Vector3& start, const Vector3& end, const Vector3& half, float skin,
	                  Callback callback) {
		(void)(callback);
		constexpr float width = MapChipField::kBlockWindth;
		constexpr float height = MapChipField::kBlockHeight;

		const int columns = static_cast<int>(field.GetNumBlockHorizontal());
		const int rows = static_cast<int>(field.GetNumBlockVirtical());

		const float mapTop = (static_cast<float>(rows) - 0.5f) * height;

		const float left = (std::min)(start.x, end.x) - half.x - skin;
		const float right = (std::max)(start.x, end.x) + half.x + skin;
		const float bottom = (std::min)(start.y, end.y) - half.y - skin;
		const float top = (std::max)(start.y, end.y) + half.y + skin;

		int x0 = static_cast<int>(std::floor((left + width * 0.5f) / width));
		int x1 = static_cast<int>(std::floor((right + width * 0.5f) / width));

		int y0 = static_cast<int>(std::floor((mapTop - top) / height));
		int y1 = static_cast<int>(std::floor((mapTop - bottom) / height));

		x0 = (std::max)(x0, 0);
		x1 = (std::min)(x1, columns - 1);
		y0 = (std::max)(y0, 0);
		y1 = (std::min)(y1, rows - 1);

		for (int y = y0; y <= y1; ++y) {
			for (int x = x0; x <= x1; ++x) {
				Shape shape;

				//if (MakeShape(field, static_cast<uint32_t>(x), static_cast<uint32_t>(y), shape)) {
				//	callback(shape);
				//}
			}
		}
	}

	// 開始時点ですでにめり込んでいるかを調べる。
	Overlap CheckOverlap(const Shape& shape, const Vector3& center, const Vector3& half, float skin) {
		Overlap result;
		result.depth = (std::numeric_limits<float>::max)();

		const Axes axes = MakeAxes(shape);

		for (size_t i = 0; i < axes.count; ++i) {
			const Vector3 axis = axes.values[i];

			float minimum;
			float maximum;
			Project(shape, axis, minimum, maximum);

			const float c = Dot(center, axis);
			const float radius = BoxRadius(half, axis, skin);

			// どちら側へ押すと地形の外へ出るか。
			const float positive = maximum + radius - c;
			const float negative = c - (minimum - radius);

			if (positive <= kEpsilon || negative <= kEpsilon) {
				return {};
			}

			const float depth = (std::min)(positive, negative);

			if (depth < result.depth) {
				result.depth = depth;
				result.contact.normal = positive < negative ? axis : -axis;
			}
		}

		result.hit = true;
		result.contact.xIndex = shape.xIndex;
		result.contact.yIndex = shape.yIndex;
		result.contact.correctionOnly = true;
		return result;
	}

	Overlap FindOverlap(MapChipField& field, const Vector3& center, const Vector3& half, float skin) {
		Overlap best;

		ForEachShape(field, center, center, half, skin, [&](const Shape& shape) {
			const Overlap current = CheckOverlap(shape, center, half, skin);

			if (current.hit && (!best.hit || current.depth > best.depth)) {
				best = current;
			}
		});

		return best;
	}

	// 動く矩形と、静止した凸形状の接触時刻を求める。
	// 各判定軸で重なる時間帯を求め、その共通部分を見る。
	bool SweepShape(const Shape& shape, const Vector3& center, const Vector3& movement, const Vector3& half, float skin,
	                float& time, Vector3& normal) {
		float enter = -std::numeric_limits<float>::infinity();
		float leave = std::numeric_limits<float>::infinity();

		const Axes axes = MakeAxes(shape);

		for (size_t i = 0; i < axes.count; ++i) {
			const Vector3 axis = axes.values[i];

			float minimum;
			float maximum;
			Project(shape, axis, minimum, maximum);

			const float radius = BoxRadius(half, axis, skin);
			minimum -= radius;
			maximum += radius;

			const float centerOnAxis = Dot(center, axis);
			const float speedOnAxis = Dot(movement, axis);

			if (std::abs(speedOnAxis) <= kEpsilon) {
				// この軸では平行移動している。
				// 境界に触れているだけなら衝突としない。
				if (centerOnAxis <= minimum + kEpsilon || centerOnAxis >= maximum - kEpsilon) {
					return false;
				}
				continue;
			}

			float nearTime = (minimum - centerOnAxis) / speedOnAxis;
			float farTime = (maximum - centerOnAxis) / speedOnAxis;

			if (nearTime > farTime) {
				std::swap(nearTime, farTime);
			}

			const Vector3 candidateNormal = speedOnAxis > 0.0f ? -axis : axis;
			const bool slopeSurfaceAtCorner =
				shape.count == 3 && i >= 2 &&
				std::abs(nearTime - enter) <= kEpsilon &&
				std::abs(candidateNormal.x) > kEpsilon &&
				candidateNormal.y > 0.0f &&
				candidateNormal.y > normal.y + kEpsilon;
			// 平地から斜面へ入る角では、X軸と斜面の接触時刻が一致する。
			// X軸を壁として選ぶと上れないので、同時なら斜面の法線を使う。
			if (nearTime > enter + kEpsilon || slopeSurfaceAtCorner) {
				normal = candidateNormal;
			}
			enter = (std::max)(enter, nearTime);

			leave = (std::min)(leave, farTime);

			if (enter >= leave) {
				return false;
			}
		}

		// 開始時のめり込みはCheckOverlapが担当する。
		// 離れる接触や、角を一瞬かすめるだけの接触は除外する。
		if (enter < -kEpsilon || enter > 1.0f || leave <= 0.0f) {
			return false;
		}

		time = (std::max)(0.0f, enter);
		return LengthSquared(normal) > 0.0f;
	}

	HitBatch FindHits(MapChipField& field, const Vector3& center, const Vector3& movement, const Vector3& half,
	                  float skin) {
		HitBatch best;

		ForEachShape(field, center, center + movement, half, skin, [&](const Shape& shape) {
			float time = 1.0f;
			Vector3 normal{};

			if (!SweepShape(shape, center, movement, half, skin, time, normal)) {
				return;
			}

			if (!best.hit || time < best.time - kEpsilon) {
				best.hit = true;
				best.time = time;
				best.contacts.clear();
			}
			else if (time < best.time) {
				best.time = time;
			}

			// 床と壁へほぼ同時に当たった場合は両方残す。
			if (std::abs(time - best.time) <= kEpsilon) {
				MapCollider::Contact contact;
				contact.normal = normal;
				contact.xIndex = shape.xIndex;
				contact.yIndex = shape.yIndex;
				best.contacts.push_back(contact);
			}
		});

		return best;
	}

	float GetUphillSlopeRise(MapChipField& field, const Vector3& center,
	                         const Vector3& movement, const Vector3& half,
	                         float skin, float maxGroundAngle) {
		if (std::abs(movement.x) <= kEpsilon) {
			return 0.0f;
		}
		const float angle = std::clamp(maxGroundAngle, 0.0f, kPi * 0.5f - 0.01f);
		const float maxRise = (std::abs(movement.x) + half.x) * std::tan(angle) + skin;
		float rise = 0.0f;

		// 矩形の前端が斜面に掛かると、中心が斜面のマスへ入る前に上昇が必要になる。
		// 地面に沿う移動だけに適用し、空中の衝突と垂直な壁は通常判定に任せる。
		ForEachShape(field, center, center + movement, half, skin, [&](const Shape& shape) {
			if (shape.count != 3) {
				return;
			}
			for (size_t i = 0; i < shape.count; ++i) {
				const Vector3 a = shape.vertices[i];
				const Vector3 b = shape.vertices[(i + 1) % shape.count];
				const Vector3 edge = b - a;
				if (std::abs(edge.x) <= kEpsilon || std::abs(edge.y) <= kEpsilon) {
					continue;
				}
				// 頂点は反時計回りなので、辺の右側が地形の外側。
				const Vector3 normal = Normalize(Vector3{edge.y, -edge.x, 0.0f});
				if (normal.y <= 0.0f || normal.y + kEpsilon < std::cos(angle) ||
					movement.x * normal.x >= 0.0f) {
					continue;
				}
				const float frontX = center.x + movement.x -
					(normal.x > 0.0f ? half.x : -half.x);
				const float left = (std::min)(a.x, b.x);
				const float right = (std::max)(a.x, b.x);
				if (frontX < left - skin || frontX > right + skin) {
					continue;
				}
				const float fraction = std::clamp((frontX - a.x) / edge.x, 0.0f, 1.0f);
				const float surfaceY = std::lerp(a.y, b.y, fraction);
				const float neededRise = surfaceY + half.y + skin - center.y;
				if (neededRise > 0.0f && neededRise <= maxRise + kEpsilon) {
					rise = (std::max)(rise, neededRise);
				}
			}
		});
		return rise;
	}
} // namespace

bool MapCollider::IsWalkableSurface(const Vector3& normal) const {
	return normal.y > 0.0f && normal.y + kEpsilon >= std::cos(settings_.maxGroundAngle);
}

Vector3 MapCollider::CornerPosition(const Vector3& center, Corner corner) const {
	const Vector3 offsets[kNumCorners] = {
		{-width_ * 0.5f, height_ * 0.5f, 0.0f},
		{width_ * 0.5f, height_ * 0.5f, 0.0f},
		{-width_ * 0.5f, -height_ * 0.5f, 0.0f},
		{width_ * 0.5f, -height_ * 0.5f, 0.0f},
	};

	return center + offsets[static_cast<int>(corner)];
}

Vector3 MapCollider::ResolveVelocity(const Vector3& velocity, const Contact& contact) {
	const float inward = Dot(velocity, contact.normal);

	if (inward >= 0.0f) {
		return velocity;
	}

	return velocity - contact.normal * ((1.0f + contact.restitution) * inward);
}

MapCollider::GroundResult MapCollider::ProbeGround(const Vector3& center, float distance) const {
	GroundResult result;

	if (mapChipField_ == nullptr || distance <= 0.0f) {
		return result;
	}

	const Vector3 half{width_ * 0.5f, height_ * 0.5f, 0.0f};
	// 接触面からわずかに離して下向きに調べる。境界ちょうどで始まる
	// SweepShape は丸め誤差で接触を見失うことがある。
	const float startOffset = settings_.skin + kEpsilon;
	const Vector3 start = center + Vector3{0.0f, startOffset, 0.0f};
	const Vector3 movement{0.0f, -(distance + startOffset), 0.0f};
	float nearestTime = (std::numeric_limits<float>::max)();

	// FindHits は最初に当たった形状だけを返す。斜面とブロックの継ぎ目では
	// その接触が歩けない面でも、別の形状に地面がある可能性を調べる。
	ForEachShape(*mapChipField_, start, start + movement, half, settings_.skin, [&](const Shape& shape) {
		float time = 1.0f;
		Vector3 normal{};
		if (!SweepShape(shape, start, movement, half, settings_.skin, time, normal) ||
			!IsWalkableSurface(normal)) {
			return;
		}

		if (!result.hit || time < nearestTime - kEpsilon ||
			(std::abs(time - nearestTime) <= kEpsilon && normal.y > result.normal.y)) {
			result.hit = true;
			result.normal = normal;
			result.movement = Vector3{0.0f, startOffset, 0.0f} + movement * time;
			nearestTime = time;
		}
	});

	return result;
}

bool MapCollider::IsOnGround(const Vector3& center) const { return ProbeGround(center, kGroundProbeDepth).hit; }

MapCollider::Result MapCollider::Check(const Vector3& center, const Vector3& movement) {
	return Check(center, movement, MoveOptions{});
}

MapCollider::Result MapCollider::Check(const Vector3& center, const Vector3& movement, const MoveOptions& options) {
	if (topDownMode_) {
		return CheckTopDown(center, movement, options);
	}
	Result result;
	result.movement = movement;

	if (mapChipField_ == nullptr) {
		return result;
	}

	const Vector3 half{width_ * 0.5f, height_ * 0.5f, 0.0f};

	Vector3 position = center;
	position.z = 0.0f;

	// 最初から地形に埋まっていた場合だけ押し出す。
	// このContactではダメージ・粒子を出さない。
	for (int i = 0; i < settings_.maxOverlapIterations; ++i) {
		const Overlap overlap = FindOverlap(*mapChipField_, position, half, settings_.skin);

		if (!overlap.hit) {
			break;
		}

		if (options.followGround && IsWalkableSurface(overlap.contact.normal) &&
			std::abs(overlap.contact.normal.x) > kEpsilon) {
			// 坂への浅いめり込みを法線方向へ押すと、X方向にも戻されて上りで詰まる。
			position.y += (overlap.depth + kEpsilon) / overlap.contact.normal.y;
		}
		else {
			position += overlap.contact.normal * (overlap.depth + kEpsilon);
		}

		result.contacts.push_back(overlap.contact);
	}

	if (FindOverlap(*mapChipField_, position, half, settings_.skin).hit) {
		result.iterationLimit = true;
		result.movement = position - center;
		return result;
	}

	Vector3 remaining = movement;
	remaining.z = 0.0f;

	if (options.followGround && IsWalkableSurface(options.groundNormal)) {
		// 操作中の水平移動量を保ったまま、
		// 接地している面に沿うY移動量を作る。
		remaining.y = -remaining.x * options.groundNormal.x / options.groundNormal.y;
		const float uphillRise = GetUphillSlopeRise(
			*mapChipField_, position, remaining, half,
			settings_.skin, settings_.maxGroundAngle);
		if (uphillRise > 0.0f) {
			remaining.y = (std::max)(remaining.y, uphillRise);
		}
	}

	float elapsed = 0.0f;

	for (int i = 0; i < settings_.maxMoveIterations; ++i) {
		if (LengthSquared(remaining) <= kEpsilon * kEpsilon) {
			remaining = {};
			break;
		}

		const HitBatch hit = FindHits(*mapChipField_, position, remaining, half, settings_.skin);

		if (!hit.hit) {
			position += remaining;
			remaining = {};
			break;
		}

		const Vector3 incoming = remaining;
		const float secondsLeft = (1.0f - elapsed) * options.deltaTime;

		position += remaining * hit.time;

		elapsed += (1.0f - elapsed) * hit.time;

		remaining *= 1.0f - hit.time;

		for (Contact contact : hit.contacts) {
			contact.time = elapsed;
			contact.center = position;
			// 法線と反対側の支持点。平面では辺の中央、斜面では角になる。
			contact.point = position;
			if (contact.normal.x > kEpsilon) {
				contact.point.x -= half.x;
			}
			else if (contact.normal.x < -kEpsilon) {
				contact.point.x += half.x;
			}
			if (contact.normal.y > kEpsilon) {
				contact.point.y -= half.y;
			}
			else if (contact.normal.y < -kEpsilon) {
				contact.point.y += half.y;
			}

			contact.incomingVelocity = secondsLeft > kEpsilon ? incoming / secondsLeft : Vector3{};
			contact.impactSpeed = (std::max)(0.0f, -Dot(contact.incomingVelocity, contact.normal));

			contact.restitution = contact.impactSpeed >= options.minBounceSpeed ? options.restitution : 0.0f;

			const bool walkable = IsWalkableSurface(contact.normal);

			if (options.followGround && walkable) {
				// Playerの操作歩行。
				// 新たに入った斜面の方向へ残りの移動を合わせる。
				contact.restitution = 0.0f;
				remaining.y = -remaining.x * contact.normal.x / contact.normal.y;
			}
			else {
				// 空中のPlayerや吹き飛び中のEnemy。
				remaining = ResolveVelocity(remaining, contact);
			}

			result.contacts.push_back(contact);
		}
	}

	if (LengthSquared(remaining) > kEpsilon * kEpsilon) {
		result.iterationLimit = true;
	}

	const float probeDistance = options.allowGroundSnap ? options.groundSnapDistance : kGroundProbeDepth;

	const GroundResult ground = ProbeGround(position, probeDistance);

	result.grounded = ground.hit;

	if (ground.hit) {
		result.groundNormal = ground.normal;

		if (options.allowGroundSnap) {
			position += ground.movement;
		}
	}

	for (const Contact& contact : result.contacts) {
		if (contact.correctionOnly) {
			continue;
		}
		result.landing |= IsWalkableSurface(contact.normal);

		result.ceiling |= contact.normal.y < -kEpsilon;

		result.wall |= std::abs(contact.normal.y) <= kEpsilon;
	}

	result.movement = position - center;
	return result;
}

MapCollider::Result MapCollider::CheckTopDown(
	const Vector3& center, const Vector3& movement, const MoveOptions& options) {
	Result result;
	result.movement = movement;
	if (mapChipField_ == nullptr) {
		return result;
	}
	const int columns = static_cast<int>(mapChipField_->GetLoadedBlockHorizontal());
	const int rows = static_cast<int>(mapChipField_->GetLoadedBlockVirtical());
	if (columns == 0 || rows == 0) {
		return result;
	}

	const float cellWidth = MapChipField::kBlockWindth;
	const float cellDepth = MapChipField::kBlockHeight;
	const float blockHeight = MapChipField::kTopDownBlockThickness;
	const float floorTop = MapChipField::kTopDownFloorTop;
	const Vector3 playerHalf{width_ * 0.5f, height_ * 0.5f, width_ * 0.5f};
	const Vector3 blockHalf{cellWidth * 0.5f, blockHeight * 0.5f, cellDepth * 0.5f};
	const float centerX = (columns - 1) * cellWidth * 0.5f;
	const float centerZ = (rows - 1) * cellDepth * 0.5f;

	// 開始点から終了点までの周辺マスだけを衝突候補にする。
	const float left = (std::min)(center.x, center.x + movement.x) - playerHalf.x - blockHalf.x;
	const float right = (std::max)(center.x, center.x + movement.x) + playerHalf.x + blockHalf.x;
	const float back = (std::min)(center.z, center.z + movement.z) - playerHalf.z - blockHalf.z;
	const float front = (std::max)(center.z, center.z + movement.z) + playerHalf.z + blockHalf.z;
	const int firstColumn = (std::max)(0, static_cast<int>(std::floor((left + centerX) / cellWidth)) - 1);
	const int lastColumn = (std::min)(columns - 1, static_cast<int>(std::ceil((right + centerX) / cellWidth)) + 1);
	const int firstRow = (std::max)(0, static_cast<int>(std::floor((centerZ - front) / cellDepth)) - 1);
	const int lastRow = (std::min)(rows - 1, static_cast<int>(std::ceil((centerZ - back) / cellDepth)) + 1);
	Vector3 position = center;

	const auto component = [](const Vector3& value, int axis) {
		return axis == 0 ? value.x : axis == 1 ? value.y : value.z;
	};
	const auto setComponent = [](Vector3& value, int axis, float number) {
		if (axis == 0) { value.x = number; }
		else if (axis == 1) { value.y = number; }
		else { value.z = number; }
	};
	const auto overlaps = [](float aCenter, float aHalf, float bCenter, float bHalf) {
		return aCenter + aHalf > bCenter - bHalf + kEpsilon &&
		       aCenter - aHalf < bCenter + bHalf - kEpsilon;
	};

	// 横方向を先に処理し、その位置で上下方向の床・天井を判定する。
	for (const int axis : {0, 2, 1}) {
		const float requested = component(movement, axis);
		if (std::abs(requested) <= kEpsilon) {
			continue;
		}
		float allowed = requested;
		bool hit = false;
		Contact contact{};
		for (int row = firstRow; row <= lastRow; ++row) {
			for (int column = firstColumn; column <= lastColumn; ++column) {
				if (mapChipField_->GetMapChipTypeByIndex(
					static_cast<uint32_t>(column), static_cast<uint32_t>(row)) != MapChipType::kBlock) {
					continue;
				}
				Vector3 blockCenter = mapChipField_->GetTopDownPositionByIndex(
					static_cast<uint32_t>(column), static_cast<uint32_t>(row));
				blockCenter.y = floorTop - blockHalf.y;
				bool otherAxesOverlap = true;
				for (int other = 0; other < 3; ++other) {
					if (other != axis && !overlaps(component(position, other), component(playerHalf, other),
					                              component(blockCenter, other), component(blockHalf, other))) {
						otherAxesOverlap = false;
						break;
					}
				}
				if (!otherAxesOverlap) {
					continue;
				}
				const float current = component(position, axis);
				const float target = component(blockCenter, axis);
				const float radius = component(playerHalf, axis) + component(blockHalf, axis);
				const float gap = requested > 0.0f ? target - radius - current : target + radius - current;
				const bool nearer = requested > 0.0f
					? gap >= -kEpsilon && gap <= allowed + kEpsilon
					: gap <= kEpsilon && gap >= allowed - kEpsilon;
				if (!nearer) {
					continue;
				}
				allowed = requested > 0.0f ? (std::max)(0.0f, gap) : (std::min)(0.0f, gap);
				hit = true;
				contact.xIndex = static_cast<uint32_t>(column);
				contact.yIndex = static_cast<uint32_t>(row);
			}
		}
		setComponent(position, axis, component(position, axis) + allowed);
		if (hit) {
			setComponent(contact.normal, axis, requested > 0.0f ? -1.0f : 1.0f);
			contact.center = position;
			contact.point = position;
			contact.time = std::abs(allowed / requested);
			contact.incomingVelocity = options.deltaTime > kEpsilon ? movement / options.deltaTime : Vector3{};
			contact.impactSpeed = std::abs(requested) / (std::max)(options.deltaTime, kEpsilon);
			result.contacts.push_back(contact);
			result.wall |= axis == 0 || axis == 2;
			result.landing |= axis == 1 && requested < 0.0f;
			result.ceiling |= axis == 1 && requested > 0.0f;
		}
	}
	result.movement = position - center;
	result.grounded = result.landing;
	if (!result.grounded && std::abs(position.y - playerHalf.y - floorTop) <= kEpsilon) {
		for (int row = firstRow; row <= lastRow && !result.grounded; ++row) {
			for (int column = firstColumn; column <= lastColumn; ++column) {
				if (mapChipField_->GetMapChipTypeByIndex(
					static_cast<uint32_t>(column), static_cast<uint32_t>(row)) != MapChipType::kBlock) {
					continue;
				}
				const Vector3 blockCenter = mapChipField_->GetTopDownPositionByIndex(
					static_cast<uint32_t>(column), static_cast<uint32_t>(row));
				result.grounded = overlaps(position.x, playerHalf.x, blockCenter.x, blockHalf.x) &&
				                  overlaps(position.z, playerHalf.z, blockCenter.z, blockHalf.z);
				if (result.grounded) { break; }
			}
		}
	}
	return result;
}
