#pragma once
#include <KamataEngine.h>
#include <chrono>
#include <memory>
#include <vector>
#include "MapChipField.h"
using namespace KamataEngine;

class Player;
class Boss;

class GameScene {
public:
	GameScene();
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

private:
	void GenerateBlocks();

	Camera camera_{};
	MapChipField mapChipField_{};
	std::vector<std::vector<std::unique_ptr<WorldTransform>>> worldTransformBlocks_;
	std::unique_ptr<Model> modelBlock_;
	Model* playerModel_ = nullptr;
	Model* snowModel_ = nullptr;
	Player* player_ = nullptr;

	std::chrono::steady_clock::time_point previousTime_{};
	float snowPhysicsAccumulator_ = 0.0f;
	static constexpr float kSnowPhysicsStep = 1.0f / 120.0f;


	Model* bossModel_ = nullptr;
	Boss* boss_ = nullptr;

};
