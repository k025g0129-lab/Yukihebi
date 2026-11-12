#pragma once

#include "GameSettings.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

// 内部のチップ種別。CSVの数値は読み込み時にこの種別へ変換する。
// slice形式のCSV値とこのenumの数値は一致しない。読み込みはDecodeSliceChipで変換する。
// sliceの値はブラウザ版エディタと同じ: 2=固定針、5=ひび床、6=すべる床、7=穴、8=廃止、9=エネミー、
// 10=廃止（台車）、11=スイッチ、12=扉。13～16=45度の坂（ゲーム独自。Stage Editorで配置可能）。
// kMovingThornはCSVから置けない旧仕様の動く針。内部値への直接変換は禁止。
// 旧形式は内部値を使うため、2=開始地点、3=ゴール、4=穴。新旧形式を混同しない。
enum class MapChipType {
	kBlank = 0,
	kBlock = 1,
	kPlayer = 2,
	kGoal = 3,
	kHole = 4,
	kThorn = 5,
	kMovingThorn,
	kSlipperyFloor,
	kCrackedFloor,
	kEnemy,
	kRampPositiveX,
	kRampNegativeX,
	kRampPositiveZ,
	kRampNegativeZ,
	kSwitch,
	kDoor
};

enum class MapBlockKind { kFloor, kWall, kCeiling, kGoal, kThorn, kSlipperyFloor, kCrackedFloor, kGimmick, kRamp, kSwitchBase, kSwitchButton, kDoor, kCount };

// 床・壁・天井・坂・表示板の配置。坂だけはcenterを立方体の底面中心とし、
// halfSize.x/zを三角柱の水平半幅、halfSize.yを立方体の高さの半分として使う。
// 描画と接触判定は同じ形状を使う。
struct MapBlock {
	Vector3 center{};
	// 全体の幅・高さ・奥行きの半分。中心±半サイズで直方体の範囲が求まる。
	Vector3 halfSize{};
	int layer = 0; // 内部は0始まり、表示とCSVは1始まり。
	MapBlockKind kind = MapBlockKind::kFloor;
	// 物理の衝突を有効にするフラグ。描画の透明度とは独立で、ゴール表示などはfalse。
	bool solid = true;
	// 坂の上り方向をY軸まわりで回す。通常ブロックは単位行列。
	Matrix4x4 localRotation = MakeScaleMatrix({1.0f, 1.0f, 1.0f});
};

// 穴の階層から、下方で最初に着地できる床までをつなぐ静的な配置。
// 待機・上昇などの動的な状態はHoleが所有する。
struct HoleDefinition {
	int upperLayer = 0;
	int lowerLayer = 1;
	Vector3 center{}; // 上層の床の上面。箱のローカル座標。
	float halfSize = 1.0f;
};

// 針とエネミーの配置。centerは床上面のマス中央。
// エネミーはpatrol=trueで、routeの点（先頭はcenter）を順にたどる。loopなら周回、そうでなければ往復。
struct ThornDefinition {
	Vector3 center{};
	int layer = 0;
	bool moving = false;
	bool patrol = false;
	std::vector<Vector3> route;
	bool loop = false;
};

// スイッチと扉。同じchannel（エディタの色番号、1始まり）どうしが連動する。centerは床上面。
struct SwitchDoorDefinition {
	Vector3 center{};
	int layer = 0;
	int channel = 1;
	bool door = false;
};

struct CrackedFloorDefinition {
	Vector3 center{}; // 床上面。破壊状態はCrackedFloorが所有する。
	int layer = 0;
};

// CSVの読み込み → 整合性検証 → ローカル配置生成を担当する。
// 入力・物理更新・描画APIはここでは扱わない。Stageが生成結果を所有・利用する。
class MapChipField {
public:
	enum class Slice { kCeiling, kSpace, kFloor };
	// 配列は[行z][列x]。objectsは空間の障害物・開始地点、floorはその階層直下の床。
	using Grid = std::vector<std::vector<MapChipType>>;

	struct Layer {
		Grid objects;
		Grid floor;
	};

