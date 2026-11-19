#pragma once
#include "KamataEngine.h"
#include <numbers>

class ClearScene {

public:

	//初期化
	void Initialize();

	//更新
	void Update();

	//描画
	void Draw();

	//デストラクタ
	~ClearScene();

public:

	//終了
	bool IsClear() const { return clearScened_; }

private:

	//クリア画面終了フラグ
	bool clearScened_ = false;



};
