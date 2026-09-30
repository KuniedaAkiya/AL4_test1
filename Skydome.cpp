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
}

void Skydome::Update() {
	// 行列を定数バッファに転送
	skydomeWorldTransform_.TransferMatrix();
}

void Skydome::Draw() {
	// 3Dモデルの描画
	model_->Draw(skydomeWorldTransform_, *camera_);
}