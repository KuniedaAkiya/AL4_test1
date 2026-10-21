#include "Enemy.h"
#include "GameScene.h"
#include "MatrixFunction.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// デストラクタ
Enemy::~Enemy() {}

void Enemy::Initialize(Model* model, Camera* camera, uint32_t textureHandle, const Vector3& position) {
	model_ = model;
	camera_ = camera;
	enemyTextureHandle_ = textureHandle;

	// 復活時に戻る座標として記録
	spawnPosition_ = position;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> * (3.0f / 2.0f);

	velocity_ = {0.0f, 0.0f, 0.0f};
	isActivated_ = false;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Enemy::Update() {
	// 振る舞い変更のリクエストがあるか確認
	if (behaviorRequest_ != Behavior::kUnknown) {
		behavior_ = behaviorRequest_;

		// 遷移時の初期化処理
		switch (behavior_) {
		case Behavior::kWalk:
			velocity_ = {0.0f, 0.0f, 0.0f};
			isCollisionDisabled_ = false;
			break;

		case Behavior::kDeath:
			deathTimer_ = 0.0f;
			velocity_ = {0.0f, 0.0f, 0.0f};
			break;

		case Behavior::kStagger:
			staggerTimer_ = 0.0f;
			break;
		}
		behaviorRequest_ = Behavior::kUnknown;
	}

	// 状態ごとの更新を実行
	switch (behavior_) {
	case Behavior::kWalk:
		BehaviorWalkUpdate();
		break;
	case Behavior::kDeath:
		BehaviorDeathUpdate();
		break;
	case Behavior::kStagger:
		BehaviorStaggerUpdate();
		break;
	}
}

void Enemy::Draw() {
	// 消えた後は描画しない
	if (isDead_) {
		return;
	}
	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_, enemyTextureHandle_);
	}
}

