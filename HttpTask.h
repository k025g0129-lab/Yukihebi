#pragma once
#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

// HTTPS通信1件を別スレッドで行う。URLと本文しか知らない汎用クラス。
// 使い方は「Get/Postで始める → 毎フレームUpdate → trueが返ったら結果を読む」。
class HttpTask {
public:
	enum class State { kIdle, kRunning, kDone, kFailed };

	HttpTask() = default;
	~HttpTask();
	// スレッドを持つためコピーは禁止する。
	HttpTask(const HttpTask&) = delete;
	HttpTask& operator=(const HttpTask&) = delete;

	// 通信を始める。実行中の呼び出しはfalseを返して何もしない。
	bool Get(const std::string& url);
	bool Post(const std::string& url, const std::string& body, const std::string& contentType = "application/json");
	// 毎フレーム呼ぶ。完了した1フレームだけtrueを返す。ここでは待たない。
	bool Update();

	bool IsRunning() const { return state_.load() == State::kRunning; }
	State GetState() const { return state_.load(); }
	// 取り込み済みの結果。Updateがtrueを返した後に読む。
	int GetStatus() const { return status_; }
	const std::string& GetBody() const { return body_; }

private:
	bool Start(const std::string& url, const std::string& body, const std::string& contentType);

	std::thread worker_;
	// ワーカースレッドが書き、Updateが取り込む。mutex_で守る。
	std::mutex mutex_;
	bool finished_ = false;
	bool resultOk_ = false;
	int resultStatus_ = 0;
	std::string resultBody_;
	// 取り込み後の結果。こちらはメインスレッドしか触らないのでロック不要。
	int status_ = 0;
	std::string body_;
	std::atomic<State> state_{State::kIdle};
};
