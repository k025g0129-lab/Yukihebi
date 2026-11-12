#pragma once
#include <KamataEngine.h>
#include "Boss.h"

using namespace KamataEngine;


class GameScene {
public:
	GameScene();
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

private:
	enum class SceneState { kTitle, kPlay, kGameClear, kPause };

	uint32_t textureHandle_ = 0;
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelBoss_ = nullptr;

	KamataEngine::WorldTransform worldTransform_;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_;

	
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	Boss* boss;

		//Skydome* skydome_ = nullptr;

		//	MapChipField* mapChipField_;

	 //   CameraController* cameraController;

	 //   Fade* fade_ = nullptr;
};
