#define NOMINMAX
#include "Player.h"
#include "GameScene.h"
#include "MapChipField.h"
#include "MatrixFunction.h"
#include "ShieldEnemy.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

// デストラクタ
Player::~Player() {
	for (int32_t i = 0; i < kMaxHP; ++i) {
		delete hpSprites_[i];
	}
	delete mpBarFrameSprite_;
	delete mpBarGaugeSprite_;
}

// 初期化
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

	UpdateWorldMatrix();

	worldTransformAttack_.Initialize();
	objectColor_.Initialize();
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
	soundHandleCounter_ = Audio::GetInstance()->LoadWave("Audio/SE_Player_Counter.mp3");
}

// 更新
void Player::Update() {
	isJustGuardSuccess_ = false;

	if (counterTimer_ > 0.0f) {
		counterTimer_ = std::max(counterTimer_ - 1.0f / 60.0f, 0.0f);
	}

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
			FaceCurrentDirection();
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
	}

	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kAttack:
		BehaviorAttackUpdate();
		break;
	case Behavior::kKnockback:
		BehaviorKnockbackUpdate();
		break;
	case Behavior::kGuard:
		BehaviorGuardUpdate();
		break;
	}
}

// 描画
void Player::Draw() {
	if (isDead_)
		return;

	// 無敵時間の点滅判定
	bool skipDrawInvincible = isInvincible_ && (static_cast<int32_t>(invincibleTimer_ * 20.0f) % 2 == 0);

	// ガード中は白く点滅させる
	const bool isGuardFlash = (behavior_ == Behavior::kGuard) && (static_cast<int32_t>(guardTimer_ * kGuardFlashSpeed) % 2 == 0);
	const float brightness = isGuardFlash ? kGuardFlashBrightness : 1.0f;
	objectColor_.SetColor({brightness, brightness, brightness, 1.0f});

	if (!skipDrawInvincible) {
		if (playerModel_ && camera_) {
			playerModel_->Draw(worldTransform_, *camera_, &objectColor_);
		}
	}

	// 剣は攻撃中だけ表示する
	if (behavior_ == Behavior::kAttack && modelAttack_ && camera_) {
		modelAttack_->Draw(worldTransformAttack_, *camera_);
	}
}

// UI描画
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

// ワールド座標を取得
Vector3 Player::GetWorldPosition() const { return {worldTransform_.matWorld_.m[3][0], worldTransform_.matWorld_.m[3][1], worldTransform_.matWorld_.m[3][2]}; }

// AABBを取得
AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

// 敵との当たり判定
void Player::OnCollision(const Enemy* /*enemy*/) { HandleEnemyContact(); }

// シールド敵との当たり判定
void Player::OnCollision(const ShieldEnemy* /*shieldEnemy*/) { HandleEnemyContact(); }

// ボスとの当たり判定
void Player::OnCollision(const Boss* /*boss*/) { HandleEnemyContact(); }

// 敵・ボスとの接触時の共通処理
void Player::HandleEnemyContact() {
	if (IsJustGuardActive()) {
		OnJustGuardSuccess();
		return;
	}
	if (IsCounterAttack() || behavior_ == Behavior::kKnockback || behavior_ == Behavior::kGuard || isInvincible_) {
		return;
	}
	TakeDamage();
}

// 現在の向きへ即座に向き直る
void Player::FaceCurrentDirection() {
	const float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
	worldTransform_.rotation_.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
	turnTimer_ = 0.0f;
}

