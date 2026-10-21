#include "Particle.h"
#include "MatrixFunction.h"
#include <algorithm>
#include <cmath>

Model* Particle::model_ = nullptr;
Camera* Particle::camera_ = nullptr;

namespace {
Vector3 LerpVector3(const Vector3& v1, const Vector3& v2, float t) { return {std::lerp(v1.x, v2.x, t), std::lerp(v1.y, v2.y, t), std::lerp(v1.z, v2.z, t)}; }

Vector4 LerpVector4(const Vector4& v1, const Vector4& v2, float t) { return {std::lerp(v1.x, v2.x, t), std::lerp(v1.y, v2.y, t), std::lerp(v1.z, v2.z, t), std::lerp(v1.w, v2.w, t)}; }
} // namespace

void Particle::Initialize(const Parameter& param) {
	param_ = param;
	currentFrame_ = 0;
	isDead_ = false;

	worldTransform_.Initialize();
	worldTransform_.translation_ = param_.position;
	worldTransform_.rotation_ = param_.rotation;
	worldTransform_.scale_ = param_.startScale;

	objectColor_.Initialize();
	objectColor_.SetColor(param_.startColor);
}

void Particle::Update() {
	if (isDead_)
		return;

	currentFrame_++;
	if (currentFrame_ >= param_.lifeTime) {
		isDead_ = true;
		return;
	}

	float t = static_cast<float>(currentFrame_) / static_cast<float>(param_.lifeTime);

	// 移動・回転の更新
	param_.velocity += param_.accel;
	worldTransform_.translation_ += param_.velocity;
	worldTransform_.rotation_ += param_.angularVelocity;

	// スケール・色の補間
	worldTransform_.scale_ = LerpVector3(param_.startScale, param_.endScale, t);
	Vector4 color = LerpVector4(param_.startColor, param_.endColor, t);
	objectColor_.SetColor(color);

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Particle::Draw() {
	if (isDead_ || !camera_)
		return;

	Model* targetModel = param_.customModel ? param_.customModel : model_;
	if (!targetModel)
		return;

	if (param_.customTextureHandle != 0) {
		targetModel->Draw(worldTransform_, *camera_, param_.customTextureHandle);
	} else {
		targetModel->Draw(worldTransform_, *camera_, &objectColor_);
	}
}