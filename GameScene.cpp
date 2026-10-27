#include "GameScene.h"
#include "AudioVolume.h"
#include "MatrixFunction.h"
#include <cmath>
#include <cstdlib>

using namespace KamataEngine;

namespace {
// 全部更新して、消えたものを削除する
template<typename T> void UpdateAndRemoveDead(std::list<T*>& list) {
	for (T* item : list) {
		item->Update();
	}
	list.remove_if([](T* item) {
		if (item->IsDead()) {
			delete item;
			return true;
		}
		return false;
	});
}
} // namespace

GameScene::~GameScene() {
	StopBgm();

	delete playerModel_;
	delete boss_;
	delete enemyModel_;
	delete deathParticleModel_;
	delete bossModel_;
	delete particleModel_;
	delete modelAttack_;
	delete player_;
	delete fade_;
	delete pauseGuideSprite_;

	for (GuardEffect* guardEffect : guardEffects_) {
		delete guardEffect;
	}
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	for (Particle* particle : particles_) {
		delete particle;
	}
	delete modelSkydome_;
	delete skydome_;
	delete debugCamera_;
	delete cameraController_;
	delete stage_;
}

void GameScene::Initialize() {
	phase_ = Phase::kFadeIn;

	Audio* audio = Audio::GetInstance();
	bgmStageHandle_ = audio->LoadWave("Audio/BGM_GameScene.mp3");
	bgmBossHandle_ = audio->LoadWave("Audio/BGM_Boss_Phase1.mp3");
	seTeleportHandle_ = audio->LoadWave("Audio/SE_TP.mp3");
	seBossStartHandle_ = audio->LoadWave("Audio/SE_Boss_Start_Sound.mp3");
	seHealHandle_ = audio->LoadWave("Audio/SE_Heal.mp3");

	playerDataHandle_ = TextureManager::Load("player/player.png");
	bossDataHandle_ = TextureManager::Load("boss/boss.png");
	enemyDataHandle_ = TextureManager::Load("enemy/enemy.png");

	playerModel_ = Model::CreateFromOBJ("player", true);
	bossModel_ = Model::CreateFromOBJ("boss", true);
	bossModel_ = Model::CreateFromOBJ("boss", true);
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	deathParticleModel_ = Model::CreateFromOBJ("deathParticle", true);
	modelSkydome_ = Model::CreateFromOBJ("SkyDome", true);
	particleModel_ = Model::CreateFromOBJ("particle", true);
	modelAttack_ = Model::CreateFromOBJ("Sword", true);

	stage_ = new Stage();
	stage_->Initialize("Resources/Blocks.csv");

	player_ = new Player();
	skydome_ = new Skydome();
	debugCamera_ = new DebugCamera(1280, 720);
	fade_ = new Fade;

	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	Vector3 playerPosition = stage_->GetPlayerSpawnPosition();
	if (s_bossCheckpoint_) {
		playerPosition = stage_->GetTeleportDestination();
	}
	player_->Initialize(playerModel_, modelAttack_, &camera_, playerDataHandle_, playerPosition);
	player_->SetmapChipField(stage_->GetMapChipField());
	player_->SetGameScene(this);

	boss_ = new Boss();
	boss_->SetMovableArea(stage_->GetBossMinX(), stage_->GetBossMaxX());
	boss_->Initialize(bossModel_, &camera_, bossDataHandle_, stage_->GetBossSpawnPosition());
	boss_->SetmapChipField(stage_->GetMapChipField());
	boss_->SetGameScene(this);
	boss_->SetTarget(player_);

	MapChipField* mapChipField = stage_->GetMapChipField();
	for (const MapChipField::IndexSet& index : mapChipField->FindAllMapChipIndex(MapChipType::kWalkEnemy)) {
		Vector3 enemyPosition = mapChipField->GetMapChipPositionByIndex(static_cast<int32_t>(index.xIndex), static_cast<int32_t>(index.yIndex));
		enemyPosition.y -= kEnemySpawnYOffset;

		Enemy* enemy = new Enemy();
		enemy->Initialize(enemyModel_, &camera_, enemyDataHandle_, enemyPosition);
		enemy->SetmapChipField(mapChipField);
		enemy->SetGameScene(this);
		enemy->SetPlayer(player_);
		enemies_.push_back(enemy);
	}

	skydome_->Initialize(modelSkydome_, &camera_);

	camera_.farZ = 1000.0f;
	camera_.Initialize();

	// ガードエフェクトとパーティクルの設定
	GuardEffect::SetModel(particleModel_);
	GuardEffect::SetCamera(&camera_);

	Particle::SetModel(particleModel_);
	Particle::SetCamera(&camera_);

	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);
	isInBossRoom_ = s_bossCheckpoint_;
	isBossFightStarted_ = s_bossCheckpoint_;

	// 道中は通常BGM。ボス戦のチェックポイントから始めた時は、最初からボスBGM
	PlayBgm(isInBossRoom_ ? bgmBossHandle_ : bgmStageHandle_);

	if (s_bossCheckpoint_) {
		skydome_->SetTargetColor(kBossFightSkyColor);
	}
	cameraController_->SetMovableArea(s_bossCheckpoint_ ? stage_->GetBossRoomCameraArea() : stage_->GetStageCameraArea());
	cameraController_->SetTarget(player_);
	cameraController_->SetBossTarget(boss_);
	cameraController_->Reset();

	// ★テスト用：チェックポイント開始でも登場演出を見る（確認が終わったら消す）
	if (s_bossCheckpoint_) {
		isBossFightStarted_ = false;
		isBossIntro_ = false;
	}

	player_->SetCameraController(cameraController_);
	boss_->SetCameraController(cameraController_);

	pauseMenu_.Initialize();
	isPaused_ = false;

	pauseGuideTextureHandle_ = TextureManager::Load("UI/pauseGuide.png");
	pauseGuideSprite_ = Sprite::Create(pauseGuideTextureHandle_, kPauseGuidePosition);
	pauseGuideSprite_->SetSize(kPauseGuideSize);
}

