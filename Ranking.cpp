#include "Ranking.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

void Ranking::Initialize(const std::string& baseUrl, const std::string& gameId) {
	baseUrl_ = baseUrl;
	gameId_ = gameId;
	values_.clear();
	ranking_.clear();
	top3_.clear();
	activeStage_.clear();
	requestStage_.clear();
	pendingFetchStage_.clear();
	pendingSubmissions_.clear();
	requestKind_ = RequestKind::kNone;
	failed_ = false;
}

std::string Ranking::MakeUrl(const std::string& stageId) const {
	return baseUrl_ + "/api/v1/games/" + gameId_ + "/stages/" + stageId + "/values";
}

void Ranking::Fetch(const std::string& stageId) {
	if (stageId != activeStage_) {
		activeStage_ = stageId;
		values_.clear();
		ranking_.clear();
		top3_.clear();
		failed_ = false;
	}
	pendingFetchStage_ = stageId;
	StartPending();
}

void Ranking::Submit(const std::string& stageId, int value) {
	if (stageId != activeStage_) {
		activeStage_ = stageId;
		values_.clear();
		ranking_.clear();
		top3_.clear();
		failed_ = false;
		pendingFetchStage_.clear();
	}

	pendingSubmissions_.push_back({stageId, value});
	StartPending();
}

void Ranking::StartPending() {
	if (task_.IsRunning()) { return; }
	if (!pendingSubmissions_.empty()) {
		const Submission& submission = pendingSubmissions_.front();
		char body[32];
		// 小数で送るとサーバーに400で弾かれるため、必ず整数で送る。
		std::snprintf(body, sizeof(body), "{\"value\":%d}", submission.value);
		if (task_.Post(MakeUrl(submission.stage), body)) {
			requestStage_ = submission.stage;
			requestKind_ = RequestKind::kPost;
			pendingSubmissions_.pop_front();
		}
		return;
	}
	if (!pendingFetchStage_.empty()) {
		if (task_.Get(MakeUrl(pendingFetchStage_))) {
			requestStage_ = pendingFetchStage_;
			requestKind_ = RequestKind::kFetch;
			pendingFetchStage_.clear();
		}
	}
}

void Ranking::ApplyValues(const std::string& json) {
	values_ = ParseValues(json);
	std::sort(values_.begin(), values_.end());
	ranking_ = values_;
	ranking_.erase(std::unique(ranking_.begin(), ranking_.end()), ranking_.end());
	top3_.assign(ranking_.begin(), ranking_.begin() + (std::min)(ranking_.size(), size_t{3}));
}

std::vector<int> Ranking::ParseValues(const std::string& json) {
	std::vector<int> values;
	// 必要なのは配列の中の整数だけ。"["から"]"までを数字単位で読む。
	const size_t begin = json.find('[');
	const size_t end = json.find(']');
	if (begin == std::string::npos || end == std::string::npos || end < begin) {
		return values;
	}
	for (size_t index = begin + 1; index < end;) {
		if (json[index] < '0' || json[index] > '9') {
			++index;
			continue;
		}
		// stoiは範囲外で例外を投げるため、strtolを使う。打数に負数は来ない。
		char* stop = nullptr;
		const long value = std::strtol(json.c_str() + index, &stop, 10);
		values.push_back(static_cast<int>(value));
		index = static_cast<size_t>(stop - json.c_str());
	}
	return values;
}

bool Ranking::Update() {
	if (!task_.Update()) {
		StartPending();
		return false;
	}
	const bool succeeded = task_.GetStatus() == 200;
	const RequestKind completedKind = requestKind_;
	const std::string completedStage = requestStage_;
	requestKind_ = RequestKind::kNone;
	requestStage_.clear();
	if (completedStage == activeStage_) {
		failed_ = !succeeded;
	}
	// 投稿後の再取得を優先し、結果画面には先読み済みの記録をその間も表示する。
	if (completedKind == RequestKind::kPost) {
		if (succeeded && completedStage == activeStage_) {
			if (pendingFetchStage_ == completedStage) { pendingFetchStage_.clear(); }
			if (task_.Get(MakeUrl(completedStage))) {
				requestStage_ = completedStage;
				requestKind_ = RequestKind::kRefetch;
				return false;
			}
			pendingFetchStage_ = completedStage;
		}
		StartPending();
		return false;
	}
	const bool updated = succeeded && completedStage == activeStage_ &&
		(completedKind == RequestKind::kFetch || completedKind == RequestKind::kRefetch);
	if (updated) {
		ApplyValues(task_.GetBody());
		// 同じステージの再取得を待っていた場合、この応答を使って通信を重複させない。
		if (pendingFetchStage_ == completedStage) { pendingFetchStage_.clear(); }
	}
	StartPending();
	return updated;
}

int Ranking::Histogram::IndexOf(int value) const {
	if (counts.empty()) {
		return 0;
	}
	const int index = (value - minValue) / (std::max)(bucketWidth, 1);
	return std::clamp(index, 0, static_cast<int>(counts.size()) - 1);
}

Ranking::Histogram Ranking::GetHistogram(int bucketCount) const {
	Histogram histogram{};
	if (values_.empty() || bucketCount <= 0) {
		return histogram;
	}
	// values_は少ない順。上位90%に当たる記録までを等分し、それより遅い記録は最後の区間へ入れる。
	const std::size_t cutoffIndex = (values_.size() * 9 + 9) / 10 - 1; // ceil(0.9 * N) - 1
	const int minValue = values_.front();
	const int cutoffValue = values_[cutoffIndex];
	histogram.minValue = minValue;
	// 全員同じ値でも幅0にならないよう、最低1にする。
	histogram.bucketWidth = (std::max)(1, (cutoffValue - minValue + bucketCount) / bucketCount);
	histogram.counts.assign(static_cast<std::size_t>(bucketCount), 0);
	for (const int value : values_) {
		++histogram.counts[static_cast<std::size_t>(histogram.IndexOf(value))];
	}
	return histogram;
}

float Ranking::GetTopPercent(int value) const {
	// 自分より少ない記録の数。重複を含めて数える。
	const auto firstNotLess = std::lower_bound(values_.begin(), values_.end(), value);
	const int fasterCount = static_cast<int>(firstNotLess - values_.begin());
	const bool includesSelf = firstNotLess != values_.end() && *firstNotLess == value;
	const int total = static_cast<int>(values_.size()) + (includesSelf ? 0 : 1);
	return 100.0f * static_cast<float>(fasterCount + 1) / static_cast<float>(total);
}

int Ranking::FindRank(int shots) const {
	// ranking_は少ない順・重複なし。見つけた位置がそのまま順位になる。
	const auto found = std::lower_bound(ranking_.begin(), ranking_.end(), shots);
	if (found == ranking_.end() || *found != shots) {
		return 0;
	}
	return static_cast<int>(found - ranking_.begin()) + 1;
}
