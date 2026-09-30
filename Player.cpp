#define NOMINMAX
#include "Player.h"
#include "GameScene.h"
#include "MapChipField.h"
#include "MatrixFunction.h"
#include "ShieldEnemy.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

Player::~Player() {
	for (int32_t i = 0; i < kMaxHP; ++i) {
		delete hpSprites_[i];
	}
	delete mpBarFrameSprite_;
	delete mpBarGaugeSprite_;
}

void Player::Initialize(Model* model, Model* modelAttack, Camera* camera, uint32_t playerHandle, const Vector3& position) {
	assert(model);
	assert(modelAttack);

	playerModel_ = model;
	modelAttack_ = modelAttack;
	camera_ = camera;
	playerTextureHandle_ = playerHandle;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	worldTransformAttack_.Initialize();
	velocity_ = {0.0f, 0.0f, 0.0f};

	hp_ = kMaxHP;
	isInvincible_ = false;
	invincibleTimer_ = 0.0f;

	uint32_t hpIconTextureHandle = TextureManager::Load("UI/playerHP.png");
	for (int32_t i = 0; i < kMaxHP; ++i) {
		Vector2 pos = {kHpIconPosition.x + kHpIconSpacing * static_cast<float>(i), kHpIconPosition.y};
		hpSprites_[i] = Sprite::Create(hpIconTextureHandle, pos);
		hpSprites_[i]->SetSize({kHpIconSize, kHpIconSize});
		hpSprites_[i]->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	}
	mp_ = kMaxMP;

	uint32_t mpGaugeTextureHandle = TextureManager::Load("UI/mpFrameGauge.png");
	mpBarGaugeSprite_ = Sprite::Create(mpGaugeTextureHandle, kMpBarPosition);
	mpBarGaugeSprite_->SetSize({kMpBarWidth, kMpBarHeight});

	uint32_t mpFrameTextureHandle = TextureManager::Load("UI/mpFrame.png");
	mpBarFrameSprite_ = Sprite::Create(mpFrameTextureHandle, kMpBarPosition);
	mpBarFrameSprite_->SetSize({kMpBarWidth, kMpBarHeight});

	soundHandleDamage_ = Audio::GetInstance()->LoadWave("Audio/SE_Player_Damage.mp3");
	soundHandleJustGuard_ = Audio::GetInstance()->LoadWave("Audio/SE_Parry_Success.mp3");
	soundHandleJump_ = Audio::GetInstance()->LoadWave("Audio/SE_Player_Jump.mp3");
}

void Player::Update() {
	isJustGuardSuccess_ = false;

	if (isInvincible_) {
		invincibleTimer_ += 1.0f / 60.0f;
		if (invincibleTimer_ >= kInvincibleTime) {
			isInvincible_ = false;
		}
	}

	if (behavior_ != Behavior::kGuard) {
		mp_ += kMPRegenPerSecond / 60.0f;
		mp_ = std::min(mp_, kMaxMP);
	}

	if (mpBarGaugeSprite_) {
		float rate = std::clamp(mp_ / kMaxMP, 0.0f, 1.0f);
		mpBarGaugeSprite_->SetSize({kMpBarWidth * rate, kMpBarHeight});
	}

	if (isKnockbackRequested_) {
		behaviorRequest_ = Behavior::kKnockback;
		isKnockbackRequested_ = false;
	}

	if (behaviorRequest_ != Behavior::kUnknown) {
		behavior_ = behaviorRequest_;
		behaviorRequest_ = Behavior::kUnknown;

		if (behavior_ != Behavior::kRoot && turnTimer_ > 0.0f) {
			float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
			worldTransform_.rotation_.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
			turnTimer_ = 0.0f;
		}

		switch (behavior_) {
		case Behavior::kRoot:
			Player::BehaviorRootInitialize();
			break;
		case Behavior::kAttack:
			Player::BehaviorAttackInitialize();
			break;
		case Behavior::kKnockback:
			Player::BehaviorKnockbackInitialize();
			break;
		case Behavior::kGuard:
			Player::BehaviorGuardInitialize();
			break;
		}
		behaviorRequest_ = Behavior::kUnknown;
	}

	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpData();
		break;
	case Behavior::kAttack:
		BehaviorAttackUpData();
		break;
	case Behavior::kKnockback:
		BehaviorKnockbackUpData();
		break;
	case Behavior::kGuard:
		BehaviorGuardUpdate();
		break;
	}
}

