#include "GameScene.h"


GameScene::GameScene() {
}

GameScene::~GameScene() {

	delete modelBlock_;
	delete modelSkydome_;
	delete modelBoss_;


}


void GameScene::Initialize() {

	//モデル入れ、後で各.hで出来るようにするかも
	modelBlock_ = Model::CreateFromOBJ("block", true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelBoss_ = Model::CreateFromOBJ("boss", true);
	
	//初期化
	worldTransform_.Initialize();	
	camera_.farZ = 1500.0f;
	camera_.Initialize();

	//カメラ関係
	PrimitiveDrawer::GetInstance()->SetCamera(&camera_);

	debugCamera_ = new DebugCamera(1280, 720);
	AxisIndicator::GetInstance()->SetVisible(true);
	AxisIndicator::GetInstance()->SetTargetCamera(&debugCamera_->GetCamera());

	//天球
	//skydome_ = new Skydome();
	//skydome_->Initialize(modelSkydome_, &camera_);

	//ブロック
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;

	worldTransformBlocks_.resize(kNumBlockVirtical);

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	
	//マップチップファイル
	//mapChipField_ = new MapChipField();


}

void GameScene::Update() {


}


void GameScene::Draw() {
	Model::PreDraw();
	Model::PostDraw();
}
