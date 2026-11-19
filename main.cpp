#include <Windows.h>
#include <KamataEngine.h>

#include "TitleScene.h"
#include "TutorialScene.h"
#include "GameScene.h"
#include "ClearScene.h"

using namespace KamataEngine;

//タイトルシーンのインスタンス作成
TitleScene* titleScene = nullptr;
//チューとリアルのインスタンス生成
TutorialScene* tutorialScene = nullptr;
//ゲームシーンのインスタンス作成
GameScene* gameScene = nullptr;
//クリアシーンのインスタンス作成
ClearScene* clearScene = nullptr;

enum class Scene {

	kUnknown = 0,

	kTitle,
	kTuto,
	kGame,
	kClear,

};

//現在のシーン(型)
Scene scene = Scene::kUnknown;

void ChangeScene();
void UpdateScene();
void DrawScene();


// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"Top Down Player");
	{
		
		//DirectXCommonインスタンスの取得
		DirectXCommon* dxCommon = DirectXCommon::GetInstance();

		//最初のシーンの初期化
		scene = Scene::kTitle;
		titleScene = new TitleScene;
		titleScene->Initialize();

		//GameScene scene;
		//scene.Initialize();

		

		while (!KamataEngine::Update()) {

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

	//チュートリアルの解放
	delete tutorialScene;
	tutorialScene = nullptr;

	KamataEngine::Finalize();
	return 0;
}

//シーン切り替えの処理
void ChangeScene() 
{

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
		if (gameScene->IsFinished()){
		    
			delete gameScene;
			gameScene = nullptr;

			scene = Scene::kTitle;
			titleScene = new TitleScene();
			titleScene->Initialize();
		
		}

		//クリア時
		else if (gameScene->IsClear()) {
			
			delete gameScene;
			gameScene = nullptr;
			
			scene = Scene::kClear;
			clearScene = new ClearScene();
			clearScene->Initialize();

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

	}
}