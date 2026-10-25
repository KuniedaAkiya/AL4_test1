#pragma once
#include <algorithm>
#include <cstdint>
#include <list>
#include <vector>

#include "Boss.h"
#include "CameraController.h"
#include "Enemy.h"
#include "Fade.h"
#include "GuardEffect.h"
#include "IScene.h"
#include "MapChipField.h"
#include "Particle.h"
#include "PauseMenu.h"
#include "Player.h"
#include "Skydome.h"
#include "Stage.h"

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

	// 全ての当たり判定を行う
	void CheckAllCollisions();

	// フェーズの切り替え判定
	void ChangePhase();

	// 終了フラグのgetter
	bool IsFinished() const override { return finished_; }

	// 次に遷移するシーンの取得
	IScene::Next GetNextScene() const override { return nextScene_; }

	// 攻撃ヒット時のパーティクル生成
	void CreateHitEffect(const Vector3& position);

	// ガード時のエフェクト生成
	void CreateGuardEffect(const Vector3& position);

	// クリア条件を満たした時に外部（Bossなど）から呼び出す
	void RequestGameClear() { isCleared_ = true; }

	// カメラシェイクの種類
	using ShakeType = CameraController::ShakeType;

	// カメラシェイクを要求する
	void RequestCameraShake(ShakeType type) {
		if (cameraController_) {
			cameraController_->RequestShake(type);
		}
	}

	// ヒットストップを要求する
	void RequestHitStop(int32_t frames) { hitStopFrames_ = (std::max)(hitStopFrames_, frames); }

	// チェックポイントリセット
	static void ResetCheckpoint() { s_bossCheckpoint_ = false; }

	// スロー演出を要求する
	void RequestSlow(int32_t frames) { slowFrames_ = (std::max)(slowFrames_, frames); }

	// プレイヤーの斬撃パーティクル
	void CreateSlashParticles(const Vector3& position, bool isRightDirection);

	// 通常攻撃ヒットパーティクル
	void CreateHitParticles(const Vector3& position);

	// ボス被弾時のパーティクル（少しだけ飛び散る）
	void CreateBossHitParticles(const Vector3& position, Model* model, uint32_t textureHandle);

	// 【共通】スライム爆発パーティクル
	void CreateSlimeExplosion(const Vector3& position, Model* model, uint32_t textureHandle, float baseScale, int32_t particleCount);

	// ボスにすぐ行ける奴（デバッグ用）
	static void SetBossCheckpoint(bool flag = true) { s_bossCheckpoint_ = flag; }

private:
	// 敵のリスト
	std::list<Enemy*> enemies_;
	static inline const float kEnemySpawnYOffset = 0.1f;

	// ジャンプ台の打ち上げ強度
	static inline const float kJumpPadPower = 1.2f;

	// カメラを引く場所の設定
	struct CameraZoomZone {
		float minX;     // 反応するX範囲（左）
		float maxX;     // 反応するX範囲（右）
		float minY;     // 反応するY範囲（下）
		float maxY;     // 反応するY範囲（上）
		float distance; // 引いた時のカメラ距離（大きいほど広く映る）
		float speed;    // 引いていく速さ
		bool onJumpPad; // true: ジャンプ台に乗った時だけ（着地まで） / false: 範囲の中にいる間
	};
	static inline const float kAnyY = 10000.0f; // Yを限定しない時に使う
	static inline const CameraZoomZone kCameraZoomZones[] = {
	    // 1個目のジャンプ台の手前：近づいたら引き始める
	    {72.0f, 86.0f,  -kAnyY, kAnyY, 24.0f, 0.08f, false},
	    // 1個目のジャンプ台：乗ったら着地するまで引く
	    {80.0f, 90.0f,  -kAnyY, kAnyY, 24.0f, 0.08f, true },
	    // 上の足場（88〜116列）の上にいる間は、ずっと引く
	    {86.0f, 118.0f, 7.0f,   kAnyY, 24.0f, 0.08f, false},
	};

	// ボス部屋チェックポイント
	static inline bool s_bossCheckpoint_ = false;

	// 背景天球色
	static inline const Vector4 kNormalSkyColor = {1.0f, 1.0f, 1.0f, 1.0f};      // 道中
	static inline const Vector4 kBossFightSkyColor = {0.9f, 0.35f, 0.35f, 1.0f}; // ボス戦

	// 演出タイマー
	int32_t hitStopFrames_ = 0;
	int32_t slowFrames_ = 0;

	// 死亡演出待ち時間タイマー（DeathParticlesの代わり）
	float deathTimer_ = 0.0f;
	static inline const float kDeathWaitTime = 1.6f;

	// エフェクト・パーティクルリスト
	std::list<GuardEffect*> guardEffects_;
	std::list<Particle*> particles_; // 汎用パーティクルリスト

	// パーティクルを1つ生成してリストに追加する
	void AddParticle(const Particle::Parameter& param);
	// プレイヤーの位置に合わせて、カメラを引く要求を出す
	void UpdateCameraZoom(bool touchedJumpPad);

	// 共通更新
	void UpdateCommon();
	void UpdateDebugCamera();
	void UpdateBossRoom();

	bool isInBossRoom_ = false;
	bool isBossFightStarted_ = false;
	bool isBossIntro_ = false;

	bool finished_ = false;
	IScene::Next nextScene_ = IScene::Next::kNone;
	Phase phase_;
	Fade* fade_ = nullptr;

	bool isCleared_ = false;
	float clearTimer_ = 0.0f;
	static inline const float kClearWaitTime = 1.0f;

	// ポーズ機能
	bool isPaused_ = false;
	PauseMenu pauseMenu_;
	Sprite* pauseGuideSprite_ = nullptr;
	uint32_t pauseGuideTextureHandle_ = 0;
	static inline const Vector2 kPauseGuidePosition = {1080.0f, 644.0f};
	static inline const Vector2 kPauseGuideSize = {160.0f, 36.0f};

	// カメラ
	Camera camera_{};
	CameraController* cameraController_ = nullptr;
	bool isDebugCameraActive_ = false;
	DebugCamera* debugCamera_ = nullptr;

	// インスタンス
	Player* player_ = nullptr;
	Boss* boss_ = nullptr;
	Skydome* skydome_ = nullptr;
	Stage* stage_ = nullptr;

	// ハンドル
	uint32_t playerDataHandle_ = 0;
	uint32_t bossDataHandle_ = 0;
	uint32_t bgmBossHandle_ = 0;
	uint32_t bgmPlayHandle_ = 0;

	// モデル
	Model* modelSkydome_ = nullptr;
	Model* playerModel_ = nullptr;
	Model* particleModel_ = nullptr;
	Model* modelAttack_ = nullptr;
	Model* bossModel_ = nullptr;
};