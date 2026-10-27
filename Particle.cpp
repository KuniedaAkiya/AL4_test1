#include "Particle.h"
#include "MatrixFunction.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

Model* Particle::model_ = nullptr;
Camera* Particle::camera_ = nullptr;

namespace {
Vector3 LerpVector3(const Vector3& v1, const Vector3& v2, float t) { return {std::lerp(v1.x, v2.x, t), std::lerp(v1.y, v2.y, t), std::lerp(v1.z, v2.z, t)}; }

Vector4 LerpVector4(const Vector4& v1, const Vector4& v2, float t) { return {std::lerp(v1.x, v2.x, t), std::lerp(v1.y, v2.y, t), std::lerp(v1.z, v2.z, t), std::lerp(v1.w, v2.w, t)}; }

// 0.0〜1.0 の乱数
float Rand01() { return static_cast<float>(rand()) / RAND_MAX; }
} // namespace

// ============================================================
// パーティクルの種類ごとのパラメータ
// ============================================================

std::vector<Particle::Parameter> Particle::MakeHit(const Vector3& position) {
	std::vector<Parameter> list;
	for (int i = 0; i < 5; ++i) {
		Parameter param;
		param.position = position;

		float vx = (Rand01() - 0.5f) * 0.2f;
		float vy = (Rand01() - 0.5f) * 0.2f;
		param.velocity = {vx, vy, 0.0f};

		param.startScale = {0.2f, 0.2f, 0.2f};
		param.endScale = {0.0f, 0.0f, 0.0f};
		param.startColor = {1.0f, 1.0f, 1.0f, 1.0f}; // 白
		param.endColor = {1.0f, 0.8f, 0.2f, 0.0f};   // 黄色フェード
		param.lifeTime = 10 + rand() % 5;

		list.push_back(param);
	}
	return list;
}

std::vector<Particle::Parameter> Particle::MakeHitSpark(const Vector3& position) {
	std::vector<Parameter> list;
	for (int i = 0; i < 10; ++i) {
		Parameter param;
		param.position = position;

		float vx = (Rand01() - 0.5f) * 0.35f;
		float vy = (Rand01() - 0.5f) * 0.35f;
		param.velocity = {vx, vy, 0.0f};

		param.startScale = {0.25f, 0.25f, 0.25f};
		param.endScale = {0.0f, 0.0f, 0.0f};
		param.startColor = {1.0f, 1.0f, 0.8f, 1.0f}; // 黄白色
		param.endColor = {1.0f, 0.3f, 0.0f, 0.0f};   // オレンジフェード
		param.lifeTime = 10 + rand() % 6;

		list.push_back(param);
	}
	return list;
}

std::vector<Particle::Parameter> Particle::MakeSlash(const Vector3& position, bool isRightDirection) {
	std::vector<Parameter> list;
	float dir = isRightDirection ? 1.0f : -1.0f;
	for (int i = 0; i < 8; ++i) {
		Parameter param;
		param.position = position;
		param.position.x += dir * 0.3f;

		float angle = (Rand01() - 0.5f) * 1.2f;
		float speed = 0.15f + Rand01() * 0.1f;
		param.velocity = {dir * std::cos(angle) * speed, std::sin(angle) * speed, 0.0f};

		param.startScale = {0.15f, 0.15f, 0.15f};
		param.endScale = {0.0f, 0.0f, 0.0f};
		param.startColor = {0.4f, 0.8f, 1.0f, 1.0f}; // 鮮やかな青光
		param.endColor = {0.0f, 0.2f, 1.0f, 0.0f};
		param.lifeTime = 8 + rand() % 6;

		list.push_back(param);
	}
	return list;
}

std::vector<Particle::Parameter> Particle::MakeBossHit(const Vector3& position, const Vector4& color) {
	std::vector<Parameter> list;
	for (int i = 0; i < 5; ++i) { // 数量は控えめ
		Parameter param;
		param.position = position;

		float vx = (Rand01() - 0.5f) * 0.15f;
		float vy = Rand01() * 0.2f + 0.05f; // やや上向き
		param.velocity = {vx, vy, 0.0f};
		param.accel = {0.0f, -0.012f, 0.0f}; // 重力で落ちる

		param.startScale = {0.15f, 0.15f, 0.15f};
		param.endScale = {0.02f, 0.02f, 0.02f};

		// 色で染めて、だんだん透明になる（モデルは指定しない＝軽い粒モデルを使う）
		param.startColor = color;
		param.endColor = {color.x * 0.5f, color.y * 0.5f, color.z * 0.5f, 0.0f};
		param.lifeTime = 15 + rand() % 8;

		list.push_back(param);
	}
	return list;
}

std::vector<Particle::Parameter> Particle::MakeDeath(const Vector3& position, const Vector4& color, float baseScale, int32_t count, Model* model) {
	const float kTwoPi = 6.2831853f;
	std::vector<Parameter> list;

	for (int32_t i = 0; i < count; ++i) {
		Parameter param;
		param.position = position;

		// 全方向に飛散（少し上向きに偏らせて、重力で落ちる）
		const float angle = Rand01() * kTwoPi;
		const float speed = 0.06f + Rand01() * 0.20f;
		param.velocity = {std::cos(angle) * speed, std::sin(angle) * speed + 0.04f, 0.0f};
		param.accel = {0.0f, -0.01f, 0.0f};

		// ランダムに回転させる
		param.rotation = {Rand01() * kTwoPi, Rand01() * kTwoPi, Rand01() * kTwoPi};
		param.angularVelocity = {(Rand01() - 0.5f) * 0.3f, (Rand01() - 0.5f) * 0.3f, (Rand01() - 0.5f) * 0.3f};

		// 大きさはばらつかせて、最後は消える
		const float scale = baseScale * (0.5f + Rand01());
		param.startScale = {scale, scale, scale};
		param.endScale = {0.0f, 0.0f, 0.0f};

		// キャラの色から、少し暗くしながら透明になる
		param.startColor = color;
		param.endColor = {color.x * 0.5f, color.y * 0.5f, color.z * 0.5f, 0.0f};
		param.lifeTime = 30 + rand() % 20;

		// 専用モデルを使う（テクスチャは指定せず、色で染める）
		param.customModel = model;

		list.push_back(param);
	}
	return list;
}

// ============================================================
// 1つのパーティクルの動き
// ============================================================

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