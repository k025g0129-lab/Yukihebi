#include "ClearScene.h"

using namespace KamataEngine;

void ClearScene::Initialize(const float ClearTime) {
	clearScened_ = false;
	ClearTime_ = ClearTime;
}

ClearScene::~ClearScene() = default;

void ClearScene::Update() {
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		clearScened_ = true;
	}
}

void ClearScene::Draw() {
	DebugText::GetInstance()->Print("BOSS DEFEATED - CLEAR!", 48.0f, 120.0f, 2.0f);
	DebugText::GetInstance()->Print("PRESS ENTER FOR TITLE", 48.0f, 220.0f);
	DebugText::GetInstance()->Print("ClearTime" + std::to_string(ClearTime_), 48.0f, 300.0f);
	Sprite::PreDraw();
	DebugText::GetInstance()->DrawAll();
	Sprite::PostDraw();
}
