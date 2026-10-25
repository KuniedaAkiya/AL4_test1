#pragma once
#include "KamataEngine.h"
class Player;
class Boss;
using namespace KamataEngine;

class CameraController {
public:
	// 矩形
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	// カメラシェイクの種類
	enum class ShakeType {
		kWeak,       // 弱：通常ヒットなど
		kMedium,     // 中：ジャストガード、被ダメージなど
		kStrong,     // 強：カウンター、撃破など
		kContinuous, // 継続：呼んでいる間だけ揺れる（毎フレーム呼ぶ）
	};

	// カメラの動き方
	enum class Mode {
		kFollow,     // 通常：プレイヤーをそのままスクロール追従（ボス戦中もこれ）
		kBossIntro,  // ボス登場演出：ボスへ寄って、戻る
		kBossDefeat, // ボス撃破演出：ボスに近づく
	};

	// 初期化
	void Initialize(Camera* camera);

	// 更新
	void Update();

	// 位置を直接合わせる（テレポート・開始時用。補間なし）
	void Reset();

	// カメラシェイクを要求する
	void RequestShake(ShakeType type);

	// カメラシェイクの更新（追従の状態に関わらず毎フレーム呼ぶ）
	void UpdateShake();

	void SetTarget(Player* target) { target_ = target; }
	void SetBossTarget(const Boss* boss) { boss_ = boss; }

	// カメラを引く要求。同じフレームに複数あれば、一番引くものを採用する
	// このフレームだけ有効（範囲の中にいる間、毎フレーム呼ぶ）
	void RequestZoom(float distance, float speed = kZoomSpeed);
	// 着地するまで有効（ジャンプ台などで呼ぶ。一度空中に出てから着地すると解除）
	void RequestZoomUntilLanding(float distance, float speed = kZoomSpeed);

	// ボス登場演出を始める
	void StartBossIntro();
	// ボス登場演出が終わったか（演出中でなければ true）
	bool IsBossIntroFinished() const { return mode_ != Mode::kBossIntro; }

	// 移動範囲（「通常の距離」でのカメラ中心の範囲。寄った/引いた分は内部で自動補正する）
	void SetMovableArea(Rect Area) { movableArea_ = Area; };

private:
	void UpdateFollow();
	void UpdateBossIntro();
	void UpdateBossDefeat();
	// 今の距離に合わせて、移動範囲を補正して返す
	Rect GetAdjustedArea() const;

	// カメラ
	Camera* camera_ = nullptr;

	// 追従ターゲット
	Player* target_ = nullptr;
	const Boss* boss_ = nullptr;

	Mode mode_ = Mode::kFollow;

	// カメラ移動範囲
	Rect movableArea_ = {-10000.0f, 10000.0f, -10000.0f, 10000.0f};

	// ==== 距離（ズーム） ====
	static inline const float kNormalDistance = 15.0f;
	static inline const float kZoomSpeed = 0.1f; // 通常距離に戻る速さ
	// 距離1あたりの、画面の半分の大きさ（視野角45度・16:9）。カメラ設定を変えたらここも直す
	static inline const float kHalfHeightPerDistance = 0.41421356f; // tan(22.5度)
	static inline const float kHalfWidthPerDistance = 0.41421356f * 16.0f / 9.0f;

	// ==== 通常の追従 ====
	static inline const float kFollowSpeedX = 0.2f;  // 横の追従の速さ
	static inline const float kFollowSpeedY = 0.15f; // 縦の追従の速さ
	static inline const float kSafeMarginX = 8.0f;   // プレイヤーが画面外に出ないための最大ずれ
	static inline const float kSafeMarginY = 5.0f;
	static inline const float kZoomEdgeMargin = 1.0f; // 引ける距離の上限から、さらに手前に余裕を持たせる量
	// ジャンプ台：打ち上がった時だけ、プレイヤーより上を多く映す

	// ==== ボス登場演出 ====
	static inline const float kIntroMoveTime = 1.2f;   // ボスへ寄っていく時間（秒）
	static inline const float kIntroHoldTime = 1.0f;   // ボスを映している時間（秒）
	static inline const float kIntroReturnTime = 0.9f; // プレイヤーへ戻る時間（秒）
	static inline const float kIntroDistance = 8.0f;   // ボスを映す時の距離（小さいほど寄る）
	static inline const float kBossFocusOffsetY = 0.5f;

	// ==== ボス撃破演出 ====
	static inline const float kDefeatDistance = 9.0f;     // 撃破時の距離
	static inline const float kDefeatZoomSpeed = 0.04f;   // 寄る速さ
	static inline const float kDefeatFollowSpeed = 0.08f; // ボスへ向く速さ

	float distance_ = kNormalDistance;      // 今のカメラ距離
	float startDistance_ = kNormalDistance; // ボス演出開始時のカメラ距離
	float leadY_ = 0.0f;                    // 縦の先読み量（ジャンプ台の時だけ上）
	bool isLookUp_ = false;                 // 打ち上がり中か
	float groundedY_ = 0.0f;                // カメラの縦の基準にする高さ
	float introTimer_ = 0.0f;
	bool introRoared_ = false; // 登場演出の揺れを出したか

	// ズーム要求（距離が大きいほど引く）
	struct ZoomRequest {
		float distance = kNormalDistance;
		float speed = kZoomSpeed;
	};
	ZoomRequest zoomRequest_;          // 今フレームの要求
	ZoomRequest landingZoom_;          // 着地まで有効な要求
	bool hasLandingZoom_ = false;      // 着地までのズームが有効か
	bool landingZoomAirborne_ = false; // その間に一度空中に出たか

	// シェイク
	//   前フレームに足した揺れを取り除く
	void RemoveShake();

	// ==== カメラシェイク ====
	struct ShakeParam {
		float power;
		int32_t frames;
	};
	// 弱・中・強（ShakeType の順）
	static inline const ShakeParam kShakeParams[3] = {
	    {0.10f, 6 },
        {0.25f, 12},
        {0.50f, 20}
    };
	// 継続の強さ
	static inline const float kContinuousShakePower = 0.10f;

	float shakePower_ = 0.0f;
	int32_t shakeFrames_ = 0;
	int32_t shakeTotalFrames_ = 0;
	bool continuousRequested_ = false; // 今フレームに継続が要求されたか
	Vector3 appliedShake_{};           // 今カメラに足している揺れ量
};