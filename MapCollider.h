#pragma once

#include "MapChipField.h"

// 球と箱ローカルの直方体・三角柱の共通判定。表示階層は参照しない。
class MapCollider {
public:
	// 判定結果はすべて箱ローカル。CheckSphereがtrueの場合に参照する。
	struct Contact {
		// ブロックから球へ向く単位法線。押し出しと反発の方向。
		Vector3 normal{};
		// ブロック上の接触点。Stageがこの点の面の速度を求める。
		Vector3 point{};
		// 球を外へ出す距離。余裕のcontactMarginはPlayer側で加える。
		float penetration = 0.0f;
	};
	// 坂は三角柱、それ以外は直方体として判定する。中心が内部に入った場合も扱う。
	static bool CheckSphere(const Vector3& center, float radius, const MapBlock& block, Contact& contact);
	// こちらの引数はワールド座標。並進速度と角速度を接触の力積で更新する。
	// 戻り値は法線方向の力積。0なら面から離れる方向で、反発も摩擦も加えていない。
	// 物理の計算は変わらない。当たりの強さを演出へ渡すために返す。
	static float ResolveVelocity(Vector3& velocity, Vector3& angularVelocity, const Vector3& normal,
		const Vector3& surfaceVelocity, const GameSettings& settings);
	// 面ごとの反発を指定する。既存の呼び出しではsettings.restitutionを使用する。
	static float ResolveVelocity(Vector3& velocity, Vector3& angularVelocity, const Vector3& normal,
		const Vector3& surfaceVelocity, const GameSettings& settings, float contactRestitution);
	// 氷床など、面ごとに摩擦も指定する場合。既存の呼び出しは共通の摩擦を使う。
	static float ResolveVelocity(Vector3& velocity, Vector3& angularVelocity, const Vector3& normal,
		const Vector3& surfaceVelocity, const GameSettings& settings, float contactRestitution, float friction);
};
