#include "GuardEffect.h"
#include "MatrixFunction.h"
#include <assert.h>
#include <random>

Model* GuardEffect::model_ = nullptr;
Camera* GuardEffect::camera_ = nullptr;

GuardEffect::~GuardEffect() {}

void GuardEffect::Initialize(const Vector3& position) {
	// オブジェクトカラーの初期化
	objectColor_.Initialize();
	objectColor_.SetColor(color_);

	// --- 円の初期設定 ---
	circleWorldTransform_.Initialize();
	circleWorldTransform_.translation_ = position;
	circleWorldTransform_.scale_ = {0.0f, 0.0f, 0.0f}; // 拡大前（サイズ0）

	// --- 乱数生成エンジンの初期化 ---
	std::random_device seedGenerator;
	std::mt19937_64 randomEngine(seedGenerator());
	const float kPi = 3.14159265f;
	std::uniform_real_distribution<float> rotationDistribution(-kPi, kPi);

	// --- 楕円の初期設定 ---
	for (WorldTransform& worldTransform : ellipseWorldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position;
		worldTransform.scale_ = {0.0f, 0.0f, 0.0f}; // 拡大前（サイズ0）
		worldTransform.rotation_ = {0.0f, 0.0f, rotationDistribution(randomEngine)};
	}

	// 状態初期化（発生直後は停止状態からスタート）
	state_ = State::Stop;
	counter_ = 0;
}

void GuardEffect::Update() {
	// 状態に応じた処理・遷移
	switch (state_) {
	case State::Stop: {
		counter_++;

		// 規定フレーム経過したら Spread 状態へ遷移
		if (counter_ >= kStopFrame) {
			state_ = State::Spread;
			counter_ = 0;
		}
		break;
	}

	case State::Spread: {
		counter_++;
		// 進行度 t (0.0 ～ 1.0)
		float t = static_cast<float>(counter_) / static_cast<float>(kSpreadFrame);

		// 円のスケールイージング (0 -> 1.0)
		circleWorldTransform_.scale_ = {t * 1.0f, t * 1.0f, t * 1.0f};

		// 楕円のスケールイージング (0 -> 目標サイズ)
		for (WorldTransform& worldTransform : ellipseWorldTransforms_) {
			worldTransform.scale_ = {t * 0.1f, t * 1.5f, t * 1.0f};
		}

		// 規定フレーム経過したら Fade 状態へ遷移
		if (counter_ >= kSpreadFrame) {
			state_ = State::Fade;
			counter_ = 0;
		}
		break;
	}

	case State::Fade: {
		counter_++;
		// 進行度 t (0.0 ～ 1.0)
		float t = static_cast<float>(counter_) / static_cast<float>(kFadeFrame);

		// アルファ値を 1.0 -> 0.0 へ変化
		color_.w = 1.0f - t;
		objectColor_.SetColor(color_);

		// 規定フレーム経過したら Death 状態へ遷移
		if (counter_ >= kFadeFrame) {
			state_ = State::Death;
		}
		break;
	}

	case State::Death:
		return;
	}

	// --- ワールド行列の更新 ---
	// 円
	circleWorldTransform_.matWorld_ = MakeAffineMatrix(circleWorldTransform_.scale_, circleWorldTransform_.rotation_, circleWorldTransform_.translation_);
	circleWorldTransform_.TransferMatrix();

	// 楕円
	for (WorldTransform& worldTransform : ellipseWorldTransforms_) {
		worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
		worldTransform.TransferMatrix();
	}
}

void GuardEffect::Draw() {
	// Death状態のとき、またはモデル・カメラが設定されていない場合は描画しない
	if (state_ == State::Death || !model_ || !camera_) {
		return;
	}

	// 円の描画（ObjectColorを適用）
	model_->Draw(circleWorldTransform_, *camera_, &objectColor_);

	// 楕円の描画
	for (const WorldTransform& worldTransform : ellipseWorldTransforms_) {
		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}

GuardEffect* GuardEffect::Create(const Vector3& position) {
	GuardEffect* instance = new GuardEffect();
	assert(instance);
	instance->Initialize(position);
	return instance;
}
