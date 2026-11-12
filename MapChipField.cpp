#include "MapChipField.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
// セル前後の空白と改行文字を除く。数値セルの中身は厳密に検証する。
std::string Trim(const std::string& text) {
	const size_t first = text.find_first_not_of(" \t\r\n");
	return first == std::string::npos ? "" : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

// このCSVは単純なカンマ区切りを前提とする。引用符付きセルやセル内カンマには非対応。
std::vector<std::string> Split(const std::string& line) {
	std::vector<std::string> cells;
	std::stringstream stream(line);
	std::string cell;
	while (std::getline(stream, cell, ',')) {
		cells.push_back(Trim(cell));
	}
	// 末尾カンマの空セルも残し、列数の不一致を検出できるようにする。
	if (!line.empty() && line.back() == ',') {
		cells.emplace_back();
	}
	return cells;
}

// Excelはマップ本体と同じ列数になるよう、見出しの右にも空セルを保存する。
// 見出しだけ末尾の空セルを除き、途中の空欄や余分な値は通常の検証で検出する。
// マップ本体はSplitを使い、指定された列数を厳密に確認する。
std::vector<std::string> SplitHeader(const std::string& line) {
	auto cells = Split(line);
	while (!cells.empty() && cells.back().empty()) {
		cells.pop_back();
	}
	return cells;
}

// 空行と#から始まるコメント行を飛ばす。物理行番号は進めてエラー表示に使う。
// 先頭のUTF-8 BOMだけを取り除き、署名付き保存でもlevelやsliceを正しく識別する。
bool ReadDataLine(std::istream& file, std::string& line, int& lineNumber) {
	while (std::getline(file, line)) {
		++lineNumber;
		if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
			line.erase(0, 3);
		}
		line = Trim(line);
		if (!line.empty() && line.front() != '#') {
			return true;
		}
	}
	return false;
}

// エラー位置は1始まり。画像フォントで表示できるASCIIを使い、行・列を明記する。
std::string CSVLocation(int line, int column = 0) {
	std::string result = "CSV line " + std::to_string(line);
	if (column > 0) {
		result += ", column " + std::to_string(column);
	}
	return result;
}

[[noreturn]] void FailAt(int line, const std::string& message) { throw std::runtime_error(CSVLocation(line) + ": " + message); }

// stoi自体の例外も、対象の項目名と実際の値が分かるエラーへ変換する。
int ReadPositiveInteger(const std::string& text, int line, const std::string& field) {
	size_t consumed = 0;
	int value = 0;
	try {
		value = std::stoi(text, &consumed);
	} catch (const std::invalid_argument&) {
		FailAt(line, field + " must be a positive integer. Found: '" + text + "'.");
	} catch (const std::out_of_range&) {
		FailAt(line, field + " is outside the supported integer range. Found: '" + text + "'.");
	}
	if (consumed != text.size() || value <= 0) {
		FailAt(line, field + " must be a positive integer. Found: '" + text + "'.");
	}
	return value;
}

// 0以上の整数。enemy行の座標と高さに使う。
int ReadNonNegativeInteger(const std::string& text, int line, const std::string& field) {
	size_t consumed = 0;
	int value = -1;
	try {
		value = std::stoi(text, &consumed);
	} catch (const std::exception&) {
		FailAt(line, field + " must be a non-negative integer. Found: '" + text + "'.");
	}
	if (consumed != text.size() || value < 0) {
		FailAt(line, field + " must be a non-negative integer. Found: '" + text + "'.");
	}
	return value;
}

// CSVの外部表現を内部種別へ変換する。enumの数値とは別の対応表。
MapChipType DecodeSliceChip(const std::string& cell, int lineNumber, int column) {
	if (cell == "0") {
		return MapChipType::kBlank;
	}
	if (cell == "1") {
		return MapChipType::kBlock;
	}
	if (cell == "2") {
		return MapChipType::kThorn;
	}
	if (cell == "3") {
		return MapChipType::kPlayer;
	}
	if (cell == "4") {
		return MapChipType::kGoal;
	}
	if (cell == "5") {
		return MapChipType::kCrackedFloor;
	}
	if (cell == "6") {
		return MapChipType::kSlipperyFloor;
	}
	if (cell == "7") {
		return MapChipType::kHole;
	}
	// 8は廃止したワープ。別の種類として読まず、置き換えを求める。
	if (cell == "8") {
		throw std::runtime_error(CSVLocation(lineNumber, column) + ": 8 (warp) is no longer used.\nUse 1 for floor, or 7 for a hole with a launcher below.");
	}
	if (cell == "9") {
		return MapChipType::kEnemy;
	}
	// 10は削除した台車。別の種類として読まず、置き換えを求める。
	if (cell == "10") {
		throw std::runtime_error(CSVLocation(lineNumber, column) + ": 10 (cart) is no longer used.\nUse 1 for floor, or 0 for an opening.");
	}
	if (cell == "11") {
		return MapChipType::kSwitch;
	}
	if (cell == "12") {
		return MapChipType::kDoor;
	}
	// 45度の坂はゲーム独自の番号。エディタの10～12と重ならないよう13から使う。
	if (cell == "13") {
		return MapChipType::kRampPositiveX;
	}
	if (cell == "14") {
		return MapChipType::kRampNegativeX;
	}
	if (cell == "15") {
		return MapChipType::kRampPositiveZ;
	}
	if (cell == "16") {
		return MapChipType::kRampNegativeZ;
	}
	throw std::runtime_error(
	    CSVLocation(lineNumber, column) + ": unknown chip '" + cell +
	    "'.\nUse 0=empty, 1=block, 2=thorn, 3=start, 4=goal, 5=cracked floor, 6=ice, 7=hole, 9=enemy,\n"
	    "11=switch, 12=door, 13/14/15/16=ramp up to +X/-X/+Z/-Z.");
}

