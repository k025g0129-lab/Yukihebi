#include "HttpClient.h"

#include <Windows.h>
#include <winhttp.h>
// WinHTTPの実体はwinhttp.dll。その呼び出し口をリンカーへ要求する。
#pragma comment(lib, "winhttp.lib")
namespace {
	// WinHTTPはwchar_tしか受け取らないため、UTF-8から変換する。
	std::wstring ToWide(const std::string& text) {
		if (text.empty()) {
			return {};
		}
		const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
		std::wstring wide(static_cast<size_t>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), length);
		return wide;
	}

	// 途中でreturnしても閉じ忘れないようにする持ち主。閉じ忘れはハンドル漏れになる。
	struct Handle {
		HINTERNET value = nullptr;

		~Handle() {
			if (value != nullptr) {
				WinHttpCloseHandle(value);
			}
		}
	};
} // namespace

HttpClient::Response HttpClient::Request(const std::string& url, const std::string& body,
                                         const std::string& contentType, int timeoutMs) {
	Response response{};
	const std::wstring wideUrl = ToWide(url);

	// URLをホスト名とパスに分ける。長さに-1を入れると、コピーせず
	// wideUrl内の位置と長さを返す。返るポインタはNUL終端ではない。
	URL_COMPONENTS components{};
	components.dwStructSize = sizeof(components);
	components.dwHostNameLength = static_cast<DWORD>(-1);
	components.dwUrlPathLength = static_cast<DWORD>(-1);
	components.dwExtraInfoLength = static_cast<DWORD>(-1);
	if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &components)) {
		return response;
	}
	const std::wstring host(components.lpszHostName, components.dwHostNameLength);
	const std::wstring path = std::wstring(components.lpszUrlPath, components.dwUrlPathLength) + std::wstring(
		components.lpszExtraInfo, components.dwExtraInfoLength);

	Handle session{
		WinHttpOpen(L"TD/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)
	};
	if (session.value == nullptr) {
		return response;
	}
	// 学内ネットワークなどで応答がない場合に、スレッドが居座らないようにする。
	WinHttpSetTimeouts(session.value, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

	Handle connect{WinHttpConnect(session.value, host.c_str(), components.nPort, 0)};
	if (connect.value == nullptr) {
		return response;
	}

	const bool post = !body.empty();
	Handle request{
		WinHttpOpenRequest(
			connect.value, post ? L"POST" : L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
	};
	if (request.value == nullptr) {
		return response;
	}

	const std::wstring headers = post ? L"Content-Type: " + ToWide(contentType) : std::wstring{};
	if (!WinHttpSendRequest(
		request.value, headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
		headers.empty() ? 0 : static_cast<DWORD>(-1),
		post ? const_cast<char*>(body.data()) : WINHTTP_NO_REQUEST_DATA, static_cast<DWORD>(body.size()),
		static_cast<DWORD>(body.size()), 0)) {
		return response;
	}
	if (!WinHttpReceiveResponse(request.value, nullptr)) {
		return response;
	}

	DWORD status = 0;
	DWORD statusSize = sizeof(status);
	WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
	                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
	response.status = static_cast<int>(status);

	// 本文は一度に全部来ない。「使える分を聞く→読む」を0になるまで繰り返す。
	for (;;) {
		DWORD available = 0;
		if (!WinHttpQueryDataAvailable(request.value, &available) || available == 0) {
			break;
		}
		const size_t offset = response.body.size();
		response.body.resize(offset + available);
		DWORD read = 0;
		if (!WinHttpReadData(request.value, response.body.data() + offset, available, &read)) {
			response.body.resize(offset);
			break;
		}
		// 実際に読めた分だけに切り直す。availableより少ないことがある。
		response.body.resize(offset + read);
	}
	response.ok = true;
	return response;
}
