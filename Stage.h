#pragma once
#include "CameraController.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "MatrixFunction.h"
#include <string>
#include <vector>

using namespace KamataEngine;

// ステージ（マップ・ブロックの表示・出現位置・カメラやボスの動ける範囲）をまとめて管理するクラス
class Stage {
public:
	~Stage();

	// 初期化（CSVの読み込みとブロックの生成）
	void Initialize(const std::string& csvPath);
	// 更新（ブロックの行列更新）
	void Update();
	// 描画（Model::PreDraw と Model::PostDraw の間で呼ぶ）
	void Draw(const Camera& camera);

	// マップチップの情報（Player・Boss などの当たり判定用）
	MapChipField* GetMapChipField() { return mapChipField_; }

	// 出現位置（マップの 2 / 3 の場所。無ければ既定の位置）
	Vector3 GetPlayerSpawnPosition();
	Vector3 GetBossSpawnPosition();

	// カメラの動ける範囲 {左端, 右端, 下端, 上端}
	CameraController::Rect GetStageCameraArea() const;
	CameraController::Rect GetBossRoomCameraArea() const;

	// ボスの動ける横の範囲（ボス部屋の中）
	float GetBossMinX() const;
	float GetBossMaxX() const;

	// 指定したX座標が、ボス部屋の中かどうか
	bool IsInBossRoom(float x) const { return x > kDividerColumn * MapChipField::kBlockWidth; }

	// テレポートで移動する先の位置（ボス部屋の入口）
	Vector3 GetTeleportDestination();

	// 棘（ダメージ床）に触れているか
	bool IsTouchingSpike(const AABB& aabb) const;
	// ジャンプ台に触れているか
	bool IsTouchingJumpPad(const AABB& aabb) const;

private:
	void GenerateBlocks();
	// 指定した種類の目印の場所を返す。無ければ既定の位置を返す
	Vector3 GetSpawnPosition(MapChipType type, uint32_t defaultX, uint32_t defaultY);

	// ステージとボス部屋を分ける壁の列番号（CSVの壁の位置と合わせる）
	static inline const uint32_t kDividerColumn = 219;
	// 画面の横幅の半分（カメラの距離と視野角で決まる。カメラを変えたら、ここだけ直す）
	static inline const float kCameraHalfWidth = 11.0f;
	// カメラの動ける縦の範囲
	static inline const float kCameraBottom = 5.5f;
	static inline const float kCameraTop = 100.0f;
	// CSVに目印が無い時の出現位置（列, 行）
	static inline const uint32_t kDefaultPlayerSpawnX = 5;
	static inline const uint32_t kDefaultPlayerSpawnY = 18;
	static inline const uint32_t kDefaultBossSpawnX = 235;
	static inline const uint32_t kDefaultBossSpawnY = 18;
	// テレポートで着く位置の、ボス部屋の左の壁からの距離
	static inline const float kTeleportEntranceOffset = 2.0f;

	// ===== 棘・ジャンプ台 =====
	static inline const int32_t kDrawColumnRange = 28;

	// 棘の当たり判定（新しい 1.0x1.0x1.0 モデルに合わせて調整）
	static inline const float kSpikeHitHalfWidth = 0.4f;
	static inline const float kSpikeHitHeight = 0.8f;

	// ジャンプ台の厚みと当たり判定の半分の幅
	static inline const float kJumpPadHeight = 0.3f;
	static inline const float kJumpPadHitHalfWidth = 0.5f;

	// 当たり判定の箱（GenerateBlocks で作る）
	std::vector<AABB> spikeAABBs_;
	std::vector<AABB> jumpPadAABBs_;

	MapChipField* mapChipField_ = nullptr;
	Model* blockModel_ = nullptr;
	uint32_t blockTextureHandle_ = 0;
	Model* spikeModel_ = nullptr;
	uint32_t spikeTextureHandle_ = 0;
	Model* jumpPadModel_ = nullptr;
	uint32_t jumpPadTextureHandle_ = 0;
	Model* teleportModel_ = nullptr;
	uint32_t teleportTextureHandle_ = 0;
	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;
};