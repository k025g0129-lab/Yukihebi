#pragma once
#include <numbers>
#include "KamataEngine.h"

class ClearScene {
public:
	//初期化
	void Initialize(float ClearTime);

	//更新
	void Update();

	//描画
	void Draw();

	//デストラクタ
	~ClearScene();

	//終了
	bool IsClear() const { return clearScened_; }

private:
	//クリア画面終了フラグ
	bool clearScened_ = false;

	float ClearTime_;
};
