#include <KamataEngine.h>
#include <Windows.h>
#include <cmath>

#include "ClearScene.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "Ranking.h"
#include "TitleScene.h"
#include "TutorialScene.h"
using namespace KamataEngine;

//タイトルシーンのインスタンス作成
TitleScene* titleScene = nullptr;
//チューとリアルのインスタンス生成
TutorialScene* tutorialScene = nullptr;
//ゲームシーンのインスタンス作成
GameScene* gameScene = nullptr;
//クリアシーンのインスタンス作成
ClearScene* clearScene = nullptr;
GameOverScene* gameOverScene = nullptr;

enum class Scene {
	kUnknown = 0,

	kTitle,
	kTuto,
	kGame,
	kClear,
	kGameOver,
};

Ranking ranking_{};
// ランキングの送り先。ゴルフのゲーム（"golf"）と記録が混ざらないよう、このゲーム用のIDにする。
// 値はクリア時間のミリ秒。
constexpr const char* kRankingGameId = "snowball";
constexpr const char* kRankingStageId = "boss_clear_ms";
//現在のシーン(型)
auto scene = Scene::kUnknown;

void ChangeScene();
void UpdateScene();
void DrawScene();

float ClearTime_;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	Initialize(L"Top Down Player");
	DebugText::GetInstance()->Initialize();
	{
		//DirectXCommonインスタンスの取得
		DirectXCommon* dxCommon = DirectXCommon::GetInstance();

		//最初のシーンの初期化
		scene = Scene::kTitle;
		titleScene = new TitleScene;
		titleScene->Initialize();
		ranking_.Initialize("https://ranking-api.k025g0019.workers.dev", kRankingGameId);
		// 起動時に先読みし、クリア画面に入る前から上位記録を表示できるようにする。
		ranking_.Fetch(kRankingStageId);
		//GameScene scene;
		//scene.Initialize();


		while (!Update()) {
			// 通信の完了はどのシーンでも受け取る。ゲーム中だけ呼ぶと、
			// クリア画面へ移った後に送信結果と再取得が処理されない。
			ranking_.Update();
			// titleScene->Update();
			// シーン切り替え
			ChangeScene();
			// 現在シーン更新
			UpdateScene();

			//描画処理
			dxCommon->PreDraw();

			// titleScene->Draw();
			// 現在シーンの描画
			DrawScene();

			//描画終了
			dxCommon->PostDraw();

			//scene.Update();
			//KamataEngine::DirectXCommon::GetInstance()->PreDraw();
			//scene.Draw();
			//KamataEngine::DirectXCommon::GetInstance()->PostDraw();
		}
	}

	//タイトルシーンの解放
	delete titleScene;
	titleScene = nullptr;

	//ゲームシーンの解放
	delete gameScene;
	gameScene = nullptr;

	//クリアシーンの解放
	delete clearScene;
	clearScene = nullptr;
	delete gameOverScene;
	gameOverScene = nullptr;

	//チュートリアルの解放
	delete tutorialScene;
	tutorialScene = nullptr;

	Finalize();
	return 0;
}

//シーン切り替えの処理
void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
			//シーン変更
			scene = Scene::kTuto;

			//旧シーンの開放
			delete titleScene;
			titleScene = nullptr;

			//新シーンの生成と初期化
			tutorialScene = new TutorialScene;
			tutorialScene->Initialize();
		}
		break;

	case Scene::kTuto:
		if (tutorialScene->IsTutorial()) {
			//シーン変更
			scene = Scene::kGame;

			//旧シーンの解放
			delete tutorialScene;
			tutorialScene = nullptr;

			//新シーンの生成と初期化
			gameScene = new GameScene;
			gameScene->Initialize();
		}

		break;

	case Scene::kGame:
		//ゲームオーバー時
		if (gameScene->IsFinished()) {
			delete gameScene;
			gameScene = nullptr;

			scene = Scene::kGameOver;
			gameOverScene = new GameOverScene();
			gameOverScene->Initialize();
		}

		//クリア時
		else if (gameScene->IsClear()) {
			scene = Scene::kClear;
			clearScene = new ClearScene();
			ClearTime_ = gameScene->GetClearTime();
			// サーバーは整数しか受け付けないので、ミリ秒に直して送る。少ないほど上位。
			const int clearMilliseconds = static_cast<int>(std::lround(ClearTime_ * 1000.0f));
			clearScene->Initialize(ClearTime_, &ranking_, clearMilliseconds);
			ranking_.Submit(kRankingStageId, clearMilliseconds);

			delete gameScene;
			gameScene = nullptr;
		}

		break;

	case Scene::kClear:
		if (clearScene->IsClear()) {
			delete clearScene;
			clearScene = nullptr;

			scene = Scene::kTitle;
			titleScene = new TitleScene();
			titleScene->Initialize();
		}

		break;

	case Scene::kGameOver:
		if (gameOverScene->IsRetryRequested() || gameOverScene->IsTitleRequested()) {
			const bool retry = gameOverScene->IsRetryRequested();
			delete gameOverScene;
			gameOverScene = nullptr;
			if (retry) {
				scene = Scene::kGame;
				gameScene = new GameScene();
				gameScene->Initialize();
			}
			else {
				scene = Scene::kTitle;
				titleScene = new TitleScene();
				titleScene->Initialize();
			}
		}
		break;
	}
}

void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;

	case Scene::kTuto:
		tutorialScene->Update();
		break;

	case Scene::kGame:
		gameScene->Update();
		break;

	case Scene::kClear:
		clearScene->Update();
		break;
	case Scene::kGameOver:
		gameOverScene->Update();
		break;
	}
}

void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;

	case Scene::kTuto:
		tutorialScene->Draw();
		break;

	case Scene::kGame:
		gameScene->Draw();
		break;

	case Scene::kClear:
		clearScene->Draw();
		break;
	case Scene::kGameOver:
		gameOverScene->Draw();
		break;
	}
}
