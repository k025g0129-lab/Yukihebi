#pragma once
#include <array>
#include <memory>
#include <numbers>
#include "KamataEngine.h"
#include "Ranking.h"

class ClearScene {
public:
	//初期化
	// ranking は表示に使うだけ。送信と更新は main 側で行う。
	void Initialize(float ClearTime, const Ranking* ranking = nullptr, int clearMilliseconds = 0);

	//更新
	void Update();

	//描画
	void Draw();

	//デストラクタ
	~ClearScene();

	//終了
	bool IsClear() const { return clearScened_; }

private:
	// 記録が揃っていれば、区間ごとの人数から棒の位置と大きさを決める。
	void UpdateGraph();

	//クリア画面終了フラグ
	bool clearScened_ = false;

	float ClearTime_;
	const Ranking* ranking_ = nullptr;
	int clearMilliseconds_ = 0;

	// 棒グラフ。棒1本ごとにSpriteを持つ（Spriteは位置・大きさ・色を1組しか持てない）。
	static constexpr int kBarCount = 10;
	static constexpr float kGraphLeft = 640.0f;
	static constexpr float kGraphBottom = 640.0f;
	static constexpr float kGraphWidth = 560.0f;
	static constexpr float kGraphHeight = 240.0f;
	static constexpr float kBarGap = 6.0f;
	std::array<std::unique_ptr<KamataEngine::Sprite>, kBarCount> bars_{};
	Ranking::Histogram histogram_{};
	int ownBarIndex_ = 0;
	bool graphVisible_ = false;
};
