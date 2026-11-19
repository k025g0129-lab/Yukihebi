#include "ClearScene.h"

using namespace KamataEngine;

void ClearScene::Initialize() {

	// スペースキーが押されたら終了フラグを立てる
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		clearScened_ = true;
	}





}

ClearScene::~ClearScene() {



}


void ClearScene::Update() {






}

void ClearScene::Draw() {








}

