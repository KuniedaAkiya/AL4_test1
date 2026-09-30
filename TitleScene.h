#pragma once
#include "IScene.h"
#include "KamataEngine.h"

using namespace KamataEngine;

class TitleScene : public IScene {
public:
	~TitleScene() override;
	void Initialize() override;
	void Update() override;
	void Draw() override;

	bool IsFinished() const override { return finished_; }
	IScene::Next GetNextScene() const override { return nextScene_; }

private:
	bool finished_ = false;
	IScene::Next nextScene_ = IScene::Next::kNone;

	// 音声ハンドル ★追記
	uint32_t soundHandleBGM_ = 0;

	// カメラ
	Camera camera_{};

	// 3Dモデル・トランスフォーム
	Model* titleFontModel_ = nullptr; // タイトルロゴ
	WorldTransform titleFontTransform_{};

	Model* spaceFontModel_ = nullptr; // 「スペースで決定」
	WorldTransform spaceFontTransform_{};
};