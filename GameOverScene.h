#pragma once
#include "Fade.h"
#include "IScene.h"
#include "KamataEngine.h"

using namespace KamataEngine;

class GameOverScene : public IScene {
public:
	enum class Phase { kFadeIn, kMain, kFadeOut };
	enum class MenuOption { kRetry, kTitle }; // 選択肢

	~GameOverScene() override;
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

	// 3Dモデル
	Model* model_ = nullptr;
	Model* retryModel_ = nullptr;
	Model* titleModel_ = nullptr;
	Model* spaceModel_ = nullptr;

	// トランスフォーム
	WorldTransform worldTransform_{};
	WorldTransform retryTransform_{};
	WorldTransform titleTransform_{};
	WorldTransform spaceTransform_{};

	// 選択状態
	MenuOption currentSelect_ = MenuOption::kRetry;

	// ロゴの最終位置
	static inline const Vector3 kLogoPosition = {0.0f, 10.0f, 0.0f};

	// ===== 登場演出 =====
	float animTimer_ = 0.0f;
	static inline const float kFallDuration = 0.5f;
	static inline const float kFallDropHeight = 6.0f;
	static inline const float kSquashDuration = 0.15f;
	static inline const float kSquashAmountY = 0.6f;
	static inline const float kSquashAmountXZ = 1.15f;

	// ===== サウンド =====
	uint32_t bgmGameOverHandle_ = 0;
	uint32_t bgmPlayHandle_ = 0;
	uint32_t seSelectHandle_ = 0;
	uint32_t seDecisionHandle_ = 0;
};