#include "GameScene.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "Boss.h"
#include  "CameraController.h"
#include "Effects.h"
#include "GameMath.h"
#include "Player.h"


namespace {
	// 接触面に沿う3D方向を選び、法線方向には少しだけ揺らぎを加える。
	// 法線の正負へ強く飛ばすと、粒が雪玉やPlayerの内部に入って隠れる。
	Vector3 ScatterAlongContact(const Vector3& normal, float normalSpread) {
		const Vector3 axis = Normalize(normal);
		Vector3 tangent{};
		do {
			const Vector3 randomDirection = Random::Direction();
			tangent = randomDirection - axis * Dot(randomDirection, axis);
		}
		while (LengthSquared(tangent) < 0.000001f);
		return Normalize(Normalize(tangent) + axis * Random::Float(-normalSpread, normalSpread));
	}

	// 粒をまとめて出すときの指定。
	struct BurstDesc {
		Vector3 position{};
		Vector3 normal{0.0f, 1.0f, 0.0f};
		Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		int count = 0;
		float minSpeed = 1.0f;
		float maxSpeed = 2.0f;
		float minSize = 0.03f;
		float maxSize = 0.06f;
		float minLife = 0.2f;
		float maxLife = 0.4f;
		float normalSpread = 0.3f; // 接触面からの浮き上がりの揺らぎ
		float normalBias = 0.0f; // 法線方向へ寄せる量。大きいほど面から噴き出す
		float drag = 0.0f;
		float gravity = 0.0f;
		bool sphere = false; // trueなら全方向へ散らす
	};

	void SpawnBurst(Effects& effects, const BurstDesc& desc) {
		for (int index = 0; index < desc.count; ++index) {
			Vector3 direction = desc.sphere ? Random::Direction() : ScatterAlongContact(desc.normal, desc.normalSpread);
			if (desc.normalBias > 0.0f) {
				direction = Normalize(direction + Normalize(desc.normal) * desc.normalBias);
			}
			if (!effects.Spawn({
				desc.position + direction * 0.03f, direction * Random::Float(desc.minSpeed, desc.maxSpeed),
				desc.color, Random::Float(desc.minSize, desc.maxSize), Random::Float(desc.minLife, desc.maxLife),
				desc.gravity, desc.drag
			})) {
				break;
			}
		}
	}

	// 接点に一瞬だけ大きな光を置く。動かないので、ヒットストップ中も同じ場所で光る。
	void SpawnFlash(Effects& effects, const Vector3& position, const Vector4& color, float size, float lifeTime) {
		effects.Spawn({position, {}, color, size, lifeTime, 0.0f});
	}

	// 上から見て輪が広がる衝撃波。真上からのカメラで最も読み取りやすい形。
	void SpawnRing(Effects& effects, const Vector3& center, const Vector4& color, int count, float speed, float size,
	               float lifeTime, float drag) {
		const float offset = Random::Float(0.0f, 6.2831853f);
		for (int index = 0; index < count; ++index) {
			const float angle = offset + 6.2831853f * static_cast<float>(index) / static_cast<float>(count);
			const Vector3 direction{std::cos(angle), 0.0f, std::sin(angle)};
			if (!effects.Spawn({center + direction * 0.05f, direction * speed, color, size, lifeTime, 0.0f, drag})) {
				break;
			}
		}
	}

	// 周りから中心へ吸い込まれる粒。寿命が尽きる頃に中心へ届く。
	void SpawnImplode(Effects& effects, const Vector3& center, const Vector4& color, int count, float radius,
	                  float lifeTime) {
		for (int index = 0; index < count; ++index) {
			const float angle = Random::Float(0.0f, 6.2831853f);
			const Vector3 direction{std::cos(angle), Random::Float(-0.2f, 0.2f), std::sin(angle)};
			const float distance = radius * Random::Float(0.8f, 1.2f);
			const float life = lifeTime * Random::Float(0.8f, 1.0f);
			if (!effects.Spawn({
				center + direction * distance, -direction * (distance / life), color,
				Random::Float(0.04f, 0.07f), life, 0.0f
			})) {
				break;
			}
		}
	}

	// 爆散。撃破・死亡・尻尾が切れるときに使う。
	void SpawnExplosion(Effects& effects, const Vector3& center, const Vector4& color, float scale) {
		SpawnFlash(effects, center, {1.0f, 1.0f, 0.9f, 1.0f}, 0.5f * scale, 0.15f);
		SpawnBurst(effects, {
			.position = center, .color = color, .count = static_cast<int>(20.0f * scale),
			.minSpeed = 3.0f * scale, .maxSpeed = 9.0f * scale, .minSize = 0.06f, .maxSize = 0.12f,
			.minLife = 0.4f, .maxLife = 0.8f, .drag = 5.0f, .sphere = true
		});
		SpawnRing(effects, center, {1.0f, 0.9f, 0.7f, 1.0f}, static_cast<int>(12.0f * scale), 8.0f * scale,
		          0.07f, 0.35f, 6.0f);
	}

