#pragma once
#include "KamataEngine.h"
#include <numbers>

#include "Fade.h"

class TitleScene {

public:

	void Initialize();

	void Update();

	void Draw();

	//デストラクタ
	~TitleScene();

public:

	// デスフラグのgetter
	bool IsFinished() const { return finished_; }


private:

	//終了フラグ
	bool finished_ = false;

	


};
