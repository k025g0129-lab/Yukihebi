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
	// 記録を1件送る。先読み中なら送信を予約し、完了後に同じステージを取り直す。
	// サーバーは整数しか受け付けない（小数は400 "value must be an integer"）。
	// 時間を送るときはミリ秒などの整数に直してから渡す。
	void Submit(const std::string& stageId, int value);
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

	// 棒グラフ用に、記録を等しい幅の区間に分けた人数。
	// 極端に遅い記録で他の棒が詰まらないよう、区間は最速から上位90%の記録までで決め、
	// それより遅い記録は最後の区間にまとめる。
	struct Histogram {
		std::vector<int> counts; // 区間ごとの人数。記録が無ければ空
		int minValue = 0; // 最初の区間の下端
		int bucketWidth = 1; // 区間の幅
		// valueが入る区間の添字。範囲外は最初か最後の区間に寄せる。
		int IndexOf(int value) const;
		// 最後の区間の下端。ここから上はまとめた区間になる。
		int LastBucketStart() const { return minValue + bucketWidth * (static_cast<int>(counts.size()) - 1); }
	};
	Histogram GetHistogram(int bucketCount) const;
	// 上位何％か。(自分より少ない記録の数 + 1) / 全体 × 100。1位で10人中なら10%。
	// 取得した記録に自分の値が無ければ、自分を1人足して数える。記録が全く無ければ100%。
	float GetTopPercent(int value) const;
	// 重複を含めた記録の数。
	int GetRecordCount() const { return static_cast<int>(values_.size()); }

private:
	enum class RequestKind { kNone, kFetch, kPost, kRefetch };

	struct Submission {
		std::string stage;
		int value = 0;
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
	// 少ない順・重複ありの全記録。割合と棒グラフは同じ値の人も1人ずつ数える。
	std::vector<int> values_;
	// 少ない順・重複なしの全記録。順位はこの添字+1。
	std::vector<int> ranking_;
	std::vector<int> top3_;
	bool failed_ = false;
};
