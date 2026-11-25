#include "ClearScene.h"

#include <algorithm>
#include <cstdio>
#include <string>

using namespace KamataEngine;

namespace {
	// ミリ秒を "12.345s" の形にする。
	std::string FormatMilliseconds(int milliseconds) {
		char text[32];
		std::snprintf(text, sizeof(text), "%d.%03ds", milliseconds / 1000, milliseconds % 1000);
		return text;
	}

	constexpr Vector4 kBarColor = {0.55f, 0.8f, 1.0f, 1.0f};
	constexpr Vector4 kOwnBarColor = {1.0f, 0.8f, 0.25f, 1.0f};
} // namespace

void ClearScene::Initialize(const float ClearTime, const Ranking* ranking, int clearMilliseconds) {
	clearScened_ = false;
	ClearTime_ = ClearTime;
	ranking_ = ranking;
	clearMilliseconds_ = clearMilliseconds;
	graphVisible_ = false;

	// 白い1x1の画像を引き伸ばして棒にする。
	const uint32_t texture = TextureManager::Load("white1x1.png");
	for (std::unique_ptr<Sprite>& bar : bars_) {
		bar.reset(Sprite::Create(texture, {kGraphLeft, kGraphBottom}, kBarColor));
	}
}

ClearScene::~ClearScene() = default;

void ClearScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		clearScened_ = true;
	}
	UpdateGraph();
}

void ClearScene::UpdateGraph() {
	graphVisible_ = false;
	// 送信後の再取得が終わるまでは、自分の記録が入っていないので描かない。
	if (ranking_ == nullptr || ranking_->IsLoading() || ranking_->HasFailed()) {
		return;
	}
	histogram_ = ranking_->GetHistogram(kBarCount);
	if (histogram_.counts.empty()) {
		return;
	}
	const int maxCount = *std::max_element(histogram_.counts.begin(), histogram_.counts.end());
	ownBarIndex_ = histogram_.IndexOf(clearMilliseconds_);

	const float step = kGraphWidth / static_cast<float>(kBarCount);
	const float barWidth = step - kBarGap;
	for (int index = 0; index < kBarCount; ++index) {
		const int count = histogram_.counts[static_cast<std::size_t>(index)];
		// 1人でもいれば見えるよう、最低2px の高さにする。
		const float height = count > 0
			                     ? (std::max)(2.0f, kGraphHeight * static_cast<float>(count) / static_cast<float>(maxCount))
			                     : 0.0f;
		Sprite* bar = bars_[static_cast<std::size_t>(index)].get();
		bar->SetPosition({kGraphLeft + step * static_cast<float>(index), kGraphBottom - height});
		bar->SetSize({barWidth, height});
		// 自分の記録が入る棒だけ色を変える。
		bar->SetColor(index == ownBarIndex_ ? kOwnBarColor : kBarColor);
	}
	graphVisible_ = true;
}

void ClearScene::Draw() {
	DebugText* debugText = DebugText::GetInstance();
	debugText->Print("BOSS DEFEATED - CLEAR!", 48.0f, 120.0f, 2.0f);
	debugText->Print("PRESS ENTER FOR TITLE", 48.0f, 220.0f);
	debugText->Print("ClearTime" + std::to_string(ClearTime_), 48.0f, 300.0f);
	if (ranking_ != nullptr) {
		// 送信と再取得が終わるまでは読み込み中。失敗したら分かるように出す。
		if (ranking_->IsLoading()) {
			debugText->Print("RANKING LOADING...", 48.0f, 340.0f);
		}
		else if (ranking_->HasFailed()) {
			debugText->Print("RANKING FAILED", 48.0f, 340.0f);
		}
		else {
			const int rank = ranking_->FindRank(clearMilliseconds_);
			debugText->Print(rank > 0 ? "YOUR RANK " + std::to_string(rank) : std::string("YOUR RANK -"),
			                 48.0f, 340.0f);
		}
		const std::vector<int>& top3 = ranking_->GetTop3();
		for (std::size_t index = 0; index < top3.size(); ++index) {
			debugText->Print(
				std::to_string(index + 1) + ". " + FormatMilliseconds(top3[index]), 48.0f,
				370.0f + 24.0f * static_cast<float>(index));
		}
	}

	if (graphVisible_) {
		char percent[64];
		std::snprintf(percent, sizeof(percent), "TOP %.1f%%  (%d PLAYERS)",
		              ranking_->GetTopPercent(clearMilliseconds_), ranking_->GetRecordCount());
		debugText->Print(percent, kGraphLeft, kGraphBottom - kGraphHeight - 64.0f, 1.5f);

		// 自分の棒の上に印を付ける。
		const float step = kGraphWidth / static_cast<float>(kBarCount);
		const float ownHeight = bars_[static_cast<std::size_t>(ownBarIndex_)]->GetSize().y;
		debugText->Print("YOU", kGraphLeft + step * static_cast<float>(ownBarIndex_),
		                 kGraphBottom - ownHeight - 24.0f);

		// 横軸の両端の値。最後の棒はそれより遅い記録もまとめているので "+" を付ける。
		debugText->Print("FAST " + FormatMilliseconds(histogram_.minValue), kGraphLeft, kGraphBottom + 8.0f);
		const std::string slowLabel = FormatMilliseconds(histogram_.LastBucketStart()) + "+ SLOW";
		// グラフの右端に揃える。1文字の幅は DebugText::kFontWidth（拡大率1のとき）。
		const float slowLabelWidth = static_cast<float>(DebugText::kFontWidth) * static_cast<float>(slowLabel.size());
		debugText->Print(slowLabel, kGraphLeft + kGraphWidth - slowLabelWidth,
		                 kGraphBottom + 8.0f);
	}

	Sprite::PreDraw();
	if (graphVisible_) {
		for (const std::unique_ptr<Sprite>& bar : bars_) {
			bar->Draw();
		}
	}
	debugText->DrawAll();
	Sprite::PostDraw();
}
