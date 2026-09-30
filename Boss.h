#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "MatrixFunction.h" // ★修正点1: AABB定義のためインクルードを追加

using namespace KamataEngine;

class Player;
class GameScene;

class Boss {
public:
	enum class Behavior {
		kUnknown,
		kIdle,
		kAttackDash,
		kAttackJump,
		kRevive,
		kDeath,
	};

	~Boss();

	void Initialize(Model* model, Camera* camera, uint32_t textureHandle, const Vector3& position);
	void Update();
	void Draw();
	void DrawUI();

	void SetTarget(Player* target) { target_ = target; }
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	void SetMovableArea(float minX, float maxX) {
		minX_ = minX;
		maxX_ = maxX;
	}
	void SetmapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	Vector3 GetWorldPosition() const;
	AABB GetAABB();
	bool IsDead() const { return isDead_; }
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	void OnCollision(const Player* player);

private:
	// ==== 音の大きさ設定 ====
	static inline const float kBGMVolume = 0.2f;
	static inline const float kSEVolume = 0.4f;

	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;

	WorldTransform worldTransform_;
	Vector3 velocity_{};

	Player* target_ = nullptr;
	GameScene* gameScene_ = nullptr;
	MapChipField* mapChipField_ = nullptr; // ★修正点2: 宣言漏れしていたメンバ変数を追加

	static inline const int32_t kMaxHP = 10;
	int32_t hp_ = kMaxHP;

	Behavior behavior_ = Behavior::kIdle;
	Behavior behaviorRequest_ = Behavior::kUnknown;
	float behaviorTimer_ = 0.0f;

	bool isDead_ = false;
	bool isCollisionDisabled_ = false;
	bool hasRevived_ = false;

	float damageCooldown_ = 0.0f;
	static inline const float kDamageCooldownTime = 0.2f;

	float minX_ = 1.5f;
	float maxX_ = 27.5f;

	float groundY_ = 0.0f;
	float dashDirection_ = 1.0f;
	float jumpTargetX_ = 0.0f;
	float deathTimer_ = 0.0f;

	static inline const float kModelScale = 2.0f;
	static inline const float kWidth = 2.0f;
	static inline const float kHeight = 2.0f;

	static inline const float kIdleTime = 2.0f;
	static inline const float kDashChargeTime = 0.5f;
	static inline const float kDashTime = 0.3f;
	static inline const float kDashSpeed = 0.6f;

	static inline const float kJumpChargeTime = 0.4f;
	static inline const float kJumpPower = 0.7f;
	static inline const float kGravity = 0.03f;

	static inline const float kReviveTime = 2.0f;
	static inline const float kDeathTime = 1.5f;

	Sprite* hpBarGaugeSprite_ = nullptr;
	Sprite* hpBarFrameSprite_ = nullptr;
	static inline const Vector2 kHpBarPosition = {400.0f, 40.0f};
	static inline const float kHpBarWidth = 480.0f;
	static inline const float kHpBarHeight = 24.0f;

	uint32_t soundHandleLand_ = 0;
	uint32_t soundHandleFormChange_ = 0;
	uint32_t soundHandleCounter_ = 0;
	uint32_t soundHandleAttack_ = 0;

	void ClampPosition();
	void TakeDamage(int32_t damage, bool isAttackUp);

	void BehaviorIdleInitialize();
	void BehaviorIdleUpdate();
	void BehaviorAttackDashInitialize();
	void BehaviorAttackDashUpdate();
	void BehaviorAttackJumpInitialize();
	void BehaviorAttackJumpUpdate();
	void BehaviorReviveInitialize();
	void BehaviorReviveUpdate();
	void BehaviorDeathInitialize();
	void BehaviorDeathUpdate();
};