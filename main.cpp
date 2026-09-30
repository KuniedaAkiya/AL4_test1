#include "GameClearScene.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "IScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"
#include "TutorialScene.h"
#include <Windows.h>

using namespace KamataEngine;

// 現在のシーン（共通インターフェースの型で1つだけ持つ）
IScene* currentScene = nullptr;

// 指定した種類のシーンを生成する
IScene* CreateScene(IScene::Next next);

// シーン切り替え処理
void ChangeScene();
// シーンの更新処理
void UpdateScene();
// シーンの描画処理
void DrawScene();

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// エンジンの初期化
	KamataEngine::Initialize(L"LE2C_08_クニエダ_アキヤ_AL3");

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// タイトルシーンから開始
	currentScene = CreateScene(IScene::Next::kTitle);
	currentScene->Initialize();

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// シーンの切り替え処理
		ChangeScene();
		// 現在シーンの更新
		UpdateScene();

		// 描画開始
		dxCommon->PreDraw();

		// 現在シーンの描画
		DrawScene();

		// 描画終了
		dxCommon->PostDraw();
	}

	// シーンの解放
	delete currentScene;
	currentScene = nullptr;

	// エンジンの終了処理
	KamataEngine::Finalize();
	return 0;
}

// 指定した種類のシーンを生成する
// （シーンの種類が増えた時はここにcaseを1つ足すだけでよい）
IScene* CreateScene(IScene::Next next) {
	switch (next) {
	case IScene::Next::kTitle:
		return new TitleScene();
	case IScene::Next::kTutorial:
		return new TutorialScene();
	case IScene::Next::kPlay:
		return new GameScene();
	case IScene::Next::kGameOver:
		return new GameOverScene();
	case IScene::Next::kGameClear:
		return new GameClearScene();
	case IScene::Next::kNone:
	default:
		return nullptr;
	}
}

// シーン切り替え処理
void ChangeScene() {
	if (!currentScene || !currentScene->IsFinished()) {
		return;
	}

	// 次のシーンの種類を取得
	IScene::Next next = currentScene->GetNextScene();

	// 旧シーンの解放
	delete currentScene;
	currentScene = nullptr;

	// 新シーンの生成と初期化
	currentScene = CreateScene(next);
	if (currentScene) {
		currentScene->Initialize();
	}
}

// シーンの更新処理
void UpdateScene() {
	if (currentScene) {
		currentScene->Update();
	}
}

// シーンの描画処理
void DrawScene() {
	if (currentScene) {
		currentScene->Draw();
	}
}