	void SpawnPlayerSnowEffect(Effects& effects, const Snow::EffectEvent& event) {
		const float power = std::clamp(event.impactSpeed / 6.0f, 0.0f, 1.0f);
		const int count = 15 + static_cast<int>(power * 20.0f);
		const Vector3 contact = event.position;
		const float size = 0.02f + event.radius * 0.05f;

		// 接点の短い閃光の後、Playerと雪玉の接触面に沿って飛散する。
		effects.Spawn({
			contact, {}, {1.0f, 0.95f, 0.8f, 1.0f},
			0.05f + event.radius * 0.12f, 0.1f, 0.0f
		});
		for (int index = 0; index < count; ++index) {
			const Vector3 direction = ScatterAlongContact(event.normal, 0.18f);
			const float speed = Random::Float(0.9f, 2.0f + power * 4.5f);
			const Vector3 position = contact + direction * 0.03f;
			if (!effects.Spawn({
				position, direction * speed, {1.0f, 0.75f, 0.55f, 1.0f},
				size, Random::Float(0.16f, 0.3f), 3.0f
			})) {
				break;
			}
		}
	}

	void SpawnSnowSnowEffect(Effects& effects, const Snow::EffectEvent& event) {
		const float power = std::clamp(event.impactSpeed / 6.0f, 0.0f, 1.0f);
		const int count = 10 + static_cast<int>(power * 20.0f);
		const float size = 0.04f + event.radius * 0.045f;

		// 二つの雪玉の接点から、球面の接線方向へ立体的に散らす。
		effects.Spawn({
			event.position, {}, {0.9f, 0.97f, 1.0f, 1.0f},
			0.05f + event.radius * 0.08f, 0.09f, 0.0f
		});
		for (int index = 0; index < count; ++index) {
			const Vector3 direction = ScatterAlongContact(event.normal, 0.25f);
			const Vector3 position = event.position + direction * 0.03f;
			const float speed = Random::Float(1.0f, 2.3f + power * 5.5f);
			if (!effects.Spawn({
				position, direction * speed, {0.62f, 0.88f, 1.0f, 1.0f},
				size, Random::Float(0.18f, 0.36f), 3.5f
			})) {
				break;
			}
		}
	}

	void SpawnMaxSizeEffect(Effects& effects, const Snow::EffectEvent& event) {
		// 最大サイズ到達時は雪玉の表面全体から外へ飛ぶ。
		for (int index = 0; index < 24; ++index) {
			const Vector3 direction = Random::Direction();
			const Vector3 position = event.position + direction * (event.radius * 1.02f);
			const Vector3 velocity = direction * Random::Float(1.3f, 2.8f);
			if (!effects.Spawn({
				position, velocity, {1.0f, 0.88f, 0.35f, 1.0f},
				0.035f + event.radius * 0.045f, Random::Float(0.4f, 0.65f), 0.8f
			})) {
				break;
			}
		}
		// 完成を知らせる閃光と、足元に広がる金色の輪。
		SpawnFlash(effects, event.position, {1.0f, 0.95f, 0.6f, 1.0f}, event.radius * 1.4f, 0.12f);
		SpawnRing(effects, event.position, {1.0f, 0.88f, 0.35f, 1.0f}, 28, 7.0f, 0.06f, 0.4f, 5.0f);
	}

	void SpawnWallHitEffect(Effects& effects, const Snow::EffectEvent& event) {
		const float power = std::clamp(event.impactSpeed / 6.0f, 0.0f, 1.0f);
		SpawnFlash(effects, event.position, {0.9f, 0.97f, 1.0f, 1.0f}, 0.08f + event.radius * 0.3f * power, 0.1f);
		// 壁からステージの内側へ雪を噴き出させる。
		SpawnBurst(effects, {
			.position = event.position, .normal = event.normal, .color = {0.75f, 0.92f, 1.0f, 1.0f},
			.count = 12 + static_cast<int>(power * 24.0f), .minSpeed = 2.0f, .maxSpeed = 4.0f + power * 7.0f,
			.minSize = 0.03f + event.radius * 0.04f, .maxSize = 0.05f + event.radius * 0.07f,
			.minLife = 0.25f, .maxLife = 0.5f, .normalSpread = 0.3f, .normalBias = 0.8f, .drag = 8.0f
		});
	}

	void SpawnKickEffect(Effects& effects, const Snow::EffectEvent& event) {
		// 蹴った足元の閃光と、Player側へ跳ね返る雪の破片。
		SpawnFlash(effects, event.position, {1.0f, 1.0f, 0.9f, 1.0f}, 0.12f + event.radius * 0.4f, 0.1f);
		SpawnBurst(effects, {
			.position = event.position, .normal = event.normal, .color = {0.9f, 0.97f, 1.0f, 1.0f},
			.count = 20, .minSpeed = 2.0f, .maxSpeed = 6.0f, .minSize = 0.03f, .maxSize = 0.06f,
			.minLife = 0.2f, .maxLife = 0.4f, .normalSpread = 0.4f, .normalBias = 0.5f, .drag = 9.0f
		});
		SpawnRing(effects, event.position, {1.0f, 1.0f, 1.0f, 1.0f}, 14, 4.5f, 0.04f, 0.22f, 8.0f);
	}

