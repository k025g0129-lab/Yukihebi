#include "GameScene.h"

#include "GameMath.h"
#include "Player.h"

GameScene::GameScene() {
}

GameScene::~GameScene() = default;


void GameScene::Initialize() {
	// XZ 平面を見下ろす固定カメラ。画面の上が +Z 方向になる。
	camera_.Initialize();
	camera_.translation_ = {0.0f, 16.0f, -7.0f};
	camera_.rotation_ = {1.16f, 0.0f, 0.0f};
	camera_.UpdateMatrix();
	camera_.TransferMatrix();

	// 床の配置はマップチップで決める。既存のマップチップ座標は XY 用なので、
	// 見下ろし用の床では列を X、行を Z へ対応させる。
	constexpr int32_t kFloorHalfSide = kFloorSide / 2;
	mapChipField_.LoadMapChipCSV("Resources/floor.csv");
	floorModel_.reset(Model::CreateFromOBJ("Player"));
	floorTiles_.clear();
	floorTiles_.reserve(kFloorSide * kFloorSide);
	for (uint32_t row = 0; row < kFloorSide; ++row) {
		for (uint32_t column = 0; column < kFloorSide; ++column) {
			auto tile = std::make_unique<WorldTransform>();
			tile->Initialize();
			// cube.obj は各軸 -1～+1。半径 0.5 で 1 マスの大きさにする。
			tile->scale_ = {
				MapChipField::kBlockWindth * 0.5f, 0.05f,
				MapChipField::kBlockHeight * 0.5f
			};
			tile->translation_ = {
				(static_cast<int32_t>(column) - kFloorHalfSide) * MapChipField::kBlockWindth,
				-0.05f,
				(kFloorHalfSide - static_cast<int32_t>(row)) * MapChipField::kBlockHeight
			};
			tile->matWorld_ = MakeAffineMatrix(tile->scale_, tile->rotation_, tile->translation_);
			tile->TransferMatrix();
			floorTiles_.push_back(std::move(tile));
		}
	}
	floorColor_.Initialize();
	floorColor_.SetColor({0.35f, 0.4f, 0.45f, 1.0f});

	player_ = std::make_unique<Player>();
	player_->Initialize(&camera_);
}

void GameScene::Update() {
	player_->Update();
}


void GameScene::Draw() {
	Model::PreDraw();
	for (uint32_t row = 0; row < kFloorSide; ++row) {
		for (uint32_t column = 0; column < kFloorSide; ++column) {
			if (mapChipField_.GetMapChipTypeByIndex(column, row) == MapChipType::kBlock) {
				floorModel_->Draw(*floorTiles_[row * kFloorSide + column], camera_, &floorColor_);
			}
		}
	}
	player_->Draw();
	Model::PostDraw();
}
