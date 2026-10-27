#pragma once
#include "Fade.h"
#include "IScene.h"
#include "KamataEngine.h"
#include "Skydome.h"

using namespace KamataEngine;

class GameClearScene : public IScene {
public:
	enum class Phase { kFadeIn, kMain, kFadeOut };
	enum class MenuOption { kRetry, kTitle };

	~GameClearScene() override;
	void Initialize() override;
	void Update() override;
	void Draw() override;
	bool IsFinished() const override { return finished_; }
	IScene::Next GetNextScene() const override { return nextScene_; }

private:
	bool finished_ = false;
	IScene::Next nextScene_ = IScene::Next::kNone;
	Phase phase_ = Phase::kFadeIn;
	Fade* fade_ = nullptr;
	Camera camera_{};

	// ===== 背景（天球） =====
	Model* skydomeModel_ = nullptr;
	Skydome* skydome_ = nullptr;
	static inline const Vector4 kSkyColor = {0.85f, 1.15f, 1.40f, 1.0f}; // 晴れた青空
	static inline const float kSkyRotationSpeed = 0.0005f;               // 回る速さ（1フレームあたりのラジアン）

	// クリアロゴ
	Model* model_ = nullptr;
	WorldTransform worldTransform_{};

	// 選択項目 ("RetryFont", "ToTitleFont", "SpaceFont")
	Model* retryModel_ = nullptr;
	WorldTransform retryTransform_{};

	Model* titleModel_ = nullptr;
	WorldTransform titleTransform_{};

	Model* spaceModel_ = nullptr;
	WorldTransform spaceTransform_{};

	// 現在の選択状態
	MenuOption currentSelect_ = MenuOption::kRetry;

	float animTimer_ = 0.0f;
	static inline const float kPopDuration = 0.25f;
	static inline const float kSettleDuration = 0.15f;
	static inline const float kPopOvershoot = 1.2f;

	// ===== サウンド =====
	uint32_t bgmGameClearHandle_ = 0;
	uint32_t bgmPlayHandle_ = 0;
	uint32_t seSelectHandle_ = 0;
	uint32_t seDecisionHandle_ = 0;
};