bool IsRampChip(MapChipType type) { return type == MapChipType::kRampPositiveX || type == MapChipType::kRampNegativeX || type == MapChipType::kRampPositiveZ || type == MapChipType::kRampNegativeZ; }

bool IsSolidFloorChip(MapChipType type) { return type == MapChipType::kBlock || type == MapChipType::kSlipperyFloor || type == MapChipType::kCrackedFloor; }
} // namespace

bool MapChipField::LoadMapChipCSV(const std::string& filePath, const GameSettings& settings, std::string& error) {
	// Rで読み直しても旧マップの配列・開始位置・ゴール有無を残さないよう初期状態へ戻す。
	*this = MapChipField();
	settings_ = settings;
	try {
		// 読む → 内容を検証する → 形状を作る、の順。検証前に隣接階層や不正な位置を使わない。
		ReadCSV(filePath);
		Validate();
		GenerateBlocks();
		error.clear();
		return true;
	} catch (const std::exception& exception) {
		// ファイル・数値変換・形式エラーをここで受け止め、Sceneへ表示用の理由を返す。
		// どのマップで失敗したかを、パスと具体的な理由を分けて表示する。
		error = "Map: " + filePath + "\n" + exception.what();
		blocks_.clear();
		holes_.clear();
		thorns_.clear();
		crackedFloors_.clear();
		return false;
	}
}

bool MapChipField::LoadForEditing(const std::string& filePath, const GameSettings& settings, std::string& error) {
	*this = MapChipField();
	settings_ = settings;
	try {
		ReadCSV(filePath);
		error.clear();
		return true;
	} catch (const std::exception& exception) {
		error = "Map: " + filePath + "\n" + exception.what();
		return false;
	}
}

void MapChipField::ReadCSV(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file) {
		throw std::runtime_error("Cannot open the map file.\nCheck mapPath, the working directory and file access.");
	}
	std::string line;
	int lineNumber = 0;
	if (!ReadDataLine(file, line, lineNumber)) {
		throw std::runtime_error("Map is empty.");
	}
	const auto header = SplitHeader(line);
	if (header.empty()) {
		FailAt(lineNumber, "Header has no values. Expected level,name,width,depth,layers.");
	}
	// 先頭キーワードで形式を判別する。新形式の不正データを旧形式として読み直す処理ではない。
	if (header[0] == "level") {
		ReadSliceCSV(file, header, lineNumber);
	} else {
		// 先に作成したstage01.csvも利用できるよう、従来形式の入口を残す。
		file.clear();
		file.seekg(0);
		ReadLegacyCSV(file);
	}
}