// ワールド行列の更新と転送
void Player::UpdateWorldMatrix() {
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

// 移動入力処理
void Player::InputMove() {
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

// 角の座標を取得
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

// 角の位置のチップを調べる。ぶつかるブロックなら true。TPチップなら、TP要求も立てる
// nextOffsetX/Y は、1つ手前（進んできた側）のチップへのずれ
bool Player::IsCornerHit(const Vector3& cornerPosition, int32_t nextOffsetX, int32_t nextOffsetY) {
	const MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(cornerPosition);
	const uint32_t nextX = static_cast<uint32_t>(static_cast<int32_t>(index.xIndex) + nextOffsetX);
	const uint32_t nextY = static_cast<uint32_t>(static_cast<int32_t>(index.yIndex) + nextOffsetY);

	const MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
	const MapChipType typeNext = mapChipField_->GetMapChipTypeByIndex(nextX, nextY);

	if (type == MapChipType::kTeleport) {
		isTeleportRequested_ = true;
	}
	return IsBlockTile(type) && !IsBlockTile(typeNext);
}

// マップとの当たり判定
void Player::CheckMapCollision(CollisionMapInfo& info) {
	TopCollision(info);
	BottomCollision(info);
	RightCollision(info);
	LeftCollision(info);
}

// 上方向の当たり判定
void Player::TopCollision(CollisionMapInfo& info) {
	if (info.move.y <= 0)
		return;

	// 移動した後の位置と、判定に使う上側の2つの角
	const Vector3 nextPosition = worldTransform_.translation_ + info.move;
	const Vector3 leftTop = CornerPosition(nextPosition, kLeftTop);
	const Vector3 rightTop = CornerPosition(nextPosition, kRightTop);

	// 角が、ぶつかるブロックに入っているか調べる（TPチップに触れた時は、ここでTP要求も立つ）
	const bool hitLeft = IsCornerHit(leftTop, 0, +1);
	const bool hitRight = IsCornerHit(rightTop, 0, +1);

	if (hitLeft || hitRight) {
		const MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(leftTop);
		const MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(CornerPosition(worldTransform_.translation_, kLeftTop));
		if (indexSetNow.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.y = rect.bottom - CornerPosition(worldTransform_.translation_, kLeftTop).y - kBlank;
			info.isHitTop = true;
		}
	}
}

// 下方向の当たり判定（床として着地できるように修正）
void Player::BottomCollision(CollisionMapInfo& info) {
	if (info.move.y >= 0)
		return;

	// 移動した後の位置と、判定に使う下側の2つの角
	const Vector3 nextPosition = worldTransform_.translation_ + info.move;
	const Vector3 leftBottom = CornerPosition(nextPosition, kLeftBottom);
	const Vector3 rightBottom = CornerPosition(nextPosition, kRightBottom);

	// 角が、ぶつかるブロックに入っているか調べる（TPチップに触れた時は、ここでTP要求も立つ）
	const bool hitLeft = IsCornerHit(leftBottom, 0, -1);
	const bool hitRight = IsCornerHit(rightBottom, 0, -1);

	if (hitLeft || hitRight) {
		const MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(leftBottom);
		const MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		if (indexSetNow.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			const float beforeBottomY = CornerPosition(worldTransform_.translation_, kLeftBottom).y;
			info.move.y = std::min(0.0f, rect.top - beforeBottomY + kBlank);
			info.isHitGround = true;
		}
	}
}

// 左方向の当たり判定
void Player::LeftCollision(CollisionMapInfo& info) {
	if (info.move.x >= 0)
		return;

	// 移動した後の位置と、判定に使う左側の2つの角
	const Vector3 nextPosition = worldTransform_.translation_ + info.move;
	const Vector3 leftTop = CornerPosition(nextPosition, kLeftTop);
	const Vector3 leftBottom = CornerPosition(nextPosition, kLeftBottom);

	// 角が、ぶつかるブロックに入っているか調べる（TPチップに触れた時は、ここでTP要求も立つ）
	const bool hitTop = IsCornerHit(leftTop, +1, 0);
	const bool hitBottom = IsCornerHit(leftBottom, +1, 0);

	if (hitTop || hitBottom) {
		const MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(leftBottom);
		const MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.x = std::max(info.move.x, rect.right - worldTransform_.translation_.x + (kWidth / 2.0f) + kBlank);
			info.isHitWall = true;
		}
	}
}

// 右方向の当たり判定
void Player::RightCollision(CollisionMapInfo& info) {
	if (info.move.x <= 0)
		return;

	// 移動した後の位置と、判定に使う右側の2つの角
	const Vector3 nextPosition = worldTransform_.translation_ + info.move;
	const Vector3 rightTop = CornerPosition(nextPosition, kRightTop);
	const Vector3 rightBottom = CornerPosition(nextPosition, kRightBottom);

	// 角が、ぶつかるブロックに入っているか調べる（TPチップに触れた時は、ここでTP要求も立つ）
	const bool hitTop = IsCornerHit(rightTop, -1, 0);
	const bool hitBottom = IsCornerHit(rightBottom, -1, 0);

	if (hitTop || hitBottom) {
		const MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(rightBottom);
		const MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.move.x = std::min(info.move.x, rect.left - worldTransform_.translation_.x - (kWidth / 2.0f) - kBlank);
			info.isHitWall = true;
		}
	}
}

// 位置更新
void Player::PositionUpdate(const CollisionMapInfo& info) { worldTransform_.translation_ += info.move; }

// 上方向の当たり判定
void Player::CeilingCollision(const CollisionMapInfo& info) {
	if (info.isHitTop) {
		velocity_.y = 0;
	}
}