Vector3 Enemy::GetWorldPosition() const {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

// ==== 内部処理用関数 ====

// 歩行状態の更新（画面に入ったら動作・壁/棘/穴で反転・重力接地）
void Enemy::BehaviorWalkUpdate() {
	// プレイヤーが近づいて（画面内に入って）いなければ動かない
	if (!isActivated_) {
		if (player_) {
			float diffX = std::abs(worldTransform_.translation_.x - player_->GetWorldPosition().x);
			if (diffX <= kActivateDistance) {
				isActivated_ = true; // 画面内に入ったので起動
			}
		}
		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
		return;
	}

	// 1. 重力処理（空中浮遊防止）
	velocity_.y -= kGravity;

	// 下方向の接地判定
	if (mapChipField_) {
		Vector3 bottomPos = worldTransform_.translation_;
		bottomPos.y += velocity_.y - (kHeight / 2.0f);

		MapChipField::IndexSet indexBottom = mapChipField_->GetMapChipIndexByPosition(bottomPos);
		MapChipType typeBottom = mapChipField_->GetMapChipTypeByIndex(indexBottom.xIndex, indexBottom.yIndex);

		if (typeBottom == MapChipType::kBlock && velocity_.y < 0.0f) {
			// 着地
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexBottom.xIndex, indexBottom.yIndex);
			worldTransform_.translation_.y = rect.top + kHeight / 2.0f;
			velocity_.y = 0.0f;
		} else {
			worldTransform_.translation_.y += velocity_.y;
		}
	} else {
		worldTransform_.translation_.y += velocity_.y;
	}

	// 2. 移動と反転判定（壁・棘・足場の途切れ）
	if (mapChipField_) {
		// ① 進行方向の前方の壁・棘チェック
		Vector3 frontPos = worldTransform_.translation_;
		frontPos.x += moveDirection_ * (kWidth / 2.0f + 0.1f);

		Vector3 checkTop = {frontPos.x, worldTransform_.translation_.y + kHeight / 2.0f - 0.1f, frontPos.z};
		Vector3 checkBottom = {frontPos.x, worldTransform_.translation_.y - kHeight / 2.0f + 0.1f, frontPos.z};

		MapChipField::IndexSet idxTop = mapChipField_->GetMapChipIndexByPosition(checkTop);
		MapChipField::IndexSet idxBottom = mapChipField_->GetMapChipIndexByPosition(checkBottom);

		MapChipType typeTop = mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex);
		MapChipType typeBottom = mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex);

		// ② 進行方向の足元（1マス下）のチェック（穴や棘の確認）
		Vector3 footPos = worldTransform_.translation_;
		footPos.x += moveDirection_ * (kWidth / 2.0f + 0.1f);
		footPos.y -= (kHeight / 2.0f + 0.3f);

		MapChipField::IndexSet idxFoot = mapChipField_->GetMapChipIndexByPosition(footPos);
		MapChipType typeFoot = mapChipField_->GetMapChipTypeByIndex(idxFoot.xIndex, idxFoot.yIndex);

		// 【条件】「前方が壁(kBlock)」「前方が棘(kSpike)」「足元にブロック(kBlock)が無い（穴・棘など）」のいずれかで反転
		bool isWall = (typeTop == MapChipType::kBlock || typeBottom == MapChipType::kBlock);
		bool isSpike = (typeTop == MapChipType::kSpike || typeBottom == MapChipType::kSpike);
		bool isNoGround = (typeFoot != MapChipType::kBlock);

		if (isWall || isSpike || isNoGround) {
			moveDirection_ *= -1.0f;
			velocity_.x = 0.0f;
		} else {
			velocity_.x = kWalkSpeed * moveDirection_;
			worldTransform_.translation_.x += velocity_.x;
		}
	} else {
		velocity_.x = kWalkSpeed * moveDirection_;
		worldTransform_.translation_.x += velocity_.x;
	}

	// 進行方向に応じた回転（左向き: 270度, 右向き: 90度）
	if (moveDirection_ < 0.0f) {
		worldTransform_.rotation_.y = std::numbers::pi_v<float> * (3.0f / 2.0f);
	} else {
		worldTransform_.rotation_.y = std::numbers::pi_v<float> * (1.0f / 2.0f);
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

// デス演出状態の更新
void Enemy::BehaviorDeathUpdate() {
	deathTimer_ += 1.0f / 60.0f;

	// デス演出の開始直後に爆発パーティクルを生成
	if (deathTimer_ <= 1.0f / 60.0f && gameScene_) {
		// 共通関数を呼び出し（モデル: bossModel_, テクスチャ: enemyTextureHandle_, スケール: 0.2f, 個数: 12）
		gameScene_->CreateSlimeExplosion(GetWorldPosition(), model_, enemyTextureHandle_, 0.2f, 12);
	}

	float t = std::clamp(deathTimer_ / kDeathTime, 0.0f, 1.0f);
	worldTransform_.scale_ = {(1.0f - t) * 0.8f, (1.0f - t) * 0.8f, (1.0f - t) * 0.8f};
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (deathTimer_ >= kDeathTime) {
		isDead_ = true;
	}
}

// 怯み状態の更新（ジャストガード時）
void Enemy::BehaviorStaggerUpdate() {
	staggerTimer_ += 1.0f / 60.0f;

	worldTransform_.translation_ += velocity_;
	velocity_ *= (1.0f - kStaggerAttenuation);

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (staggerTimer_ >= kStaggerTime) {
		behaviorRequest_ = Behavior::kWalk;
	}
}

void Enemy::OnCollision(const Player* player) {
	if (behavior_ == Behavior::kDeath || isDead_) {
		return;
	}

	if (player->IsAttack()) {
		behaviorRequest_ = Behavior::kDeath;
		isCollisionDisabled_ = true;

		if (gameScene_) {
			Vector3 effectPos = (GetWorldPosition() + player->GetWorldPosition()) / 2.0f;
			gameScene_->CreateHitEffect(effectPos);
			gameScene_->RequestHitStop(kHitStopFrames);
			gameScene_->RequestCameraShake(GameScene::ShakeType::kWeak);
		}
		return;
	}

	if (player->WasJustGuardSuccessful() && behavior_ != Behavior::kStagger) {
		behaviorRequest_ = Behavior::kStagger;

		float direction = (GetWorldPosition().x < player->GetWorldPosition().x) ? -1.0f : 1.0f;
		velocity_ = {kStaggerKnockback * direction, 0.0f, 0.0f};
	}
}