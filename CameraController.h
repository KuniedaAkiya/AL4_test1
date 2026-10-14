#include "KamataEngine.h"
class Player;
using namespace KamataEngine;
#pragma once

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

	// 初期化
	void Initialize(Camera* camera);

	// 更新
	void Update();

	void Reset();

	// カメラシェイクを要求する
	void RequestShake(ShakeType type);

	// カメラシェイクの更新（追従の状態に関わらず毎フレーム呼ぶ）
	void UpdateShake();

	void SetTarget(Player* target) { target_ = target; }

	void SetMovableArea(Rect Area) { movableArea_ = Area; };

private:
	// カメラ
	Camera* camera_ = nullptr;

	// 追従ターゲット
	Player* target_ = nullptr;

	// 追従対象とカメラの座標の差（オフセット）
	Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};

	// カメラの目標座標
	Vector3 targetPos_ = {};

	// カメラ移動範囲
	Rect movableArea_ = {-10000.0f, 10000.0f, -10000.0f, 10000.0f};

	static inline const float kInterpolationSpeed = 0.3f;
	static inline const float kVelocityBias = 10.0f;
	static inline const Rect margin = {-10.0f, 10.0f, 0.0f, 0.0f};

	// シェイク
	//   前フレームに足した揺れを取り除く（何度呼んでも安全）
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
