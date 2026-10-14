#include "CameraController.h"
#include "MatrixFunction.h"
#include "Player.h"
#include <algorithm>
#include <cstdlib>

void CameraController::Initialize(Camera* camera) {
	// カメラの初期化
	camera_ = camera;
	camera_->Initialize();
};

void CameraController::Update() {
	// 補完の前に前フレームの揺れを外す
	RemoveShake();

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	targetPos_.x = targetWorldTransform.translation_.x + targetOffset_.x + target_->GetVelocity().x * kVelocityBias;
	targetPos_.y = targetWorldTransform.translation_.y + targetOffset_.y + target_->GetVelocity().y * kVelocityBias;
	targetPos_.z = targetWorldTransform.translation_.z + targetOffset_.z + target_->GetVelocity().z;

	// 座標補完を使ってゆったり追従
	camera_->translation_ = Lerp(camera_->translation_, targetPos_, kInterpolationSpeed);

	// 追従対象が画面外に行かないようにする処理
	camera_->translation_.x = std::clamp(camera_->translation_.x, targetWorldTransform.translation_.x + margin.left, targetWorldTransform.translation_.x + margin.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, targetWorldTransform.translation_.y + margin.bottom, targetWorldTransform.translation_.y + margin.top);

	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	// 行列を更新
	camera_->UpdateMatrix();
};

void CameraController::Reset() {
	// 追従対象のワールドトランスフォーム
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	camera_->translation_.x = targetWorldTransform.translation_.x + targetOffset_.x;
	camera_->translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;
	// 位置を直接設定したので、揺れの記録も捨てる
	appliedShake_ = {0.0f, 0.0f, 0.0f};
	shakeFrames_ = 0;
	continuousRequested_ = false;
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