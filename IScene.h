#pragma once

/// <summary>
/// 全シーン共通のインターフェース
/// Title / Tutorial / Play(GameScene) / GameOver / GameClear は
/// すべてこれを継承して実装する。
/// main.cpp側はこの型としてしかシーンを扱わないため、
/// シーンの種類が増えてもmain.cpp側の変更は最小限で済む。
/// </summary>
class IScene {
public:
	// 次に遷移するシーンの種類
	enum class Next {
		kNone,      // 遷移なし（このシーンを継続する）
		kTitle,     // タイトルシーンへ
		kTutorial,  // チュートリアルシーンへ
		kPlay,      // プレイ（ゲーム本編）シーンへ
		kGameOver,  // ゲームオーバーシーンへ
		kGameClear, // ゲームクリアシーンへ
	};

	virtual ~IScene() = default;

	// 初期化
	virtual void Initialize() = 0;

	// 更新
	virtual void Update() = 0;

	// 描画
	virtual void Draw() = 0;

	// このシーンが終了したかどうか
	virtual bool IsFinished() const = 0;

	// 次に遷移するシーンの種類を取得する
	// （IsFinished()がtrueを返すようになったタイミングで意味を持つ）
	virtual Next GetNextScene() const = 0;
};