void MapChipField::ReadSliceCSV(std::istream& file, const std::vector<std::string>& header, int& lineNumber) {
	// 見出し末尾の空セルはSplitHeaderで除去済み。必須項目と余分な値を確認する。
	if (header.size() != 5) {
		const std::string hint = header.size() > 5 ? "Remove extra non-empty fields after the layer count." : "Add the missing header fields.";
		FailAt(lineNumber, "level header has " + std::to_string(header.size()) + " fields; expected 5.\n" + hint + "\nExpected: level,name,width,depth,layers");
	}
	if (header[1].empty()) {
		FailAt(lineNumber, "Map name is empty (column 2).");
	}
	name_ = header[1];
	width_ = static_cast<uint32_t>(ReadPositiveInteger(header[2], lineNumber, "Width (column 3)"));
	depth_ = static_cast<uint32_t>(ReadPositiveInteger(header[3], lineNumber, "Depth (column 4)"));
	const int layerCount = ReadPositiveInteger(header[4], lineNumber, "Layer count (column 5)");
	if (layerCount > (std::numeric_limits<int>::max)() / 2) {
		FailAt(lineNumber, "Too many layers.");
	}
	std::string line;
	// 2階層なら4,ceiling,0 → 3,space,1 → 2,floor,1 → 1,space,2 → 0,floor,2。
	for (int slice = layerCount * 2; slice >= 0; --slice) {
		const bool ceiling = slice == layerCount * 2;
		const bool space = !ceiling && slice % 2 == 1;
		const int layer = ceiling ? 0 : (layerCount * 2 - slice + 1) / 2;
		const std::string kind = ceiling ? "ceiling" : (space ? "space" : "floor");
		const std::string expected = "slice," + std::to_string(slice) + "," + kind + "," + std::to_string(layer);
		if (!ReadDataLine(file, line, lineNumber)) {
			FailAt(lineNumber + 1, "Missing slice header.\nExpected: " + expected);
		}
		const auto cells = SplitHeader(line);
		if (cells.size() != 4) {
			const std::string hint = cells.size() > 4 ? "Remove extra non-empty header fields." : "Add the missing header fields.";
			FailAt(lineNumber, "slice header has " + std::to_string(cells.size()) + " fields; expected 4.\n" + hint + "\nExpected: " + expected);
		}
		if (cells[0] != "slice" || cells[1] != std::to_string(slice) || cells[2] != kind || cells[3] != std::to_string(layer)) {
			FailAt(lineNumber, "Wrong slice header.\nExpected: " + expected + "\nFound: " + line);
		}
		if (space) {
			layers_.emplace_back();
			sourceLines_.emplace_back();
		}
		Grid& grid = ceiling ? ceiling_ : (space ? layers_.back().objects : layers_.back().floor);
		std::vector<int>& rowLines = ceiling ? ceilingLines_ : (space ? sourceLines_.back().objects : sourceLines_.back().floor);
		for (uint32_t z = 0; z < depth_; ++z) {
			if (!ReadDataLine(file, line, lineNumber)) {
				FailAt(lineNumber + 1, "Missing row " + std::to_string(z + 1) + " in " + expected + ".");
			}
			const auto rowCells = Split(line);
			if (rowCells.size() != width_) {
				FailAt(
				    lineNumber,
				    kind + " layer " + std::to_string(layer) + ", row " + std::to_string(z + 1) + ": expected " + std::to_string(width_) + " columns, found " + std::to_string(rowCells.size()) + ".");
			}
			std::vector<MapChipType> row;
			for (uint32_t x = 0; x < width_; ++x) {
				row.push_back(DecodeSliceChip(rowCells[x], lineNumber, static_cast<int>(x + 1)));
			}
			grid.push_back(std::move(row));
			rowLines.push_back(lineNumber);
		}
	}
	// エディタはエネミー・スイッチの追加情報をsliceの後ろへ1行ずつ書く。
	while (ReadDataLine(file, line, lineNumber)) {
		const auto cells = SplitHeader(line);
		const std::string head = cells.empty() ? std::string() : cells[0];
		if (head == "enemy") {
			ReadRouteRow(cells, lineNumber);
			continue;
		}
		if (head == "cart") {
			FailAt(lineNumber, "cart rows are no longer used (the cart was removed). Delete this row.");
		}
		if (head == "link") {
			ReadLinkRow(cells, lineNumber);
			continue;
		}
		FailAt(lineNumber, "Unexpected data after the last slice. Remove the extra section or row.");
	}
}

int MapChipField::LayerFromSliceHeight(int height, bool space, int lineNumber) const {
	const int slices = GetLayerCount() * 2 + 1;
	const int fromTop = slices - 1 - height;
	if (height >= slices || fromTop <= 0 || (fromTop % 2 == 1) != space) {
		FailAt(lineNumber, "Height " + std::to_string(height) + " is not a " + (space ? "space" : "floor") + " slice.");
	}
	return space ? (fromTop - 1) / 2 : fromTop / 2 - 1;
}

// enemy,高さ,x,y[,x1,y1,...]。高さは空間のsliceの番号（下から0始まり）。
// x,yが置いたマス、続く座標が曲がり角・終点。
void MapChipField::ReadRouteRow(const std::vector<std::string>& cells, int lineNumber) {
	const std::string head = "enemy";
	if (cells.size() < 4 || cells.size() % 2 != 0) {
		FailAt(lineNumber, "Expected " + head + ",height,x,y[,x1,y1,...].");
	}
	std::vector<int> values;
	for (size_t index = 1; index < cells.size(); ++index) {
		values.push_back(ReadNonNegativeInteger(cells[index], lineNumber, head + " value (column " + std::to_string(index + 1) + ")"));
	}
	RouteRow row;
	row.layer = LayerFromSliceHeight(values[0], true, lineNumber);
	row.x = values[1];
	row.z = values[2];
	row.line = lineNumber;
	for (size_t index = 3; index + 1 < values.size(); index += 2) {
		row.corners.emplace_back(values[index], values[index + 1]);
	}
	patrols_.push_back(row);
}

// link,高さ,x,y,色番号。スイッチ・扉のマス1つにつき1行。色番号が同じものどうしが連動する。
void MapChipField::ReadLinkRow(const std::vector<std::string>& cells, int lineNumber) {
	if (cells.size() != 5) {
		FailAt(lineNumber, "Expected link,height,x,y,color.");
	}
	LinkRow link;
	link.layer = LayerFromSliceHeight(ReadNonNegativeInteger(cells[1], lineNumber, "link height"), true, lineNumber);
	link.x = ReadNonNegativeInteger(cells[2], lineNumber, "link x");
	link.z = ReadNonNegativeInteger(cells[3], lineNumber, "link y");
	link.channel = ReadPositiveInteger(cells[4], lineNumber, "link color");
	link.line = lineNumber;
	links_.push_back(link);
}

bool MapChipField::IsLoop(const RouteRow& row) { return row.corners.size() >= 2 && row.corners.back().first == row.x && row.corners.back().second == row.z; }

std::vector<Vector3> MapChipField::RoutePoints(const RouteRow& row) const {
	std::vector<Vector3> points{GetMapChipPositionByIndex(static_cast<uint32_t>(row.x), static_cast<uint32_t>(row.z), row.layer)};
	for (const auto& corner : row.corners) {
		points.push_back(GetMapChipPositionByIndex(static_cast<uint32_t>(corner.first), static_cast<uint32_t>(corner.second), row.layer));
	}
	return points;
}

