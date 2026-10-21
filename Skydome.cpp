#include "Skydome.h"
#include <cassert>
using namespace KamataEngine;

// デストラクタ
Skydome::~Skydome() {}

void Skydome::Initialize(Model* model, Camera* camera) {
	// NULLポインタチェック
	assert(model);

	model_ = model;
	camera_ = camera;

	// ワールドトランスフォームの初期化
	skydomeWorldTransform_.Initialize();

	// 色の初期化（最初は白＝元の色のまま）
	objectColor_.Initialize();
	currentColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	targetColor_ = currentColor_;
	objectColor_.SetColor(currentColor_);
}

void Skydome::Update() {
	// 色を目標の色へ少しずつ近づける
	currentColor_.x += (targetColor_.x - currentColor_.x) * kColorChangeRate;
	currentColor_.y += (targetColor_.y - currentColor_.y) * kColorChangeRate;
	currentColor_.z += (targetColor_.z - currentColor_.z) * kColorChangeRate;
	currentColor_.w += (targetColor_.w - currentColor_.w) * kColorChangeRate;
	objectColor_.SetColor(currentColor_);

	// 行列を定数バッファに転送
	skydomeWorldTransform_.TransferMatrix();
}

void Skydome::Draw() {
	// 3Dモデルの描画
	model_->Draw(skydomeWorldTransform_, *camera_, &objectColor_);
}