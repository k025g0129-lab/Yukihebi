#include "GameOverScene.h"

using namespace KamataEngine;

void GameOverScene::Initialize() {
	retryRequested_ = false;
	titleRequested_ = false;
}

void GameOverScene::Update() {
	const Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_R)) {
		retryRequested_ = true;
	} else if (input->TriggerKey(DIK_RETURN)) {
		titleRequested_ = true;
	}
}

void GameOverScene::Draw() {
	DebugText::GetInstance()->Print("GAME OVER", 48.0f, 120.0f, 2.0f);
	DebugText::GetInstance()->Print("THE BOSS HIT YOU", 48.0f, 190.0f);
	DebugText::GetInstance()->Print("R: RETRY    ENTER: TITLE", 48.0f, 250.0f);
	Sprite::PreDraw();
	DebugText::GetInstance()->DrawAll();
	Sprite::PostDraw();
}