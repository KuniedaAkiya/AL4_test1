#pragma once
#include "KamataEngine.h"
#include <array>

using namespace KamataEngine;
/// <summary>
/// デス演出用パーティクル
/// </summary>
class DeathParticles {
public:
	
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void Draw();

	// 終了フラグの取得
	bool IsFinished() const { return isFinished_; }

private:
	// パーティクル1個分の速度ベクトルを計算する
	Vector3 CalculateVelocity(uint32_t index) const;

private:
	// モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;

	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;
	std::array<WorldTransform, kNumParticles> worldTransform_;

	// 存続時間（消滅までの時間）＜秒＞
	static inline const float kDuration = 1.6f;
	// 移動速度の速さ
	static inline const float kSpeed = 0.07f;
	// 分割した1個分の角度（ラジアン）
	static inline const float kAngleUnit = 3.14159265f * 2.0f / static_cast<float>(kNumParticles);

	// 終了フラグ
	bool isFinished_ = false;
	// 経過カウント
	float counter_ = 0.0f;

	// 色変更オブジェクト
	ObjectColor objectColor_;
	// 色の数値
	Vector4 color_;
};