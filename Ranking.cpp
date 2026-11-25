#include "Ranking.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

void Ranking::Initialize(const std::string& baseUrl, const std::string& gameId) {
	baseUrl_ = baseUrl;
	gameId_ = gameId;
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
		ranking_.clear();
		top3_.clear();
		failed_ = false;
	}
	pendingFetchStage_ = stageId;
	StartPending();
}

void Ranking::Submit(const std::string& stageId, float shots) {
	if (stageId != activeStage_) {
		activeStage_ = stageId;
		ranking_.clear();
		top3_.clear();
		failed_ = false;
		pendingFetchStage_.clear();
	}

	pendingSubmissions_.push_back({stageId, shots});
	StartPending();
}

void Ranking::StartPending() {
	if (task_.IsRunning()) { return; }
	if (!pendingSubmissions_.empty()) {
		const Submission& submission = pendingSubmissions_.front();
		char body[32];
		std::snprintf(body, sizeof(body), "{\"value\":%.1f}", submission.shots);
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
	ranking_ = ParseValues(json);
	std::sort(ranking_.begin(), ranking_.end());
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

int Ranking::FindRank(int shots) const {
	// ranking_は少ない順・重複なし。見つけた位置がそのまま順位になる。
	const auto found = std::lower_bound(ranking_.begin(), ranking_.end(), shots);
	if (found == ranking_.end() || *found != shots) {
		return 0;
	}
	return static_cast<int>(found - ranking_.begin()) + 1;
}
