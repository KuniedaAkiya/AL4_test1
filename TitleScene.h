#pragma once
#include "IScene.h"
#include "KamataEngine.h"
#include "Skydome.h"

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

	// 音声ハンドル
	uint32_t soundHandleBGM_ = 0;
	uint32_t bgmPlayHandle_ = 0; // 再生中BGM停止用

	// カメラ
	Camera camera_{};

	// ===== 背景（天球） =====
	Model* skydomeModel_ = nullptr;
	Skydome* skydome_ = nullptr;
	static inline const Vector4 kSkyColor = {1.25f, 0.95f, 0.95f, 1.0f}; // 夕暮れのピンク紫
	static inline const float kSkyRotationSpeed = 0.0005f;               // 回る速さ（1フレームあたりのラジアン）

	// ===== タイトルの動き =====
	// 経過時間（秒）
	float animTimer_ = 0.0f;
	// ロゴの基準位置（この位置を中心に上下にゆれる）
	static inline const float kTitleBaseY = 2.0f;
	// ロゴのゆれの大きさ
	static inline const float kTitleFloatAmplitude = 0.4f;
	// ロゴのゆれの速さ（ラジアン/秒）
	static inline const float kTitleFloatSpeed = 2.0f;
	// 「スペースで決定」の点滅の速さ（ラジアン/秒）
	static inline const float kBlinkSpeed = 4.0f;
	// 点滅の一番暗い時の透明度
	static inline const float kBlinkMinAlpha = 0.25f;
	// 点滅用の色
	ObjectColor spaceFontColor_;

	// 3Dモデル・トランスフォーム
	Model* titleFontModel_ = nullptr; // タイトルロゴ
	WorldTransform titleFontTransform_{};

	Model* spaceFontModel_ = nullptr; // 「スペースで決定」
	WorldTransform spaceFontTransform_{};
};