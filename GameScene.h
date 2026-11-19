#pragma once
#include <KamataEngine.h>
#include <chrono>
#include <memory>
#include <vector>
#include "MapChipField.h"
using namespace KamataEngine;

class Player;
class Boss;
class CameraController;

class GameScene {
public:
	GameScene();
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

	void CheckAllCollisions();

private:
	void GenerateBlocks();
	std::unique_ptr<LightGroup> playerLightGroup_;

	MapChipField mapChipField_{};
	std::vector<std::vector<std::unique_ptr<WorldTransform>>> worldTransformBlocks_;
	std::unique_ptr<Model> modelBlock_;
	Model* playerModel_ = nullptr;
	Model* snowModel_ = nullptr;
	Player* player_ = nullptr;

	std::chrono::steady_clock::time_point previousTime_{};
	float snowPhysicsAccumulator_ = 0.0f;
	static constexpr float kSnowPhysicsStep = 1.0f / 120.0f;

	CameraController* cameraController_;
	Model* bossModel_ = nullptr;
	Model* bossTailModel_ = nullptr;
	Boss* boss_ = nullptr;
	Camera camera_;
	void InitializePlayerLighting();
	/// <summary>ステージ全体を1方向から照らす平行光源を設定する。</summary>
	void InitializeDirectionalLight();
	void UpdateLighting();
};
