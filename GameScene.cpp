#include "GameScene.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include "Boss.h"
#include  "CameraController.h"
#include "Effects.h"
#include "GameMath.h"
#include "Player.h"

namespace {
	// 雪玉の通知を見た目へ変換する場所。粒の設定はここでまとめて調整する。
	void SpawnSnowEffect(Effects& effects, const Snow::EffectEvent& event) {
		int particleCount = 0;
		Vector4 color{};
		float lifeTime = 0.0f;
		float gravity = 0.0f;

		switch (event.type) {
		case Snow::EffectType::kPlayerHit:
			particleCount = 12;
			color = {1.0f, 0.73f, 0.52f, 0.95f};
			lifeTime = 0.32f;
			gravity = 3.0f;
			break;
		case Snow::EffectType::kSnowHit:
			particleCount = 16;
			color = {0.62f, 0.88f, 1.0f, 0.9f};
			lifeTime = 0.42f;
			gravity = 2.0f;
			break;
		case Snow::EffectType::kMaxSize:
			particleCount = 24;
			color = {1.0f, 0.88f, 0.35f, 1.0f};
			lifeTime = 0.65f;
			gravity = 0.8f;
			break;
		}

		const float impact = std::clamp(event.impactSpeed, 0.0f, 8.0f);
		for (int index = 0; index < particleCount; ++index) {
			const float angle = kTwoPi * static_cast<float>(index) / static_cast<float>(particleCount);
			const Vector3 outward = {std::cos(angle), 0.0f, std::sin(angle)};

			Vector3 position = event.position;
			Vector3 velocity{};

			if (event.type == Snow::EffectType::kMaxSize) {
				// 最大サイズでは雪玉の周囲から金色の輪が広がる。
				position += outward * (event.radius * 0.75f);
				velocity = outward * 2.0f + Vector3{0.0f, 1.2f, 0.0f};
			}
			else if (event.type == Snow::EffectType::kPlayerHit) {
				// Playerとの接触では接触面から外向きに短く散らす。
				velocity = event.normal * (0.8f + impact * 0.25f) + outward * 0.8f
					+ Vector3{0.0f, 20.0f, 0.0f};
			}
			else {
				// 雪玉同士では接点を中心に冷たい色の粒を広げる。
				velocity = event.normal * (impact * 0.15f) + outward * (1.1f + impact * 0.2f)
					+ Vector3{0.0f, 0.65f, 0.0f};
			}

			const float size = 0.035f + event.radius * 0.045f;
			effects.Spawn({position, velocity, color, size, lifeTime, gravity});
		}
	}
} // namespace

GameScene::GameScene() {
}

GameScene::~GameScene() {
	delete player_;
	delete playerModel_;
	delete bossModel_;
	delete cameraController_;
	delete effects_;
	delete boss_;
}


void GameScene::Initialize() {
	InitializePlayerLighting();


	// CSV のマップチップからブロックを生成する。
	mapChipField_.LoadMapChipCSV("Resources/floor.csv");
	const float halfMapWidth = mapChipField_.GetLoadedBlockHorizontal() * MapChipField::kBlockWindth * 0.5f;
	const float halfMapDepth = mapChipField_.GetLoadedBlockVirtical() * MapChipField::kBlockHeight * 0.5f;
	modelBlock_.reset(Model::CreateFromOBJ("cube"));
	GenerateBlocks();

	playerModel_ = Model::CreateFromOBJ("Player");
	player_ = new Player();

	snowModel_ = Model::CreateFromOBJ("snow");
	player_->Initialize(playerModel_, snowModel_, &camera_);
	player_->SetMapChipField(&mapChipField_);
	previousTime_ = std::chrono::steady_clock::now();
	snowPhysicsAccumulator_ = 0.0f;

	effects_ = new Effects();
	effects_->Initialize();

	bossModel_ = Model::CreateFromOBJ("Boss");
	boss_ = new Boss();
	boss_->Initialize(bossModel_, &camera_, player_);

	cameraController_ = new CameraController();
	cameraController_->SetCamera(&camera_);
	cameraController_->Initialize();
	cameraController_->SetTarget(player_);
	cameraController_->targetOffset_ = {0.0f, 15.0f, -6.0f};

	CameraController::Rect movableArea{};
	movableArea.left = -halfMapWidth;
	movableArea.right = halfMapWidth;
	movableArea.bottom = -halfMapDepth;
	movableArea.top = halfMapDepth;
	cameraController_->SetMovableArea(movableArea);

	cameraController_->Reset();
	UpdateLighting();
}

void GameScene::Update() {
	const auto now = std::chrono::steady_clock::now();
	const float elapsed = +std::chrono::duration<float>(now - previousTime_).count();
	previousTime_ = now;
	const float deltaTime = std::clamp(elapsed, 0.0f, 0.05f);
	player_->Update(deltaTime);
	UpdateLighting();
	snowPhysicsAccumulator_ += deltaTime;
	while (snowPhysicsAccumulator_ >= kSnowPhysicsStep) {
		player_->UpdateSnowPhysics(kSnowPhysicsStep, mapChipField_);
		snowPhysicsAccumulator_ -= kSnowPhysicsStep;
	}
	std::vector<Snow::EffectEvent> snowEffectEvents;
	player_->DrainSnowEffectEvents(snowEffectEvents);
	for (const Snow::EffectEvent& event : snowEffectEvents) {
		SpawnSnowEffect(*effects_, event);
	}
	effects_->Update(deltaTime);
	cameraController_->Update();
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
	effects_->Draw(camera_);

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

	constexpr Vector3 color{1.0f, 1.0f, 1.0f}; // RGB
	constexpr float intensity = 1.0f; // 0で消灯、値を上げるほど明るくなる

	for (int i = 0; i < LightGroup::kDirLightNum; ++i) {
		playerLightGroup_->SetDirLightActive(i, i == 0);
	}
	playerLightGroup_->SetDirLightDir(0, direction);
	playerLightGroup_->SetDirLightColor(0, color * intensity);
}

void GameScene::UpdateLighting() {
	//constexpr float speed = 0.3f;
	//constexpr float intensity = 10.0f;

	//Timer += speed;

	//Vector3 color{
	//	(std::sin(Timer) + 1.0f) * 0.5f, (std::sin(Timer + 2.094395f) + 1.0f) * 0.5f,
	//	(std::sin(Timer + 4.188790f) + 1.0f) * 0.5f
	//};

	//playerLightGroup_->SetDirLightColor(0, color * intensity);
	//playerLightGroup_->SetPointLightPos(0, {0.0f, 0.0f, 0.0f});
	//playerLightGroup_->SetPointLightColor(0, {
	//	                                      (std::sin(Timer + 2.094395f) + 1.0f) * 0.5f,
	//	                                      (std::sin(Timer + 4.188790f) + 1.0f) * 0.5f,
	//	                                      (std::sin(Timer) + 1.0f) * 0.5f
	//                                      });
	//playerLightGroup_->SetPointLightIntensity(0, 100.0f);
	effects_->SetLightGroup(playerLightGroup_.get());
	modelBlock_->SetLightGroup(playerLightGroup_.get());
	playerModel_->SetLightGroup(playerLightGroup_.get());
	snowModel_->SetLightGroup(playerLightGroup_.get());
	bossModel_->SetLightGroup(playerLightGroup_.get());
	playerLightGroup_->Update();
}
