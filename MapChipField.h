#pragma once
#include "KamataEngine.h"
#include <string>
#include <vector>

using namespace KamataEngine;

enum class MapChipType {
	kBlank,       // 空白
	kBlock,       // ブロック
	kPlayerSpawn, // プレイヤー出現位置
	kBossSpawn,   // ボス出現位置
	kWalkEnemy,   // 歩く敵
	kTeleport,    // ★追加：ボス部屋への転送
};

class MapChipField {
public:
	// インデックスセット
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	// 範囲矩形
	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

	// 1ブロックのサイズ
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;

	// ブロックの個数
	static inline const uint32_t kNumBlockVertical = 20;
	static inline const uint32_t kNumBlockHorizontal = 250;

	// マップチップの読み込み
	void LoadMapChipData(const std::string& filePath);

	// インデックスからマップチップのタイプを取得
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	// インデックスからマップチップのワールド座標を取得
	KamataEngine::Vector3 GetMapChipPositionByIndex(int32_t xIndex, int32_t yIndex);

	// 座標によって、マップチップのインデックスを取得する
	IndexSet GetMapChipIndexByPosition(const Vector3& position);

	// インデックスから範囲矩形を取得
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	// 指定した種類のマップチップを探して、最初に見つかった位置を返す（無ければ false）
	bool FindMapChipIndex(MapChipType type, IndexSet& outIndexSet);

	// 指定した種類のマップチップを、全部探して位置の一覧で返す
	std::vector<IndexSet> FindAllMapChipIndex(MapChipType type);

private:
	// マップチップデータ構造体
	struct MapChipData {
		std::vector<std::vector<MapChipType>> data;
	};

	// マップチップデータ
	MapChipData mapChipData_;

	// マップチップデータをリセット
	void ResetMapChipData();
};