#include "Fade.h"

using namespace KamataEngine;

void Fade::Initialize() {

	//テクスチャを読み込む
	uint32_t textureHandle = TextureManager::Load("white1x1.png");

	//スプライト生成
	sprite_ = Sprite::Create(textureHandle, {0.0f, 0.0f});
	sprite_->SetSize(Vector2());
	sprite_->SetColor(Vector4(0, 0, 0, 1));

}

Fade::~Fade() {







}

void Update() {






}

void Fade::Draw() {


	Sprite::PreDraw();

	sprite_->Draw();

	Sprite::PostDraw();


}