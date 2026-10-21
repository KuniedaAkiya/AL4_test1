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
		kWalk,    // 待機/移動
		kDeath,   // デス演出
		kStagger, // ジャストガードされた時の怯み
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

	// プレイヤーのポインタを設定（画面内に入ったか判定用）
	void SetPlayer(Player* player) { player_ = player; }

	// 倒れて、消えたかどうか
	bool IsDead() const { return isDead_; }

	// 死亡フラグ
	bool isDead_ = false;

	// コリジョン無効判定
	bool isCollisionDisabled_ = false;

private:
	GameScene* gameScene_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	Player* player_ = nullptr; // プレイヤーへのポインタ

	// ワールドトランスフォーム
	WorldTransform worldTransform_;

	// モデル・カメラ・テクスチャ
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	uint32_t enemyTextureHandle_ = 0;

	// 速度
	Vector3 velocity_ = {};

	// 起動フラグ（画面内に入ったら true）
	bool isActivated_ = false;
	// 画面内に映ったと判定する距離（横幅）
	static inline const float kActivateDistance = 14.0f;

	// パトロール移動・重力設定
	float moveDirection_ = -1.0f;                 // 移動方向 (-1.0f: 左, 1.0f: 右)
	static inline const float kWalkSpeed = 0.03f; // 歩行速度
	static inline const float kGravity = 0.03f;   // 重力

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

	// ワールド行列を更新する
	void UpdateWorldTransform();

	// デス演出用タイマー
	float deathTimer_ = 0.0f;
	static inline const float kDeathTime = 0.5f;

	// 倒した時のヒットストップ（フレーム数）
	static inline const int32_t kHitStopFrames = 2;

	// 怯み（ジャストガード時）演出用タイマー
	float staggerTimer_ = 0.0f;
	static inline const float kStaggerTime = 0.4f;
	static inline const float kStaggerAttenuation = 0.2f;
	static inline const float kStaggerKnockback = 0.15f;
};