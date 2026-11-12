#include <Windows.h>
#include <KamataEngine.h>

#include "GameScene.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"Top Down Player");
	{
		GameScene scene;
		scene.Initialize();

		while (!KamataEngine::Update()) {
			scene.Update();
			KamataEngine::DirectXCommon::GetInstance()->PreDraw();
			scene.Draw();
			KamataEngine::DirectXCommon::GetInstance()->PostDraw();
		}
	}
	KamataEngine::Finalize();
	return 0;
}
