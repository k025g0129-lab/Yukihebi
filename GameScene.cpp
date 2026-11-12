#include "GameScene.h"

#include "GameMath.h"
#include "Player.h"
#include "Boss.h"

GameScene::GameScene() {
}

GameScene::~GameScene() {
	delete player_;
	delete playerModel_;
	delete bossModel_;
	delete boss_ ;
}


void GameScene::Initialize() {
	// XZ 平面を見下ろす固定カメラ。画面の上が +Z 方向になる。
	camera_.Initialize();
	camera_.translation_ = {0.0f, 16.0f, -7.0f};
	camera_.rotation_ = {1.16f, 0.0f, 0.0f};
	camera_.UpdateMatrix();
	camera_.TransferMatrix();
	

	// CSV のマップチップからブロックを生成する。
	mapChipField_.LoadMapChipCSV("Resources/floor.csv");
	modelBlock_.reset(Model::CreateFromOBJ("bloc"));
	GenerateBlocks();

	playerModel_ = Model::CreateFromOBJ("Player");
	player_ = new Player();
	player_->Initialize(playerModel_, &camera_);

	bossModel_ = Model::CreateFromOBJ("Boss");
	boss_ = new Boss();
	boss_->Initialize(bossModel_, &camera_,player_);

}

void GameScene::Update() {
	player_->Update();
	boss_->Update();
}


void GameScene::Draw() {
	Model::PreDraw();
	for (size_t row = 0; row < worldTransformBlocks_.size(); ++row) {
		for (size_t column = 0; column < worldTransformBlocks_[row].size(); ++column) {
			const WorldTransform* block = worldTransformBlocks_[row][column].get();
			if (block == nullptr) {
				continue;
			}
			if (mapChipField_.GetMapChipTypeByIndex(
					static_cast<uint32_t>(column), static_cast<uint32_t>(row)) == MapChipType::kBlock) {
				modelBlock_->Draw(*block, camera_);
			}
		}
	}
	player_->Draw();
	boss_->Draw();

	Model::PostDraw();
}

void GameScene::GenerateBlocks() {
	const uint32_t rows = mapChipField_.GetNumBlockVirtical();
	const uint32_t columns = mapChipField_.GetNumBlockHorizontal();
	worldTransformBlocks_.clear();
	worldTransformBlocks_.resize(rows);
	for (auto& line : worldTransformBlocks_) {
		line.resize(columns);
	}

	// MapChipField の位置は XY。見下ろし用に XZ へ写し、9×9 のマップを原点中心に置く。
	const float centerX = (kFloorSide - 1) * MapChipField::kBlockWindth * 0.5f;
	const float centerY =
		(rows - 1 - (kFloorSide - 1) * 0.5f) * MapChipField::kBlockHeight;
	for (uint32_t row = 0; row < rows; ++row) {
		for (uint32_t column = 0; column < columns; ++column) {
			if (mapChipField_.GetMapChipTypeByIndex(column, row) != MapChipType::kBlock) {
				continue;
			}

			auto block = std::make_unique<WorldTransform>();
			block->Initialize();
			// bloc.obj は -1～+1 の立方体。床一マスの幅と奥行きに合わせる。
			block->scale_ = {
				MapChipField::kBlockWindth * 0.5f, 0.05f,
				MapChipField::kBlockHeight * 0.5f};
			const Vector3 chipPosition = mapChipField_.GetMapChipPositionByIndex(column, row);
			block->translation_ = {chipPosition.x - centerX, -0.05f, chipPosition.y - centerY};
			block->matWorld_ = MakeAffineMatrix(block->scale_, block->rotation_, block->translation_);
			block->TransferMatrix();
			worldTransformBlocks_[row][column] = std::move(block);
		}
	}
}
