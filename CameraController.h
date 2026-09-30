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
	// 初期化
	void Initialize(Camera* camera);

	// 更新
	void Update();

	void Reset();

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
	Rect movableArea_ = {10.5f, 18.5f, 5.5f, 100.0f};

	static inline const float kInterpolationSpeed = 0.3f;
	static inline const float kVelocityBias = 10.0f;
	static inline const Rect margin = {-10.0f, 10.0f, 0.0f, 0.0f};
};