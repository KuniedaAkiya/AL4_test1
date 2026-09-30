#include "GameOverScene.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

GameOverScene::~GameOverScene() {
	// ★ シーン破棄時にBGMを停止
	Audio::GetInstance()->StopWave(bgmPlayHandle_);
	Audio::GetInstance()->StopWave(bgmGameOverHandle_);

	delete model_;
	delete retryModel_;
	delete titleModel_;
	delete spaceModel_;
	delete fade_;
}

void GameOverScene::Initialize() {
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
	phase_ = Phase::kFadeIn;

	// ★ 音声データの読み込みとBGM再生
	Audio* audio = Audio::GetInstance();
	bgmGameOverHandle_ = audio->LoadWave("Audio/BGM_GameOver.mp3");
	seSelectHandle_ = audio->LoadWave("Audio/SE_Select.mp3");
	seDecisionHandle_ = audio->LoadWave("Audio/SE_Decision.mp3");

	bgmPlayHandle_ = audio->PlayWave(bgmGameOverHandle_, true, 0.4f);

	camera_.Initialize();

	// 3Dモデル読み込み
	model_ = Model::CreateFromOBJ("gameOverFont", true);
	retryModel_ = Model::CreateFromOBJ("reTry", true);      // 「やり直す」
	titleModel_ = Model::CreateFromOBJ("titleGuide", true); // 「タイトルへ」
	spaceModel_ = Model::CreateFromOBJ("decision", true);   // 「スペースで決定」

	// 初期位置設定
	worldTransform_.Initialize();
	worldTransform_.translation_ = kLogoPosition + Vector3{0.0f, kFallDropHeight, 0.0f};

	retryTransform_.Initialize();
	retryTransform_.translation_ = {-12.0f, -13.0f, 0.0f};

	titleTransform_.Initialize();
	titleTransform_.translation_ = {12.0f, -13.0f, 0.0f};

	spaceTransform_.Initialize();
	spaceTransform_.translation_ = {0.0f, -4.5f, -10.0f};

	currentSelect_ = MenuOption::kRetry;
	animTimer_ = 0.0f;
}

void GameOverScene::Update() {
	Input* input = Input::GetInstance();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
			fade_->Stop();
		}
		break;

	case Phase::kMain:
		// 左右キーで「やり直す」と「タイトルへ」を切替
		if (input->TriggerKey(DIK_LEFT) && currentSelect_ != MenuOption::kRetry) {
			currentSelect_ = MenuOption::kRetry;
			Audio::GetInstance()->PlayWave(seSelectHandle_, false); // ★ 選択切り替え音
		} else if (input->TriggerKey(DIK_RIGHT) && currentSelect_ != MenuOption::kTitle) {
			currentSelect_ = MenuOption::kTitle;
			Audio::GetInstance()->PlayWave(seSelectHandle_, false); // ★ 選択切り替え音
		}

		// スペースキーで決定
		if (input->TriggerKey(DIK_SPACE)) {
			Audio::GetInstance()->PlayWave(seDecisionHandle_, false); // ★ 決定音

			if (currentSelect_ == MenuOption::kRetry) {
				nextScene_ = IScene::Next::kPlay;
			} else if (currentSelect_ == MenuOption::kTitle) {
				nextScene_ = IScene::Next::kTitle;
			}
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	// 選ばれている方を大きく(1.2倍)、選ばれていない方を小さく(0.8倍)
	float retryTarget = (currentSelect_ == MenuOption::kRetry) ? 1.2f : 0.8f;
	float titleTarget = (currentSelect_ == MenuOption::kTitle) ? 1.2f : 0.8f;

	retryTransform_.scale_ = {retryTarget, retryTarget, retryTarget};
	titleTransform_.scale_ = {titleTarget, titleTarget, titleTarget};

	// ===== ロゴの落下演出 =====
	if (phase_ != Phase::kFadeIn) {
		animTimer_ += 1.0f / 60.0f;
	}

	if (animTimer_ < kFallDuration) {
		float t = animTimer_ / kFallDuration;
		worldTransform_.translation_.y = EaseIn(kLogoPosition.y + kFallDropHeight, kLogoPosition.y, t);
		worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	} else {
		worldTransform_.translation_.y = kLogoPosition.y;
		float squashT = animTimer_ - kFallDuration;
		if (squashT < kSquashDuration) {
			float s = squashT / kSquashDuration;
			float scaleY = EaseOut(kSquashAmountY, 1.0f, s);
			float scaleXZ = EaseOut(kSquashAmountXZ, 1.0f, s);
			worldTransform_.scale_ = {scaleXZ, scaleY, scaleXZ};
		} else {
			worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
		}
	}

	// 行列更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	retryTransform_.matWorld_ = MakeAffineMatrix(retryTransform_.scale_, retryTransform_.rotation_, retryTransform_.translation_);
	retryTransform_.TransferMatrix();

	titleTransform_.matWorld_ = MakeAffineMatrix(titleTransform_.scale_, titleTransform_.rotation_, titleTransform_.translation_);
	titleTransform_.TransferMatrix();

	spaceTransform_.matWorld_ = MakeAffineMatrix(spaceTransform_.scale_, spaceTransform_.rotation_, spaceTransform_.translation_);
	spaceTransform_.TransferMatrix();
}

void GameOverScene::Draw() {
	Model::PreDraw();

	if (model_)
		model_->Draw(worldTransform_, camera_);
	if (retryModel_)
		retryModel_->Draw(retryTransform_, camera_);
	if (titleModel_)
		titleModel_->Draw(titleTransform_, camera_);
	if (spaceModel_)
		spaceModel_->Draw(spaceTransform_, camera_);

	Model::PostDraw();

	Sprite::PreDraw();
	fade_->Draw();
	Sprite::PostDraw();
}