int MapChipField::ChannelAt(int layer, uint32_t x, uint32_t z) const {
	for (const LinkRow& link : links_) {
		if (link.layer == layer && link.x == static_cast<int>(x) && link.z == static_cast<int>(z)) {
			return link.channel;
		}
	}
	// link行が無いマスはエディタと同じく色1として扱う。
	return 1;
}

MapChipType MapChipField::GetSliceCell(int layer, Slice section, uint32_t x, uint32_t z) const {
	if (x >= width_ || z >= depth_ || (section != Slice::kCeiling && (layer < 0 || layer >= GetLayerCount()))) {
		return MapChipType::kBlank;
	}
	const Grid& grid = section == Slice::kCeiling ? ceiling_ : (section == Slice::kSpace ? layers_[layer].objects : layers_[layer].floor);
	return grid[z][x];
}

bool MapChipField::SetSliceCell(int layer, Slice section, uint32_t x, uint32_t z, MapChipType type) {
	if (x >= width_ || z >= depth_ || (section != Slice::kCeiling && (layer < 0 || layer >= GetLayerCount()))) {
		return false;
	}
	Grid& grid = section == Slice::kCeiling ? ceiling_ : (section == Slice::kSpace ? layers_[layer].objects : layers_[layer].floor);
	grid[z][x] = type;
	return true;
}

bool MapChipField::SaveMapChipCSV(const std::string& filePath, std::string& error) const {
	try {
		Validate();
	} catch (const std::exception& exception) {
		error = exception.what();
		return false;
	}
	std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
	if (!file) {
		error = "Cannot write map: " + filePath;
		return false;
	}
	file.write("\xEF\xBB\xBF", 3);
	file << "level," << name_ << ',' << width_ << ',' << depth_ << ',' << GetLayerCount() << '\n';
	const auto value = [](MapChipType chip) {
		switch (chip) {
		case MapChipType::kBlank:
			return 0;
		case MapChipType::kBlock:
			return 1;
		case MapChipType::kThorn:
			return 2;
		case MapChipType::kPlayer:
			return 3;
		case MapChipType::kGoal:
			return 4;
		case MapChipType::kCrackedFloor:
			return 5;
		case MapChipType::kSlipperyFloor:
			return 6;
		case MapChipType::kHole:
			return 7;
		case MapChipType::kEnemy:
			return 9;
		case MapChipType::kRampPositiveX:
			return 13;
		case MapChipType::kRampNegativeX:
			return 14;
		case MapChipType::kRampPositiveZ:
			return 15;
		case MapChipType::kRampNegativeZ:
			return 16;
		case MapChipType::kSwitch:
			return 11;
		case MapChipType::kDoor:
			return 12;
		// CSVから読めない旧仕様のため到達しない。
		case MapChipType::kMovingThorn:
			return 0;
		}
		return 0;
	};
	const auto writeGrid = [&](int slice, const char* kind, int layer, const Grid& grid) {
		file << "slice," << slice << ',' << kind << ',' << layer << '\n';
		for (const auto& row : grid) {
			for (size_t x = 0; x < row.size(); ++x) {
				if (x != 0) {
					file << ',';
				}
				file << value(row[x]);
			}
			file << '\n';
		}
	};
	writeGrid(GetLayerCount() * 2, "ceiling", 0, ceiling_);
	for (int layer = 0; layer < GetLayerCount(); ++layer) {
		writeGrid(GetLayerCount() * 2 - layer * 2 - 1, "space", layer + 1, layers_[layer].objects);
		writeGrid(GetLayerCount() * 2 - layer * 2 - 2, "floor", layer + 1, layers_[layer].floor);
	}
	// エネミーの経路と、スイッチ・扉の色はエディタと同じ行で書く。高さはsliceの番号（下から0始まり）。
	const auto writeRoute = [&](const char* head, const RouteRow& row, bool space) {
		const int height = GetLayerCount() * 2 - row.layer * 2 - (space ? 1 : 2);
		file << head << ',' << height << ',' << row.x << ',' << row.z;
		for (const auto& corner : row.corners) {
			file << ',' << corner.first << ',' << corner.second;
		}
		file << '\n';
	};
	for (const RouteRow& row : patrols_) {
		writeRoute("enemy", row, true);
	}
	for (const LinkRow& link : links_) {
		file << "link," << GetLayerCount() * 2 - link.layer * 2 - 1 << ',' << link.x << ',' << link.z << ',' << link.channel << '\n';
	}
	if (!file) {
		error = "Failed while writing map: " + filePath;
		return false;
	}
	error.clear();
	return true;
}

