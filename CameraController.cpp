#include "CameraController.h"
#include "Boss.h"
#include "MatrixFunction.h"
#include "Player.h"
#include <algorithm>
#include <cstdlib>

namespace {
// 0→1 をなめらかに（始まりと終わりがゆっくり）
float EaseInOut(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
} // namespace

void CameraController::Initialize(Camera* camera) {
	// カメラの初期化
	camera_ = camera;
	camera_->Initialize();
};

// 寄った/引いた分だけ画面の広さが変わるので、カメラ中心が動ける範囲を補正する。
// 下端は「ブロックの下端が画面の下端」のまま変わらないように計算される。
CameraController::Rect CameraController::GetAdjustedArea() const {
	const float extraW = (distance_ - kNormalDistance) * kHalfWidthPerDistance;
	const float extraH = (distance_ - kNormalDistance) * kHalfHeightPerDistance;

	Rect area = movableArea_;
	area.left += extraW;
	area.right -= extraW;
	area.bottom += extraH;
	area.top -= extraH;

	// 範囲が逆転したら（部屋が画面より狭い）、中央に固定する
	if (area.left > area.right) {
		area.left = area.right = (area.left + area.right) * 0.5f;
	}
	if (area.bottom > area.top) {
		area.bottom = area.top = (area.bottom + area.top) * 0.5f;
	}
	return area;
}

void CameraController::Update() {
	// 補完の前に前フレームの揺れを外す
	RemoveShake();

	// ボスの撃破演出が始まったら、撃破カメラへ
	if (mode_ == Mode::kFollow && boss_ && boss_->IsDying()) {
		mode_ = Mode::kBossDefeat;
	}

	switch (mode_) {
	case Mode::kFollow:
		UpdateFollow();
		break;
	case Mode::kBossIntro:
		UpdateBossIntro();
		break;
	case Mode::kBossDefeat:
		UpdateBossDefeat();
		break;
	}

	// このフレームのズーム要求は使い終わったので捨てる
	zoomRequest_ = ZoomRequest{};

	camera_->translation_.z = target_->GetWorldTransform().translation_.z - distance_;

	// ステージの範囲から出ない（どの演出でも共通）
	const Rect area = GetAdjustedArea();
	camera_->translation_.x = std::clamp(camera_->translation_.x, area.left, area.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, area.bottom, area.top);

	// 行列を更新
	camera_->UpdateMatrix();
};

void CameraController::UpdateFollow() {
	const Vector3 playerPos = target_->GetWorldTransform().translation_;
	const bool onGround = target_->IsOnGround();

	// 着地するまでのズーム：一度空中に出てから着地したら解除
	if (hasLandingZoom_) {
		if (!onGround) {
			landingZoomAirborne_ = true;
		} else if (landingZoomAirborne_) {
			hasLandingZoom_ = false;
			landingZoomAirborne_ = false;
		}
	}

	// 今フレームの要求と、着地まで有効な要求のうち、より引くほうを採用
	ZoomRequest zoom = zoomRequest_;
	if (hasLandingZoom_ && landingZoom_.distance > zoom.distance) {
		zoom = landingZoom_;
	}
	distance_ += (zoom.distance - distance_) * zoom.speed;

	// マップの外が映らないように、引ける距離に上限を付ける
	const float maxDistanceH = kNormalDistance + (movableArea_.top - movableArea_.bottom) / (2.0f * kHalfHeightPerDistance);
	const float maxDistanceW = kNormalDistance + (movableArea_.right - movableArea_.left) / (2.0f * kHalfWidthPerDistance);
	distance_ = (std::min)(distance_, (std::min)(maxDistanceH, maxDistanceW) - kZoomEdgeMargin);

	// 地上にいる時だけ基準となるY座標を更新（空中・ジャンプ台移動中は groundedY_ に固定）
	if (onGround) {
		groundedY_ = playerPos.y;
	}

	// X軸はプレイヤーを追従、Y軸は地上の高さ(groundedY_)に固定
	camera_->translation_.x += (playerPos.x - camera_->translation_.x) * kFollowSpeedX;
	camera_->translation_.y += (groundedY_ - camera_->translation_.y) * kFollowSpeedY;

	// プレイヤーが画面外に出ないようにする
	camera_->translation_.x = std::clamp(camera_->translation_.x, playerPos.x - kSafeMarginX, playerPos.x + kSafeMarginX);
	camera_->translation_.y = std::clamp(camera_->translation_.y, playerPos.y - kSafeMarginY, playerPos.y + kSafeMarginY);
}

void CameraController::StartBossIntro() {
	if (!boss_) {
		return;
	}
	mode_ = Mode::kBossIntro;
	introTimer_ = 0.0f;
	introRoared_ = false;
	startDistance_ = distance_;
}

// ボス登場：プレイヤー → ボスへ移動（引いたまま） → ボスにズーム（揺れ） → プレイヤーへ戻る
void CameraController::UpdateBossIntro() {
	introTimer_ += 1.0f / 60.0f;

	const float moveTime = kIntroMoveTime;     // ボスへ移動する時間（1.2秒）
	const float holdTime = kIntroHoldTime;     // ボスを映す時間（1.0秒）
	const float returnTime = kIntroReturnTime; // プレイヤーへ戻る時間（0.9秒）

	const float holdStart = moveTime;
	const float returnStart = moveTime + holdTime;
	const float endTime = returnStart + returnTime;

	const Vector3 playerPos = target_->GetWorldTransform().translation_;
	const Vector3 bossPos = boss_->GetWorldPosition();

	if (introTimer_ < holdStart) {
		// --- 1. プレイヤーからボスへカメラ移動 ---
		float t = introTimer_ / moveTime;
		float wPos = EaseInOut(t);

		// カメラ位置の更新（プレイヤー -> ボス）
		camera_->translation_.x = playerPos.x + (bossPos.x - playerPos.x) * wPos;
		camera_->translation_.y = playerPos.y + (bossPos.y + kBossFocusOffsetY - playerPos.y) * wPos;

		// ズーム更新：移動前半（0〜70%）はズームせず、ボス直前（70〜100%）でボスへズームイン
		float zoomT = std::clamp((t - 0.7f) / 0.3f, 0.0f, 1.0f);
		float wZoom = EaseInOut(zoomT);
		distance_ = startDistance_ + (kIntroDistance - startDistance_) * wZoom;

	} else if (introTimer_ < returnStart) {
		// --- 2. ボス注視・咆哮 ---
		camera_->translation_.x = bossPos.x;
		camera_->translation_.y = bossPos.y + kBossFocusOffsetY;
		distance_ = kIntroDistance;

		if (!introRoared_) {
			introRoared_ = true;
			RequestShake(ShakeType::kStrong);
		}

	} else if (introTimer_ < endTime) {
		// --- 3. ボスからプレイヤーへカメラ復帰 ---
		float t = (introTimer_ - returnStart) / returnTime;
		float wReturn = EaseInOut(t);

		// カメラ位置の復帰（ボス -> プレイヤー）
		camera_->translation_.x = bossPos.x + (playerPos.x - bossPos.x) * wReturn;
		camera_->translation_.y = (bossPos.y + kBossFocusOffsetY) + (playerPos.y - (bossPos.y + kBossFocusOffsetY)) * wReturn;

		// 距離の復帰（ボスズーム -> 通常距離）
		distance_ = kIntroDistance + (kNormalDistance - kIntroDistance) * wReturn;

	} else {
		// 演出終了：通常の追従モードへ移行
		mode_ = Mode::kFollow;
		distance_ = kNormalDistance;
		leadY_ = 0.0f;
		isLookUp_ = false;
		return;
	}
}

// ボス撃破：ボスへ向きながら、ゆっくり近づく
void CameraController::UpdateBossDefeat() {
	const Vector3 bossPos = boss_->GetWorldPosition();

	distance_ += (kDefeatDistance - distance_) * kDefeatZoomSpeed;
	camera_->translation_.x += (bossPos.x - camera_->translation_.x) * kDefeatFollowSpeed;
	camera_->translation_.y += (bossPos.y + kBossFocusOffsetY - camera_->translation_.y) * kDefeatFollowSpeed;
}

void CameraController::Reset() {
	const Vector3 playerPos = target_->GetWorldTransform().translation_;

	// 補間用の状態を初期化
	mode_ = Mode::kFollow;
	distance_ = kNormalDistance;
	leadY_ = 0.0f;
	isLookUp_ = false;
	groundedY_ = playerPos.y;

	zoomRequest_ = ZoomRequest{};
	hasLandingZoom_ = false;
	landingZoomAirborne_ = false;

	// 位置を直接設定（移動範囲の外には出さない）
	const Rect area = GetAdjustedArea();
	camera_->translation_.x = std::clamp(playerPos.x, area.left, area.right);
	camera_->translation_.y = std::clamp(playerPos.y, area.bottom, area.top);
	camera_->translation_.z = playerPos.z - distance_;

	// 位置を直接設定したので、揺れの記録も捨てる
	appliedShake_ = {0.0f, 0.0f, 0.0f};
	shakeFrames_ = 0;
	continuousRequested_ = false;
}

void CameraController::RequestZoom(float distance, float speed) {
	if (distance > zoomRequest_.distance) {
		zoomRequest_ = {distance, speed};
	}
}

void CameraController::RequestZoomUntilLanding(float distance, float speed) {
	if (!hasLandingZoom_ || distance > landingZoom_.distance) {
		landingZoom_ = {distance, speed};
	}
	hasLandingZoom_ = true;
}

void CameraController::RemoveShake() {
	camera_->translation_.x -= appliedShake_.x;
	camera_->translation_.y -= appliedShake_.y;
	appliedShake_ = {0.0f, 0.0f, 0.0f};
}

void CameraController::RequestShake(ShakeType type) {
	// 継続は今フレームだけ有効（毎フレーム呼ぶと続く）
	if (type == ShakeType::kContinuous) {
		continuousRequested_ = true;
		return;
	}

	const ShakeParam& param = kShakeParams[static_cast<size_t>(type)];

	// 今の揺れの残りより弱い要求は無視（強い揺れを優先）
	float current = (shakeTotalFrames_ > 0) ? shakePower_ * static_cast<float>(shakeFrames_) / static_cast<float>(shakeTotalFrames_) : 0.0f;
	if (param.power >= current) {
		shakePower_ = param.power;
		shakeFrames_ = param.frames;
		shakeTotalFrames_ = param.frames;
	}
}

void CameraController::UpdateShake() {
	// Update() が呼ばれたフレームでは外し済み。呼ばれなかったフレームではここで外す
	RemoveShake();

	float power = 0.0f;

	// 弱・中・強：だんだん弱くなる
	if (shakeFrames_ > 0) {
		power = shakePower_ * static_cast<float>(shakeFrames_) / static_cast<float>(shakeTotalFrames_);
		--shakeFrames_;
	}

	// 継続：要求があったフレームだけ。1回の揺れとは大きいほうを採用
	if (continuousRequested_) {
		power = (std::max)(power, kContinuousShakePower);
		continuousRequested_ = false;
	}

	if (power <= 0.0f) {
		return;
	}

	auto rnd = []() { return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f; };
	appliedShake_ = {rnd() * power, rnd() * power, 0.0f};

	camera_->translation_.x += appliedShake_.x;
	camera_->translation_.y += appliedShake_.y;
}