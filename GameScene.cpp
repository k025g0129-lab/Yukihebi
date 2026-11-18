#include "GameScene.h"

#include <algorithm>
#include "Boss.h"
#include  "CameraController.h"
#include "GameMath.h"
#include "Player.h"

GameScene::GameScene() {
}

GameScene::~GameScene() {
	delete player_;
	delete playerModel_;
	delete bossModel_;
	delete boss_;
}


void GameScene::Initialize() {
	InitializePlayerLighting();
	// CSV のマップチップからブロックを生成する。
	mapChipField_.LoadMapChipCSV("Resources/floor.csv");
	modelBlock_.reset(Model::CreateFromOBJ("cube"));
	GenerateBlocks();

	playerModel_ = Model::CreateFromOBJ("Player");
	player_ = new Player();

	snowModel_ = Model::CreateFromOBJ("snow");
	player_->Initialize(playerModel_, snowModel_, &camera_);
	player_->SetMapChipField(&mapChipField_);
	previousTime_ = std::chrono::steady_clock::now();
	snowPhysicsAccumulator_ = 0.0f;


	bossModel_ = Model::CreateFromOBJ("Boss");
	boss_ = new Boss();
	boss_->Initialize(bossModel_, &camera_, player_);

	cameraController_ = new CameraController();
	cameraController_->SetCamera(&camera_);
	// 初期化
	cameraController_->Initialize();
	// マップの端は最外周のマスの中心から半マス外側にある。
	// CameraController::Initialize() が入れている固定値のままだと、
	// マップの外へカメラが寄ってPlayerが画面から外れる。
	CameraController::Rect movableArea;
	movableArea.left = -MapChipField::kBlockWindth * 0.5f;
	movableArea.right = (18.0f - 0.5f) * MapChipField::kBlockWindth;
	movableArea.bottom = -MapChipField::kBlockHeight * 0.5f;
	movableArea.top = (18.0 - 0.5f) * MapChipField::kBlockHeight;
	cameraController_->SetMovableArea(movableArea);

	cameraController_->SetTarget(player_);

	// セット
	// cameraController_->SetTarget(player_);

	// カメラのリセット
	cameraController_->Reset();
	// const Vector3 gameplayCameraTranslation = camera_.translation_;
	//// Title 中だけ title.csv の全体を収める。Play では CameraController::Reset で戻す。
	// const float halfFovTangent = std::tan(camera_.fovAngleY * 0.5f);
	camera_.UpdateMatrix();
	// XZ 平面を見下ろす固定カメラ。画面の上が +Z 方向になる。
	camera_.Initialize();
	camera_.translation_ = {0.0f, 16.0f, -7.0f};
	camera_.rotation_ = {1.16f, 0.0f, 0.0f};
	camera_.UpdateMatrix();
	camera_.TransferMatrix();
	UpdateLighting();
}

void GameScene::Update() {
	const auto now = std::chrono::steady_clock::now();
	const float elapsed = +std::chrono::duration<float>(now - previousTime_).count();
	previousTime_ = now;
	player_->Update();

	snowPhysicsAccumulator_ += +std::clamp(elapsed, 0.0f, 0.05f);
	while (snowPhysicsAccumulator_ >= kSnowPhysicsStep) {
		player_->UpdateSnowPhysics(kSnowPhysicsStep, mapChipField_);
		snowPhysicsAccumulator_ -= kSnowPhysicsStep;
	}

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
	const uint32_t rows = mapChipField_.GetLoadedBlockVirtical();
	const uint32_t columns = mapChipField_.GetLoadedBlockHorizontal();
	worldTransformBlocks_.clear();
	if (rows == 0 || columns == 0) {
		return;
	}
	worldTransformBlocks_.resize(rows);
	for (auto& line : worldTransformBlocks_) {
		line.resize(columns);
	}

	for (uint32_t row = 0; row < rows; ++row) {
		for (uint32_t column = 0; column < columns; ++column) {
			if (mapChipField_.GetMapChipTypeByIndex(column, row) != MapChipType::kBlock) {
				continue;
			}

			auto block = std::make_unique<WorldTransform>();
			block->Initialize();
			// bloc.obj は -1～+1 の立方体。床一マスの幅と奥行きに合わせる。
			block->scale_ = {
				MapChipField::kBlockWindth * 0.5f, MapChipField::kTopDownBlockThickness * 0.5f,
				MapChipField::kBlockHeight * 0.5f
			};
			block->translation_ = mapChipField_.GetTopDownPositionByIndex(column, row);
			block->translation_.y = MapChipField::kTopDownFloorTop - MapChipField::kTopDownBlockThickness * 0.5f;
			block->matWorld_ = MakeAffineMatrix(block->scale_, block->rotation_, block->translation_);
			block->TransferMatrix();
			worldTransformBlocks_[row][column] = std::move(block);
		}
	}
}

void GameScene::InitializePlayerLighting() {
	playerLightGroup_.reset(LightGroup::Create());
	// 環境光は全方向の基礎の明るさ。平行光源とは別に調節する。
	playerLightGroup_->SetAmbientColor({0.5f, 0.5f, 0.5f});
	InitializeDirectionalLight();
	for (int i = 0; i < LightGroup::kPointLightNum; ++i) {
		playerLightGroup_->SetPointLightActive(i, i == 0);
	}
	for (int i = 0; i < LightGroup::kSpotLightNum; ++i) {
		playerLightGroup_->SetSpotLightActive(i, false);
	}
	for (int i = 0; i < LightGroup::kCircleShadowNum; ++i) {
		playerLightGroup_->SetCircleShadowActive(i, false);
	}
	playerLightGroup_->SetPointLightColor(0, {1.0f, 0.90f, 0.72f});
}

void GameScene::InitializeDirectionalLight() {
	// ゲームシーンの平行光源の調節箇所。位置はなく、全体に同じ方向から光が届く。
	constexpr Vector3 direction{-0.3f, 1.0f, -0.4f}; // 光が進む方向（ゼロベクトル不可）
	constexpr Vector3 color{1.0f, 0.0f, 0.0f}; // RGB
	constexpr float intensity = 10.0f; // 0で消灯、値を上げるほど明るくなる

	for (int i = 0; i < LightGroup::kDirLightNum; ++i) {
		playerLightGroup_->SetDirLightActive(i, i == 0);
	}
	playerLightGroup_->SetDirLightDir(0, direction);
	playerLightGroup_->SetDirLightColor(0, color * intensity);
}

void GameScene::UpdateLighting() {
	modelBlock_->SetLightGroup(playerLightGroup_.get());
	playerModel_->SetLightGroup(playerLightGroup_.get());
	snowModel_->SetLightGroup(playerLightGroup_.get());
	bossModel_->SetLightGroup(playerLightGroup_.get());
	playerLightGroup_->Update();
}