void MapChipField::ReadLegacyCSV(std::istream& file) {
	// 旧形式は見出しで書き込み先を切り替える。layerとfloorを対にして番号順を検証する。
	name_ = "legacy_stage";
	Grid* current = nullptr;
	std::vector<int>* currentLines = nullptr;
	std::string line;
	int lineNumber = 0;
	bool hasCeiling = false;
	bool waitingForFloor = false;
	while (std::getline(file, line)) {
		++lineNumber;
		// UTF-8署名付きCSVのBOMを、最初のセルへ混入させない。
		if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
			line.erase(0, 3);
		}
		line = Trim(line);
		if (line.empty() || line.front() == '#') {
			continue;
		}
		const auto cells = Split(line);
		const auto fail = [lineNumber]() { throw std::runtime_error("Invalid CSV at line " + std::to_string(lineNumber)); };
		if (cells[0] == "ceiling") {
			if (hasCeiling || !layers_.empty() || cells.size() != 1) {
				fail();
			}
			hasCeiling = true;
			current = &ceiling_;
			currentLines = &ceilingLines_;
		} else if (cells[0] == "layer") {
			if (!hasCeiling || waitingForFloor || cells.size() != 2 || cells[1] != std::to_string(layers_.size() + 1)) {
				fail();
			}
			layers_.emplace_back();
			sourceLines_.emplace_back();
			current = &layers_.back().objects;
			currentLines = &sourceLines_.back().objects;
			waitingForFloor = true;
		} else if (cells[0] == "floor") {
			if (!waitingForFloor || cells.size() != 2 || cells[1] != std::to_string(layers_.size())) {
				fail();
			}
			current = &layers_.back().floor;
			currentLines = &sourceLines_.back().floor;
			waitingForFloor = false;
		} else {
			if (current == nullptr) {
				fail();
			}
			std::vector<MapChipType> row;
			for (const std::string& cell : cells) {
				if (cell.size() != 1 || cell[0] < '0' || cell[0] > '4') {
					fail();
				}
				// ここだけは旧形式の内部数値を使う。slice形式の2=針・7=穴とは対応が異なる。
				row.push_back(static_cast<MapChipType>(cell[0] - '0'));
			}
			current->push_back(std::move(row));
			currentLines->push_back(lineNumber);
		}
	}
	if (!hasCeiling || layers_.empty() || waitingForFloor || ceiling_.empty() || ceiling_.front().empty()) {
		throw std::runtime_error("Map requires ceiling, layer,N and floor,N sections.");
	}
	width_ = static_cast<uint32_t>(ceiling_.front().size());
	depth_ = static_cast<uint32_t>(ceiling_.size());
}

// 配置検証でも、データの添字だけでなく元CSVの物理行へ戻れるようにする。
std::string MapChipField::GetCellLocation(int layer, bool floor, uint32_t x, uint32_t z) const {
	const auto& lines = floor ? sourceLines_[layer].floor : sourceLines_[layer].objects;
	return CSVLocation(lines[z], static_cast<int>(x + 1)) + " (" + (floor ? "floor" : "space") + " layer " + std::to_string(layer + 1) + ", row " + std::to_string(z + 1) + ")";
}

