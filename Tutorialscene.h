#pragma once
#include <vector>

#include "CameraController.h"
#include "Enemy.h"
#include "Fade.h"
#include "IScene.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "PauseMenu.h"
#include "Player.h"
#include "Skydome.h"

using namespace KamataEngine;

/// <summary>
///  チュートリアルシーン
///  実際に自機を動かせる練習エリア。マップの序盤（Play/GameSceneと同じマップ）を流用し、
///  初期位置付近に練習用のダミーを置いて操作を試させる。
///  練習用ダミーを攻撃するとPlayシーン（ボス戦）へ遷移する。
///  BackSpaceキーでいつでもスキップしてPlayシーンへ進むこともできる。
/// </summary>
class TutorialScene : public IScene {
public:
	// シーンのフェーズ
	enum class Phase {
		kFadeIn,  // フェードイン
		kMain,    // メイン（練習エリア）
		kFadeOut, // フェードアウト
	};

	// デストラクタ
	~TutorialScene() override;

	// 初期化
	void Initialize() override;

	// 更新
	void Update() override;

	// 描画
	void Draw() override;

	// 終了フラグのgetter
	bool IsFinished() const override { return finished_; }

	// 次に遷移するシーンの取得
	IScene::Next GetNextScene() const override { return nextScene_; }

private:
	// ===== ポーズ機能 =====
	bool isPaused_ = false;
	PauseMenu pauseMenu_;
	// ===== ポーズ操作ガイド（常時表示） =====
	Sprite* pauseGuideSprite_ = nullptr;
	uint32_t pauseGuideTextureHandle_ = 0;
	static inline const Vector2 kPauseGuidePosition = {1080.0f, 644.0f}; // 右下
	static inline const Vector2 kPauseGuideSize = {160.0f, 36.0f};

	// ブロックの生成
	void GenerateBlocks();
	// ブロックのワールド行列更新処理
	void UpdateBlocks();
	// デバッグカメラの切り替え・行列更新処理
	void UpdateDebugCamera();

	// 終了フラグ
	bool finished_ = false;
	// 次に遷移するシーン
	IScene::Next nextScene_ = IScene::Next::kNone;
	// フェーズ
	Phase phase_ = Phase::kFadeIn;

	// フェード
	Fade* fade_ = nullptr;

	// カメラ
	Camera camera_{};
	CameraController* cameraController_ = nullptr;

	// デバッグカメラ
	bool isDebugCameraActive_ = false;
	DebugCamera* debugCamera_ = nullptr;

	// マップチップフィールド（Play/GameSceneと同じマップを流用）
	MapChipField* mapChipField_ = nullptr;
	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;

	// 自機
	Player* player_ = nullptr;
	// 練習用ダミー（Enemyをダミーモードで使用）
	Enemy* dummy_ = nullptr;
	// 天球
	Skydome* skydome_ = nullptr;

	// テクスチャハンドル
	uint32_t playerDataHandle_ = 0;
	uint32_t dummyDataHandle_ = 0;
	uint32_t blockTextureHandle_ = 0;

	// 音声ハンドル
	uint32_t soundHandleBGM_ = 0;
	uint32_t bgmPlayHandle_ = 0; // ★ 再生中BGM停止用のハンドル

	// 3Dモデル
	Model* playerModel_ = nullptr;
	Model* dummyModel_ = nullptr;
	Model* blockModel_ = nullptr;
	Model* modelSkydome_ = nullptr;
	Model* modelAttack_ = nullptr; // 自機の攻撃エフェクト用（Player::Initializeに必要）

	// スポーン・ダミー配置に使うマップチップの行インデックス
	static inline const uint32_t kRowIndex = 18;
	// ダミーを配置する列インデックス（初期位置のすぐ近く）
	static inline const uint32_t kDummyColumnIndex = 20;
};