void Player::Draw() {
	if (isDead_)
		return;

	bool skipDraw = isInvincible_ && (static_cast<int32_t>(invincibleTimer_ * 20.0f) % 2 == 0);

	if (!skipDraw) {
		if (playerModel_ && camera_) {
			playerModel_->Draw(worldTransform_, *camera_);
		}
	}
}

void Player::DrawUI() {
	for (int32_t i = 0; i < kMaxHP; ++i) {
		if (hpSprites_[i]) {
			hpSprites_[i]->Draw();
		}
	}

	if (mpBarGaugeSprite_) {
		mpBarGaugeSprite_->Draw();
	}
	if (mpBarFrameSprite_) {
		mpBarFrameSprite_->Draw();
	}
}

Vector3 Player::GetWorldPosition() const {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::OnCollision(const Enemy* /*enemy*/) {
	if (IsJustGuardActive()) {
		OnJustGuardSuccess();
		return;
	}
	if (IsAttack() || behavior_ == Behavior::kKnockback || behavior_ == Behavior::kGuard || isInvincible_) {
		return;
	}
	TakeDamage();
}

void Player::OnCollision(const ShieldEnemy* /*shieldEnemy*/) {
	if (IsJustGuardActive()) {
		OnJustGuardSuccess();
		return;
	}
	if (IsAttack() || behavior_ == Behavior::kKnockback || behavior_ == Behavior::kGuard || isInvincible_) {
		return;
	}
	TakeDamage();
}

void Player::OnCollision(const Boss* /*boss*/) {
	if (IsJustGuardActive()) {
		OnJustGuardSuccess();
		return;
	}
	if (IsAttack() || behavior_ == Behavior::kKnockback || behavior_ == Behavior::kGuard || isInvincible_) {
		return;
	}
	TakeDamage();
}

void Player::inputMove() {
	if (onGround_) {
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			Vector3 acceleration_{};
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (velocity_.x < 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration_.x = +kAcceleration;
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				if (velocity_.x > 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration_.x = -kAcceleration;
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
			}
			velocity_.x += acceleration_.x;
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}
		// ジャンプ時に音量を指定してSE再生
		if (Input::GetInstance()->PushKey(DIK_UP)) {
			velocity_ += Vector3(0, kJumpAcceleration, 0);
			Audio::GetInstance()->PlayWave(soundHandleJump_, false, kSEVolume);
		}
	} else {
		velocity_ += Vector3(0, -kGravityAcceleration, 0);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorners]{
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
        {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
        {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
        {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}
    };
	Vector3 result;
	result.x = center.x + offsetTable[static_cast<uint32_t>(corner)].x;
	result.y = center.y + offsetTable[static_cast<uint32_t>(corner)].y;
	result.z = center.z + offsetTable[static_cast<uint32_t>(corner)].z;
	return result;
}

void Player::CheckMapCollision(CollisionMapInfo& info) {
	TopCollision(info);
	BottomCollision(info);
	RightCollision(info);
	LeftCollision(info);
}

void Player::TopCollision(CollisionMapInfo& info) {
	if (info.move.y <= 0)
		return;
	std::array<Vector3, kNumCorners> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++) {
		Vector3 nextPosition = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionNew[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
	}
	std::array<Vector3, kNumCorners> positionBefore;
	for (uint32_t i = 0; i < positionBefore.size(); i++) {
		positionBefore[i] = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
	}
	float beforeTopY = positionBefore[kLeftTop].y;

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;

	bool hit = false;
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftTop]);
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexByPosition(CornerPosition(worldTransform_.translation_, kLeftTop));
		if (indexSetNow.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			beforeTopY = CornerPosition(worldTransform_.translation_, kLeftTop).y;
			info.move.y = rect.bottom - CornerPosition(worldTransform_.translation_, kLeftTop).y - kBlank;
			info.isHitTop = true;
		}
	}
}

void Player::BottomCollision(CollisionMapInfo& info) {
	if (info.move.y >= 0)
		return;

	std::array<Vector3, kNumCorners> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++) {
		Vector3 nextPosition = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionNew[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
	}

	std::array<Vector3, kNumCorners> positionBefore;
	for (uint32_t i = 0; i < positionBefore.size(); i++) {
		positionBefore[i] = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
	}
	float beforeBottomY = positionBefore[kLeftBottom].y;

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftBottom]);
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		if (indexSetNow.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			beforeBottomY = CornerPosition(worldTransform_.translation_, kLeftBottom).y;
			info.move.y = std::min(0.0f, rect.top - beforeBottomY + kBlank);
			info.isHitGround = true;
		}
	}
}

