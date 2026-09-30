#include "TitleScene.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

TitleScene::~TitleScene() {
	Audio::GetInstance()->StopWave(soundHandleBGM_);
	delete titleFontModel_;
	delete spaceFontModel_;
}

void TitleScene::Initialize() {
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

	// BGMの読み込みとループ再生（第2引数を true にするとループ)
	soundHandleBGM_ = Audio::GetInstance()->LoadWave("Audio/BGM_Title.mp3");
	Audio::GetInstance()->PlayWave(soundHandleBGM_, true);
}

void TitleScene::Update() {
	// SPACEキーで決定
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		nextScene_ = IScene::Next::kTutorial; // または kPlay
		finished_ = true;
	}

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
		spaceFontModel_->Draw(spaceFontTransform_, camera_);
	}

	Model::PostDraw();
}