#pragma once
#pragma once

#include <string>

// WinHTTPでHTTPS通信を1回行う。通信が終わるまで戻らない（同期）。
// ゲームループから直接呼ばない。HttpTaskがワーカースレッドから使う。
class HttpClient {
public:
	struct Response {
		// 通信そのものが成立したか。404が返った場合もtrueで、statusで判断する。
		bool ok = false;
		int status = 0;
		// 本文。UTF-8のまま。解析は呼び出し側の仕事。
		std::string body;
	};

	// urlは "https://host/path" 形式。bodyが空ならGET、空でなければPOST。
	// timeoutMsは接続・送信・受信それぞれの上限。
	static Response Request(const std::string& url, const std::string& body = {},
	                        const std::string& contentType = "application/json", int timeoutMs = 3000);
};
