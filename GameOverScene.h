#pragma once
#include <KamataEngine.h>

class GameOverScene {
public:
	void Initialize();
	void Update();
	void Draw();
	bool IsRetryRequested() const { return retryRequested_; }
	bool IsTitleRequested() const { return titleRequested_; }

private:
	bool retryRequested_ = false;
	bool titleRequested_ = false;
};