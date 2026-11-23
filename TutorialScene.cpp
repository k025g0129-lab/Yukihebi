#include "TutorialScene.h"

using namespace KamataEngine;

void TutorialScene::Initialize() 
{






}

TutorialScene::~TutorialScene()
{





}


void TutorialScene::Update()
{

	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
	
		tutorial_ = true;
	
	}




}

void TutorialScene::Draw()
{
	DebugText::GetInstance()->Print("SIX PETAL SNOW SNAKE", 48.0f, 72.0f, 2.0f);
	DebugText::GetInstance()->Print("WASD: MOVE", 48.0f, 150.0f);
	DebugText::GetInstance()->Print("SPACE: MAKE AND ROLL SNOWBALL", 48.0f, 180.0f);
	DebugText::GetInstance()->Print("RELEASE SPACE: LET GO", 48.0f, 210.0f);
	DebugText::GetInstance()->Print("E: KICK NEARBY SNOWBALL", 48.0f, 240.0f);
	DebugText::GetInstance()->Print("AVOID THE BOSS HEAD AND BODY", 48.0f, 270.0f);
	DebugText::GetInstance()->Print("PRESS ENTER TO START", 48.0f, 330.0f);
	Sprite::PreDraw();
	DebugText::GetInstance()->DrawAll();
	Sprite::PostDraw();






}

