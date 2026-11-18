#pragma once

#include <KamataEngine.h>
#include <cstdint>
#include <vector>

#include "GameMath.h"

using namespace KamataEngine;

class MapChipField;

class MapCollider {
public:
	enum Corner {
		kTopLeft,
		kTopRight,
		kBottomLeft,
		kBottomRight,
		kNumCorners,
	};

	static constexpr float kBlank = 0.01f;
	static constexpr float kGroundProbeDepth = 0.05f;
	static constexpr float kEpsilon = 0.00001f;

	struct Settings {
		float skin = kBlank;
		float maxGroundAngle = kPi * 0.25f; // 45度
		int maxOverlapIterations = 8;
		int maxMoveIterations = 8;
	};

	struct MoveOptions {
		// 接地中の歩行や滑走で、地面の傾きに沿って移動するときに使う。
		bool followGround = false;
		Vector3 groundNormal{0.0f, 1.0f, 0.0f};

		bool allowGroundSnap = false;
		float groundSnapDistance = kGroundProbeDepth;

		// 0なら滑る・止まる。1なら法線方向の速さを保って跳ねる。
		float restitution = 0.0f;
		float minBounceSpeed = 0.0f;
		float deltaTime = 1.0f / kFramesPerSecond;
	};

	struct Contact {
		// 地形から移動物体へ向かう法線。
		Vector3 normal{};
		Vector3 center{}; // 接触した時点の物体中心
		// 接触時の矩形上の代表点。粒子の発生位置に使う。
		Vector3 point{};

		uint32_t xIndex = 0;
		uint32_t yIndex = 0;

		float time = 0.0f; // このフレーム内の接触時刻、0～1
		Vector3 incomingVelocity{}; // 接触直前の速度、毎秒単位
		float impactSpeed = 0.0f; // 面へ向かう速度、毎秒単位
		float restitution = 0.0f;

		// 初期めり込みの補正。衝突エフェクトには使わない。
		bool correctionOnly = false;
	};

	struct Result {
		bool ceiling = false;
		bool landing = false;
		bool wall = false;

		Vector3 movement{};
		std::vector<Contact> contacts;

		bool grounded = false;
		Vector3 groundNormal{0.0f, 1.0f, 0.0f};
		bool iterationLimit = false;
	};

	struct GroundResult {
		bool hit = false;
		Vector3 normal{0.0f, 1.0f, 0.0f};
		Vector3 movement{};
	};

	void SetMapChipField(MapChipField* field) { mapChipField_ = field; }
	// XZ に並べた床ブロックを三次元の箱として判定する。
	void SetTopDownMode(bool enabled) { topDownMode_ = enabled; }

	void SetSize(float width, float height) {
		width_ = width;
		height_ = height;
	}

	void SetSettings(const Settings& settings) { settings_ = settings; }

	bool IsWalkableSurface(const Vector3& normal) const;

	Result Check(const Vector3& center, const Vector3& movement);
	Result Check(const Vector3& center, const Vector3& movement, const MoveOptions& options);

	GroundResult ProbeGround(const Vector3& center, float distance) const;
	bool IsOnGround(const Vector3& center) const;

	// 既存APIを使う箇所のため残す。
	Vector3 CornerPosition(const Vector3& center, Corner corner) const;

	static Vector3 ResolveVelocity(const Vector3& velocity, const Contact& contact);

private:
	Result CheckTopDown(const Vector3& center, const Vector3& movement, const MoveOptions& options);
	MapChipField* mapChipField_ = nullptr;
	bool topDownMode_ = false;
	float width_ = 0.8f;
	float height_ = 0.8f;
	Settings settings_{};
};