void GameScene::Update() {
	if (!isPaused_ && Input::GetInstance()->TriggerKey(DIK_P)) {
		isPaused_ = true;
		pauseMenu_.ResetSelection();
	}

	if (isPaused_) {
		PauseMenu::Action action = pauseMenu_.Update(isPaused_);
		if (action == PauseMenu::Action::kToTitle) {
			nextScene_ = IScene::Next::kTitle;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}

		if (isPaused_) {
			return;
		}
	}

	ChangePhase();

	if (phase_ == Phase::kPlay) {
		if (hitStopFrames_ > 0) {
			--hitStopFrames_;
			UpdateCommon();
			return;
		}

		if (slowFrames_ > 0) {
			--slowFrames_;
			if (slowFrames_ % 2 == 0) {
				UpdateCommon();
				return;
			}
		}
	}

	skydome_->Update();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		cameraController_->Update();
		break;

	case Phase::kPlay:
		// 演出中はプレイヤーを止める
		if (!isBossIntro_) {
			player_->Update();
		}

		if (player_->ConsumeTeleportRequest()) {
			Audio::GetInstance()->PlayWave(seTeleportHandle_, false, AudioVolume::kSeTeleport);
			WorldTransform& wt = player_->GetWorldTransform();
			const Vector3 destination = stage_->GetTeleportDestination();
			wt.translation_.x = destination.x;
			wt.translation_.y = destination.y;
			player_->GetVelocity() = {};

			wt.matWorld_ = MakeAffineMatrix(wt.scale_, wt.rotation_, wt.translation_);
			wt.TransferMatrix();
			cameraController_->SetMovableArea(stage_->GetBossRoomCameraArea());
			cameraController_->Reset();
		}

		UpdateBossRoom();

		if (boss_ && isBossFightStarted_) {
			boss_->Update();
		}

		UpdateAndRemoveDead(enemies_);

		CheckAllCollisions();

		cameraController_->Update();
		break;

	case Phase::kDeath:
		// 死亡演出中もカメラの更新を継続
		if (cameraController_) {
			cameraController_->Update();
		}
		break;

	case Phase::kClear:
		cameraController_->Update();
		break;

	case Phase::kFadeOut:
		fade_->Update();
		break;
	}

	// プレイ中・死亡中を問わず、ガードエフェクトとパーティクルを常に更新・削除する
	UpdateAndRemoveDead(guardEffects_);
	UpdateAndRemoveDead(particles_);

	UpdateCommon();
}

