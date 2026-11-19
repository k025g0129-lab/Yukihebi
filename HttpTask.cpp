#include "HttpTask.h"

#include "HttpClient.h"

HttpTask::~HttpTask() {
	// 走っているスレッドを残して解放すると落ちる。必ず終わるまで待つ。
	if (worker_.joinable()) {
		worker_.join();
	}
}

bool HttpTask::Get(const std::string& url) { return Start(url, {}, {}); }

bool HttpTask::Post(const std::string& url, const std::string& body, const std::string& contentType) {
	return Start(url, body, contentType);
}

bool HttpTask::Start(const std::string& url, const std::string& body, const std::string& contentType) {
	if (state_.load() == State::kRunning) {
		return false;
	}
	// 前回のスレッドをここで片付ける。joinせずに代入すると終了時に落ちる。
	if (worker_.joinable()) {
		worker_.join();
	}
	{
		std::lock_guard<std::mutex> lock(mutex_);
		finished_ = false;
		resultOk_ = false;
		resultStatus_ = 0;
		resultBody_.clear();
	}
	state_.store(State::kRunning);
	// url・bodyは値で渡す。呼び出し側の文字列が先に消えても安全にする。
	worker_ = std::thread([this, url, body, contentType] {
		const HttpClient::Response result = HttpClient::Request(url, body, contentType);
		std::lock_guard<std::mutex> lock(mutex_);
		resultOk_ = result.ok;
		resultStatus_ = result.status;
		resultBody_ = result.body;
		finished_ = true;
	});
	return true;
}

bool HttpTask::Update() {
	if (state_.load() != State::kRunning) {
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(mutex_);
		// まだ終わっていなければ何もしない。ここで待つとゲームが止まる。
		if (!finished_) {
			return false;
		}
		status_ = resultStatus_;
		body_ = resultBody_;
		state_.store(resultOk_ ? State::kDone : State::kFailed);
	}
	// 終わったスレッドの後片付け。すでに走り終わっているので待ち時間はない。
	worker_.join();
	return true;
}
