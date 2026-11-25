#pragma once
#include <KamataEngine.h>
#include <chrono>
#include <memory>
#include <vector>
#include "MapChipField.h"
#include "Snow.h"
using namespace KamataEngine;

class Player;
class Boss;
class CameraController;
class Effects;

class GameScene {
public:
	struct SnowEffectFeedbackSettings {
		float playerHitStopSeconds = 0.035f;
		float snowHitStopSeconds = 0.045f;
		float maxSizeHitStopSeconds = 0.07f;
		float playerShakeAmplitude = 0.06f;
		float snowShakeAmplitude = 0.09f;
		float maxSizeShakeAmplitude = 0.12f;
		float shakeDurationFrames = 8.0f;
		float fullStrengthImpactSpeed = 6.0f;
	};
	struct BossFeedbackSettings {
		float playerHitStopSeconds = 0.08f;
		float snowBreakHitStopSeconds = 0.045f;
		float eatHitStopSeconds = 0.07f;
		float damageHitStopSeconds = 0.12f;
		float fullStrengthSpeed = 8.0f;
		float playerKnockbackSpeed = 7.0f;
		float shakeAmplitude = 0.16f;
		float shakeDurationFrames = 8.0f;
		float snowDamageMinSpeed = 3.0f;
		int snowBodyDamage = 1;
	};

	GameScene();
	~GameScene();
	void Initialize();
	void Update();
	void Draw();
	void SetSnowEffectFeedbackSettings(const SnowEffectFeedbackSettings& settings);
	void SetPlayerContactDamage(int headDamage, int tailDamage);
	void SetBossFeedbackSettings(const BossFeedbackSettings& settings);


	//デストラクタのgetter
	bool IsFinished() const { return finished_; }

	//チュートリアルのgetter
	bool IsTutorial() const { return tutorial_; }

	//クリアフラグのgetterを追加
	bool IsClear() const { return isClear_; }

	void CheckAllCollisions();

private:
	const Input* input = Input::GetInstance();
	void GenerateBlocks();
	void RequestHitStop(float durationSeconds);
	bool UpdateHitStop(float deltaTime);
	void RequestSnowEffectFeedback(const Snow::EffectEvent& event);
	void RequestImpactFeedback(float stopSeconds, float shakeAmplitude, float impactSpeed, float fullStrengthSpeed);
	void SpawnBossEffect(const Vector3& position, const Vector3& normal, float radius, const Vector4& color, int count, float speed);
	void ProcessBossDamage(float impactSpeed);
	std::unique_ptr<LightGroup> playerLightGroup_;

	MapChipField mapChipField_{};
	std::vector<std::vector<std::unique_ptr<WorldTransform>>> worldTransformBlocks_;
	std::unique_ptr<Model> modelBlock_;
	Model* playerModel_ = nullptr;
	Model* snowModel_ = nullptr;
	Player* player_ = nullptr;
	float timer = 0.0f;
	std::chrono::steady_clock::time_point previousTime_{};
	float snowPhysicsAccumulator_ = 0.0f;
	float hitStopRemainingSeconds_ = 0.0f;
	SnowEffectFeedbackSettings snowEffectFeedbackSettings_{};
	BossFeedbackSettings bossFeedbackSettings_{};
	int previousBossHp_ = 0;
	float bossFrameSpeed_ = 0.0f;
	float snowBuildParticleTimer_ = 0.0f;
	int playerHeadContactDamage_ = 1;
	int playerTailContactDamage_ = 1;
	static constexpr float kSnowPhysicsStep = 1.0f / 120.0f;
	float Timer = 0.0f;
	CameraController* cameraController_;
	Model* bossModel_ = nullptr;
	Model* bossTailModel_ = nullptr;
	Boss* boss_ = nullptr;

	//終了フラグ
	bool finished_ = false;

	//クリアフラグ
	bool isClear_ = false;

	//チュートリアルフラグ
	bool tutorial_ = false;

	Effects* effects_ = nullptr;
	Camera camera_;
	void InitializePlayerLighting();
	/// <summary>ステージ全体を1方向から照らす平行光源を設定する。</summary>
	void InitializeDirectionalLight();
	void UpdateLighting();
};