	void SpawnCreateEffect(Effects& effects, const Snow::EffectEvent& event) {
		// 雪をすくい上げたような小さな粒をふわっと出す。
		SpawnBurst(effects, {
			.position = event.position, .color = {0.85f, 0.95f, 1.0f, 1.0f}, .count = 12,
			.minSpeed = 0.6f, .maxSpeed = 1.8f, .minSize = 0.025f, .maxSize = 0.045f,
			.minLife = 0.3f, .maxLife = 0.5f, .drag = 3.0f, .sphere = true
		});
	}

	void SpawnSnowEffect(Effects& effects, const Snow::EffectEvent& event) {
		switch (event.type) {
		case Snow::EffectType::kPlayerHit:
			SpawnPlayerSnowEffect(effects, event);
			break;
		case Snow::EffectType::kSnowHit:
			SpawnSnowSnowEffect(effects, event);
			break;
		case Snow::EffectType::kMaxSize:
			SpawnMaxSizeEffect(effects, event);
			break;
		case Snow::EffectType::kWallHit:
			SpawnWallHitEffect(effects, event);
			break;
		case Snow::EffectType::kKick:
			SpawnKickEffect(effects, event);
			break;
		case Snow::EffectType::kCreate:
			SpawnCreateEffect(effects, event);
			break;
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
	input = Input::GetInstance();
	InitializePlayerLighting();
	clearTime = 0.0f;

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
	hitStopRemainingSeconds_ = 0.0f;
	ending_ = Ending::kNone;
	endingTimer_ = 0.0f;
	defeatChain_.clear();
	defeatChainIndex_ = 0;
	bossHidden_ = false;

	effects_ = new Effects();
	effects_->Initialize();

	bossModel_ = Model::CreateFromOBJ("Boss");
	bossTailModel_ = Model::CreateFromOBJ("BossTail");
	boss_ = new Boss();
	boss_->Initialize(bossModel_, bossTailModel_, &camera_, player_);
	previousBossHp_ = boss_->GetHp();
	bossFrameSpeed_ = 0.0f;
	snowBuildParticleTimer_ = 0.0f;
	cameraController_ = new CameraController();
	cameraController_->SetCamera(&camera_);
	cameraController_->Initialize();
	cameraController_->SetTarget(player_);
	cameraController_->targetOffset_ = {0.0f, 20.0f, -3.0f};
	cameraController_->SetRotation_({1.4835298641952f, 0.0f, 0.0f});
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
	// 撃破・死亡の演出中は記録に含めない。
	if (ending_ == Ending::kNone) {
		clearTime += deltaTime;
	}
	if (deltaTime <= 0.0f) {
		return;
	}
	if (UpdateHitStop(deltaTime)) {
		// 世界の時間だけ止める。揺れは実フレームで進め、粒の描画状態は保持する。
		effects_->Update(0.0f);
		cameraController_->UpdateShakeOnly(cameraController_->GetFollowPosition());
		return;
	}
	if (ending_ != Ending::kNone) {
		UpdateEnding(deltaTime);
		return;
	}
	player_->Update(deltaTime);
	UpdateLighting();
	// 雪玉を転がしている間、移動した雪玉の周囲に小さな雪粒を出す。
	snowBuildParticleTimer_ -= deltaTime;
	for (const Snow& snow : player_->GetSnows()) {
		if (!snow.IsHeld() || LengthSquared(snow.GetVelocity()) < 0.04f || snowBuildParticleTimer_ > 0.0f) {
			continue;
		}
		snowBuildParticleTimer_ = 0.06f;
		for (int index = 0; index < 3; ++index) {
			const Vector3 direction = Random::Direction();
			effects_->Spawn({
				snow.GetPos() + direction * snow.GetRadius(),
				direction * Random::Float(0.3f, 1.0f), {0.85f, 0.95f, 1.0f, 1.0f},
				0.018f, Random::Float(0.2f, 0.4f), 2.0f
			});
		}
		break;
	}

	snowPhysicsAccumulator_ += deltaTime;
	while (snowPhysicsAccumulator_ >= kSnowPhysicsStep) {
		player_->UpdateSnowPhysics(kSnowPhysicsStep, mapChipField_);
		snowPhysicsAccumulator_ -= kSnowPhysicsStep;
	}
	std::vector<Snow::EffectEvent> snowEffectEvents;
	player_->DrainSnowEffectEvents(snowEffectEvents);
	for (const Snow::EffectEvent& event : snowEffectEvents) {
		SpawnSnowEffect(*effects_, event);
		RequestSnowEffectFeedback(event);
	}
	if (hitStopRemainingSeconds_ > 0.0f) {
		// 衝突が起きたフレームから停止し、生成直後の粒を接点に表示する。
		effects_->Update(0.0f);
		cameraController_->UpdateShakeOnly(cameraController_->GetFollowPosition());
		return;
	}
	const Vector3 previousBossPosition = boss_->GetWorldTransform().translation_;
	boss_->Update(deltaTime);
	bossFrameSpeed_ = Length(boss_->GetWorldTransform().translation_ - previousBossPosition) / deltaTime;
	// 自分の尻尾に当たった場合は、切れた節に演出を出し、当たった節を被ダメージ位置にする。
	std::vector<Vector3> removedTailPositions;
	boss_->DrainRemovedTailPositions(removedTailPositions);
	SpawnTailCutEffects(removedTailPositions);
	ProcessBossDamage(removedTailPositions.empty()
		                  ? boss_->GetWorldTransform().translation_
		                  : removedTailPositions.front());
	if (boss_->IsDead()) {
		StartBossDefeat();
	}


	//当たり判定
	CheckAllCollisions();
	if (boss_->IsDead()) {
		StartBossDefeat();
	}
	if (player_->IsDead()) {
		StartPlayerDefeat();
	}
	effects_->Update(hitStopRemainingSeconds_ > 0.0f ? 0.0f : deltaTime);
	if (hitStopRemainingSeconds_ > 0.0f) {
		cameraController_->UpdateShakeOnly(cameraController_->GetFollowPosition());
	}
	else {
		cameraController_->Update();
	}
}

void GameScene::StartBossDefeat() {
	if (ending_ != Ending::kNone) {
		return;
	}
	ending_ = Ending::kBossDefeated;
	endingTimer_ = 0.0f;
	// 頭は今すぐ大きく爆ぜる。尻尾はヒットストップ明けに頭側から順に爆ぜる。
	const Vector3 headPosition = boss_->GetWorldTransform().translation_;
	defeatChain_.clear();
	for (std::size_t index = 0; index < boss_->GetTailCount(); ++index) {
		defeatChain_.push_back(boss_->GetTailPos(index));
	}
	defeatChainIndex_ = 0;

	boss_->RequestDamageFlash();
	SpawnExplosion(*effects_, headPosition, {1.0f, 0.5f, 0.25f, 1.0f}, 2.5f);
	SpawnRing(*effects_, headPosition, {1.0f, 1.0f, 1.0f, 1.0f}, 36, 14.0f, 0.1f, 0.5f, 5.0f);
	RequestFixedFeedback(bossFeedbackSettings_.defeatHitStopSeconds, bossFeedbackSettings_.defeatShakeAmplitude,
	                     bossFeedbackSettings_.defeatShakeDurationFrames);
	cameraController_->RequestZoomPunch(bossFeedbackSettings_.defeatZoomPunch, 0.9f);
}

void GameScene::StartPlayerDefeat() {
	if (ending_ != Ending::kNone) {
		return;
	}
	ending_ = Ending::kPlayerDefeated;
	endingTimer_ = 0.0f;
	// Playerの本体は Player::Draw で消え、代わりにその場で爆散する。
	const Vector3 playerPosition = player_->GetWorldTransform().translation_;
	SpawnExplosion(*effects_, playerPosition, {1.0f, 0.4f, 0.25f, 1.0f}, 2.0f);
	RequestFixedFeedback(bossFeedbackSettings_.defeatHitStopSeconds * 0.75f,
	                     bossFeedbackSettings_.defeatShakeAmplitude * 0.75f,
	                     bossFeedbackSettings_.defeatShakeDurationFrames);
	cameraController_->RequestZoomPunch(bossFeedbackSettings_.defeatZoomPunch * 0.7f, 0.9f);
}

void GameScene::UpdateEnding(float deltaTime) {
	endingTimer_ += deltaTime;
	float duration = bossFeedbackSettings_.playerDefeatSeconds;
	if (ending_ == Ending::kBossDefeated) {
		// ヒットストップが明けたら本体を消し、尻尾を頭側から順に爆ぜさせる。
		bossHidden_ = true;
		const float interval = bossFeedbackSettings_.defeatChainInterval;
		while (defeatChainIndex_ < defeatChain_.size() &&
		       endingTimer_ >= interval * static_cast<float>(defeatChainIndex_)) {
			SpawnExplosion(*effects_, defeatChain_[defeatChainIndex_], {1.0f, 0.65f, 0.3f, 1.0f}, 1.0f);
			cameraController_->RequestShake(0.3f, 8.0f);
			++defeatChainIndex_;
		}
		duration = bossFeedbackSettings_.bossDefeatSeconds + interval * static_cast<float>(defeatChain_.size());
	}
	effects_->Update(deltaTime);
	cameraController_->Update();
	if (endingTimer_ < duration) {
		return;
	}
	if (ending_ == Ending::kBossDefeated) {
		isClear_ = true;
	}
	else {
		finished_ = true;
	}
}

void GameScene::SpawnTailCutEffects(const std::vector<Vector3>& positions) {
	for (std::size_t index = 0; index < positions.size(); ++index) {
		// 当たった節（先頭）を大きく、後ろの節は小さく爆ぜさせる。
		const float scale = index == 0 ? 1.2f : 0.6f;
		SpawnExplosion(*effects_, positions[index], {1.0f, 0.7f, 0.4f, 1.0f}, scale);
	}
}

void GameScene::SetPlayerContactDamage(int headDamage, int tailDamage) {
	playerHeadContactDamage_ = (std::max)(0, headDamage);
	playerTailContactDamage_ = (std::max)(0, tailDamage);
}

void GameScene::SetBossFeedbackSettings(const BossFeedbackSettings& settings) {
	bossFeedbackSettings_ = settings;
	bossFeedbackSettings_.playerHitStopSeconds = (std::max)(0.0f, settings.playerHitStopSeconds);
	bossFeedbackSettings_.snowBreakHitStopSeconds = (std::max)(0.0f, settings.snowBreakHitStopSeconds);
	bossFeedbackSettings_.eatHitStopSeconds = (std::max)(0.0f, settings.eatHitStopSeconds);
	bossFeedbackSettings_.damageHitStopSeconds = (std::max)(0.0f, settings.damageHitStopSeconds);
	bossFeedbackSettings_.fullStrengthSpeed = (std::max)(0.001f, settings.fullStrengthSpeed);
	bossFeedbackSettings_.playerKnockbackSpeed = (std::max)(0.0f, settings.playerKnockbackSpeed);
	bossFeedbackSettings_.shakeAmplitude = (std::max)(0.0f, settings.shakeAmplitude);
	bossFeedbackSettings_.shakeDurationFrames = (std::max)(0.0f, settings.shakeDurationFrames);
	bossFeedbackSettings_.snowDamageMinSpeed = (std::max)(0.0f, settings.snowDamageMinSpeed);
	bossFeedbackSettings_.snowBodyDamage = (std::max)(0, settings.snowBodyDamage);
	bossFeedbackSettings_.snowBreakShakeAmplitude = (std::max)(0.0f, settings.snowBreakShakeAmplitude);
	bossFeedbackSettings_.eatShakeAmplitude = (std::max)(0.0f, settings.eatShakeAmplitude);
	bossFeedbackSettings_.damageShakeAmplitude = (std::max)(0.0f, settings.damageShakeAmplitude);
	bossFeedbackSettings_.strongShakeDurationFrames = (std::max)(0.0f, settings.strongShakeDurationFrames);
	bossFeedbackSettings_.damageZoomPunch = (std::max)(0.0f, settings.damageZoomPunch);
	bossFeedbackSettings_.defeatHitStopSeconds = (std::max)(0.0f, settings.defeatHitStopSeconds);
	bossFeedbackSettings_.defeatShakeAmplitude = (std::max)(0.0f, settings.defeatShakeAmplitude);
	bossFeedbackSettings_.defeatShakeDurationFrames = (std::max)(0.0f, settings.defeatShakeDurationFrames);
	bossFeedbackSettings_.defeatZoomPunch = (std::max)(0.0f, settings.defeatZoomPunch);
	bossFeedbackSettings_.bossDefeatSeconds = (std::max)(0.0f, settings.bossDefeatSeconds);
	bossFeedbackSettings_.playerDefeatSeconds = (std::max)(0.0f, settings.playerDefeatSeconds);
	bossFeedbackSettings_.defeatChainInterval = (std::max)(0.0f, settings.defeatChainInterval);
}

void GameScene::ProcessBossDamage(const Vector3& hitPosition) {
	if (boss_->GetHp() >= previousBossHp_) {
		return;
	}
	previousBossHp_ = boss_->GetHp();
	// 当たった位置の閃光、全方向への火花、地面に広がる衝撃波。
	SpawnFlash(*effects_, hitPosition, {1.0f, 1.0f, 1.0f, 1.0f}, 0.6f, 0.12f);
	SpawnBurst(*effects_, {
		.position = hitPosition, .color = {1.0f, 0.45f, 0.25f, 1.0f}, .count = 40,
		.minSpeed = 3.0f, .maxSpeed = 9.0f, .minSize = 0.06f, .maxSize = 0.12f,
		.minLife = 0.3f, .maxLife = 0.6f, .drag = 6.0f, .sphere = true
	});
	SpawnRing(*effects_, hitPosition, {1.0f, 0.6f, 0.3f, 1.0f}, 28, 9.0f, 0.08f, 0.35f, 7.0f);
	// ボス本体を白く光らせて一瞬膨らませる。ヒットストップ中もこの見た目で止まる。
	boss_->RequestDamageFlash();
	boss_->RequestPulse(0.3f, 0.2f);
	RequestFixedFeedback(bossFeedbackSettings_.damageHitStopSeconds, bossFeedbackSettings_.damageShakeAmplitude,
	                     bossFeedbackSettings_.strongShakeDurationFrames);
	cameraController_->RequestZoomPunch(bossFeedbackSettings_.damageZoomPunch, 0.85f);
}

void GameScene::RequestFixedFeedback(float stopSeconds, float shakeAmplitude, float shakeDurationFrames) {
	RequestHitStop(stopSeconds);
	cameraController_->RequestShake(shakeAmplitude, shakeDurationFrames);
}

void GameScene::RequestImpactFeedback(float stopSeconds, float shakeAmplitude,
                                      float impactSpeed, float fullStrengthSpeed) {
	const float strength = std::clamp(impactSpeed / (std::max)(fullStrengthSpeed, 0.001f), 0.0f, 1.0f);
	RequestHitStop(stopSeconds * strength);
	cameraController_->RequestShake(shakeAmplitude * strength, bossFeedbackSettings_.shakeDurationFrames);
}

void GameScene::SpawnBossEffect(const Vector3& position, const Vector3& normal, float radius,
                                const Vector4& color, int count, float speed) {
	effects_->Spawn({position, {}, color, (std::max)(0.05f, radius * 0.18f), 0.10f, 0.0f});
	for (int index = 0; index < count; ++index) {
		const Vector3 direction = ScatterAlongContact(normal, 0.55f);
		if (!effects_->Spawn({
			position + direction * 0.03f, direction * Random::Float(speed * 0.4f, speed),
			color, Random::Float(0.025f, 0.055f), Random::Float(0.2f, 0.45f), 3.0f
		})) {
			break;
		}
	}
}

void GameScene::SetSnowEffectFeedbackSettings(const SnowEffectFeedbackSettings& settings) {
	snowEffectFeedbackSettings_ = settings;
	snowEffectFeedbackSettings_.playerHitStopSeconds = (std::max)(0.0f, settings.playerHitStopSeconds);
	snowEffectFeedbackSettings_.snowHitStopSeconds = (std::max)(0.0f, settings.snowHitStopSeconds);
	snowEffectFeedbackSettings_.maxSizeHitStopSeconds = (std::max)(0.0f, settings.maxSizeHitStopSeconds);
	snowEffectFeedbackSettings_.playerShakeAmplitude = (std::max)(0.0f, settings.playerShakeAmplitude);
	snowEffectFeedbackSettings_.snowShakeAmplitude = (std::max)(0.0f, settings.snowShakeAmplitude);
	snowEffectFeedbackSettings_.maxSizeShakeAmplitude = (std::max)(0.0f, settings.maxSizeShakeAmplitude);
	snowEffectFeedbackSettings_.shakeDurationFrames = (std::max)(0.0f, settings.shakeDurationFrames);
	snowEffectFeedbackSettings_.fullStrengthImpactSpeed = (std::max)(0.001f, settings.fullStrengthImpactSpeed);
	snowEffectFeedbackSettings_.wallHitStopSeconds = (std::max)(0.0f, settings.wallHitStopSeconds);
	snowEffectFeedbackSettings_.wallImpactKickAmplitude = (std::max)(0.0f, settings.wallImpactKickAmplitude);
	snowEffectFeedbackSettings_.kickHitStopSeconds = (std::max)(0.0f, settings.kickHitStopSeconds);
	snowEffectFeedbackSettings_.kickShakeAmplitude = (std::max)(0.0f, settings.kickShakeAmplitude);
}

void GameScene::RequestHitStop(float durationSeconds) {
	hitStopRemainingSeconds_ = (std::max)(hitStopRemainingSeconds_, durationSeconds);
}

bool GameScene::UpdateHitStop(float deltaTime) {
	if (hitStopRemainingSeconds_ <= 0.0f) {
		return false;
	}
	hitStopRemainingSeconds_ = (std::max)(0.0f, hitStopRemainingSeconds_ - deltaTime);
	return true;
}

void GameScene::RequestSnowEffectFeedback(const Snow::EffectEvent& event) {
	float stopSeconds = 0.0f;
	float shakeAmplitude = 0.0f;
	float strength = 1.0f;
	// 衝突は速いほど強くする。最大サイズ・キック・生成は操作の結果なので一定にする。
	bool scaleBySpeed = false;
	switch (event.type) {
	case Snow::EffectType::kPlayerHit:
		//stopSeconds = snowEffectFeedbackSettings_.playerHitStopSeconds;
		//shakeAmplitude = snowEffectFeedbackSettings_.playerShakeAmplitude;
		scaleBySpeed = true;
		break;
	case Snow::EffectType::kSnowHit:
		stopSeconds = snowEffectFeedbackSettings_.snowHitStopSeconds;
		shakeAmplitude = snowEffectFeedbackSettings_.snowShakeAmplitude;
		scaleBySpeed = true;
		break;
	case Snow::EffectType::kMaxSize:
		stopSeconds = snowEffectFeedbackSettings_.maxSizeHitStopSeconds;
		shakeAmplitude = snowEffectFeedbackSettings_.maxSizeShakeAmplitude;
		break;
	case Snow::EffectType::kWallHit:
		// 揺れは速度から CameraController が決める。壁へ向かってカメラも少し押し込む。
		stopSeconds = snowEffectFeedbackSettings_.wallHitStopSeconds;
		scaleBySpeed = true;
		cameraController_->RequestMapImpactShake(event.impactSpeed);
		cameraController_->RequestImpactKick(
			event.normal,
			snowEffectFeedbackSettings_.wallImpactKickAmplitude *
			std::clamp(event.impactSpeed / snowEffectFeedbackSettings_.fullStrengthImpactSpeed, 0.0f, 1.0f),
			0.75f);
		break;
	case Snow::EffectType::kKick:
		stopSeconds = snowEffectFeedbackSettings_.kickHitStopSeconds;
		shakeAmplitude = snowEffectFeedbackSettings_.kickShakeAmplitude;
		break;
	case Snow::EffectType::kCreate:
		break;
	}
	if (scaleBySpeed) {
		strength = std::clamp(
			event.impactSpeed / snowEffectFeedbackSettings_.fullStrengthImpactSpeed, 0.0f, 1.0f);
	}
	RequestHitStop(stopSeconds * strength);
	cameraController_->RequestShake(shakeAmplitude * strength, snowEffectFeedbackSettings_.shakeDurationFrames);
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
	if (!bossHidden_) {
		boss_->Draw();
	}
	effects_->Draw(camera_);

	Model::PostDraw();
	DebugText::GetInstance()->Print(
		"HP " + std::to_string(player_->GetHp()) + "/" + std::to_string(player_->GetMaxHp()), 16.0f, 16.0f);
	DebugText::GetInstance()->Print(
		"BOSS " + std::to_string(boss_->GetHp()) + "/" + std::to_string(boss_->GetMaxHp()), 16.0f, 64.0f);
	DebugText::GetInstance()->Print("WASD MOVE  SPACE ROLL/RELEASE  E KICK", 16.0f, 40.0f);
	DebugText::GetInstance()->Print("Time" + std::to_string(clearTime), 16.0f, 100.0f);
	Sprite::PreDraw();
	DebugText::GetInstance()->DrawAll();
	Sprite::PostDraw();
}

void GameScene::CheckAllCollisions() {
	if (boss_->IsDead()) {
		return;
	}
	// 頭または胴体との接触でダメージを与える。連続接触はPlayer側の無敵時間で抑える。
	const Vector3 playerPosition = player_->GetWorldTransform().translation_;
	const float playerRadius = player_->GetCollisionRadius();
	const WorldTransform& bossTransform = boss_->GetWorldTransform();
	Vector3 contactPosition = bossTransform.translation_;
	int contactDamage = 0;
	if (IsCollisionBallToBall(playerPosition, playerRadius,
	                          contactPosition, boss_->GetHeadBodyRadius())) {
		contactDamage = playerHeadContactDamage_;
	}
	else {
		for (std::size_t index = 0; index < boss_->GetTailCount(); ++index) {
			if (IsCollisionBallToBall(playerPosition, playerRadius,
			                          boss_->GetTailPos(index), boss_->GetTailRadius())) {
				contactPosition = boss_->GetTailPos(index);
				contactDamage = playerTailContactDamage_;
				break;
			}
		}
	}
	if (player_->ApplyDamage(contactDamage)) {
		Vector3 knockback = playerPosition - contactPosition;
		knockback.y = 0.0f;
		if (LengthSquared(knockback) <= 0.000001f) {
			const float yaw = bossTransform.rotation_.y;
			knockback = {std::sin(yaw), 0.0f, std::cos(yaw)};
		}
		player_->ApplyKnockback(knockback, bossFeedbackSettings_.playerKnockbackSpeed);
		const float speed = (std::max)(bossFrameSpeed_, Length(player_->GetVelocity()));
		RequestImpactFeedback(bossFeedbackSettings_.playerHitStopSeconds,
		                      bossFeedbackSettings_.shakeAmplitude, speed,
		                      bossFeedbackSettings_.fullStrengthSpeed);
		SpawnBossEffect(playerPosition, Normalize(knockback), playerRadius,
		                {1.0f, 0.55f, 0.3f, 1.0f}, 16, 3.0f);
	}
	// ダメージの有無（無敵時間中を含む）に関わらず、頭・口・胴体の中へは入れない。
	// ダメージ判定の後に押し出すので、次のフレームに接触すればダメージ判定は働く。
	// 胴体の球は互いに重なっているため、一つから出した先が隣に入ることがある。数回繰り返して解消する。
	constexpr int kPushOutIterations = 3;
	for (int iteration = 0; iteration < kPushOutIterations; ++iteration) {
		bool pushed = player_->PushOutFromSphere(bossTransform.translation_, boss_->GetHeadBodyRadius());
		pushed = player_->PushOutFromSphere(boss_->GetMouthPos(), boss_->GetMouthRadius()) || pushed;
		for (std::size_t index = 0; index < boss_->GetTailCount(); ++index) {
			pushed = player_->PushOutFromSphere(boss_->GetTailPos(index), boss_->GetTailRadius()) || pushed;
		}
		if (!pushed) {
			break;
		}
	}

	// 口を先に判定する。口以外の本体や尻尾に触れた雪玉はサイズに関わらず壊す。
	auto& snows = player_->GetSnows();
	for (auto it = snows.begin(); it != snows.end();) {
		const Vector3 snowPosition = it->GetPos();
		const float snowRadius = it->GetRadius();
		const float impactSpeed = Length(it->GetVelocity());
		const Vector3 mouthPosition = boss_->GetMouthPos();
		const bool atMouth = IsCollisionBallToBall(snowPosition, snowRadius,
		                                           mouthPosition, boss_->GetMouthRadius()) != 0;
		if (atMouth && it->IsSizeMax() && boss_->TryEatSnow()) {
			// 雪玉が口へ吸い込まれ、口で光り、増えた尻尾の位置で粒が弾ける。
			SpawnImplode(*effects_, mouthPosition, {0.75f, 0.93f, 1.0f, 1.0f}, 28, snowRadius * 2.5f, 0.25f);
			SpawnFlash(*effects_, mouthPosition, {1.0f, 0.9f, 0.5f, 1.0f}, 0.45f, 0.12f);
			SpawnBossEffect(mouthPosition, Normalize(snowPosition - mouthPosition), snowRadius,
			                {1.0f, 0.85f, 0.45f, 1.0f}, 16, 3.5f);
			const Vector3 newTailPosition = boss_->GetTailPos(boss_->GetTailCount() - 1);
			SpawnBurst(*effects_, {
				.position = newTailPosition, .color = {1.0f, 0.85f, 0.45f, 1.0f}, .count = 18,
				.minSpeed = 2.0f, .maxSpeed = 5.0f, .minSize = 0.05f, .maxSize = 0.09f,
				.minLife = 0.3f, .maxLife = 0.5f, .drag = 6.0f, .sphere = true
			});
			boss_->RequestPulse(0.25f, 0.25f);
			// 食べたのはボスの動作なので、雪玉の速度に関わらず一定の強さで返す。
			RequestFixedFeedback(bossFeedbackSettings_.eatHitStopSeconds, bossFeedbackSettings_.eatShakeAmplitude,
			                     bossFeedbackSettings_.shakeDurationFrames);
			it = player_->RemoveSnow(it);
			continue;
		}
		bool bodyHit = IsCollisionBallToBall(snowPosition, snowRadius,
		                                     bossTransform.translation_, bossTransform.scale_.x) != 0;
		Vector3 bodyPosition = bossTransform.translation_;
		if (!bodyHit) {
			for (std::size_t index = 0; index < boss_->GetTailCount(); ++index) {
				if (IsCollisionBallToBall(snowPosition, snowRadius,
				                          boss_->GetTailPos(index), boss_->GetTailRadius())) {
					bodyHit = true;
					bodyPosition = boss_->GetTailPos(index);
					break;
				}
			}
		}
		if (!bodyHit || atMouth) {
			++it;
			continue;
		}
		const Vector3 normal = Normalize(snowPosition - bodyPosition);
		const Vector3 snowContactPosition = snowPosition - normal * snowRadius;
		// 大きい雪玉ほど、粒の数・大きさ・速さを増やして盛大に砕く。
		const float sizeRatio = std::clamp(snowRadius / (std::max)(it->GetMaxRadius(), 0.001f), 0.0f, 1.0f);
		SpawnFlash(*effects_, snowContactPosition, {1.0f, 1.0f, 1.0f, 1.0f}, 0.15f + 0.4f * sizeRatio, 0.1f);
		SpawnBossEffect(snowContactPosition, normal, snowRadius,
		                {1.0f, 0.65f, 0.3f, 1.0f}, 10 + static_cast<int>(20.0f * sizeRatio), 2.5f + 4.0f * sizeRatio);
		SpawnBurst(*effects_, {
			.position = snowPosition, .color = {0.75f, 0.93f, 1.0f, 1.0f},
			.count = 16 + static_cast<int>(40.0f * sizeRatio),
			.minSpeed = 2.0f, .maxSpeed = 4.0f + 7.0f * sizeRatio,
			.minSize = 0.04f + 0.04f * sizeRatio, .maxSize = 0.07f + 0.08f * sizeRatio,
			.minLife = 0.3f, .maxLife = 0.6f, .drag = 6.0f, .sphere = true
		});
		if (it->IsSizeMax()) {
			SpawnRing(*effects_, snowPosition, {0.9f, 0.97f, 1.0f, 1.0f}, 24, 9.0f, 0.07f, 0.35f, 6.0f);
		}
		if (impactSpeed >= bossFeedbackSettings_.snowDamageMinSpeed &&
			boss_->ApplyDamage(bossFeedbackSettings_.snowBodyDamage)) {
			ProcessBossDamage(snowContactPosition);
		}
		RequestImpactFeedback(bossFeedbackSettings_.snowBreakHitStopSeconds,
		                      bossFeedbackSettings_.snowBreakShakeAmplitude * (0.5f + 0.5f * sizeRatio), impactSpeed,
		                      bossFeedbackSettings_.fullStrengthSpeed);
		it = player_->RemoveSnow(it);
	}
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
	bossTailModel_->SetLightGroup(playerLightGroup_.get());
	playerLightGroup_->Update();
}