// 左右方向の当たり判定
void Player::WallCollision(const CollisionMapInfo& info) {
	if (info.isHitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// 接地状態の変更
void Player::ChangeGroundingState(const CollisionMapInfo& info) {
	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 移動した後の、足元の2つの角にブロックがあるか調べる（TPチップも床として扱う）
			const Vector3 nextPosition = worldTransform_.translation_ + info.move;
			const MapChipField::IndexSet leftIndex = mapChipField_->GetMapChipIndexByPosition(CornerPosition(nextPosition, kLeftBottom));
			const MapChipField::IndexSet rightIndex = mapChipField_->GetMapChipIndexByPosition(CornerPosition(nextPosition, kRightBottom));
			const bool hit =
			    IsBlockTile(mapChipField_->GetMapChipTypeByIndex(leftIndex.xIndex, leftIndex.yIndex)) || IsBlockTile(mapChipField_->GetMapChipTypeByIndex(rightIndex.xIndex, rightIndex.yIndex));
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

// 行動状態の初期化
void Player::BehaviorRootInitialize() {}

// 行動状態の更新
void Player::BehaviorRootUpdate() {
	InputMove();

	if (onGround_) {
		coyoteTimer_ = kCoyoteTime;
	} else if (coyoteTimer_ > 0.0f) {
		coyoteTimer_ -= 1.0f / 60.0f;
	}

	if (counterTimer_ > 0.0f && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		RequestCounterAttack();
	} else if ((onGround_ || coyoteTimer_ > 0.0f) && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		behaviorRequest_ = Behavior::kAttack;
		coyoteTimer_ = 0.0f;
	} else if (Input::GetInstance()->PushKey(DIK_LCONTROL) && mp_ > 0.0f) {
		behaviorRequest_ = Behavior::kGuard;
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;

	CheckMapCollision(collisionMapInfo);
	PositionUpdate(collisionMapInfo);
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

	UpdateWorldMatrix();
}

// 攻撃行動状態の初期化
void Player::BehaviorAttackInitialize() {
	velocity_ = {0.0f, 0.0f, 0.0f};
	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0.0f;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// カウンター要求の反映
	isCounterAttack_ = isCounterRequested_;
	isCounterRequested_ = false;

	// 次段へ繋ぐ場合以外は1段目から
	if (!isComboContinue_ || isCounterAttack_) {
		comboStep_ = 0;
	}
	isComboContinue_ = false;
	comboInputBuffered_ = false;
	attackHit_ = false;

	if (isCounterAttack_) {
		isAttackUp_ = true;
		Audio::GetInstance()->PlayWave(soundHandleCounter_, false, kSEVolume);
	}
}

// 攻撃行動状態の更新
void Player::BehaviorAttackUpdate() {
	Vector3 attackVelocity{};
	attackParameter_ += 1.0f / 60.0f;

	// 現在の攻撃のパラメータ（通常は段ごと、カウンターは専用値）
	float chargeTime = kComboChargeTimes[comboStep_];
	float swingTime = kComboSwingTimes[comboStep_];
	float afterglowTime = kComboAfterglowTimes[comboStep_];
	float lungeSpeed = kComboLungeSpeeds[comboStep_];
	float startDeg = kSwordStartDegrees[comboStep_];
	float endDeg = kSwordEndDegrees[comboStep_];
	if (isCounterAttack_) {
		chargeTime = kCounterChargeTime;
		swingTime = kCounterSwingTime;
		afterglowTime = kCounterAfterglowTime;
		lungeSpeed = kCounterLungeSpeed;
		startDeg = kCounterSwordDegree;
		endDeg = kCounterSwordDegree;
	}

	// 突きの時の剣の前後位置（通常攻撃は0のまま）
	float swordReach = 0.0f;

	// 攻撃中のSPACEは次段の先行入力として受け付ける
	if (!isCounterAttack_ && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		comboInputBuffered_ = true;
	}

	float swordDeg = startDeg;

	switch (attackPhase_) {
	case AttackPhase::kCharge: {
		// 構え：剣を振りかぶる
		float t = std::clamp(attackParameter_ / chargeTime, 0.0f, 1.0f);
		swordDeg = EaseOut(kSwordRestDegrees, startDeg, t);

		if (isCounterAttack_) {
			swordReach = EaseOut(0.0f, -kCounterPullBack, t);
		}

		if (attackParameter_ >= chargeTime) {
			attackPhase_ = AttackPhase::kSwing;
			attackParameter_ = 0.0f;
		}
		break;
	}

	case AttackPhase::kSwing: {
		// 振る：最初が速く終わりが遅い（EaseOut）と、斬った感じが出る
		float t = std::clamp(attackParameter_ / swingTime, 0.0f, 1.0f);
		swordDeg = EaseOut(startDeg, endDeg, t);

		if (isCounterAttack_) {
			swordReach = EaseOut(-kCounterPullBack, kCounterReach, t);
		}

		// 少しだけ前へ踏み込む
		attackVelocity.x = (lrDirection_ == LRDirection::kRight) ? lungeSpeed : -lungeSpeed;

		if (attackParameter_ >= swingTime) {
			attackPhase_ = AttackPhase::kAfterglow;
			attackParameter_ = 0.0f;
		}
		break;
	}

	case AttackPhase::kAfterglow: {
		// 後隙：振り終わりの角度で止まる
		swordDeg = endDeg;

		if (isCounterAttack_) {
			swordReach = kCounterReach;
		}

		if (attackParameter_ >= afterglowTime) {
			attackParameter_ = 0.0f;

			// 終わった後隙でダメージを受けないように無敵
			if (isCounterAttack_) {
				isInvincible_ = true;
				invincibleTimer_ = kInvincibleTime - kCounterEndInvincibleTime;
			}

			if (!isCounterAttack_ && comboInputBuffered_ && comboStep_ + 1 < kMaxComboStep) {
				// 次段へ繋ぐ（入力された方向へ向き直れる）
				Input* input = Input::GetInstance();
				if (input->PushKey(DIK_RIGHT)) {
					lrDirection_ = LRDirection::kRight;
				} else if (input->PushKey(DIK_LEFT)) {
					lrDirection_ = LRDirection::kLeft;
				}
				FaceCurrentDirection();

				comboStep_++;
				isComboContinue_ = true;
				behaviorRequest_ = Behavior::kAttack;
			} else {
				behaviorRequest_ = Behavior::kRoot;
			}
		}
		break;
	}
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = attackVelocity;

	CheckMapCollision(collisionMapInfo);
	PositionUpdate(collisionMapInfo);

	// 自機の位置が決まってから剣を配置する
	UpdateSwordTransform(swordDeg);

	UpdateWorldMatrix();

	UpdateSwordTransform(swordDeg, swordReach);
}

// 攻撃力を取得
int32_t Player::GetAttackDamage() const {
	int32_t damage = 1;
	if (behavior_ == Behavior::kAttack) {
		if (isCounterAttack_) {
			damage = kCounterDamage;
		} else if (comboStep_ == kMaxComboStep - 1) {
			damage = kFinisherDamage;
		}
	}
	// ジャストガード後の強化攻撃はダメージ2倍
	if (isAttackUp_) {
		damage *= 2;
	}
	return damage;
}

// 剣の位置と角度を更新する
void Player::UpdateSwordTransform(float swordDegrees, float reach) {
	const float dir = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;

	// 左向きは、角度と持ち手の位置を左右反転する
	const float yOffset = kSwordPivotOffset.y + (IsCounterAttack() ? kCounterSwordYOffset : 0.0f);
	worldTransformAttack_.translation_ = {worldTransform_.translation_.x + (kSwordPivotOffset.x + reach) * dir, worldTransform_.translation_.y + yOffset, worldTransform_.translation_.z};
	worldTransformAttack_.rotation_ = {0.0f, 0.0f, swordDegrees * std::numbers::pi_v<float> / 180.0f * dir};

	worldTransformAttack_.matWorld_ = MakeAffineMatrix(worldTransformAttack_.scale_, worldTransformAttack_.rotation_, worldTransformAttack_.translation_);
	worldTransformAttack_.TransferMatrix();
}

// 剣の当たり判定（持ち手と剣先を囲むAABB）
AABB Player::GetSwordAABB() const {
	const Vector3& pivot = worldTransformAttack_.translation_;
	const float angle = worldTransformAttack_.rotation_.z;

	// Z回転で、刀身（+Y方向）が指す先
	Vector3 tip = {pivot.x - std::sin(angle) * kSwordLength, pivot.y + std::cos(angle) * kSwordLength, pivot.z};

	const float half = kSwordThickness / 2.0f;
	AABB aabb;
	aabb.min = {std::min(pivot.x, tip.x) - half, std::min(pivot.y, tip.y) - half, pivot.z - kWidth / 2.0f};
	aabb.max = {std::max(pivot.x, tip.x) + half, std::max(pivot.y, tip.y) + half, pivot.z + kWidth / 2.0f};
	return aabb;
}

// 他の物体のAABBと重なっている分だけ横に押し出される
void Player::PushOutFrom(const AABB& other) {
	const AABB self = GetAABB();
	const float selfCenterX = (self.min.x + self.max.x) * 0.5f;
	const float otherCenterX = (other.min.x + other.max.x) * 0.5f;

	// 相手の反対側へ押し出す
	const float dir = (selfCenterX < otherCenterX) ? -1.0f : 1.0f;
	const float overlap = (dir < 0.0f) ? (self.max.x - other.min.x) : (other.max.x - self.min.x);
	if (overlap <= 0.0f) {
		return;
	}

	// 壁にめり込まないよう、マップとの当たり判定を通してから移動する
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = {dir * overlap, 0.0f, 0.0f};
	CheckMapCollision(collisionMapInfo);
	PositionUpdate(collisionMapInfo);
	UpdateWorldMatrix();
}

// カウンター攻撃要求
void Player::RequestCounterAttack() {
	isCounterRequested_ = true;
	counterTimer_ = 0.0f;
	coyoteTimer_ = 0.0f;
	behaviorRequest_ = Behavior::kAttack;
}

// 攻撃中かどうかを取得
bool Player::IsAttack() const { return behavior_ == Behavior::kAttack; }

// ノックバック要求
void Player::RequestKnockback() { isKnockbackRequested_ = true; }

// ノックバック行動状態の初期化
void Player::BehaviorKnockbackInitialize() {
	// ノックバックしたときにコンボ状態を解除する
	isComboContinue_ = false;
	comboInputBuffered_ = false;
	isCounterRequested_ = false;

	knockbackTimer_ = 0.0f;
	knockbackPhase_ = KnockbackPhase::kFly;

	float direction = (lrDirection_ == LRDirection::kRight) ? -1.0f : 1.0f;
	velocity_.x = kKnockbackSpeed * direction;
}

// ノックバック行動状態の更新
void Player::BehaviorKnockbackUpdate() {
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
	PositionUpdate(collisionMapInfo);
	CeilingCollision(collisionMapInfo);
	WallCollision(collisionMapInfo);
	ChangeGroundingState(collisionMapInfo);

	UpdateWorldMatrix();
}

// ガード行動状態の初期化
void Player::BehaviorGuardInitialize() {
	velocity_.x = 0.0f;
	guardTimer_ = 0.0f;
	hasJustGuarded_ = false;
}

// ガード行動状態の更新
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

	// ジャストガード直後はSPACEでカウンター攻撃へ
	if (counterTimer_ > 0.0f && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		RequestCounterAttack();
	}

	velocity_.x = 0.0f;
	if (!onGround_) {
		velocity_ += Vector3(0, -kGravityAcceleration, 0);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;
	CheckMapCollision(collisionMapInfo);
	PositionUpdate(collisionMapInfo);
	CeilingCollision(collisionMapInfo);
	WallCollision(collisionMapInfo);
	ChangeGroundingState(collisionMapInfo);

	UpdateWorldMatrix();
}

// ジャストガードが有効かどうかを取得
bool Player::IsJustGuardActive() const { return behavior_ == Behavior::kGuard && guardTimer_ <= kJustGuardWindowTime && !hasJustGuarded_; }

// ジャストガード成功時の処理
void Player::OnJustGuardSuccess() {
	hasJustGuarded_ = true;

	isJustGuardSuccess_ = true;
	Audio::GetInstance()->PlayWave(soundHandleJustGuard_, false, kSEVolume);

	isInvincible_ = true;
	invincibleTimer_ = -kJustGuardBonusInvincibleTime;
	mp_ = std::min(mp_ + kJustGuardMpReward, kMaxMP);
	isAttackUp_ = true;
	counterTimer_ = kCounterWindowTime;

	if (gameScene_) {
		gameScene_->CreateGuardEffect(GetWorldPosition());
	}

	// シェイク
	if (cameraController_) {
		cameraController_->RequestShake(CameraController::ShakeType::kMedium);
	}
}

// ダメージを受ける
void Player::TakeDamage() {
	hp_--;
	counterTimer_ = 0.0f;
	Audio::GetInstance()->PlayWave(soundHandleDamage_, false, kSEVolume);

	if (hp_ <= 0) {
		hp_ = 0;
		isDead_ = true;
	}

	// シェイク
	if (cameraController_) {
		cameraController_->RequestShake(isDead_ ? CameraController::ShakeType::kStrong : CameraController::ShakeType::kMedium);
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
