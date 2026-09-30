#include "TutorialScene.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

TutorialScene::~TutorialScene() {
	// ★ シーン破棄時にBGMを停止
	Audio::GetInstance()->StopWave(bgmPlayHandle_);
	Audio::GetInstance()->StopWave(soundHandleBGM_);

	delete playerModel_;
	delete dummyModel_;
	delete blockModel_;
	delete modelSkydome_;
	delete modelAttack_;
	delete player_;
	delete pauseGuideSprite_;
	delete dummy_;
	delete skydome_;
	delete debugCamera_;
	delete mapChipField_;
	delete cameraController_;
	delete fade_;
	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			delete block;
		}
	}
	worldTransformBlocks_.clear();
}

void TutorialScene::Initialize() {
	phase_ = Phase::kFadeIn;

	// ★ BGMの読み込みと再生
	Audio* audio = Audio::GetInstance();
	soundHandleBGM_ = audio->LoadWave("Audio/BGM_Tutorial.mp3");
	bgmPlayHandle_ = audio->PlayWave(soundHandleBGM_, true, 0.4f);

	// テクスチャの読み込み
	playerDataHandle_ = TextureManager::Load("player/player.png");
	dummyDataHandle_ = TextureManager::Load("boss/boss.png");
	blockTextureHandle_ = TextureManager::Load("block/block.png");

	// 3Dモデルの生成
	playerModel_ = Model::CreateFromOBJ("player", true);
	dummyModel_ = Model::CreateFromOBJ("boss", true);
	blockModel_ = Model::CreateFromOBJ("block", true);
	modelSkydome_ = Model::CreateFromOBJ("SkyDome", true);
	modelAttack_ = Model::CreateFromOBJ("hit_effect", true);

	// マップチップフィールドの生成（Play/GameSceneと同じマップを流用）
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipData("Resources/Blocks.csv");

	// インスタンスの生成
	player_ = new Player();
	dummy_ = new Enemy();
	skydome_ = new Skydome();
	debugCamera_ = new DebugCamera(1280, 720);
	fade_ = new Fade();

	// ブロックの生成
	GenerateBlocks();

	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// 自機の初期化
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, kRowIndex);
	player_->Initialize(playerModel_, modelAttack_, &camera_, playerDataHandle_, playerPosition);
	player_->SetmapChipField(mapChipField_);

	// 練習用ダミーの初期化（初期位置のすぐ近く）
	// Enemyをダミーモードで使う：歩行せず、倒されても少し待って復活する
	dummy_->SetTrainingDummy(true);
	Vector3 dummyPosition = mapChipField_->GetMapChipPositionByIndex(kDummyColumnIndex, kRowIndex);
	dummy_->Initialize(dummyModel_, &camera_, dummyDataHandle_, dummyPosition);
	dummy_->SetmapChipField(mapChipField_);

	// 天球
	skydome_->Initialize(modelSkydome_, &camera_);

	// カメラ
	camera_.farZ = 1000.0f;
	camera_.Initialize();

	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_);
	cameraController_->Reset();

	// ポーズメニューの初期化
	pauseMenu_.Initialize();
	isPaused_ = false;
	// ポーズ操作ガイドの初期化
	pauseGuideTextureHandle_ = TextureManager::Load("UI/pauseGuide.png");
	pauseGuideSprite_ = Sprite::Create(pauseGuideTextureHandle_, kPauseGuidePosition);
	pauseGuideSprite_->SetSize(kPauseGuideSize);
}

void TutorialScene::Update() {
	// ポーズを開く（Pキー）。ポーズ中のPの扱い（再開/メニューに戻る）はPauseMenu側に任せる
	if (!isPaused_ && Input::GetInstance()->TriggerKey(DIK_P)) {
		isPaused_ = true;
		pauseMenu_.ResetSelection();
	}

	if (isPaused_) {
		// 矢印キーでの選択・SPACEでの決定を処理する
		PauseMenu::Action action = pauseMenu_.Update(isPaused_);
		if (action == PauseMenu::Action::kToTitle) {
			nextScene_ = IScene::Next::kTitle;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}

		if (isPaused_) {
			// まだメニュー操作中なら、これ以上更新しない
			return;
		}
	}

	// チュートリアルが面倒なプレイヤー向けのスキップ（BackSpaceキー）
	if (phase_ == Phase::kMain && Input::GetInstance()->TriggerKey(DIK_BACK)) {
		nextScene_ = IScene::Next::kPlay;
		fade_->Start(Fade::Status::FadeOut, 1.0f);
		phase_ = Phase::kFadeOut;
	}

	skydome_->Update();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
			fade_->Stop();
		}
		cameraController_->Update();
		break;

	case Phase::kMain: {
		player_->Update();
		dummy_->Update();

		// 自機とダミーの当たり判定（ダミーは反撃してこないので、殴られた時の反応だけでよい）
		if (!dummy_->IsCollisionDisabled()) {
			AABB aabbPlayer = player_->GetAABB();
			AABB aabbDummy = dummy_->GetAABB();
			if (IsCollisionAABB(aabbPlayer, aabbDummy)) {
				dummy_->OnCollision(player_);
			}
		}

		// ダミーを一度攻撃したら（IsCollisionDisabled()がtrueになったら）Playシーンへ
		if (dummy_->IsCollisionDisabled()) {
			nextScene_ = IScene::Next::kPlay;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}

		cameraController_->Update();
		break;
	}

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	UpdateDebugCamera();
	UpdateBlocks();
}

void TutorialScene::UpdateDebugCamera() {
#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}
}

void TutorialScene::UpdateBlocks() {
	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			if (!block) {
				continue;
			}
			block->matWorld_ = MakeAffineMatrix(block->scale_, block->rotation_, block->translation_);
			block->TransferMatrix();
		}
	}
}

void TutorialScene::GenerateBlocks() {
	uint32_t numBlockVertical = MapChipField::kNumBlockVertical;
	uint32_t numBlockHorizontal = MapChipField::kNumBlockHorizontal;

	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; i++) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	for (uint32_t i = 0; i < numBlockVertical; i++) {
		for (uint32_t j = 0; j < numBlockHorizontal; j++) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

void TutorialScene::Draw() {
	Model::PreDraw();

	skydome_->Draw();

	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			if (!block) {
				continue;
			}
			blockModel_->Draw(*block, camera_, blockTextureHandle_);
		}
	}

	player_->Draw();
	dummy_->Draw();

	Model::PostDraw();

	Sprite::PreDraw();
	player_->DrawUI();
	fade_->Draw();
	// ポーズメニュー描画
	if (isPaused_) {
		pauseMenu_.Draw();
	}
	if (!isPaused_ && pauseGuideSprite_) {
		pauseGuideSprite_->Draw();
	}
	Sprite::PostDraw();
}