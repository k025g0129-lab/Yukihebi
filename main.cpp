#include <Windows.h>
#include "KamataEngine.h"

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	
	KamataEngine::Initialize(L"雪蛇");

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ImGuiManager* imGuiManager = ImGuiManager::GetInstance();

	while (true) {
	
		if (KamataEngine::Update()) {
			break;
		}
	

		imGuiManager->Begin();
		
		
		imGuiManager->End();
	

		
		// ここから描画
		dxCommon->PreDraw();

		

		AxisIndicator::GetInstance()->Draw();

		imGuiManager->Draw();

		dxCommon->PostDraw();
	}

	return 0;
}