void Player::LeftCollision(CollisionMapInfo& info) {
	if (info.move.x >= 0)
		return;

	std::array<Vector3, kNumCorners> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++) {
		Vector3 nextPosition = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionNew[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftBottom]);
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.x = std::max(info.move.x, rect.right - worldTransform_.translation_.x + (kWidth / 2.0f) + kBlank);
			info.isHitWall = true;
		}
	}
}

void Player::RightCollision(CollisionMapInfo& info) {
	if (info.move.x <= 0)
		return;

	std::array<Vector3, kNumCorners> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++) {
		Vector3 nextPosition = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionNew[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kRightBottom]);
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.x = std::min(info.move.x, rect.left - worldTransform_.translation_.x - (kWidth / 2.0f) - kBlank);
			info.isHitWall = true;
		}
	}
}

void Player::PositionUpDate(const CollisionMapInfo& info) { worldTransform_.translation_ += info.move; }

void Player::CeilingCollision(const CollisionMapInfo& info) {
	if (info.isHitTop) {
		velocity_.y = 0;
	}
}

void Player::WallCollision(const CollisionMapInfo& info) {
	if (info.isHitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

void Player::ChangeGroundingState(const CollisionMapInfo& info) {
	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			std::array<Vector3, kNumCorners> positionNew;
			for (uint32_t i = 0; i < positionNew.size(); i++) {
				Vector3 nextPosition = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
				positionNew[i] = CornerPosition(nextPosition, static_cast<Corner>(i));
			}
			MapChipType mapChipType;
			bool hit = false;
			MapChipField::IndexSet indexSet;
			indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kLeftBottom]);
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}
			indexSet = mapChipField_->GetMapChipIndexByPosition(positionNew[kRightBottom]);
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}
			if (!hit) {
				onGround_ = false;
			}
		}
	} else {
		if (info.isHitGround) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding);
			velocity_.y = 0.0f;
		}
	}
}

void Player::BehaviorRootInitialize() {}

