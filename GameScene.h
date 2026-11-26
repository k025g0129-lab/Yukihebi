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
		// ステージ外周への衝突。揺れの強さは CameraController::MapImpactShakeSettings で決める。
		float wallHitStopSeconds = 0.04f;
		float wallImpactKickAmplitude = 0.15f;
		// キックは操作の手応えなので、速度に関わらず一定の強さで返す。
		float kickHitStopSeconds = 0.03f;
		float kickShakeAmplitude = 0.12f;
	};

	struct BossFeedbackSettings {
		float playerHitStopSeconds = 0.08f;
		float snowBreakHitStopSeconds = 0.045f;
		float eatHitStopSeconds = 0.08f;
		float damageHitStopSeconds = 0.15f;
		float fullStrengthSpeed = 8.0f;
		float playerKnockbackSpeed = 7.0f;
		float shakeAmplitude = 0.16f; // Playerがボスに当たったときの揺れ
		float shakeDurationFrames = 8.0f;
		float snowDamageMinSpeed = 3.0f;
		int snowBodyDamage = 1;
		// 食べる・被ダメージは結果のイベントなので、衝突速度に比例させず一定の強さにする。
		float snowBreakShakeAmplitude = 0.3f;
		float eatShakeAmplitude = 0.3f;
		float damageShakeAmplitude = 0.5f;
		float strongShakeDurationFrames = 14.0f;
		float damageZoomPunch = 1.5f;
		// 撃破・死亡の演出。演出が終わるまでシーンを切り替えない。
		float defeatHitStopSeconds = 0.4f;
		float defeatShakeAmplitude = 0.8f;
		float defeatShakeDurationFrames = 30.0f;
		float defeatZoomPunch = 3.0f;
		float bossDefeatSeconds = 2.0f;
		float playerDefeatSeconds = 1.5f;
		float defeatChainInterval = 0.05f; // 尻尾が順に爆ぜる間隔
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

	float GetClearTime() const {
		return clearTime;
	}

private:
	const Input* input = Input::GetInstance();
	void GenerateBlocks();
	void RequestHitStop(float durationSeconds);
	bool UpdateHitStop(float deltaTime);
	void RequestSnowEffectFeedback(const Snow::EffectEvent& event);
	void RequestImpactFeedback(float stopSeconds, float shakeAmplitude, float impactSpeed, float fullStrengthSpeed);
	// 速度に比例させず、指定した強さでヒットストップと揺れを出す。
	void RequestFixedFeedback(float stopSeconds, float shakeAmplitude, float shakeDurationFrames);
	void SpawnBossEffect(const Vector3& position, const Vector3& normal, float radius, const Vector4& color, int count,
	                     float speed);
	// HPが減っていれば、hitPosition に被ダメージの演出を出す。
	void ProcessBossDamage(const Vector3& hitPosition);
	void SpawnTailCutEffects(const std::vector<Vector3>& positions);
	void StartBossDefeat();
	void StartPlayerDefeat();
	// 撃破・死亡の演出中の更新。ゲームの進行は止め、粒とカメラだけ進める。
	void UpdateEnding(float deltaTime);

	enum class Ending { kNone, kBossDefeated, kPlayerDefeated };

	Ending ending_ = Ending::kNone;
	float endingTimer_ = 0.0f;
	std::vector<Vector3> defeatChain_; // 撃破時に順に爆ぜる尻尾の位置
	std::size_t defeatChainIndex_ = 0;
	bool bossHidden_ = false;
	std::unique_ptr<LightGroup> playerLightGroup_;
	float clearTime;
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
	// 雪玉の山で衝突が連鎖すると、ヒットストップが明けるたびに次が入って重く見える。
	// 雪玉同士のヒットストップは一度入れたらこの秒数は入れない。
	float snowHitStopCooldown_ = 0.0f;
	static constexpr float kSnowHitStopCooldown = 0.25f;
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
	bool firstPerson = false;
};
