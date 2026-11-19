#pragma once
#include "KamataEngine.h"
#include <numbers>

class Fade {
public:

	void Initialize();

	void Update();

	void Draw();

	//デストラクタ
	 ~Fade();

private:

	KamataEngine::Sprite* sprite_ = nullptr;



};