void Player::BehaviorRootUpData() {
	inputMove();

	if (onGround_) {
		coyoteTimer_ = kCoyoteTime;
	} else if (coyoteTimer_ > 0.0f) {
		coyoteTimer_ -= 1.0f / 60.0f;
	}

	if ((onGround_ || coyoteTimer_ > 0.0f) && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		behaviorRequest_ = Behavior::kAttack;
		coyoteTimer_ = 0.0f;
	} else if (Input::GetInstance()->PushKey(DIK_LCONTROL) && mp_ > 0.0f) {
		behaviorRequest_ = Behavior::kGuard;
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;

	CheckMapCollision(collisionMapInfo);
	PositionUpDate(collisionMapInfo);
	CeilingCollision(collisionMapInfo);
	WallCollision(collisionMapInfo);
	ChangeGroundingState(collisionMapInfo);

	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;
		if (turnTimer_ < 0.0f) {
			turnTimer_ = 0.0f;
		}

		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, destinationRotationY, turnTimer_, kTimeTurn);
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::BehaviorAttackInitialize() {
	velocity_ = {0.0f, 0.0f, 0.0f};
	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0.0f;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
}

void Player::BehaviorAttackUpData() {
	Vector3 attackVelocity{};
	attackParameter_ += 1.0f / 60.0f;

	switch (attackPhase_) {
	case AttackPhase::kCharge: {
		float t = std::clamp(attackParameter_ / kChargeTime, 0.0f, 1.0f);
		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);

		if (attackParameter_ >= kChargeTime) {
			attackPhase_ = AttackPhase::kDash;
			attackParameter_ = 0.0f;
		}
		break;
	}

	case AttackPhase::kDash: {
		float t = std::clamp(attackParameter_ / kDashTime, 0.0f, 1.0f);
		worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
		worldTransform_.scale_.y = EaseIn(1.6f, 0.7f, t);

		if (lrDirection_ == LRDirection::kRight) {
			attackVelocity.x = kDashSpeed;
		} else {
			attackVelocity.x = -kDashSpeed;
		}

		if (attackParameter_ >= kDashTime) {
			attackPhase_ = AttackPhase::kAfterglow;
			attackParameter_ = 0.0f;
		}
		break;
	}

	case AttackPhase::kAfterglow: {
		float t = std::clamp(attackParameter_ / kAfterglowTime, 0.0f, 1.0f);
		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseIn(0.7f, 1.0f, t);

		if (attackParameter_ >= kAfterglowTime) {
			attackParameter_ = 0.0f;
			behaviorRequest_ = Behavior::kRoot;
		}
		break;
	}
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = attackVelocity;

	CheckMapCollision(collisionMapInfo);
	PositionUpDate(collisionMapInfo);

	if (collisionMapInfo.isHitWall) {
		attackVelocity.x = 0.0f;
	}

	worldTransformAttack_.translation_ = worldTransform_.translation_;
	worldTransformAttack_.rotation_ = worldTransform_.rotation_;

	worldTransformAttack_.matWorld_ = MakeAffineMatrix(worldTransformAttack_.scale_, worldTransformAttack_.rotation_, worldTransformAttack_.translation_);
	worldTransformAttack_.TransferMatrix();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

bool Player::IsAttack() const { return behavior_ == Behavior::kAttack; }

void Player::RequestKnockback() { isKnockbackRequested_ = true; }

void Player::BehaviorKnockbackInitialize() {
	knockbackTimer_ = 0.0f;
	knockbackPhase_ = KnockbackPhase::kFly;

	float direction = (lrDirection_ == LRDirection::kRight) ? -1.0f : 1.0f;
	velocity_.x = kKnockbackSpeed * direction;
}

void Player::BehaviorKnockbackUpData() {
	knockbackTimer_ += 1.0f / 60.0f;

	switch (knockbackPhase_) {
	case KnockbackPhase::kFly:
		velocity_.x *= (1.0f - kKnockbackAttenuation);
		if (knockbackTimer_ >= kKnockbackFlyTime) {
			knockbackPhase_ = KnockbackPhase::kRecover;
			knockbackTimer_ = 0.0f;
			velocity_.x = 0.0f;
		}
		break;

	case KnockbackPhase::kRecover:
		velocity_.x = 0.0f;
		if (knockbackTimer_ >= kKnockbackRecoverTime) {
			behaviorRequest_ = Behavior::kRoot;
		}
		break;
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;
	CheckMapCollision(collisionMapInfo);
	PositionUpDate(collisionMapInfo);
	CeilingCollision(collisionMapInfo);
	WallCollision(collisionMapInfo);
	ChangeGroundingState(collisionMapInfo);

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::BehaviorGuardInitialize() {
	velocity_.x = 0.0f;
	guardTimer_ = 0.0f;
}

void Player::BehaviorGuardUpdate() {
	guardTimer_ += 1.0f / 60.0f;

	if (!Input::GetInstance()->PushKey(DIK_LCONTROL)) {
		behaviorRequest_ = Behavior::kRoot;
	}

	mp_ -= kMPDrainPerSecond / 60.0f;
	if (mp_ <= 0.0f) {
		mp_ = 0.0f;
		behaviorRequest_ = Behavior::kRoot;
	}

	velocity_.x = 0.0f;
	if (!onGround_) {
		velocity_ += Vector3(0, -kGravityAcceleration, 0);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;
	CheckMapCollision(collisionMapInfo);
	PositionUpDate(collisionMapInfo);
	CeilingCollision(collisionMapInfo);
	WallCollision(collisionMapInfo);
	ChangeGroundingState(collisionMapInfo);

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

bool Player::IsJustGuardActive() const { return behavior_ == Behavior::kGuard && guardTimer_ <= kJustGuardWindowTime; }

void Player::OnJustGuardSuccess() {
	isJustGuardSuccess_ = true;
	Audio::GetInstance()->PlayWave(soundHandleJustGuard_, false, kSEVolume);

	isInvincible_ = true;
	invincibleTimer_ = -kJustGuardBonusInvincibleTime;
	mp_ = std::min(mp_ + kJustGuardMpReward, kMaxMP);
	isAttackUp_ = true;

	if (gameScene_) {
		gameScene_->CreateGuardEffect(GetWorldPosition());
	}
}

void Player::TakeDamage() {
	hp_--;
	Audio::GetInstance()->PlayWave(soundHandleDamage_, false, kSEVolume);

	if (hp_ <= 0) {
		hp_ = 0;
		isDead_ = true;
	}

	for (int32_t i = 0; i < kMaxHP; ++i) {
		if (hpSprites_[i]) {
			hpSprites_[i]->SetColor(i < hp_ ? Vector4{1.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 0.4f});
		}
	}

	if (isDead_) {
		return;
	}

	isInvincible_ = true;
	invincibleTimer_ = 0.0f;
	RequestKnockback();
}