void GameScene::UpdateCommon() {
	cameraController_->UpdateShake();
	UpdateDebugCamera();
	stage_->Update();
}

void GameScene::UpdateDebugCamera() {
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

void GameScene::CheckAllCollisions() {
#pragma region 自機と敵の当たり判定
	AABB aabb1 = player_->GetAABB();

	if (boss_ && !boss_->IsCollisionDisabled()) {
		AABB aabbBoss = boss_->GetAABB();

		if (IsCollisionAABB(aabb1, aabbBoss)) {
			player_->OnCollision(boss_);
			if (player_->IsGuard()) {
				player_->OnGuardHit(aabbBoss);
			}

			if (!player_->IsAttack()) {
				boss_->OnCollision(player_);
			}
		}

		if (player_->IsSwordActive() && !player_->HasAttackHit()) {
			AABB swordAabb = player_->GetSwordAABB();
			if (IsCollisionAABB(swordAabb, aabbBoss)) {
				boss_->OnCollision(player_);
			}
		}
	}
#pragma endregion

#pragma region 自機と敵（雑魚）
	for (Enemy* enemy : enemies_) {
		if (enemy->IsCollisionDisabled()) {
			continue;
		}
		const AABB enemyAabb = enemy->GetAABB();

		if (player_->IsSwordActive() && IsCollisionAABB(player_->GetSwordAABB(), enemyAabb)) {
			enemy->OnCollision(player_);
		}
		if (enemy->IsCollisionDisabled()) {
			continue;
		}

		if (IsCollisionAABB(player_->GetAABB(), enemyAabb)) {
			player_->OnCollision(enemy);
			if (player_->IsGuard()) {
				player_->OnGuardHit(enemyAabb);
			}
			if (!player_->IsAttack()) {
				enemy->OnCollision(player_);
			}
		}
	}
#pragma endregion

#pragma region 棘・ジャンプ台
	const AABB playerAabb = player_->GetAABB();

	if (stage_->IsTouchingSpike(playerAabb)) {
		player_->OnHazardHit();
	}

	const bool touchedJumpPad = stage_->IsTouchingJumpPad(playerAabb);
	if (touchedJumpPad) {
		player_->Launch(kJumpPadPower);
	}
	UpdateCameraZoom(touchedJumpPad);
#pragma endregion
}

// プレイヤーの位置に合わせて、カメラを引く要求を出す
void GameScene::UpdateCameraZoom(bool touchedJumpPad) {
	const Vector3 playerPos = player_->GetWorldPosition();

	for (const CameraZoomZone& zone : kCameraZoomZones) {
		if (playerPos.x < zone.minX || playerPos.x > zone.maxX || playerPos.y < zone.minY || playerPos.y > zone.maxY) {
			continue;
		}

		if (zone.onJumpPad) {
			if (touchedJumpPad) {
				cameraController_->RequestZoomUntilLanding(zone.distance, zone.speed);
			}
		} else {
			cameraController_->RequestZoom(zone.distance, zone.speed);
		}
	}
}

// BGMを切り替える（すでに同じBGMが流れていたら何もしない）
void GameScene::PlayBgm(uint32_t soundHandle) {
	if (hasBgm_ && currentBgmHandle_ == soundHandle) {
		return;
	}
	StopBgm();
	// BGMごとに音量が違う（音源の大きさに合わせて AudioVolume.h で調整）
	const float volume = (soundHandle == bgmBossHandle_) ? AudioVolume::kBgmBoss : AudioVolume::kBgmStage;
	bgmPlayHandle_ = Audio::GetInstance()->PlayWave(soundHandle, true, volume);
	currentBgmHandle_ = soundHandle;
	hasBgm_ = true;
}

// BGMを止める
void GameScene::StopBgm() {
	if (hasBgm_) {
		Audio::GetInstance()->StopWave(bgmPlayHandle_);
		hasBgm_ = false;
	}
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
		if (player_->isDead()) {
			phase_ = Phase::kDeath;
			deathTimer_ = 0.0f; // タイマー初期化
		} else if (isCleared_) {
			phase_ = Phase::kClear;
			clearTimer_ = 0.0f;
			s_bossCheckpoint_ = false;
		}
		break;

	case Phase::kDeath:
		// 死亡演出用に一定時間（1.6秒）待ってからゲームオーバーフェードアウトへ移行
		deathTimer_ += 1.0f / 60.0f;
		if (deathTimer_ >= kDeathWaitTime) {
			nextScene_ = IScene::Next::kGameOver;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kClear:
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

// パーティクルを1つ生成してリストに追加する
void GameScene::AddParticle(const Particle::Parameter& param) {
	Particle* particle = new Particle();
	particle->Initialize(param);
	particles_.push_back(particle);
}

// 攻撃ヒット時の演出（Particleを使用）
void GameScene::CreateHitEffect(const Vector3& position) { AddParticles(Particle::MakeHit(position)); }

// パーティクルをまとめてリストに追加する
void GameScene::AddParticles(const std::vector<Particle::Parameter>& params) {
	for (const Particle::Parameter& param : params) {
		AddParticle(param);
	}
}

void GameScene::CreateGuardEffect(const Vector3& position) { guardEffects_.push_back(GuardEffect::Create(position)); }

void GameScene::UpdateBossRoom() {
	// ボスの撃破演出が始まったら、空を元の青空に戻す
	if (boss_->IsDying()) {
		skydome_->SetTargetColor(kNormalSkyColor);
	}

	const bool inBossRoom = stage_->IsInBossRoom(player_->GetWorldPosition().x);

	if (inBossRoom != isInBossRoom_) {
		isInBossRoom_ = inBossRoom;
		cameraController_->SetMovableArea(inBossRoom ? stage_->GetBossRoomCameraArea() : stage_->GetStageCameraArea());
	}

	// ボス部屋に初めて入った時演出
	if (inBossRoom && !isBossFightStarted_ && !isBossIntro_) {
		skydome_->SetTargetColor(kBossFightSkyColor);
		s_bossCheckpoint_ = true;
		isBossIntro_ = true;
		cameraController_->StartBossIntro();

		// 道中のBGMを止めて、ボスの咆哮を鳴らす
		StopBgm();
		Audio::GetInstance()->PlayWave(seBossStartHandle_, false, AudioVolume::kSeBossStart);
	}

	// 演出が終わったら戦闘開始
	if (isBossIntro_ && cameraController_->IsBossIntroFinished()) {
		isBossIntro_ = false;
		isBossFightStarted_ = true;

		// ムービーが終わった時に、1回だけ全回復する
		player_->HealFull();
		Audio::GetInstance()->PlayWave(seHealHandle_, false, AudioVolume::kSeHeal);

		// 戦闘開始でボスBGM
		PlayBgm(bgmBossHandle_);
	}
}

// プレイヤーの斬撃パーティクル
void GameScene::CreateSlashParticles(const Vector3& position, bool isRightDirection) { AddParticles(Particle::MakeSlash(position, isRightDirection)); }

// 通常攻撃ヒットパーティクル（白〜黄色の鋭い花火）
void GameScene::CreateHitParticles(const Vector3& position) { AddParticles(Particle::MakeHitSpark(position)); }

// ボス攻撃ヒットパーティクル（少しだけスライムの体液が飛び散る）
void GameScene::CreateBossHitParticles(const Vector3& position) { AddParticles(Particle::MakeBossHit(position, kDeathColorBoss)); }

// 死亡パーティクル（敵・ボス・プレイヤーの死亡処理で共有）
void GameScene::CreateDeathParticles(const Vector3& position, const Vector4& color, float baseScale, int32_t particleCount) {
	AddParticles(Particle::MakeDeath(position, color, baseScale, particleCount, deathParticleModel_));
}

void GameScene::Draw() {
	Model::PreDraw();

	skydome_->Draw();
	stage_->Draw(camera_);
	player_->Draw();

	if (boss_) {
		boss_->Draw();
	}

	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}

	for (GuardEffect* guardEffect : guardEffects_) {
		guardEffect->Draw();
	}

	// パーティクルの描画
	for (Particle* particle : particles_) {
		particle->Draw();
	}

	Model::PostDraw();

	Sprite::PreDraw();
	if (boss_ && isBossFightStarted_) {
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