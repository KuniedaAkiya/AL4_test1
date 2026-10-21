#include "TitleScene.h"
#include "GameScene.h"
#include "MatrixFunction.h"
#include <cmath>

using namespace KamataEngine;

TitleScene::~TitleScene() {
	Audio::GetInstance()->StopWave(bgmPlayHandle_);
	delete titleFontModel_;
	delete spaceFontModel_;
}

void TitleScene::Initialize() {
	// タイトルに戻ったら、ボス部屋からのやり直し記録を消す（次は最初から）
	// GameScene::ResetCheckpoint();
	GameScene::SetBossCheckpoint(true);
	// カメラの設定
	camera_.farZ = 1000.0f;
	camera_.translation_ = {0.0f, 0.0f, -15.0f};
	camera_.Initialize();

	// 3Dモデルの読み込み
	titleFontModel_ = Model::CreateFromOBJ("titleFont", true);
	spaceFontModel_ = Model::CreateFromOBJ("decision", true); // 「スペースで決定」

	// トランスフォームの初期化
	titleFontTransform_.Initialize();
	titleFontTransform_.translation_ = {0.0f, 2.0f, 5.0f};

	spaceFontTransform_.Initialize();
	spaceFontTransform_.translation_ = {0.0f, -2.5f, 10.0f};

	// 点滅用の色の初期化
	spaceFontColor_.Initialize();
	spaceFontColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	animTimer_ = 0.0f;

	// BGMの読み込みとループ再生（第2引数を true にするとループ)
	soundHandleBGM_ = Audio::GetInstance()->LoadWave("Audio/BGM_Title.mp3");
	bgmPlayHandle_ = Audio::GetInstance()->PlayWave(soundHandleBGM_, true);
}

void TitleScene::Update() {
	// SPACEキーで決定
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		nextScene_ = IScene::Next::kPlay;
		finished_ = true;
	}

	// ===== タイトルの動き =====
	animTimer_ += 1.0f / 60.0f;

	// ロゴをふわふわ上下にゆらす
	titleFontTransform_.translation_.y = kTitleBaseY + std::sin(animTimer_ * kTitleFloatSpeed) * kTitleFloatAmplitude;

	// 「スペースで決定」をゆっくり点滅させる（0.0〜1.0 の波を、最小透明度〜1.0 に変換）
	float wave = (std::sin(animTimer_ * kBlinkSpeed) + 1.0f) * 0.5f;
	float alpha = kBlinkMinAlpha + (1.0f - kBlinkMinAlpha) * wave;
	spaceFontColor_.SetColor({1.0f, 1.0f, 1.0f, alpha});

	// 行列の更新
	titleFontTransform_.matWorld_ = MakeAffineMatrix(titleFontTransform_.scale_, titleFontTransform_.rotation_, titleFontTransform_.translation_);
	titleFontTransform_.TransferMatrix();

	spaceFontTransform_.matWorld_ = MakeAffineMatrix(spaceFontTransform_.scale_, spaceFontTransform_.rotation_, spaceFontTransform_.translation_);
	spaceFontTransform_.TransferMatrix();

	camera_.UpdateMatrix();
}

void TitleScene::Draw() {
	Model::PreDraw();

	if (titleFontModel_) {
		titleFontModel_->Draw(titleFontTransform_, camera_);
	}
	if (spaceFontModel_) {
		spaceFontModel_->Draw(spaceFontTransform_, camera_, &spaceFontColor_);
	}

	Model::PostDraw();
}