void MapChipField::Validate() const {
	// 全区画の寸法を確認してから、隣接階層の同じマスを参照する。
	const auto checkGrid = [this](const Grid& grid, const std::vector<int>& lines, const std::string& section, bool objects, bool floor) {
		if (grid.size() != depth_) {
			throw std::runtime_error(section + ": expected " + std::to_string(depth_) + " rows, found " + std::to_string(grid.size()) + ".");
		}
		for (uint32_t z = 0; z < depth_; ++z) {
			if (grid[z].size() != width_) {
				FailAt(lines[z], section + ": expected " + std::to_string(width_) + " columns, found " + std::to_string(grid[z].size()) + ".");
			}
			for (uint32_t x = 0; x < width_; ++x) {
				const MapChipType type = grid[z][x];
				const std::string location = CSVLocation(lines[z], static_cast<int>(x + 1)) + " (" + section + ", row " + std::to_string(z + 1) + ")";
				if (type == MapChipType::kHole && !floor) {
					throw std::runtime_error(location + ": hole is not allowed here.\nPut the hole in floor at the same row/column; set space to 0.");
				}
				if (!objects && (type == MapChipType::kPlayer || type == MapChipType::kGoal)) {
					throw std::runtime_error(location + ": start/goal is not allowed here.\nPut it in space at the same row/column; set floor to 1.");
				}
				if ((type == MapChipType::kThorn || type == MapChipType::kMovingThorn || type == MapChipType::kEnemy || type == MapChipType::kSwitch || type == MapChipType::kDoor) && !objects) {
					throw std::runtime_error(location + ": thorns/enemies/switches/doors must be placed in space.");
				}
				if ((type == MapChipType::kSlipperyFloor || type == MapChipType::kCrackedFloor) && !floor) {
					throw std::runtime_error(location + ": ice/cracked floor must be placed in floor.");
				}
				if (IsRampChip(type) && !objects && !floor) {
					throw std::runtime_error(location + ": ramp must be placed in floor or space.");
				}
			}
		}
	};
	checkGrid(ceiling_, ceilingLines_, "ceiling", false, false);
	for (int layer = 0; layer < GetLayerCount(); ++layer) {
		const std::string number = std::to_string(layer + 1);

		checkGrid(layers_[layer].objects, sourceLines_[layer].objects, "space layer " + number, true, false);
		checkGrid(layers_[layer].floor, sourceLines_[layer].floor, "floor layer " + number, false, true);
	}
	int starts = 0;
	int goals = 0;
	std::string firstStart;
	std::string firstGoal;
	for (int layer = 0; layer < GetLayerCount(); ++layer) {
		const Layer& data = layers_[layer];
		for (uint32_t z = 0; z < depth_; ++z) {
			for (uint32_t x = 0; x < width_; ++x) {
				const MapChipType object = data.objects[z][x];
				if (object == MapChipType::kPlayer) {
					if (++starts > 1) {
						throw std::runtime_error(GetCellLocation(layer, false, x, z) + ": extra start. Keep exactly one start.\nFirst start: " + firstStart);
					}
					firstStart = GetCellLocation(layer, false, x, z);
				}
				if (object == MapChipType::kGoal) {
					if (++goals > 1) {
						throw std::runtime_error(GetCellLocation(layer, false, x, z) + ": extra goal. Keep at most one goal.\nFirst goal: " + firstGoal);
					}
					firstGoal = GetCellLocation(layer, false, x, z);
				}
				if ((object == MapChipType::kPlayer || object == MapChipType::kGoal) && !IsSolidFloorChip(data.floor[z][x])) {
					throw std::runtime_error(GetCellLocation(layer, false, x, z) + ": start/goal needs a solid floor.\nSet the cell below to 1: " + GetCellLocation(layer, true, x, z));
				}
				// トゲ・エネミーは下に床がなくても置ける（穴や坂の上で宙に浮く）。高さはその階層の床の上面。
				if (object == MapChipType::kSwitch && !IsSolidFloorChip(data.floor[z][x])) {
					throw std::runtime_error(GetCellLocation(layer, false, x, z) + ": switch needs a floor below.");
				}
				if (data.floor[z][x] == MapChipType::kCrackedFloor) {
					const std::string location = GetCellLocation(layer, true, x, z);
					if (object != MapChipType::kBlank) {
						throw std::runtime_error(location + ": keep space above a cracked floor empty.");
					}
					bool landingFloor = false;
					for (int lower = layer + 1; lower < GetLayerCount(); ++lower) {
						// 壁だけを拒否する。トゲなどは「崩れた先が罠」という配置として許可する（エディタと同じ）。
						if (layers_[lower].objects[z][x] == MapChipType::kBlock) {
							throw std::runtime_error(location + ": a wall blocks the falling path below the cracked floor.");
						}
						if (IsSolidFloorChip(layers_[lower].floor[z][x])) {
							landingFloor = true;
							break;
						}
					}
					// 着地床が無い場合は外へ落ちる。GameSceneが球を開始地点へ戻す（エディタでも注意止まり）。
					static_cast<void>(landingFloor);
				}
				// 下層のない穴は開口として扱う。下層がある穴は、中間階層のfloor 0を通り抜けて
				// 最初に見つかる床へ着地する。上昇装置もその床に置かれる。
				if (data.floor[z][x] == MapChipType::kHole && layer + 1 < GetLayerCount()) {
					const std::string hole = GetCellLocation(layer, true, x, z);
					if (object != MapChipType::kBlank) {
						throw std::runtime_error(hole + ": space above the hole must be empty.\nSet to 0: " + GetCellLocation(layer, false, x, z));
					}
					bool landingFloor = false;
					for (int lower = layer + 1; lower < GetLayerCount(); ++lower) {
						const Layer& lowerData = layers_[lower];
						if (lowerData.objects[z][x] != MapChipType::kBlank) {
							throw std::runtime_error(hole + ": clear the falling path below the hole.\nSet to 0: " + GetCellLocation(lower, false, x, z));
						}
						if (IsSolidFloorChip(lowerData.floor[z][x])) {
							landingFloor = true;
							break;
						}
						if (lowerData.floor[z][x] != MapChipType::kBlank) {
							throw std::runtime_error(hole + ": use floor 0 in intermediate layers of this shaft.\nSet to 0: " + GetCellLocation(lower, true, x, z));
						}
					}
					if (!landingFloor) {
						throw std::runtime_error(hole + ": no solid landing floor below the shaft.");
					}
				}
			}
		}
	}
	// enemy/link行と、対応するマス(9/11/12)を確認する。
	const auto inside = [this](int x, int z) { return x >= 0 && z >= 0 && x < static_cast<int>(width_) && z < static_cast<int>(depth_); };
	const auto checkRoute = [&](const RouteRow& row) {
		const std::string where = CSVLocation(row.line);
		const Grid& grid = layers_[row.layer].objects;
		if (!inside(row.x, row.z) || grid[row.z][row.x] != MapChipType::kEnemy) {
			throw std::runtime_error(where + ": enemy row does not point at an enemy (9) cell.");
		}
		int fromX = row.x;
		int fromZ = row.z;
		for (const auto& corner : row.corners) {
			if (!inside(corner.first, corner.second)) {
				throw std::runtime_error(where + ": route point (" + std::to_string(corner.first) + "," + std::to_string(corner.second) + ") is outside the map.");
			}
			// 線分をマスの1/4ずつたどり、空間の壁にぶつかる経路を拒否する。エディタでは注意として表示される。
			const int steps = (std::max)(std::abs(corner.first - fromX), std::abs(corner.second - fromZ)) * 4;
			for (int step = 0; step <= steps; ++step) {
				const float t = steps == 0 ? 0.0f : static_cast<float>(step) / static_cast<float>(steps);
				const int x = static_cast<int>(std::floor(static_cast<float>(fromX) + 0.5f + static_cast<float>(corner.first - fromX) * t));
				const int z = static_cast<int>(std::floor(static_cast<float>(fromZ) + 0.5f + static_cast<float>(corner.second - fromZ) * t));
				if (layers_[row.layer].objects[z][x] == MapChipType::kBlock) {
					throw std::runtime_error(where + ": a wall blocks the enemy route at (" + std::to_string(x) + "," + std::to_string(z) + ").");
				}
			}
			fromX = corner.first;
			fromZ = corner.second;
		}
	};
	for (const RouteRow& row : patrols_) {
		checkRoute(row);
	}
	for (const LinkRow& link : links_) {
		if (!inside(link.x, link.z) || (layers_[link.layer].objects[link.z][link.x] != MapChipType::kSwitch && layers_[link.layer].objects[link.z][link.x] != MapChipType::kDoor)) {
			throw std::runtime_error(CSVLocation(link.line) + ": link row does not point at a switch (11) or door (12).");
		}
	}
	std::vector<int> switchChannels;
	for (int layer = 0; layer < GetLayerCount(); ++layer) {
		for (uint32_t z = 0; z < depth_; ++z) {
			for (uint32_t x = 0; x < width_; ++x) {
				const auto countRows = [&](const std::vector<RouteRow>& rows) {
					return std::count_if(rows.begin(), rows.end(), [&](const RouteRow& row) { return row.layer == layer && row.x == static_cast<int>(x) && row.z == static_cast<int>(z); });
				};
				if (layers_[layer].objects[z][x] == MapChipType::kEnemy && countRows(patrols_) > 1) {
					throw std::runtime_error(GetCellLocation(layer, false, x, z) + ": this enemy has more than one enemy row.");
				}
				if (layers_[layer].objects[z][x] == MapChipType::kSwitch) {
					switchChannels.push_back(ChannelAt(layer, x, z));
				}
			}
		}
	}
	// 対応するスイッチの無い扉は開けられないため、エディタと同じくエラーにする。
	for (int layer = 0; layer < GetLayerCount(); ++layer) {
		for (uint32_t z = 0; z < depth_; ++z) {
			for (uint32_t x = 0; x < width_; ++x) {
				if (layers_[layer].objects[z][x] != MapChipType::kDoor) {
					continue;
				}
				const int channel = ChannelAt(layer, x, z);
				if (std::find(switchChannels.begin(), switchChannels.end(), channel) == switchChannels.end()) {
					throw std::runtime_error(GetCellLocation(layer, false, x, z) + ": no switch has the same color (" + std::to_string(channel) + ") as this door.");
				}
			}
		}
	}
	if (starts == 0) {
		throw std::runtime_error("No start found. Put exactly one start in a space section on a solid floor.");
	}
}

