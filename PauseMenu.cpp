#include "PauseMenu.h"

PauseMenu::~PauseMenu() {
	delete pauseSprite_;
	delete guideSprite_;
	delete cursorSprite_;
}

void PauseMenu::Initialize() {
	// 音声ファイルの読み込み
	Audio* audio = Audio::GetInstance();
	seSelectHandle_ = audio->LoadWave("Audio/SE_Select.mp3");
	seDecisionHandle_ = audio->LoadWave("Audio/SE_Decision.mp3");

	// ポーズメニュー本体（暗転・枠・3項目のテキストまで全部込みの1枚絵）
	uint32_t pauseTextureHandle = TextureManager::Load("UI/PAUSE.png");
	pauseSprite_ = Sprite::Create(pauseTextureHandle, {0, 0});
	pauseSprite_->SetSize(kFullScreenSize);

	// 操作説明画面（同じく1枚絵。ESCで戻る案内もこの中に描かれている）
	uint32_t guideTextureHandle = TextureManager::Load("UI/Guide.png");
	guideSprite_ = Sprite::Create(guideTextureHandle, {0, 0});
	guideSprite_->SetSize(kFullScreenSize);

	// 選択カーソル
	uint32_t cursorTextureHandle = TextureManager::Load("UI/Cursor.png");
	cursorSprite_ = Sprite::Create(cursorTextureHandle, {kCursorX, kCursorY[0]});
	cursorSprite_->SetSize({40.0f, 40.0f});

	ResetSelection();
}

void PauseMenu::ResetSelection() {
	selectedIndex_ = 0;
	subState_ = SubState::kMenu;
	justOpened_ = true; // 開いた直後の1フレームは入力を無視する
	if (cursorSprite_) {
		cursorSprite_->SetPosition({kCursorX, kCursorY[0]});
	}
}

PauseMenu::Action PauseMenu::Update(bool& isPaused) {
	if (!isPaused) {
		return Action::kNone;
	}

	// ポーズを開いたその同じフレームでPキーの入力を再検知して
	// 即座に再開してしまわないよう、最初の1フレームだけ入力判定をスキップする
	if (justOpened_) {
		justOpened_ = false;
		return Action::kNone;
	}

	switch (subState_) {
	case SubState::kMenu: {
		// 上下キーで選択項目を移動（ループさせる）
		if (Input::GetInstance()->TriggerKey(DIK_UP)) {
			selectedIndex_ = (selectedIndex_ - 1 + kOptionCount) % kOptionCount;
			Audio::GetInstance()->PlayWave(seSelectHandle_, false); // ★ 選択音
		}
		if (Input::GetInstance()->TriggerKey(DIK_DOWN)) {
			selectedIndex_ = (selectedIndex_ + 1) % kOptionCount;
			Audio::GetInstance()->PlayWave(seSelectHandle_, false); // ★ 選択音
		}

		// カーソルの位置を選択項目に合わせる
		if (cursorSprite_) {
			cursorSprite_->SetPosition({kCursorX, kCursorY[selectedIndex_]});
		}

		// SPACEキーで決定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			Audio::GetInstance()->PlayWave(seDecisionHandle_, false); // ★ 決定音

			switch (selectedIndex_) {
			case 0: // ゲームを再開
				isPaused = false;
				ResetSelection();
				return Action::kResume;
			case 1: // 操作説明
				subState_ = SubState::kControls;
				break;
			case 2: // タイトルに戻る
				isPaused = false;
				ResetSelection();
				return Action::kToTitle;
			}
		}
		break;
	}

	case SubState::kControls:
		// SPACEキーでポーズ画面（メニュー）に戻る
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			Audio::GetInstance()->PlayWave(seDecisionHandle_, false); // ★ 決定音（またはキャンセル音）
			subState_ = SubState::kMenu;
		}
		break;
	}

	return Action::kNone;
}

void PauseMenu::Draw() {
	if (subState_ == SubState::kMenu) {
		if (pauseSprite_) {
			pauseSprite_->Draw();
		}
		if (cursorSprite_) {
			cursorSprite_->Draw();
		}
	} else {
		if (guideSprite_) {
			guideSprite_->Draw();
		}
	}
}