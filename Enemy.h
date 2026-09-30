#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

// 前方宣言
class Player;
class GameScene;

class Enemy {
public:
	// 振る舞い
	enum class Behavior {
		kUnknown,
		kWalk,        // 待機（旧歩行状態：動かない）
		kDeath,       // デス演出
		kStagger,     // ジャストガードされた時の怯み
		kRespawnWait, // 復活待ち（非表示）
	};

	// デストラクタ
	~Enemy();

	// 初期化
	void Initialize(Model* model, Camera* camera, uint32_t textureHandle, const Vector3& position);

	// 更新
	void Update();

	// 描画
	void Draw();

	// 当たり判定で使用するマップチップを設定する関数
	void SetmapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// ワールド座標を取得
	Vector3 GetWorldPosition() const;

	// AABBを取得
	AABB GetAABB();

	// 衝突応答
	void OnCollision(const Player* player);

	// 当たり判定無効の無敵フラグ
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	// GameSceneのポインタを設定
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }

	// 練習用ダミー設定（互換性維持のため残しています）
	void SetTrainingDummy(bool isTrainingDummy) { (void)isTrainingDummy; }

	// 死亡フラグ
	bool isDead_ = false;

	// コリジョン無効判定
	bool isCollisionDisabled_ = false;

private:
	GameScene* gameScene_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	// 初期配置座標（復活時に戻る場所）
	Vector3 spawnPosition_{};

	// ワールドトランスフォーム
	WorldTransform worldTransform_;

	// モデル・カメラ・テクスチャ
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	uint32_t enemyTextureHandle_ = 0;

	// 速度（ノックバックなどで使用）
	Vector3 velocity_ = {};

	// 当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	// ===== 状態管理 =====
	Behavior behavior_ = Behavior::kWalk;
	Behavior behaviorRequest_ = Behavior::kUnknown;

	// 状態別の更新処理
	void BehaviorWalkUpdate();
	void BehaviorDeathUpdate();
	void BehaviorStaggerUpdate();
	void BehaviorRespawnWaitUpdate();

	// デス演出用タイマー
	float deathTimer_ = 0.0f;
	static inline const float kDeathTime = 0.5f;

	// 復活待ち用タイマー
	float respawnTimer_ = 0.0f;
	static inline const float kRespawnTime = 1.0f;

	// 怯み（ジャストガード時）演出用タイマー
	float staggerTimer_ = 0.0f;
	static inline const float kStaggerTime = 0.4f;
	static inline const float kStaggerAttenuation = 0.2f;
	static inline const float kStaggerKnockback = 0.15f;
};