// 最下層の床を0、1階層上がるごとにlayerHeightぶん高くする。
float MapChipField::GetFloorHeight(int layer) const { return static_cast<float>(GetLayerCount() - 1 - layer) * settings_.layerHeight; }

// マップ中央をXZの原点にそろえる。列が右へ進むと+X、行が下へ進むと-Z。
// すべての区画で同じXZ計算を使い、上下の穴と着地点を一致させる。
Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t zIndex, int layer) const {
	// 各区画はXZ平面。同じマスのX/Zは全階層で一致する。
	return {
	    (static_cast<float>(xIndex) - static_cast<float>(width_ - 1) * 0.5f) * settings_.cellSize, GetFloorHeight(layer),
	    (static_cast<float>(depth_ - 1) * 0.5f - static_cast<float>(zIndex)) * settings_.cellSize};
}

// 最下層床上面と最上部天井下面の中点を、箱全体の回転中心にする。
Vector3 MapChipField::GetPivot() const { return {0.0f, (GetFloorHeight(0) + settings_.layerHeight - settings_.floorThickness) * 0.5f, 0.0f}; }

int MapChipField::GetLayerByHeight(float localFootHeight) const {
	// 階層間の中点を境界にする。ワールドYではなく箱の中での高さを調べる。
	const float layer = std::floor((GetFloorHeight(0) - localFootHeight) / settings_.layerHeight + 0.5f);
	return static_cast<int>(std::clamp(layer, 0.0f, static_cast<float>(GetLayerCount() - 1)));
}

