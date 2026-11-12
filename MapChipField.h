#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "GameMath.h"

enum class MapChipType {
	kBlank, //空白 0
	kBlock, // 通常ブロック 1
};

struct MapChipDate {
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {
public:
	// ブロックの幅と高さは、ワールド座標での単位。1.0f が 1 マス。
	static constexpr float kBlockWindth = 1.0f;
	static constexpr float kBlockHeight = 1.0f;
	static constexpr float kTopDownBlockThickness = 0.1f;
	static constexpr float kTopDownFloorTop = 0.0f;

	//blocの個数
	static constexpr uint32_t kNumBlockVirtical = 500;
	static constexpr uint32_t kNumBlockHorizontal = 500;

	// マップチップデータ
	MapChipDate mapChipDate;
	// マップチップデータをリセット
	void ResetMapChipData();
	// CSVファイルからマップチップデータを読み込む
	void LoadMapChipCSV(const std::string& filePath);
	// インデックスからマップチップの種類を取得
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	// インデックスからマップチップの種類を設定
	void SetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex, MapChipType type);
	//static bool IsSolid(MapChipType type);
	static bool IsSlippery(MapChipType type);

	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);
	// CSV の行・列を、原点中心の見下ろし用 XZ 座標へ変換する。
	Vector3 GetTopDownPositionByIndex(uint32_t xIndex, uint32_t yIndex) const;

	uint32_t GetNumBlockVirtical() { return kNumBlockVirtical; }
	uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal; }
	// CSV に実際に記載された列数。固定配列の余白を外周扱いしないために使う。
	uint32_t GetLoadedBlockHorizontal() const { return loadedBlockHorizontal_; }
	// CSV に実際に記載された行数。
	uint32_t GetLoadedBlockVirtical() const { return loadedBlockVirtical_; }

	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

	// 2 次元インデックス
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	struct SlopeShape {
		// ワールド座標での判定用頂点。
		std::array<Vector3, 3> vertices{};

		// 基準OBJに適用するZ回転角。
		float rotationZ = 0.0f;
	};

	// 通常ブロックや空白ならfalse。
	// 斜面なら、そのマスの頂点と描画角度を返す。
	bool TryGetSlopeShape(uint32_t xIndex, uint32_t yIndex, SlopeShape& shape);
	// インデックスからブロック矩形を取得
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);
	// ワールド座標からインデックスを取得
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position);

private:
	uint32_t loadedBlockHorizontal_ = 0;
	uint32_t loadedBlockVirtical_ = 0;
};
