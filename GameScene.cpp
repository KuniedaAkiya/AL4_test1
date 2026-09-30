#include "GameScene.h"
#include "MatrixFunction.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	// ★ シーン終了（破棄）時に再生中および保持しているBGMを停止
	Audio::GetInstance()->StopWave(bgmPlayHandle_);
	Audio::GetInstance()->StopWave(bgmBossHandle_);

	delete playerModel_;
	delete blockModel_;
	delete boss_;
	delete bossModel_;
	delete deathParticleModel_;
	delete modelAttack_;
	delete player_;
	delete fade_;
	delete pauseGuideSprite_;
	if (deathParticles_) {
		delete deathParticles_;
	}
	for (HitEffect* hitEffect : hitEffects_) {
		delete hitEffect;
	}
	for (GuardEffect* guardEffect : guardEffects_) {
		delete guardEffect;
	}
	delete modelSkydome_;
	delete skydome_;
	delete debugCamera_;
	delete mapChipField_;
	delete cameraController_;
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

void GameScene::Initialize() {
	// フェーズの初期化
	phase_ = Phase::kFadeIn;

	Audio* audio = Audio::GetInstance();

	// 1. サウンドデータの読み込み
	bgmBossHandle_ = audio->LoadWave("Audio/BGM_Boss_Phase1.mp3");

	seBossLandHandle_ = audio->LoadWave("Audio/SE_Boss_Land.mp3");
	seFormChangeHandle_ = audio->LoadWave("Audio/SE_FormChange.mp3");
	seParrySuccessHandle_ = audio->LoadWave("Audio/SE_Parry_Success.mp3");
	sePlayerAttackHandle_ = audio->LoadWave("Audio/SE_Player_Attack.mp3");
	sePlayerCounterHandle_ = audio->LoadWave("Audio/SE_Player_Counter.mp3");
	sePlayerDamageHandle_ = audio->LoadWave("Audio/SE_Player_Damage.mp3");

	// 2. ボス戦フェーズ1のBGMを再生（第3引数は音量 0.0f～1.0f）
	bgmPlayHandle_ = audio->PlayWave(bgmBossHandle_, true, 0.4f);

	// テクスチャの読み込み
	playerDataHandle_ = TextureManager::Load("player/player.png");
	bossDataHandle_ = TextureManager::Load("boss/boss.png");
	blockTextureHandle_ = TextureManager::Load("block/block.png");

	// 3Dモデルの生成
	playerModel_ = Model::CreateFromOBJ("player", true);
	bossModel_ = Model::CreateFromOBJ("boss", true);
	modelSkydome_ = Model::CreateFromOBJ("SkyDome", true);
	blockModel_ = Model::CreateFromOBJ("block", true);
	particleModel_ = Model::CreateFromOBJ("particle", true);
	deathParticleModel_ = Model::CreateFromOBJ("deathParticle", true);
	modelAttack_ = Model::CreateFromOBJ("hit_effect", true);

	// マップチップフィールドの生成
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipData("Resources/Blocks.csv");

	// インスタンスの生成
	player_ = new Player();
	skydome_ = new Skydome();
	debugCamera_ = new DebugCamera(1280, 720);
	fade_ = new Fade;

	// ブロックの生成
	GenerateBlocks();

	// フェーズの初期化
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// Player初期化
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);

	// ★ 引数の受け渡しを修正して modelAttack_ を渡す
	player_->Initialize(playerModel_, modelAttack_, &camera_, playerDataHandle_, playerPosition);
	player_->SetmapChipField(mapChipField_);
	player_->SetGameScene(this);

	// デスパーティクル（仮の生成処理）
	deathParticles_ = new DeathParticles;
	deathParticles_->Initialize(deathParticleModel_, &camera_, playerPosition);

	boss_ = new Boss();
	Vector3 bossPosition = mapChipField_->GetMapChipPositionByIndex(40, 18); // 好きな出現位置に調整
	boss_->Initialize(bossModel_, &camera_, bossDataHandle_, bossPosition);
	boss_->SetmapChipField(mapChipField_);
	boss_->SetGameScene(this);
	boss_->SetTarget(player_);

	// Skydome
	skydome_->Initialize(modelSkydome_, &camera_);

	// Camera
	camera_.farZ = 1000.0f;
	camera_.Initialize();

	// ヒットエフェクト
	HitEffect::SetModel(particleModel_);
	HitEffect::SetCamera(&camera_);

	GuardEffect::SetModel(particleModel_); // 同じパーティクルモデルを流用
	GuardEffect::SetCamera(&camera_);

	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_);
	cameraController_->Reset();

	// カメラの移動範囲を設定
	CameraController::Rect movableArea;

	// ポーズメニューの初期化
	pauseMenu_.Initialize();
	isPaused_ = false;

	// ポーズ操作ガイドの初期化
	pauseGuideTextureHandle_ = TextureManager::Load("UI/pauseGuide.png");
	pauseGuideSprite_ = Sprite::Create(pauseGuideTextureHandle_, kPauseGuidePosition);
	pauseGuideSprite_->SetSize(kPauseGuideSize);
}

