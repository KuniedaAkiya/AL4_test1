#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

class Enemy;
class ShieldEnemy;
class Boss;
class GameScene;
class CameraController;

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
	void InputMove();
	void CheckMapCollision(CollisionMapInfo& info);
	void TopCollision(CollisionMapInfo& info);
	void BottomCollision(CollisionMapInfo& info);
	void LeftCollision(CollisionMapInfo& info);
	void RightCollision(CollisionMapInfo& info);
	void PositionUpdate(const CollisionMapInfo& info);
	void CeilingCollision(const CollisionMapInfo& info);
	void WallCollision(const CollisionMapInfo& info);
	void ChangeGroundingState(const CollisionMapInfo& info);

	void BehaviorRootInitialize();
	void BehaviorRootUpdate();
	void BehaviorAttackInitialize();
	void BehaviorAttackUpdate();
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();
	void BehaviorGuardInitialize();
	void BehaviorGuardUpdate();

	// ガード中に敵・ボスが当たったとき、少し後ろにのけぞる
	void OnGuardHit(const AABB& other);
	bool IsAttack() const { return behavior_ == Behavior::kAttack; }
	void RequestKnockback() { isKnockbackRequested_ = true; }
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
	void MarkAttackHit();
	void ClearAttackUp() { isAttackUp_ = false; }
	// 無敵を付ける（今の残り時間より長い時だけ上書き）
	void GrantInvincible(float seconds);
	// 棘（ダメージ床）に触れた時の処理。無敵中は何もしない。ガードでは防げない
	void OnHazardHit();
	// 上向きに打ち上げる（ジャンプ台）。power は上向きの初速
	void Launch(float power);
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	// 剣の当たり判定が有効か（振っている間だけ true）
	bool IsSwordActive() const { return behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kSwing; }
	// 剣の当たり判定
	AABB GetSwordAABB() const;
	// 攻撃命中時の反動
	void ApplyRecoil(float distance);

	// TPチップに触れたかどうか（確認と同時に、要求を消す）
	bool ConsumeTeleportRequest() {
		const bool requested = isTeleportRequested_;
		isTeleportRequested_ = false;
		return requested;
	}

	// カメラシェイク
	void SetCameraController(CameraController* cameraController) { cameraController_ = cameraController; }

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

	static inline const float kFallDeathY = -2.0f; // 落下即死となるY座標の限界値

	static inline const float kJustGuardWindowTime = 0.15f;
	static inline const float kJustGuardBonusInvincibleTime = 0.6f;
	static inline const float kJustGuardMpReward = 20.0f;
	bool isAttackUp_ = false;

	// ==== 剣 ====
	static inline const float kSwordScale = 1.5f;                       // 剣の大きさ倍率（追加）
	static inline const float kSwordLength = 1.6f * kSwordScale;        // 刀身の長さ（変更）
	static inline const float kSwordThickness = 0.3f * kSwordScale;     // 判定の太さ（変更）
	static inline const Vector3 kSwordPivotOffset = {0.4f, 0.1f, 0.0f}; // 持ち手の位置（0.3f→0.4f）
	static inline const float kSwordRestDegrees = -30.0f;
	// ==== コンボ攻撃 ====
	static inline const int32_t kMaxComboStep = 3;
	static inline const float kComboChargeTimes[kMaxComboStep] = {0.08f, 0.05f, 0.12f};
	static inline const float kComboSwingTimes[kMaxComboStep] = {0.16f, 0.16f, 0.25f};
	static inline const float kComboAfterglowTimes[kMaxComboStep] = {0.12f, 0.14f, 0.22f};
	static inline const float kComboLungeSpeeds[kMaxComboStep] = {0.18f, 0.18f, 0.3f}; // 踏み込み
	// 剣の角度（度）。0=真上、-90=前方、-180=真下。右向き基準で、左向きは自動で反転
	static inline const float kSwordStartDegrees[kMaxComboStep] = {40.0f, -140.0f, 60.0f}; // 袈裟斬り → 切り上げ → 大振り
	static inline const float kSwordEndDegrees[kMaxComboStep] = {-130.0f, 10.0f, -150.0f};
	static inline const int32_t kFinisherDamage = 2;
	int32_t comboStep_ = 0;
	bool comboInputBuffered_ = false; // 攻撃中に押された次段入力
	bool isComboContinue_ = false;    // 次段へ繋ぐ再突入かどうか
	bool attackHit_ = false;          // この1段の攻撃でヒット済みか

	// 攻撃が当たった時に付ける無敵時間（秒）
	static inline const float kHitInvincibleTime = 0.3f;
	// ==== カウンター攻撃 ====
	static inline const float kCounterWindowTime = 1.0f; // ジャスガ成功後に反撃できる時間
	static inline const float kCounterChargeTime = 0.08f;
	static inline const float kCounterAfterglowTime = 0.15f;
	static inline const float kCounterSwingTime = 0.2f;
	static inline const float kCounterLungeSpeed = 0.45f;       // 突進の速さ（敵を貫通する）
	static inline const float kCounterSwordDegree = -90.0f;     // 突き：剣を正面に向けたまま
	static inline const float kCounterPullBack = 0.3f;          // 突く前に剣を引く距離
	static inline const float kCounterReach = 0.8f;             // 突き出した時の剣の前進量
	static inline const float kCounterSwordYOffset = -0.2f;     // 突きの時だけ剣を下げる量（マイナスで下がる）
	static inline const float kCounterEndInvincibleTime = 0.5f; // 突き終わり後の無敵時間
	static inline const int32_t kCounterDamage = 3;
	float counterTimer_ = 0.0f;
	bool isCounterRequested_ = false;
	bool isCounterAttack_ = false;

	bool isKnockbackRequested_ = false;
	float knockbackTimer_ = 0.0f;

	// カメラシェイク
	CameraController* cameraController_ = nullptr;

	MapChipField* mapChipField_ = nullptr;
	Model* playerModel_ = nullptr;
	Camera* camera_ = nullptr;
	uint32_t playerTextureHandle_ = 0;

	WorldTransform worldTransform_;
	KamataEngine::Model* modelAttack_ = nullptr;
	KamataEngine::WorldTransform worldTransformAttack_;

	// ==== ガード中の白点滅 ====
	ObjectColor objectColor_;
	static inline const float kGuardFlashSpeed = 15.0f;     // 点滅の速さ
	static inline const float kGuardFlashBrightness = 5.0f; // 白く見せるための乗算値

	static inline const float kGuardKnockbackDistance = 1.0f; // ガードで後ろに下がる距離
	static inline const float kGuardEndInvincibleTime = 0.5f; // ガード解除後に付く無敵時間

	Vector3 velocity_{};
	Behavior behavior_ = Behavior::kRoot;
	Behavior behaviorRequest_ = Behavior::kUnknown;
	AttackPhase attackPhase_ = AttackPhase::kCharge;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	bool onGround_ = true;
	bool isDead_ = false;

	bool isTeleportRequested_ = false;

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
	static inline const float kAirAcceleration = 0.005f;

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

	// 当たり判定をしながら横に動く
	void MoveHorizontal(float dx);
	// velocity_ どおりに動いて、マップとの当たり判定をして、行列を更新する
	void MoveWithCollision();
	// 死亡処理（HP0・死亡フラグ・爆発パーティクル）
	void Die();
	// HPアイコンの色を今のHPに合わせる
	void UpdateHpSprites();
	// ガードキーが押されているか（左右どちらのShiftでも可）
	static bool IsGuardKeyPressed();

	void TakeDamage();
	void OnJustGuardSuccess();
	void RequestCounterAttack();
	void UpdateSwordTransform(float swordDegrees, float reach = 0.0f);
	// 敵・ボスとの接触時の共通処理
	void HandleEnemyContact();
	// 現在の向き（lrDirection_）へ即座に向き直る
	void FaceCurrentDirection();
	// ワールド行列の更新と転送
	void UpdateWorldMatrix();
	// すり抜け防止の補助関数
	bool IsBlockTile(MapChipType type) const { return type == MapChipType::kBlock || type == MapChipType::kTeleport; }
	// 角の位置のチップを調べる。ぶつかるブロックなら true。TPチップなら、TP要求も立てる
	// nextOffsetX/Y は、1つ手前（進んできた側）のチップへのずれ
	bool IsCornerHit(const Vector3& cornerPosition, int32_t nextOffsetX, int32_t nextOffsetY);
};