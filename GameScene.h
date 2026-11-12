#pragma once
#include <KamataEngine.h>
#include <memory>
#include <vector>
#include "MapChipField.h"

using namespace KamataEngine;

class Player;

class GameScene {
public:
	GameScene();
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

private:
	void GenerateBlocks();

	static constexpr uint32_t kFloorSide = 9;
	Camera camera_{};
	MapChipField mapChipField_{};
	std::vector<std::vector<std::unique_ptr<WorldTransform>>> worldTransformBlocks_;
	std::unique_ptr<Model> modelBlock_;
	Model* playerModel_ = nullptr;
	Player* player_ = nullptr;
};
