#pragma once
#include "KamataEngine.h"
using namespace KamataEngine;

class Skydome {
public:
	// デストラクタ
	~Skydome();

	// 初期化
	void Initialize(Model* model, Camera* camera);

	// 更新
	void Update();

	// 描画
	void Draw();

	// 天球の色を、少しずつこの色に近づける（ボス戦で背景を変える時などに使う）
	void SetTargetColor(const Vector4& color) { targetColor_ = color; }

private:
	// モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;

	// ワールド変換データ
	WorldTransform skydomeWorldTransform_;

	// 色の変更用
	ObjectColor objectColor_;
	// 今の色
	Vector4 currentColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	// 近づけたい色
	Vector4 targetColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	// 1フレームで目標の色に近づく割合
	static inline const float kColorChangeRate = 0.03f;
};