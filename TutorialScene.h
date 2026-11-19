#pragma once
#include "KamataEngine.h"
#include <numbers>

class TutorialScene {
public:

	void Initialize();

	void Update();

	void Draw();

	//デストラクタ
	~TutorialScene();

public:

	//チュートリアルフラグのgetter
	bool IsTutorial() const { return tutorial_; }


private:

	//チュートリアルフラグ
	bool tutorial_ = false;


};
