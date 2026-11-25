#pragma once
#pragma once

#include "HttpTask.h"

#include <deque>
#include <string>
#include <vector>

// 打数のランキング。並べ替え・重複まとめ・上位3つ・順位をすべてここで行う。
// 通信はHttpTaskに任せ、このクラスは「何を送って、結果をどう解釈するか」だけを持つ。
class Ranking {
public:
	// baseUrlは末尾の"/"なし。gameIdはURLに入る識別子（例: "golf"）。
	void Initialize(const std::string& baseUrl, const std::string& gameId);
	// 打数を1件送る。先読み中なら送信を予約し、完了後に同じステージを取り直す。
	void Submit(const std::string& stageId, float shots);
	// プレイ開始時に取得し、リザルトへ入る前から上位記録を表示可能にする。
	void Fetch(const std::string& stageId);
	// 毎フレーム呼ぶ。順位が更新された1フレームだけtrueを返す。
	bool Update();

	// 送信から表示が揃うまでの間を知るため。表示側で「読み込み中」に使う。
	bool IsLoading() const { return task_.IsRunning(); }
	bool HasFailed() const { return failed_; }
	// 少ない順・重複なしの先頭3つ。記録が少なければ0〜2個。
	const std::vector<int>& GetTop3() const { return top3_; }
	// shotsが何位か。1位から数え、見つからなければ0。
	int FindRank(int shots) const;

private:
	enum class RequestKind { kNone, kFetch, kPost, kRefetch };

	struct Submission {
		std::string stage;
		float shots = 0.0f;
	};

	std::string MakeUrl(const std::string& stageId) const;
	void StartPending();
	void ApplyValues(const std::string& json);
	// {"values":[8,5,3]} から整数だけを取り出す。JSONライブラリは使わない。
	static std::vector<int> ParseValues(const std::string& json);

	HttpTask task_{};
	std::string baseUrl_;
	std::string gameId_;
	std::string activeStage_;
	std::string requestStage_;
	std::string pendingFetchStage_;
	std::deque<Submission> pendingSubmissions_;
	RequestKind requestKind_ = RequestKind::kNone;
	// 少ない順・重複なしの全記録。順位はこの添字+1。
	std::vector<int> ranking_;
	std::vector<int> top3_;
	bool failed_ = false;
};