void GameScene::Update() {
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
			// まだメニュー操作中（再開もタイトルへ戻るも選ばれていない）なら、これ以上更新しない
			return;
		}
	}

	// フェーズの切り替え判定
	ChangePhase();

	// 天球・敵はフェーズに関わらず（デス演出中も含めて）常に動かす仕様のため、switch文の外で共通更新する
	skydome_->Update();

	// フェーズごとに異なる更新処理
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		// カメラコントローラー
		cameraController_->Update();
		break;

	case Phase::kPlay:
		// 自機
		player_->Update();

		if (boss_) {
			boss_->Update();
		}

		// ヒットエフェクトの更新
		for (HitEffect* hitEffect : hitEffects_) {
			hitEffect->Update();
		}

		// 消滅したヒットエフェクトをリストから削除
		hitEffects_.remove_if([](HitEffect* hitEffect) {
			if (hitEffect->IsDead()) {
				delete hitEffect; // メモリの解放
				return true;      // リストから除外
			}
			return false;
		});

		// ガードエフェクトの更新
		for (GuardEffect* guardEffect : guardEffects_) {
			guardEffect->Update();
		}

		// 消滅したガードエフェクトをリストから削除
		guardEffects_.remove_if([](GuardEffect* guardEffect) {
			if (guardEffect->IsDead()) {
				delete guardEffect;
				return true;
			}
			return false;
		});

		// 全ての当たり判定を行う（ここで衝突したら敵の isDead_ が true になります）
		CheckAllCollisions();

		// カメラコントローラー
		cameraController_->Update();
		break;

	case Phase::kDeath:
		// デスパーティクルの更新
		if (deathParticles_) {
			deathParticles_->Update();
		}
		break;

	case Phase::kClear:
		// クリア演出中も敵などは動かしたくないので、ここでは特に何もしない
		// （演出を追加したくなったらここに実装する）
		break;

	case Phase::kFadeOut:
		fade_->Update();
		break;
	}

	UpdateCommon();
}

void GameScene::UpdateCommon() {
	// デバッグカメラの切り替え・カメラ行列の更新
	UpdateDebugCamera();

	// ブロックのワールド行列更新
	UpdateBlocks();
}

void GameScene::UpdateDebugCamera() {
#ifdef _DEBUG
	// デバッグカメラのオンオフ切り替え（TABキー）
	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		// ビュープロジェクション行列の更新と転送
		camera_.TransferMatrix();
	} else {
		// ビュープロジェクション行列の更新と転送
		camera_.UpdateMatrix();
	}
}

void GameScene::UpdateBlocks() {
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			// 定数バッファに転送する
			worldTransformBlock->TransferMatrix();
		}
	}
}

void GameScene::GenerateBlocks() {
	// 要素数
	uint32_t numBlockVertical = MapChipField::kNumBlockVertical;
	uint32_t numBlockHorizontal = MapChipField::kNumBlockHorizontal;

	// 要素数の変更
	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; i++) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
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

void GameScene::CheckAllCollisions() {
#pragma region 自機と敵の当たり判定
	// 対象1と2の座標
	AABB aabb1, aabb2{};

	// 自機の座標
	aabb1 = player_->GetAABB();

	if (boss_ && !boss_->IsCollisionDisabled()) {
		AABB aabbBoss = boss_->GetAABB();
		if (IsCollisionAABB(aabb1, aabbBoss)) {
			player_->OnCollision(boss_);
			boss_->OnCollision(player_);
		}
	}
#pragma endregion
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = Phase::kPlay;
			fade_->Stop();
		}
		break;

	case Phase::kPlay:
		// 自キャラがデス状態
		if (player_->isDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;

			// 自キャラの座標を取得
			const Vector3& deathParticlesPosition = player_->GetWorldPosition();

			// 自キャラの座標にデスパーティクルを発生、初期化
			deathParticles_->Initialize(deathParticleModel_, &camera_, deathParticlesPosition);
		} else if (isCleared_) {
			// クリア条件を満たしたらクリア演出フェーズへ
			phase_ = Phase::kClear;
			clearTimer_ = 0.0f;
		}
		break;

	case Phase::kDeath:
		// デスパーティクルが有効で、なおかつ演出が終了したら
		if (deathParticles_ && deathParticles_->IsFinished()) {
			nextScene_ = IScene::Next::kGameOver;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kClear:
		// クリア演出を少し見せてからゲームクリアシーンへ
		clearTimer_ += 1.0f / 60.0f;
		if (clearTimer_ >= kClearWaitTime) {
			nextScene_ = IScene::Next::kGameClear;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void GameScene::CreateHitEffect(const Vector3& position) {
	HitEffect* newHitEffect = HitEffect::Create(position);
	hitEffects_.push_back(newHitEffect);
}

void GameScene::CreateGuardEffect(const Vector3& position) {
	GuardEffect* newGuardEffect = GuardEffect::Create(position);
	guardEffects_.push_back(newGuardEffect);
}

void GameScene::Draw() {
	// 3Dモデルの描画前処理
	Model::PreDraw();

	// 天球
	skydome_->Draw();

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			blockModel_->Draw(*worldTransformBlock, camera_, blockTextureHandle_);
		}
	}

	// 自機の描画
	player_->Draw();

	// Boss
	if (boss_) {
		boss_->Draw();
	}

	for (HitEffect* hitEffect : hitEffects_) {
		hitEffect->Draw();
	}

	for (GuardEffect* guardEffect : guardEffects_) {
		guardEffect->Draw();
	}

	// デスパーティクルの描画
	if (deathParticles_ != nullptr) {
		deathParticles_->Draw();
	}

	// 3Dモデルの描画後処理
	Model::PostDraw();

	// フェードの描画
	Sprite::PreDraw();
	if (boss_) {
		boss_->DrawUI();
	}
	player_->DrawUI();
	if (!isPaused_ && pauseGuideSprite_) {
		pauseGuideSprite_->Draw();
	}
	fade_->Draw();
	if (isPaused_) {
		pauseMenu_.Draw();
	}
	Sprite::PostDraw();
}