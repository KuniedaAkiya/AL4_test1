#pragma once
#include "KamataEngine.h"
#include <algorithm>
using namespace KamataEngine;

/// <summary>
/// フェード
/// </summary>
class Fade {
public:
	// フェードの状態
	enum class Status {
		None,    // フェードなし
		FadeIn,  // フェードイン
		FadeOut, // フェードアウト
	};

	// デストラクタ
	~Fade();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	// フェード開始
	void Start(Status status, float duration);

	// フェード終了判定
	bool IsFinished() const;

	// フェード停止（終了）
	void Stop();

private:
	// フェードイン中の更新処理
	void UpdateFadeIn();

	// フェードアウト中の更新処理
	void UpdateFadeOut();

	// メンバ変数
	Sprite* sprite_ = nullptr;
	uint32_t textureHandle_ = 0;
	Status status_ = Status::None; // 現在のフェードの状態
	float duration_ = 0.0f;        // フェードの持続時間
	float counter_ = 0.0f;         // 経過時間カウンター
};