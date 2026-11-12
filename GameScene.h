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
	static constexpr uint32_t kFloorSide = 9;
	Camera camera_{};
	MapChipField mapChipField_{};
	std::vector<std::unique_ptr<WorldTransform>> floorTiles_;
	ObjectColor floorColor_{};
	std::unique_ptr<Model> floorModel_;
	std::unique_ptr<Player> player_;
};
