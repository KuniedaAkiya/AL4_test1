#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

class Player;
class GameScene;
class CameraController;
class MapChipField;

class Boss {
public:
	// ボスの行動状態
	enum class Behavior {
		// 未設定
		kUnknown,
		// 待機
		kIdle,
		// 突進攻撃
		kAttackDash,
		// ジャンプ攻撃
		kAttackJump,
		// 復活演出
		kRevive,
		// 撃破演出
		kDeath,
	};

	// デストラクタ（HPバーのスプライトを解放する）
	~Boss();

	// 初期化
	void Initialize(Model* model, Camera* camera, uint32_t textureHandle, const Vector3& position);
	// 更新
	void Update();
	// 3Dモデルの描画する
	void Draw();
	// ボスのHPバーを描画する
	void DrawUI();

	// 狙う相手（プレイヤー）を設定する
	void SetTarget(Player* target) { target_ = target; }
	// ヒットエフェクトの生成やクリア通知のために、GameSceneを設定する
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	// ボスが動ける左右の範囲を設定する
	void SetMovableArea(float minX, float maxX) {
		minX_ = minX;
		maxX_ = maxX;
	}
	// マップチップのデータを設定する
	void SetmapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	// ボスのワールド座標を取得する
	Vector3 GetWorldPosition() const;
	// 当たり判定用の箱（AABB）を取得する
	AABB GetAABB();
	// 撃破演出が終わって、ボスが倒れたかどうか
	bool IsDead() const { return isDead_; }
	// 当たり判定を無効にしているかどうか（復活・撃破の演出中はtrue）
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	// プレイヤーとぶつかったときの処理
	void OnCollision(Player* player);

	// カメラシェイク
	void SetCameraController(CameraController* cameraController) { cameraController_ = cameraController; }

private:
	// ==== 音の大きさ設定 ====
	// BGMの音量（0.0〜1.0）
	static inline const float kBGMVolume = 0.2f;
	// 効果音の音量（0.0〜1.0）
	static inline const float kSEVolume = 0.4f;

	// ボスの3Dモデル
	Model* model_ = nullptr;
	// 描画に使うカメラ
	Camera* camera_ = nullptr;
	// ボスのテクスチャハンドル
	uint32_t textureHandle_ = 0;

	// ボスの位置・回転・拡大率
	WorldTransform worldTransform_;
	// ボスの移動速度
	Vector3 velocity_{};

	// 狙う相手（プレイヤー）
	Player* target_ = nullptr;
	// エフェクト生成やクリア通知をするためのGameScene
	GameScene* gameScene_ = nullptr;
	// マップチップのデータ
	MapChipField* mapChipField_ = nullptr;
	// カメラシェイク
	CameraController* cameraController_ = nullptr;

	// ボスの最大HP
	static inline const int32_t kMaxHP = 10;
	// ボスの現在のHP
	int32_t hp_ = kMaxHP;

	// 現在の行動状態
	Behavior behavior_ = Behavior::kIdle;
	// 次に切り替えたい行動状態（切り替えが不要なときはkUnknown）
	Behavior behaviorRequest_ = Behavior::kUnknown;
	// 現在の行動に入ってからの経過時間（秒）
	float behaviorTimer_ = 0.0f;

	// 撃破演出が終わったかどうか
	bool isDead_ = false;
	// 当たり判定を無効にしているかどうか
	bool isCollisionDisabled_ = false;
	// 復活を使用済みかどうか（復活は1回だけ）
	bool hasRevived_ = false;

	// ダメージを受けてから次のダメージを受けられるまでの残り時間（秒）
	float damageCooldown_ = 0.0f;
	// ダメージを受けた後の無敵時間（秒）
	static inline const float kDamageCooldownTime = 0.2f;

	// ボスが動ける範囲の左端
	float minX_ = -10000.0f;
	// ボスが動ける範囲の右端
	float maxX_ = 10000.0f;

	// 地面の高さ（ジャンプ攻撃の着地位置）
	float groundY_ = 0.0f;
	// 突進する向き（右が1、左が-1）
	float dashDirection_ = 1.0f;
	// ジャンプ中の横方向の速度
	float jumpTargetX_ = 0.0f;
	// 撃破演出の経過時間（秒）
	float deathTimer_ = 0.0f;

	// モデルの基本の大きさ
	static inline const float kModelScale = 1.0f;
	// 当たり判定の横幅
	static inline const float kWidth = 1.0f;
	// 当たり判定の高さ
	static inline const float kHeight = 1.0f;

	// 待機してから次の攻撃に移るまでの時間（秒）
	static inline const float kIdleTime = 2.0f;
	// 突進前の溜めの時間（秒）
	static inline const float kDashChargeTime = 0.5f;
	// 突進で走っている時間（秒）
	static inline const float kDashTime = 0.3f;
	// 突進の速さ
	static inline const float kDashSpeed = 0.6f;

	// ジャンプ前の溜めの時間（秒）
	static inline const float kJumpChargeTime = 0.4f;
	// ジャンプの初速（上方向）
	static inline const float kJumpPower = 0.7f;
	// ジャンプ中にかかる重力
	static inline const float kGravity = 0.03f;

	// 復活演出の長さ（秒）
	static inline const float kReviveTime = 2.0f;
	// 撃破演出の長さ（秒）
	static inline const float kDeathTime = 1.5f;

	// HPバーの中身（HPに合わせて長さが変わる）
	Sprite* hpBarGaugeSprite_ = nullptr;
	// HPバーの枠
	Sprite* hpBarFrameSprite_ = nullptr;
	// HPバーの表示位置
	static inline const Vector2 kHpBarPosition = {400.0f, 40.0f};
	// HPバーの最大の幅
	static inline const float kHpBarWidth = 480.0f;
	// HPバーの高さ
	static inline const float kHpBarHeight = 24.0f;

	// 着地の効果音
	uint32_t soundHandleLand_ = 0;
	// 復活（形態変化）の効果音
	uint32_t soundHandleFormChange_ = 0;
	// 強化攻撃（ジャスガ後・カウンター）を受けたときの効果音
	uint32_t soundHandleCounter_ = 0;
	// 通常の攻撃を受けたときの効果音
	uint32_t soundHandleAttack_ = 0;

	// ボスの位置を、動ける範囲の中に収める
	void ClampPosition();
	// ダメージを受ける（効果音・HPの減少・復活/撃破への切り替え）
	void TakeDamage(int32_t damage, bool isAttackUp);

	// 待機状態に入るときの初期化
	void BehaviorIdleInitialize();
	// 待機状態の更新
	void BehaviorIdleUpdate();
	// 突進攻撃に入るときの初期化
	void BehaviorAttackDashInitialize();
	// 突進攻撃の更新
	void BehaviorAttackDashUpdate();
	// ジャンプ攻撃に入るときの初期化
	void BehaviorAttackJumpInitialize();
	// ジャンプ攻撃の更新
	void BehaviorAttackJumpUpdate();
	// 復活演出に入るときの初期化
	void BehaviorReviveInitialize();
	// 復活演出の更新
	void BehaviorReviveUpdate();
	// 撃破演出に入るときの初期化
	void BehaviorDeathInitialize();
	// 撃破演出の更新
	void BehaviorDeathUpdate();
	// ワールド行列を更新する
	void UpdateWorldTransform();
};