void MapChipField::GenerateBlocks() {
	// 坂は1マスの立方体を45度で切った形。floorとspaceで形は同じで、置くマスの高さだけ変える。
	const float halfCell = settings_.cellSize * 0.5f;
	// Goal.objの底面とくぼみを床上面から離し、遠距離描画での深度の競合を防ぐ。
	constexpr float kGoalHalfThickness = 0.025f;
	constexpr float kGoalFloorGap = 0.02f;
	const auto addRamp = [this, halfCell](MapChipType type, const Vector3& bottom, int layer) {
		float yaw = 0.0f;
		switch (type) {
		case MapChipType::kRampNegativeX:
			yaw = ToRadian(180.0f);
			break;
		case MapChipType::kRampPositiveZ:
			yaw = ToRadian(-90.0f);
			break;
		case MapChipType::kRampNegativeZ:
			yaw = ToRadian(90.0f);
			break;
		default:
			break;
		}
		MapBlock ramp;
		ramp.kind = MapBlockKind::kRamp;
		ramp.layer = layer;
		// 坂モデルと衝突判定は、centerを立方体の底面中心として扱う。
		ramp.center = bottom;
		ramp.halfSize = {halfCell, halfCell, halfCell};
		ramp.localRotation = MakeRotateYMatrix(yaw);
		blocks_.push_back(ramp);
	};
	for (uint32_t z = 0; z < depth_; ++z) {
		for (uint32_t x = 0; x < width_; ++x) {
			if (ceiling_[z][x] == MapChipType::kBlock) {
				Vector3 center = GetMapChipPositionByIndex(x, z, 0);
				center.y += settings_.layerHeight - settings_.floorThickness * 0.5f;
				blocks_.push_back({
				    center, {halfCell, settings_.floorThickness * 0.5f, halfCell},
                     -1, MapBlockKind::kCeiling, true
                });
			}
			for (int layer = 0; layer < GetLayerCount(); ++layer) {
				const Vector3 cell = GetMapChipPositionByIndex(x, z, layer);
				const MapChipType floor = layers_[layer].floor[z][x];
				if (floor == MapChipType::kBlock || floor == MapChipType::kSlipperyFloor) {
					// 床の立方体はcell.yを上面とし、spaceの立方体と隙間なく接する。
					blocks_.push_back({
					    cell - Vector3{0.0f,     halfCell, 0.0f    },
                          {halfCell, halfCell, halfCell},
                          layer, floor == MapChipType::kSlipperyFloor ? MapBlockKind::kSlipperyFloor : MapBlockKind::kFloor, true
                    });
					// 穴のマスには床ブロックを作らない。下層がある場合だけHole用の定義を追加する。
					// 空白0も床なしだが、上昇装置や一時足場のある穴7とは区別する。
				} else if (floor == MapChipType::kHole && layer + 1 < GetLayerCount()) {
					// 中間階層のfloor 0を通り抜け、最初の着地床へつなぐ。
					int landingLayer = layer + 1;
					while (landingLayer < GetLayerCount() && layers_[landingLayer].floor[z][x] == MapChipType::kBlank) {
						++landingLayer;
					}
					if (landingLayer < GetLayerCount() && IsSolidFloorChip(layers_[landingLayer].floor[z][x])) {
						holes_.push_back({layer, landingLayer, cell, halfCell});
					}
				} else if (floor == MapChipType::kCrackedFloor) {
					// 通常のblocks_へ重複登録しない。破壊後の衝突はCrackedFloorのsolidで切り替える。
					crackedFloors_.push_back({cell, layer});
				} else if (IsRampChip(floor)) {
					// floorの坂は床の立方体と同じ範囲に置く。
					addRamp(floor, cell - Vector3{0.0f, settings_.cellSize, 0.0f}, layer);
				}
				// 空間チップから壁・開始地点・ゴール・上段の坂を生成する。
				switch (layers_[layer].objects[z][x]) {
				case MapChipType::kBlock:
					blocks_.push_back({
					    cell + Vector3{0.0f,     halfCell, 0.0f    },
                          {halfCell, halfCell, halfCell},
                          layer, MapBlockKind::kWall, true
                    });
					break;
				// 開始地点は球中心を床上面+半径+接触余裕に置く。マップ形状として球を追加しない。
				case MapChipType::kPlayer:
					startPosition_ = cell + Vector3{0.0f, settings_.ballRadius + settings_.contactMargin, 0.0f};
					startLayer_ = layer;
					break;
				// 旧形式などでゴールがある場合の表示板。solid=falseで通過でき、クリア判定はStageが行う。
				case MapChipType::kGoal:
					goalPosition_ = cell;
					goalLayer_ = layer;
					blocks_.push_back({
					    cell + Vector3{0.0f, kGoalHalfThickness + kGoalFloorGap, 0.0f},
	                          {halfCell, kGoalHalfThickness, halfCell},
                          layer, MapBlockKind::kGoal, false
                    });
					break;
				case MapChipType::kThorn:
				case MapChipType::kMovingThorn: {
					ThornDefinition thorn;
					thorn.center = cell;
					thorn.layer = layer;
					thorn.moving = layers_[layer].objects[z][x] == MapChipType::kMovingThorn;
					thorns_.push_back(thorn);
					break;
				}
				case MapChipType::kEnemy: {
					// enemy行があれば経路をたどる。無ければその場に止まるエネミー。
					ThornDefinition enemy;
					enemy.center = cell;
					enemy.layer = layer;
					enemy.patrol = true;
					for (const RouteRow& row : patrols_) {
						if (row.layer == layer && row.x == static_cast<int>(x) && row.z == static_cast<int>(z) && !row.corners.empty()) {
							enemy.moving = true;
							enemy.route = RoutePoints(row);
							enemy.loop = IsLoop(row);
						}
					}
					thorns_.push_back(enemy);
					break;
				}
				case MapChipType::kSwitch:
				case MapChipType::kDoor:
					switchDoors_.push_back({cell, layer, ChannelAt(layer, x, z), layers_[layer].objects[z][x] == MapChipType::kDoor});
					break;
				case MapChipType::kRampPositiveX:
				case MapChipType::kRampNegativeX:
				case MapChipType::kRampPositiveZ:
				case MapChipType::kRampNegativeZ:
					// spaceの坂は空間の立方体と同じ範囲に置く。
					addRamp(layers_[layer].objects[z][x], cell, layer);
					break;
				default:
					break;
				}
			}
		}
	}
}