	// 成功ならtrue。失敗ならfalseとerrorを返す。壊れた途中データのブロックは利用させない。
	bool LoadMapChipCSV(const std::string& filePath, const GameSettings& settings, std::string& error);
	bool LoadForEditing(const std::string& filePath, const GameSettings& settings, std::string& error);
	// Editor用。編集中は一時的に不正な配置も許し、保存時に全体を検証する。
	bool SetSliceCell(int layer, Slice section, uint32_t x, uint32_t z, MapChipType type);
	MapChipType GetSliceCell(int layer, Slice section, uint32_t x, uint32_t z) const;
	bool SaveMapChipCSV(const std::string& filePath, std::string& error) const;
	const std::string& GetName() const { return name_; }
	// マス中心のXZと、その階層の床上面Yを返す。壁中心や球中心へのY補正は生成側で行う。
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t zIndex, int layer) const;
	// 空中の球の足元のローカルYから、近い床の階層を求める。接地中はPlayerが支持面を優先。
	int GetLayerByHeight(float localFootHeight) const;
	// 内部の0が最上層、数字が増えるほど下層。最下層の床上面をY=0とする。
	float GetFloorHeight(int layer) const;
	uint32_t GetNumBlockHorizontal() const { return width_; }
	uint32_t GetNumBlockDepth() const { return depth_; }
	int GetLayerCount() const { return static_cast<int>(layers_.size()); }
	const std::vector<MapBlock>& GetBlocks() const { return blocks_; }
	const std::vector<HoleDefinition>& GetHoles() const { return holes_; }
	const Vector3& GetStartPosition() const { return startPosition_; }
	const Vector3& GetGoalPosition() const { return goalPosition_; }
	int GetStartLayer() const { return startLayer_; }
	int GetGoalLayer() const { return goalLayer_; }
	// ゴールなしのマップも有効。GoalPositionだけを見ず、このフラグを先に確認する。
	bool HasGoal() const { return goalLayer_ >= 0; }
	Vector3 GetPivot() const;
	const std::vector<ThornDefinition>& GetThorns() const { return thorns_; }
	const std::vector<CrackedFloorDefinition>& GetCrackedFloors() const { return crackedFloors_; }
	const std::vector<SwitchDoorDefinition>& GetSwitchDoors() const { return switchDoors_; }

private:
	void ReadCSV(const std::string& filePath);
	// levelヘッダーと、ceiling/space/floorのsliceを上から順に読む現在のCSV形式。
	void ReadSliceCSV(std::istream& file, const std::vector<std::string>& header, int& lineNumber);
	// 先に作成したceiling/layer,N/floor,N形式用。数値の対応は現在のslice形式と異なる。
	void ReadLegacyCSV(std::istream& file);
	// 寸法・使用可能なチップ・開始地点・穴の着地点を確認してから形状を生成する。
	void Validate() const;
	// CSVの物理行と区画内の行・列を結び付け、配置エラーの修正箇所を示す。
	std::string GetCellLocation(int layer, bool floor, uint32_t x, uint32_t z) const;

	struct SourceLines {
		std::vector<int> objects;
		std::vector<int> floor;
	};

	std::vector<int> ceilingLines_;
	std::vector<SourceLines> sourceLines_;
	void GenerateBlocks();
	GameSettings settings_{};
	std::string name_;
	Grid ceiling_;
	std::vector<Layer> layers_;
	std::vector<MapBlock> blocks_;
	std::vector<HoleDefinition> holes_;
	uint32_t width_ = 0;
	uint32_t depth_ = 0;
	Vector3 startPosition_{};
	Vector3 goalPosition_{};
	// CSV末尾のenemy行。layerは内部の0始まり。cornersは置いたマスの後に続く曲がり角・終点。
	// 最後の点が置いたマスと同じなら周回、それ以外は往復（エディタと同じ）。
	struct RouteRow {
		int layer = 0;
		int x = 0;
		int z = 0;
		std::vector<std::pair<int, int>> corners;
		int line = 0;
	};
	// link行。スイッチ・扉のマスと色番号。
	struct LinkRow {
		int layer = 0;
		int x = 0;
		int z = 0;
		int channel = 1;
		int line = 0;
	};
	// エディタ形式のenemy/link行を1行読む。cellsは区切り済みの全セル。
	void ReadRouteRow(const std::vector<std::string>& cells, int lineNumber);
	void ReadLinkRow(const std::vector<std::string>& cells, int lineNumber);
	// sliceの番号（下から0始まり）を内部の階層番号へ変換する。種類が合わなければエラー。
	int LayerFromSliceHeight(int height, bool space, int lineNumber) const;
	// 経路の点を床上面のローカル座標へ変換する。先頭は置いたマス。
	std::vector<Vector3> RoutePoints(const RouteRow& row) const;
	static bool IsLoop(const RouteRow& row);
	int ChannelAt(int layer, uint32_t x, uint32_t z) const;
	std::vector<RouteRow> patrols_;
	std::vector<LinkRow> links_;
	std::vector<SwitchDoorDefinition> switchDoors_;
	std::vector<ThornDefinition> thorns_;
	std::vector<CrackedFloorDefinition> crackedFloors_;
	int startLayer_ = 0;
	// -1はゴール未配置。slice形式では4がゴール。最大1個の制限を維持する。
	int goalLayer_ = -1;
};
