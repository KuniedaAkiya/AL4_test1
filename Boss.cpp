#include "Boss.h"
#include "GameScene.h"
#include "MatrixFunction.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

Boss::~Boss() {
	delete hpBarFrameSprite_;
	delete hpBarGaugeSprite_;
}

void Boss::Initialize(Model* model, Camera* camera, uint32_t textureHandle, const Vector3& position) {
	model_ = model;
	camera_ = camera;
	textureHandle_ = textureHandle;

	soundHandleLand_ = Audio::GetInstance()->LoadWave("Audio/SE_Boss_Land.mp3");
	soundHandleFormChange_ = Audio::GetInstance()->LoadWave("Audio/SE_FormChange.mp3");
	soundHandleCounter_ = Audio::GetInstance()->LoadWave("Audio/SE_Player_Counter.mp3");
	soundHandleAttack_ = Audio::GetInstance()->LoadWave("Audio/SE_Player_Attack.mp3");

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {kModelScale, kModelScale, kModelScale};
	groundY_ = position.y;

	velocity_ = {};
	hp_ = kMaxHP;
	behavior_ = Behavior::kIdle;
	behaviorRequest_ = Behavior::kUnknown;
	behaviorTimer_ = 0.0f;
	damageCooldown_ = 0.0f;
	isDead_ = false;
	isCollisionDisabled_ = false;
	hasRevived_ = false;

	uint32_t hpGaugeTextureHandle = TextureManager::Load("UI/bossHPGauge.png");
	hpBarGaugeSprite_ = Sprite::Create(hpGaugeTextureHandle, kHpBarPosition);
	hpBarGaugeSprite_->SetSize({kHpBarWidth, kHpBarHeight});

	uint32_t hpFrameTextureHandle = TextureManager::Load("UI/bossHPFrame.png");
	hpBarFrameSprite_ = Sprite::Create(hpFrameTextureHandle, kHpBarPosition);
	hpBarFrameSprite_->SetSize({kHpBarWidth, kHpBarHeight});

	ClampPosition();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Boss::ClampPosition() { worldTransform_.translation_.x = std::clamp(worldTransform_.translation_.x, minX_, maxX_); }

void Boss::Update() {
	if (damageCooldown_ > 0.0f) {
		damageCooldown_ -= 1.0f / 60.0f;
	}

	if (behaviorRequest_ != Behavior::kUnknown) {
		behavior_ = behaviorRequest_;
		behaviorRequest_ = Behavior::kUnknown;

		switch (behavior_) {
		case Behavior::kIdle:
			BehaviorIdleInitialize();
			break;
		case Behavior::kAttackDash:
			BehaviorAttackDashInitialize();
			break;
		case Behavior::kAttackJump:
			BehaviorAttackJumpInitialize();
			break;
		case Behavior::kRevive:
			BehaviorReviveInitialize();
			break;
		case Behavior::kDeath:
			BehaviorDeathInitialize();
			break;
		}
	}

	switch (behavior_) {
	case Behavior::kIdle:
		BehaviorIdleUpdate();
		break;
	case Behavior::kAttackDash:
		BehaviorAttackDashUpdate();
		break;
	case Behavior::kAttackJump:
		BehaviorAttackJumpUpdate();
		break;
	case Behavior::kRevive:
		BehaviorReviveUpdate();
		break;
	case Behavior::kDeath:
		BehaviorDeathUpdate();
		break;
	}

	if (hpBarGaugeSprite_) {
		float rate = std::clamp(static_cast<float>(hp_) / static_cast<float>(kMaxHP), 0.0f, 1.0f);
		hpBarGaugeSprite_->SetSize({kHpBarWidth * rate, kHpBarHeight});
	}
}

void Boss::Draw() {
	if (isDead_)
		return;
	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_, textureHandle_);
	}
}

void Boss::DrawUI() {
	if (isDead_)
		return;
	if (hpBarGaugeSprite_)
		hpBarGaugeSprite_->Draw();
	if (hpBarFrameSprite_)
		hpBarFrameSprite_->Draw();
}

