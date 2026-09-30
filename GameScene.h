#pragma once
#include <list>
#include <vector>

#include "Boss.h"
#include "CameraController.h"
#include "DeathParticles.h"
#include "Fade.h"
#include "GuardEffect.h"
#include "HitEffect.h"
#include "IScene.h"
#include "MapChipField.h"
#include "PauseMenu.h"
#include "Player.h"
#include "Skydome.h"

using namespace KamataEngine;

// ゲームシーン
class GameScene : public IScene {
public:
	// フェーズ
	enum class Phase {
		kFadeIn, // フェードイン
		kPlay,   // ゲームプレイ
		kDeath,  // デス演出
		kClear,  // クリア演出
		kFadeOut // フェードアウト
	};

	// デストラクタ
	~GameScene() override;

	// 初期化
	void Initialize() override;

	// 更新
	void Update() override;

	// 描画
	void Draw() override;

	// ブロックの生成
	void GenerateBlocks();

	// 全ての当たり判定を行う
	void CheckAllCollisions();

	// フェーズの切り替え判定
	void ChangePhase();

	// 終了フラグのgetter
	bool IsFinished() const override { return finished_; }

	// 次に遷移するシーンの取得
	IScene::Next GetNextScene() const override { return nextScene_; }

	void CreateHitEffect(const Vector3& position);

	void CreateGuardEffect(const Vector3& position);

	// クリア条件を満たした時に外部（Bossなど）から呼び出す
	// 例: ボスを倒した瞬間に boss側から gameScene_->RequestGameClear() を呼ぶ
	void RequestGameClear() { isCleared_ = true; }

private:
	// ヒットエフェクトのリスト
	std::list<HitEffect*> hitEffects_;

	// ガードエフェクトのリスト
	std::list<GuardEffect*> guardEffects_;

	// フェーズに関わらず共通して行う更新処理（デバッグカメラ・ブロック更新など）
	void UpdateCommon();

	// デバッグカメラの切り替え・行列更新処理
	void UpdateDebugCamera();

	// ブロックのワールド行列更新処理
	void UpdateBlocks();

	// 終了フラグ
	bool finished_ = false;
	// 次に遷移するシーン（プレイ終了時に kGameOver / kGameClear のどちらかになる）
	IScene::Next nextScene_ = IScene::Next::kNone;
	// 現在のフェーズ
	Phase phase_;
	Fade* fade_ = nullptr;

	// クリア条件が満たされたかどうか（RequestGameClear()で立てる）
	bool isCleared_ = false;
	// クリア演出の経過時間
	float clearTimer_ = 0.0f;
	// クリア演出の待機時間＜秒＞
	static inline const float kClearWaitTime = 1.0f;

	// ===== ポーズ機能 =====
	bool isPaused_ = false;
	PauseMenu pauseMenu_;
	// ===== ポーズ操作ガイド（常時表示） =====
	Sprite* pauseGuideSprite_ = nullptr;
	uint32_t pauseGuideTextureHandle_ = 0;
	static inline const Vector2 kPauseGuidePosition = {1080.0f, 644.0f}; // 右下
	static inline const Vector2 kPauseGuideSize = {160.0f, 36.0f};

	// カメラ
	Camera camera_{};
	// カメラコントローラー
	CameraController* cameraController_ = nullptr;

	// デバッグカメラの有効フラグ
	bool isDebugCameraActive_ = false;
	// デバッグカメラ
	DebugCamera* debugCamera_ = nullptr;

	// インスタンス
	Player* player_ = nullptr;
	Boss* boss_ = nullptr;
	Skydome* skydome_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	DeathParticles* deathParticles_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0;
	uint32_t skydomeTextureHandle_ = 0;
	uint32_t blockTextureHandle_ = 0;
	uint32_t playerDataHandle_ = 0;
	uint32_t bossDataHandle_ = 0;
	uint32_t particleDataHandle_ = 0;
	uint32_t deathParticleDataHandle_ = 0;

	// --- BGMハンドル ---
	uint32_t bgmBossHandle_ = 0;
	uint32_t bgmPlayHandle_ = 0; // 再生中BGMの停止用

	// --- SEハンドル ---
	uint32_t seBossLandHandle_ = 0;
	uint32_t seFormChangeHandle_ = 0;
	uint32_t seParrySuccessHandle_ = 0;
	uint32_t sePlayerAttackHandle_ = 0;
	uint32_t sePlayerCounterHandle_ = 0;
	uint32_t sePlayerDamageHandle_ = 0;

	// 3Dモデル
	Model* modelSkydome_ = nullptr;
	Model* playerModel_ = nullptr;
	Model* blockModel_ = nullptr;
	Model* particleModel_ = nullptr;
	Model* deathParticleModel_ = nullptr;
	Model* modelAttack_ = nullptr;
	Model* bossModel_ = nullptr;

	// ワールドトランスフォーム
	WorldTransform worldTransform_{};
	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;
};