#include "MapCollider.h"

#include <algorithm>
#include <cmath>

namespace {
	// 三角柱の断面上で、点から線分への最近点を求める。
	Vector3 ClosestOnSegment(const Vector3& point, const Vector3& a, const Vector3& b) {
		const Vector3 edge = b - a;
		const float lengthSquared = LengthSquared(edge);
		const float t = lengthSquared > 0.0f ? std::clamp(Dot(point - a, edge) / lengthSquared, 0.0f, 1.0f) : 0.0f;
		return a + edge * t;
	}

	bool CheckRampSphere(const Vector3& center, float radius, const MapBlock& block, MapCollider::Contact& contact) {
		const Vector3 local = InverseRotateVector(center - block.center, block.localRotation);
		const float half = block.halfSize.x;
		const float rise = block.halfSize.y * 2.0f;
		const float slope = rise / (half * 2.0f);
		const float slopeLength = std::sqrt(1.0f + slope * slope);
		const bool inside = local.x >= -half && local.x <= half && local.y >= 0.0f &&
			local.y <= (local.x + half) * slope && std::abs(local.z) <= block.halfSize.z;
		Vector3 nearest{};
		Vector3 normal{};
		float penetration = 0.0f;
		if (inside) {
			// 球心が内部なら、最も近い面の外向き法線で押し出す。
			const float distances[] = {((local.x + half) * slope - local.y) / slopeLength,
				local.y, half - local.x,
				block.halfSize.z + local.z, block.halfSize.z - local.z};
			const Vector3 normals[] = {{-slope / slopeLength, 1.0f / slopeLength, 0},
				{0, -1, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
			int face = 0;
			for (int i = 1; i < 5; ++i) {
				if (distances[i] < distances[face]) { face = i; }
			}
			normal = normals[face];
			nearest = local + normal * distances[face];
			penetration = radius + distances[face];
		} else {
			// 押し出し側面も含め、三角形断面の3辺と奥行きから最近点を作る。
			const Vector3 point{local.x, local.y, 0.0f};
			const Vector3 a{-half, 0.0f, 0.0f};
			const Vector3 b{half, 0.0f, 0.0f};
			const Vector3 c{half, rise, 0.0f};
			const bool insideSection = local.x >= -half && local.x <= half && local.y >= 0.0f &&
				local.y <= (local.x + half) * slope;
			Vector3 sectionPoint = point;
			if (!insideSection) {
				sectionPoint = ClosestOnSegment(point, a, b);
				const Vector3 highSide = ClosestOnSegment(point, b, c);
				const Vector3 slopeTop = ClosestOnSegment(point, c, a);
				if (LengthSquared(highSide - point) < LengthSquared(sectionPoint - point)) { sectionPoint = highSide; }
				if (LengthSquared(slopeTop - point) < LengthSquared(sectionPoint - point)) { sectionPoint = slopeTop; }
			}
			nearest = {sectionPoint.x, sectionPoint.y,
				std::clamp(local.z, -block.halfSize.z, block.halfSize.z)};
			const Vector3 difference = local - nearest;
			const float distanceSquared = LengthSquared(difference);
			if (distanceSquared > radius * radius) { return false; }
			const float distance = std::sqrt(distanceSquared);
			if (distance > 0.000001f) {
				normal = difference / distance;
				penetration = radius - distance;
			} else {
				// 境界上では外側の面を選ぶ。ゼロ法線での速度解決を避ける。
				normal = local.z >= 0.0f ? Vector3{0, 0, 1} : Vector3{0, 0, -1};
				penetration = radius;
			}
		}
		contact.normal = RotateVector(normal, block.localRotation);
		contact.point = block.center + RotateVector(nearest, block.localRotation);
		contact.penetration = penetration;
		return true;
	}
}

bool MapCollider::CheckSphere(const Vector3& center, float radius, const MapBlock& block, Contact& contact) {
	if (block.kind == MapBlockKind::kRamp) { return CheckRampSphere(center, radius, block, contact); }
	// 通常ブロックは直方体として判定する。
	const Vector3 localCenter = InverseRotateVector(center - block.center, block.localRotation);
	const Vector3 minimum = -block.halfSize;
	const Vector3 maximum = block.halfSize;
	// 球中心の各成分を直方体の範囲へclampすると、直方体上の最近点になる。
	const Vector3 nearestPoint = {std::clamp(localCenter.x, minimum.x, maximum.x),
		std::clamp(localCenter.y, minimum.y, maximum.y), std::clamp(localCenter.z, minimum.z, maximum.z)};
	const Vector3 difference = localCenter - nearestPoint;
	const float distanceSquared = LengthSquared(difference);
	// 最近点までの距離が半径より大きければ非接触。平方距離で比較してsqrtを避ける。
	if (distanceSquared > radius * radius) { return false; }
	// 中心が外側なら最近点から中心への方向を法線にする。
	// 中心が面上・内部なら差がほぼ0になるので、下の最近面を探す処理へ進む。
	if (distanceSquared > 0.00000001f) {
		const float distance = std::sqrt(distanceSquared);
		contact.normal = RotateVector(difference / distance, block.localRotation);
		contact.point = block.center + RotateVector(nearestPoint, block.localRotation);
		contact.penetration = radius - distance;
		return true;
	}

	// 球の中心まで内部へ入った場合は、最も近い面へ押し出す。
	// 6面への距離の並びはnormals配列と対応する。最も近い面を選ぶ。
	const float distances[] = {localCenter.x - minimum.x, maximum.x - localCenter.x,
		localCenter.y - minimum.y, maximum.y - localCenter.y, localCenter.z - minimum.z, maximum.z - localCenter.z};
	const Vector3 normals[] = {{-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}};
	int nearest = 0;
	for (int index = 1; index < 6; ++index) {
		if (distances[index] < distances[nearest]) { nearest = index; }
	}
	contact.normal = RotateVector(normals[nearest], block.localRotation);
	contact.point = block.center + RotateVector(localCenter + normals[nearest] * distances[nearest], block.localRotation);
	// 中心から面までの距離に球半径を足して、球全体を外へ出す。
	contact.penetration = radius + distances[nearest];
	return true;
}

float MapCollider::ResolveVelocity(Vector3& velocity, Vector3& angularVelocity, const Vector3& normal,
	const Vector3& surfaceVelocity, const GameSettings& settings) {
	return ResolveVelocity(velocity, angularVelocity, normal, surfaceVelocity, settings, settings.restitution);
}

float MapCollider::ResolveVelocity(Vector3& velocity, Vector3& angularVelocity, const Vector3& normal,
	const Vector3& surfaceVelocity, const GameSettings& settings, float contactRestitution) {
	return ResolveVelocity(velocity, angularVelocity, normal, surfaceVelocity, settings, contactRestitution, settings.friction);
}

float MapCollider::ResolveVelocity(Vector3& velocity, Vector3& angularVelocity, const Vector3& normal,
	const Vector3& surfaceVelocity, const GameSettings& settings, float contactRestitution, float friction) {
	// 固定床でも回転で動く床でも、球速度 - 面速度の相対速度を使う。
	Vector3 relative = velocity - surfaceVelocity;
	const float normalSpeed = Dot(relative, normal);
	// 面から離れる方向なら反発を加えない。位置のめり込みは呼び出し元で補正済み。
	if (normalSpeed >= 0.0f) { return 0.0f; }
	// 十分な速さで近づいた場合だけ反発する。低速接地の小刻みな跳ねを抑える。
	const float restitution = normalSpeed < -settings.bounceThreshold ? contactRestitution : 0.0f;
	// 法線方向の速度を反発係数ぶん反転させる力積。力積/質量が速度変化。
	const float normalImpulse = -(1.0f + restitution) * normalSpeed * settings.ballMass;
	velocity += normal * (normalImpulse / settings.ballMass);

	// 球の接触点の速度も考慮する。摩擦の力積を並進と回転へ分配することで転がる。
	// 球中心から接触点までの腕。角速度×腕による接触点の回転速度も含める。
	const Vector3 arm = -normal * settings.ballRadius;
	relative = velocity - surfaceVelocity + Cross(angularVelocity, arm);
	// 法線成分を除き、面に沿う滑りの方向と速さを取り出す。
	const Vector3 tangent = relative - normal * Dot(relative, normal);
	const float tangentSpeed = Length(tangent);
	if (tangentSpeed <= 0.000001f) { return normalImpulse; }
	// 一様な中身の詰まった球の慣性モーメント I = (2/5)mr^2 を仮定する。
	const float inertia = (2.0f / 5.0f) * settings.ballMass * settings.ballRadius * settings.ballRadius;
	// 摩擦は並進と回転の両方を変えるため、回転分も含む有効質量を使う。
	const float inverseEffectiveMass = 1.0f / settings.ballMass + settings.ballRadius * settings.ballRadius / inertia;
	// 滑りを止めるための力積を、摩擦係数×法線力積までに制限する（クーロン摩擦）。
	const float frictionImpulse = (std::min)(tangentSpeed / inverseEffectiveMass, friction * normalImpulse);
	const Vector3 impulse = -tangent * (frictionImpulse / tangentSpeed);
	velocity += impulse / settings.ballMass;
	// 摩擦力積で並進を変え、腕×力積によるトルク成分で球を回転させる。
	angularVelocity += Cross(arm, impulse) / inertia;
	// 当たりの強さ。呼び出し元がしきい値と比べ、演出を出すかどうかを決める。
	return normalImpulse;
}
