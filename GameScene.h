#pragma once
#include <KamataEngine.h>

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
};
