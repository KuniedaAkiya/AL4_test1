#include "DeathParticles.h"
#include "MatrixFunction.h"
#include <algorithm>

void DeathParticles::Initialize(Model* model, Camera* camera, const Vector3& position) {
	model_ = model;
	camera_ = camera;

	objectColor_.Initialize();
	color_ = {1, 1, 1, 1};

	// ワールド変換の初期化
	for (WorldTransform& worldTransform : worldTransform_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position;
	}
}

Vector3 DeathParticles::CalculateVelocity(uint32_t index) const {
	// 基本となる速度ベクトル
	Vector3 velocity = {kSpeed, 0.0f, 0.0f};
	// 回転角を計算
	float angle = kAngleUnit * static_cast<float>(index);
	// Z軸まわり回転行列
	Matrix4x4 matrixRotation = MakeRotateZMatrix(angle);
	// 基本ベクトルを回転させて速度ベクトルを得る
	return MathUtility::Transform(velocity, matrixRotation);
}

void DeathParticles::Update() {
	// 終了していたらこれ以上更新しない
	if (isFinished_) {
		return;
	}

	// パーティクルの移動と行列更新
	for (uint32_t i = 0; i < kNumParticles; i++) {
		worldTransform_[i].translation_ += CalculateVelocity(i);
		worldTransform_[i].matWorld_ = MakeAffineMatrix(worldTransform_[i].scale_, worldTransform_[i].rotation_, worldTransform_[i].translation_);
		worldTransform_[i].TransferMatrix();
	}

	// カウンターを1フレーム分の秒数で進める
	counter_ += 1.0f / 60.0f;

	// 存続時間の上限に達したら終了扱いにする
	if (counter_ >= kDuration) {
		counter_ = kDuration;
		isFinished_ = true;
		return;
	}

	// 色（透明度）を経過時間に合わせて減衰させる
	color_.w = std::clamp(color_.w - 1.0f / (kDuration * 60.0f), 0.0f, 1.0f);
	objectColor_.SetColor(color_);
}

void DeathParticles::Draw() {
	// 終了していたら描画しない
	if (isFinished_) {
		return;
	}

	for (auto& worldTransform : worldTransform_) {
		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}