#include "Enemy.h"
#include "GameScene.h"
#include "MatrixFunction.h"
#include "Player.h"
#include <algorithm>
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
			// 待機状態へのリセット（見た目と座標を初期状態に戻す）
			velocity_ = {0.0f, 0.0f, 0.0f};
			worldTransform_.translation_ = spawnPosition_;
			worldTransform_.rotation_ = {0.0f, std::numbers::pi_v<float> * (3.0f / 2.0f), 0.0f};
			worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
			isCollisionDisabled_ = false;
			break;

		case Behavior::kDeath:
			deathTimer_ = 0.0f;
			velocity_ = {0.0f, 0.0f, 0.0f};
			break;

		case Behavior::kStagger:
			staggerTimer_ = 0.0f;
			break;

		case Behavior::kRespawnWait:
			respawnTimer_ = 0.0f;
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
	case Behavior::kRespawnWait:
		BehaviorRespawnWaitUpdate();
		break;
	}
}

void Enemy::Draw() {
	// 復活待ち中は非表示
	if (behavior_ == Behavior::kRespawnWait) {
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

// 待機状態の更新（動かず行列計算のみ行う）
void Enemy::BehaviorWalkUpdate() {
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

// デス演出状態の更新
void Enemy::BehaviorDeathUpdate() {
	deathTimer_ += 1.0f / 60.0f;
	float t = std::clamp(deathTimer_ / kDeathTime, 0.0f, 1.0f);

	// 回転して小さくなりながら消える演出
	worldTransform_.rotation_.y += 0.5f;
	worldTransform_.rotation_.x = t * std::numbers::pi_v<float>;
	worldTransform_.scale_ = {1.0f - t, 1.0f - t, 1.0f - t};

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 演出完了後、復活待ちへ
	if (deathTimer_ >= kDeathTime) {
		behaviorRequest_ = Behavior::kRespawnWait;
	}
}

// 復活待ち状態の更新（非表示で指定時間カウント）
void Enemy::BehaviorRespawnWaitUpdate() {
	respawnTimer_ += 1.0f / 60.0f;
	if (respawnTimer_ >= kRespawnTime) {
		behaviorRequest_ = Behavior::kWalk;
	}
}

// 怯み状態の更新（ジャストガード時）
void Enemy::BehaviorStaggerUpdate() {
	staggerTimer_ += 1.0f / 60.0f;

	// 弾かれた速度で移動しつつ減衰
	worldTransform_.translation_ += velocity_;
	velocity_ *= (1.0f - kStaggerAttenuation);

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 時間経過で通常の待機状態へ復帰
	if (staggerTimer_ >= kStaggerTime) {
		behaviorRequest_ = Behavior::kWalk;
	}
}

void Enemy::OnCollision(const Player* player) {
	// すでにデス演出中・復活待ち中なら何もしない
	if (behavior_ == Behavior::kDeath || behavior_ == Behavior::kRespawnWait) {
		return;
	}

	// プレイヤーが攻撃中ならデス演出へ移行
	if (player->IsAttack()) {
		behaviorRequest_ = Behavior::kDeath;
		isCollisionDisabled_ = true;

		if (gameScene_) {
			Vector3 effectPos = (GetWorldPosition() + player->GetWorldPosition()) / 2.0f;
			gameScene_->CreateHitEffect(effectPos);
		}
		return;
	}

	// プレイヤーにジャストガードされたら怯み状態へ移行
	if (player->WasJustGuardSuccessful() && behavior_ != Behavior::kStagger) {
		behaviorRequest_ = Behavior::kStagger;

		float direction = (GetWorldPosition().x < player->GetWorldPosition().x) ? -1.0f : 1.0f;
		velocity_ = {kStaggerKnockback * direction, 0.0f, 0.0f};
	}
}