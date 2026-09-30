#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

/// <summary>
/// ポーズ中に表示する、矢印キーで選択するメニュー。
/// 「ゲームを再開」「操作説明」「タイトルに戻る」の3項目を持つ。
/// GameScene / TutorialScene の両方から共通で使う。
/// </summary>
class PauseMenu {
public:
	// このフレームで選ばれた結果
	enum class Action {
		kNone,    // まだ選ばれていない（操作中）
		kResume,  // 「ゲームを再開」が選ばれた（またはESCで即再開）
		kToTitle, // 「タイトルに戻る」が選ばれた
	};

	~PauseMenu();

	// 初期化
	void Initialize();

	// ポーズが開いた瞬間に呼ぶ（選択位置・サブ画面をリセットする）
	void ResetSelection();

	// isPausedがtrueの間、毎フレーム呼び出す。
	// 「再開」「タイトルへ戻る」が選ばれた時、isPausedをfalseに書き換えて返す。
	Action Update(bool& isPaused);

	// 描画
	void Draw();

private:
	// メニューのサブ状態
	enum class SubState {
		kMenu,     // PAUSE.png（3項目から選択中）
		kControls, // Guide.png（操作説明）を表示中
	};
	SubState subState_ = SubState::kMenu;

	// ポーズを開いた直後の1フレームだけtrue。
	// ポーズを開くキー（P）と、メニュー内で再開するキー（P）が同じなので、
	// 開いたその同じフレームで「開く」と「再開する」を両方検知してしまい、
	// 一瞬で閉じてしまう不具合を防ぐためのガード。
	bool justOpened_ = false;

	// 選択中の項目インデックス（0:ゲームを再開 1:操作説明 2:タイトルに戻る）
	int selectedIndex_ = 0;
	static constexpr int kOptionCount = 3;

	// ポーズメニュー本体
	Sprite* pauseSprite_ = nullptr;
	// 操作説明画面
	Sprite* guideSprite_ = nullptr;
	// 選択中の項目を指すカーソル（▶）
	Sprite* cursorSprite_ = nullptr;

	static inline const Vector2 kFullScreenSize = {1280.0f, 720.0f};

	// カーソルのX座標
	static inline const float kCursorX = 400.0f;
	// 各項目のカーソルY座標
	static inline const float kCursorY[kOptionCount] = {240.0f, 337.0f, 437.0f};

	// ===== サウンド =====
	uint32_t seSelectHandle_ = 0;
	uint32_t seDecisionHandle_ = 0;
};