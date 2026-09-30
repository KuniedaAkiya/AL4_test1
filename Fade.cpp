#include "Fade.h"
using namespace KamataEngine;

// フェード時間（秒）
const float kFadeDuration = 1.0f;

// デストラクタ
Fade::~Fade() { delete sprite_; }

// 初期化
void Fade::Initialize() {

	textureHandle_ = TextureManager::Load("uvChecker.png");
	// スプライトを生成
	sprite_ = Sprite::Create(textureHandle_, {0, 0});
	// スプライトを画面全体サイズに設定
	sprite_->SetSize(Vector2(1280.0f, 720.0f)); // 画面幅、画面高さ

	// スプライトの色を黒に設定（初期は透明）
	sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));

	// 初期状態
	status_ = Status::None;
	duration_ = 0.0f;
	counter_ = 0.0f;
}

// フェード開始
void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;

	// ★ 開始直後に初期のアルファ値を強制設定（1フレーム目のチラつき・不正描画防止）
	if (sprite_) {
		if (status_ == Status::FadeIn) {
			// フェードイン開始時：真っ黒（Alpha = 1.0）からスタート
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		} else if (status_ == Status::FadeOut) {
			// フェードアウト開始時：透明（Alpha = 0.0）からスタート
			sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

// 更新
void Fade::Update() {
	switch (status_) {
	case Status::None:
		break;

	case Status::FadeIn:
		UpdateFadeIn();
		// ★ フェードインが完了（時間が達した）したら自動停止して描画（Draw）を行わないようにする
		if (counter_ >= duration_) {
			Stop(); // status_ = Status::None に変更
		}
		break;

	case Status::FadeOut:
		UpdateFadeOut();
		break;
	}
}

// フェードイン中の更新処理（黒 → 透明）
void Fade::UpdateFadeIn() {
	// ★ 0除算ガード
	if (duration_ <= 0.0f) {
		counter_ = 0.0f;
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
		return;
	}

	counter_ += 1.0f / 60.0f;
	if (counter_ >= duration_) {
		counter_ = duration_;
	}

	float alpha = std::clamp(1.0f - (counter_ / duration_), 0.0f, 1.0f);
	sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, alpha));
}

// フェードアウト中の更新処理（透明 → 黒）
void Fade::UpdateFadeOut() {
	// ★ 0除算ガード
	if (duration_ <= 0.0f) {
		counter_ = duration_;
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		return;
	}

	counter_ += 1.0f / 60.0f;
	if (counter_ >= duration_) {
		counter_ = duration_;
	}

	float alpha = std::clamp(counter_ / duration_, 0.0f, 1.0f);
	sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, alpha));
}
// フェード終了判定
bool Fade::IsFinished() const {
	// フェード状態による分岐
	switch (status_) {
	case Status::FadeIn:
	case Status::FadeOut:
		if (counter_ >= duration_) {
			return true;
		} else {
			return false;
		}
	}

	return true;
}

// フェード停止
void Fade::Stop() { status_ = Status::None; }

// 描画
void Fade::Draw() {
	if (status_ == Status::None) {
		return;
	}
	if (sprite_) { // ★ ヌルチェックを徹底
		sprite_->Draw();
	}
}