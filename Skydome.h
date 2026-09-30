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

private:
	// モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;

	// ワールド変換データ
	WorldTransform skydomeWorldTransform_;
};