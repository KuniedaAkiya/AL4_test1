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
		kSwing,
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
	// 現在のコンボ段数（0始まり）
	int32_t GetComboStep() const { return comboStep_; }
	// カウンター攻撃中かどうか
	bool IsCounterAttack() const { return behavior_ == Behavior::kAttack && isCounterAttack_; }
	// 今の攻撃のダメージ量（通常1、コンボ最終段2、カウンター3。ジャスガ後の強化は2倍）
	int32_t GetAttackDamage() const;
	// 1回の攻撃（1段）につき1ヒットだけにするための判定
	bool HasAttackHit() const { return attackHit_; }
	void MarkAttackHit() { attackHit_ = true; }
	void ClearAttackUp() { isAttackUp_ = false; }
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	// 剣の当たり判定が有効か（振っている間だけ true）
	bool IsSwordActive() const { return behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kSwing; }
	// 剣の当たり判定
	AABB GetSwordAABB() const;

private:
	// ==== 音の大きさ設定 ====
	static inline const float kSEVolume = 0.4f;

	enum class KnockbackPhase {
		kFly,
		kRecover,
	};
	KnockbackPhase knockbackPhase_ = KnockbackPhase::kFly;

	float guardTimer_ = 0.0f;
	bool isJustGuardSuccess_ = false;
	bool hasJustGuarded_ = false;
	GameScene* gameScene_ = nullptr;

	static inline const float kJustGuardWindowTime = 0.15f;
	static inline const float kJustGuardBonusInvincibleTime = 0.6f;
	static inline const float kJustGuardMpReward = 20.0f;
	bool isAttackUp_ = false;

	// ==== 剣 ====
	static inline const float kSwordLength = 1.6f;                      // 刀身の長さ（モデルの先端 Y=1.6）
	static inline const float kSwordThickness = 0.3f;                   // 判定の太さ
	static inline const Vector3 kSwordPivotOffset = {0.3f, 0.1f, 0.0f}; // 持ち手の位置（右向き基準）
	static inline const float kSwordRestDegrees = -30.0f;               // 構える前の角度

	// ==== コンボ攻撃 ====
	static inline const int32_t kMaxComboStep = 3;
	static inline const float kComboChargeTimes[kMaxComboStep] = {0.08f, 0.05f, 0.12f};
	static inline const float kComboSwingTimes[kMaxComboStep] = {0.12f, 0.12f, 0.2f};
	static inline const float kComboAfterglowTimes[kMaxComboStep] = {0.1f, 0.12f, 0.2f};
	static inline const float kComboLungeSpeeds[kMaxComboStep] = {0.08f, 0.08f, 0.15f}; // 踏み込み
	// 剣の角度（度）。0=真上、-90=前方、-180=真下。右向き基準で、左向きは自動で反転
	static inline const float kSwordStartDegrees[kMaxComboStep] = {40.0f, -140.0f, 60.0f}; // 袈裟斬り → 切り上げ → 大振り
	static inline const float kSwordEndDegrees[kMaxComboStep] = {-130.0f, 10.0f, -150.0f};
	static inline const int32_t kFinisherDamage = 2;
	int32_t comboStep_ = 0;
	bool comboInputBuffered_ = false; // 攻撃中に押された次段入力
	bool isComboContinue_ = false;    // 次段へ繋ぐ再突入かどうか
	bool attackHit_ = false;          // この1段の攻撃でヒット済みか

	// ==== カウンター攻撃 ====
	static inline const float kCounterWindowTime = 1.0f; // ジャスガ成功後に反撃できる時間
	static inline const float kCounterChargeTime = 0.08f;
	static inline const float kCounterSwingTime = 0.25f;
	static inline const float kCounterAfterglowTime = 0.15f;
	static inline const float kCounterLungeSpeed = 0.2f;
	static inline const float kCounterSwordStartDegree = 90.0f; // 背中側から
	static inline const float kCounterSwordEndDegree = -180.0f; // 一回転近く振り抜く
	static inline const int32_t kCounterDamage = 3;
	float counterTimer_ = 0.0f;
	bool isCounterRequested_ = false;
	bool isCounterAttack_ = false;

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
	uint32_t soundHandleDamage_ = 0;
	uint32_t soundHandleJustGuard_ = 0;
	uint32_t soundHandleJump_ = 0;

	void TakeDamage();
	void OnJustGuardSuccess();
	void RequestCounterAttack();
	void UpdateSwordTransform(float swordDegrees);
	// 敵・ボスとの接触時の共通処理
	void HandleEnemyContact();
	// 現在の向き（lrDirection_）へ即座に向き直る
	void FaceCurrentDirection();
	// ワールド行列の更新と転送
	void UpdateWorldMatrix();
};