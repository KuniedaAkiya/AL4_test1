#pragma once
#include "KamataEngine.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

class Enemy;
class ShieldEnemy;
class Boss;
class MapChipField;
class GameScene;

class Player {
public:
	enum class LRDirection {
		kRight,
		kLeft,
	};

	enum class Behavior {
		kUnknown,
		kRoot,
		kAttack,
		kKnockback,
		kGuard,
	};

	enum class AttackPhase {
		kCharge,
		kDash,
		kAfterglow,
	};

	struct CollisionMapInfo {
		bool isHitTop = false;
		bool isHitGround = false;
		bool isHitWall = false;
		Vector3 move{};
	};

	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorners };

	LRDirection lrDirection_ = LRDirection::kRight;

	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Model* modelAttack, KamataEngine::Camera* camera, uint32_t playerTextureHandle, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	WorldTransform& GetWorldTransform() { return worldTransform_; }
	Vector3& GetVelocity() { return velocity_; }
	Vector3 GetWorldPosition() const;
	LRDirection GetLRDirection() const { return lrDirection_; }
	AABB GetAABB();
	bool isDead() const { return isDead_; }

	void SetmapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	void OnCollision(const Enemy* enemy);
	void OnCollision(const ShieldEnemy* shieldEnemy);
	void OnCollision(const Boss* boss);

	Vector3 CornerPosition(const Vector3& center, Corner corner);
	void inputMove();
	void CheckMapCollision(CollisionMapInfo& info);
	void TopCollision(CollisionMapInfo& info);
	void BottomCollision(CollisionMapInfo& info);
	void LeftCollision(CollisionMapInfo& info);
	void RightCollision(CollisionMapInfo& info);
	void PositionUpDate(const CollisionMapInfo& info);
	void CeilingCollision(const CollisionMapInfo& info);
	void WallCollision(const CollisionMapInfo& info);
	void ChangeGroundingState(const CollisionMapInfo& info);

	void BehaviorRootInitialize();
	void BehaviorRootUpData();
	void BehaviorAttackInitialize();
	void BehaviorAttackUpData();
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpData();
	void BehaviorGuardInitialize();
	void BehaviorGuardUpdate();

	bool IsAttack() const;
	void RequestKnockback();
	int32_t GetHP() const { return hp_; }
	int32_t GetMaxHP() const { return kMaxHP; }
	void DrawUI();
	float GetMP() const { return mp_; }
	float GetMaxMP() const { return kMaxMP; }
	bool IsGuard() const { return behavior_ == Behavior::kGuard; }
	bool IsJustGuardActive() const;
	bool WasJustGuardSuccessful() const { return isJustGuardSuccess_; }
	bool IsAttackUp() const { return isAttackUp_; }
	void ClearAttackUp() { isAttackUp_ = false; }
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }

private:
	// ==== 音の大きさ設定 ====
	static inline const float kBGMVolume = 0.2f;
	static inline const float kSEVolume = 0.4f;

	enum class KnockbackPhase {
		kFly,
		kRecover,
	};
	KnockbackPhase knockbackPhase_ = KnockbackPhase::kFly;

	float guardTimer_ = 0.0f;
	bool isJustGuardSuccess_ = false;
	GameScene* gameScene_ = nullptr;

	static inline const float kJustGuardWindowTime = 0.15f;
	static inline const float kJustGuardBonusInvincibleTime = 0.6f;
	static inline const float kJustGuardMpReward = 20.0f;
	bool isAttackUp_ = false;

	bool isKnockbackRequested_ = false;
	float knockbackTimer_ = 0.0f;

	MapChipField* mapChipField_ = nullptr;
	Model* playerModel_ = nullptr;
	Camera* camera_ = nullptr;
	uint32_t playerTextureHandle_ = 0;

	WorldTransform worldTransform_;
	KamataEngine::Model* modelAttack_ = nullptr;
	KamataEngine::WorldTransform worldTransformAttack_;

	Vector3 velocity_{};
	Behavior behavior_ = Behavior::kRoot;
	Behavior behaviorRequest_ = Behavior::kUnknown;
	AttackPhase attackPhase_ = AttackPhase::kCharge;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	bool onGround_ = true;
	bool isDead_ = false;

	static inline const int32_t kMaxHP = 3;
	int32_t hp_ = kMaxHP;

	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;
	static inline const float kInvincibleTime = 1.0f;

	Sprite* hpSprites_[kMaxHP] = {nullptr, nullptr, nullptr};
	static inline const Vector2 kHpIconPosition = {40.0f, 40.0f};
	static inline const float kHpIconSize = 32.0f;
	static inline const float kHpIconSpacing = 40.0f;

	static inline const float kAcceleration = 0.05f;
	static inline const float kAttenuation = 0.5f;
	static inline const float kLimitRunSpeed = 0.2f;
	static inline const float kTimeTurn = 0.3f;

	static inline const float kGravityAcceleration = 0.07f;
	static inline const float kLimitFallSpeed = 0.5f;
	static inline const float kJumpAcceleration = 0.8f;

	float attackParameter_ = 0.0f;
	static inline const float kChargeTime = 0.1f;
	static inline const float kDashTime = 0.2f;
	static inline const float kAfterglowTime = 0.1f;
	static inline const float kDashSpeed = 0.5f;

	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;
	static inline const float kBlank = 0.01f;

	static inline const float kAttenuationLanding = 0.01f;
	static inline const float kAttenuationWall = 0.01f;

	static inline const float kKnockbackSpeed = 0.6f;
	static inline const float kKnockbackAttenuation = 0.2f;
	static inline const float kKnockbackFlyTime = 0.2f;
	static inline const float kKnockbackRecoverTime = 0.3f;

	static inline const float kMaxMP = 100.0f;
	float mp_ = kMaxMP;
	static inline const float kMPDrainPerSecond = 30.0f;
	static inline const float kMPRegenPerSecond = 15.0f;

	Sprite* mpBarGaugeSprite_ = nullptr;
	Sprite* mpBarFrameSprite_ = nullptr;
	static inline const Vector2 kMpBarPosition = {40.0f, 90.0f};
	static inline const float kMpBarWidth = 160.0f;
	static inline const float kMpBarHeight = 16.0f;

	float coyoteTimer_ = 0.0f;
	static inline const float kCoyoteTime = 0.1f;

	uint32_t soundHandleCounter_ = 0;
	uint32_t soundHandleAttack_ = 0;
	uint32_t soundHandleDamage_ = 0;
	uint32_t soundHandleJustGuard_ = 0;
	uint32_t soundHandleJump_ = 0;

	void TakeDamage();
	void OnJustGuardSuccess();
};