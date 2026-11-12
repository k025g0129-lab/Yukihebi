#include "MapChipField.h"
#include <algorithm>
#include <map>

#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {
	std::map<std::string, MapChipType> mapChipTable = {
		{"0", MapChipType::kBlank},
		{"1", MapChipType::kBlock},


	};
}

void MapChipField::ResetMapChipData() {
	loadedBlockHorizontal_ = 0;
	mapChipDate.data.clear();
	mapChipDate.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipType>& mapchipDataLine : mapChipDate.data) {
		mapchipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCSV(const std::string& filePath) {
	// マップチップデータをリセット
	ResetMapChipData();

	// CSVファイルを開く
	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	// CSVファイルを1行ずつ読み込む
	std::stringstream mapChipCsv;
	//ファイル内容を文字列ストリームにcopy
	mapChipCsv << file.rdbuf();

	file.close();

	//csvからマップチップデーターを読み込む
	for (uint32_t i = 0; i < kNumBlockVirtical; i++) {
		std::string line;
		getline(mapChipCsv, line);
		if (!line.empty()) {
			const uint32_t columns = static_cast<uint32_t>(std::count(line.begin(), line.end(), ',')) + 1;
			loadedBlockHorizontal_ = (std::max)(loadedBlockHorizontal_,
			                                    (std::min)(columns, kNumBlockHorizontal));
		}

		//1行分の文字列をストリームに変換して解析しやすくする
		std::stringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; j++) {
			std::string word;
			std::getline(lineStream, word, ',');
			// UTF-8署名付きCSVの先頭セルも通常の番号として読む。
			if (i == 0 && j == 0 && word.starts_with("\xEF\xBB\xBF")) {
				word.erase(0, 3);
			}
			// CRLF の行末セルにも 1 などの番号を正しく適用する。
			if (!word.empty() && word.back() == '\r') {
				word.pop_back();
			}

			if (mapChipTable.contains(word)) {
				mapChipDate.data[i][j] = mapChipTable[word];
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	// xIndex と yIndex は符号なしなので、負の値は巨大な値になって上限側で弾かれる。
	// そのため「0未満か」の判定は書かない。書くと常に false の比較になって警告が出る。
	if (xIndex >= kNumBlockHorizontal) {
		return MapChipType::kBlank;
	}

	if (yIndex >= kNumBlockVirtical) {
		return MapChipType::kBlank;
	}

	return mapChipDate.data[yIndex][xIndex];
}

void MapChipField::SetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex, MapChipType type) {
	if (xIndex < kNumBlockHorizontal && yIndex < kNumBlockVirtical) {
		mapChipDate.data[yIndex][xIndex] = type;
	}
}


Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	return Vector3(kBlockWindth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex), 0);
}


// インデックスから、そのブロックが占める矩形を返す。
// 当たり判定でめり込み量を求めるのに使う。
MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	// 指定ブロックの中心座標を取得する
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = center.x - kBlockWindth * 0.5f;
	rect.right = center.x + kBlockWindth * 0.5f;
	rect.bottom = center.y - kBlockHeight * 0.5f;
	rect.top = center.y + kBlockHeight * 0.5f;
	return rect;
}


MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) {
	IndexSet indexSet;
	float xCell = std::floor((position.x + kBlockWindth * 0.5f) / kBlockWindth);
	float yCell = std::floor((position.y + kBlockHeight * 0.5f) / kBlockHeight);
	indexSet.xIndex = static_cast<uint32_t>(xCell);
	indexSet.yIndex = static_cast<uint32_t>((kNumBlockVirtical - 1) - yCell);
	return indexSet;
}