Vector3 Boss::GetWorldPosition() const {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Boss::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Boss::TakeDamage(int32_t damage, bool isAttackUp) {
	if (isAttackUp) {
		Audio::GetInstance()->PlayWave(soundHandleCounter_, false, kSEVolume);
	} else {
		Audio::GetInstance()->PlayWave(soundHandleAttack_, false, kSEVolume);
	}

	hp_ -= damage;
	damageCooldown_ = kDamageCooldownTime;

	if (hp_ <= 0) {
		isCollisionDisabled_ = true;
		if (!hasRevived_) {
			hasRevived_ = true;
			behaviorRequest_ = Behavior::kRevive;
		} else {
			behaviorRequest_ = Behavior::kDeath;
		}
	}
}

void Boss::OnCollision(const Player* player) {
	if (behavior_ == Behavior::kDeath || behavior_ == Behavior::kRevive) {
		return;
	}

	if (!player->IsAttack() || damageCooldown_ > 0.0f) {
		return;
	}

	bool isAttackUp = player->IsAttackUp();
	int32_t damage = isAttackUp ? 2 : 1;

	if (isAttackUp) {
		const_cast<Player*>(player)->ClearAttackUp();
	}

	if (gameScene_) {
		Vector3 effectPos = (GetWorldPosition() + player->GetWorldPosition()) / 2.0f;
		gameScene_->CreateHitEffect(effectPos);
	}

	TakeDamage(damage, isAttackUp);
}

void Boss::BehaviorIdleInitialize() {
	velocity_ = {};
	behaviorTimer_ = 0.0f;
}

void Boss::BehaviorIdleUpdate() {
	behaviorTimer_ += 1.0f / 60.0f;

	if (target_) {
		float targetRotationY = (target_->GetWorldPosition().x < worldTransform_.translation_.x) ? -std::numbers::pi_v<float> / 2.0f : std::numbers::pi_v<float> / 2.0f;

		constexpr float kTurnSpeed = 0.15f;
		worldTransform_.rotation_.y = std::lerp(worldTransform_.rotation_.y, targetRotationY, kTurnSpeed);
	}

	float currentIdleTime = (hp_ <= kMaxHP / 2 || hasRevived_) ? (kIdleTime * 0.5f) : kIdleTime;

	if (behaviorTimer_ >= currentIdleTime) {
		bool doDash = (std::rand() % 2 == 0);
		behaviorRequest_ = doDash ? Behavior::kAttackDash : Behavior::kAttackJump;
	}

	ClampPosition();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Boss::BehaviorAttackDashInitialize() {
	behaviorTimer_ = 0.0f;
	velocity_ = {};
	if (target_) {
		dashDirection_ = (target_->GetWorldPosition().x < worldTransform_.translation_.x) ? -1.0f : 1.0f;
		if (dashDirection_ < 0.0f) {
			worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
		} else {
			worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
		}
	}
}

void Boss::BehaviorAttackDashUpdate() {
	behaviorTimer_ += 1.0f / 60.0f;

	if (behaviorTimer_ < kDashChargeTime) {
		float t = std::clamp(behaviorTimer_ / kDashChargeTime, 0.0f, 1.0f);
		worldTransform_.scale_.x = EaseOut(kModelScale, 0.7f * kModelScale, t);
		worldTransform_.scale_.z = EaseOut(kModelScale, 1.3f * kModelScale, t);
	} else if (behaviorTimer_ < kDashChargeTime + kDashTime) {
		float t = std::clamp((behaviorTimer_ - kDashChargeTime) / kDashTime, 0.0f, 1.0f);
		worldTransform_.scale_.x = EaseOut(0.7f * kModelScale, kModelScale, t);
		worldTransform_.scale_.z = EaseOut(1.3f * kModelScale, kModelScale, t);

		velocity_.x = dashDirection_ * kDashSpeed;
		worldTransform_.translation_ += velocity_;
	} else {
		worldTransform_.scale_ = {kModelScale, kModelScale, kModelScale};
		velocity_ = {};
		behaviorRequest_ = Behavior::kIdle;
	}

	ClampPosition();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Boss::BehaviorAttackJumpInitialize() {
	behaviorTimer_ = 0.0f;
	velocity_ = {};
	if (target_) {
		float diffX = target_->GetWorldPosition().x - worldTransform_.translation_.x;
		jumpTargetX_ = diffX / 36.0f;

		if (diffX < 0.0f) {
			worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
		} else {
			worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
		}
	}
}

void Boss::BehaviorAttackJumpUpdate() {
	behaviorTimer_ += 1.0f / 60.0f;

	if (behaviorTimer_ < kJumpChargeTime) {
		float t = std::clamp(behaviorTimer_ / kJumpChargeTime, 0.0f, 1.0f);
		worldTransform_.scale_.y = EaseOut(kModelScale, 0.6f * kModelScale, t);
	} else {
		if (velocity_.y == 0.0f && worldTransform_.translation_.y <= groundY_ + 0.001f) {
			worldTransform_.scale_.y = kModelScale;
			velocity_.y = kJumpPower;
			velocity_.x = jumpTargetX_;
		}

		velocity_.y -= kGravity;
		worldTransform_.translation_ += velocity_;

		if (worldTransform_.translation_.y <= groundY_ && velocity_.y < 0.0f) {
			worldTransform_.translation_.y = groundY_;
			velocity_ = {};

			Audio::GetInstance()->PlayWave(soundHandleLand_, false, kSEVolume);

			behaviorRequest_ = Behavior::kIdle;
		}
	}

	ClampPosition();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Boss::BehaviorReviveInitialize() {
	behaviorTimer_ = 0.0f;
	velocity_ = {};
	hp_ = kMaxHP;

	Audio::GetInstance()->PlayWave(soundHandleFormChange_, false, kSEVolume);
}

void Boss::BehaviorReviveUpdate() {
	behaviorTimer_ += 1.0f / 60.0f;

	worldTransform_.rotation_.z = std::sin(behaviorTimer_ * 50.0f) * 0.15f;
	float pulse = 1.0f + std::sin(behaviorTimer_ * 30.0f) * 0.15f;
	worldTransform_.scale_ = {kModelScale * pulse, kModelScale * pulse, kModelScale * pulse};

	if (behaviorTimer_ >= kReviveTime) {
		worldTransform_.rotation_.z = 0.0f;
		worldTransform_.scale_ = {kModelScale, kModelScale, kModelScale};
		isCollisionDisabled_ = false;
		behaviorRequest_ = Behavior::kIdle;
	}

	ClampPosition();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Boss::BehaviorDeathInitialize() {
	deathTimer_ = 0.0f;
	velocity_ = {};
}

void Boss::BehaviorDeathUpdate() {
	deathTimer_ += 1.0f / 60.0f;
	float t = std::clamp(deathTimer_ / kDeathTime, 0.0f, 1.0f);

	worldTransform_.rotation_.y += 0.3f;
	worldTransform_.rotation_.x = t * std::numbers::pi_v<float>;
	worldTransform_.scale_ = {(1.0f - t) * kModelScale, (1.0f - t) * kModelScale, (1.0f - t) * kModelScale};

	ClampPosition();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (deathTimer_ >= kDeathTime) {
		isDead_ = true;
		if (gameScene_) {
			gameScene_->RequestGameClear();
		}
	}
}