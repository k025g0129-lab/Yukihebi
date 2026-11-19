#include "TitleScene.h"

using namespace KamataEngine;

void TitleScene::Initialize() 
{





}

void TitleScene::Update() 
{

	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
	
		finished_ = true;
	
	}


}

TitleScene::~TitleScene() {



}

void TitleScene::Draw()
{






}
