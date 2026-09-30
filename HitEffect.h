#pragma once
#include "KamataEngine.h"
#include <array>

using namespace KamataEngine;

class HitEffect {
public:
	// --- 状態の定義 ---
	enum class State {
		Spread, // スプレッド（拡大）
		Fade,   // フェード（透明化）
		Death   // デス（削除対象）
	};

	~HitEffect();
	void Initialize(const Vector3& position);
	void Update();
	void Draw();

	// デスフラグの取得（State::Death になったら true を返す）
	bool IsDead() const { return state_ == State::Death; }

	static void SetModel(Model* model) { model_ = model; };
	static void SetCamera(Camera* camera) { camera_ = camera; };
	static HitEffect* Create(const Vector3& position);

private:
	// 楕円の個数
	static constexpr size_t kNumEllipses = 3;

	// 各フェーズのフレーム数（イージングの時間設定）
	static constexpr int kSpreadFrame = 10; // 拡大にかかる時間
	static constexpr int kFadeFrame = 15;   // 消滅にかかる時間

	// モデル・カメラ（借りてくる用）
	static Model* model_;
	static Camera* camera_;

	// ワールドトランスフォーム
	WorldTransform circleWorldTransform_;
	std::array<WorldTransform, kNumEllipses> ellipseWorldTransforms_;

	// 描画色・透明度制御用
	ObjectColor objectColor_;
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// 状態管理変数
	State state_ = State::Spread;
	int counter_ = 0; // フレームカウンタ
};