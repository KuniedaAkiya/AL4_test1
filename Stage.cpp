#include "Stage.h"
#include "MatrixFunction.h"

Stage::~Stage() {
	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			delete block;
		}
	}
	worldTransformBlocks_.clear();
	delete blockModel_;
	delete mapChipField_;
}

void Stage::Initialize(const std::string& csvPath) {
	blockModel_ = Model::CreateFromOBJ("block", true);
	blockTextureHandle_ = TextureManager::Load("block/block.png");

	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipData(csvPath);

	GenerateBlocks();
}

void Stage::GenerateBlocks() {
	const uint32_t numBlockVertical = MapChipField::kNumBlockVertical;
	const uint32_t numBlockHorizontal = MapChipField::kNumBlockHorizontal;

	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; i++) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	for (uint32_t i = 0; i < numBlockVertical; i++) {
		for (uint32_t j = 0; j < numBlockHorizontal; j++) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlock || type == MapChipType::kTeleport) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformBlocks_[i][j] = worldTransform;
			}
		}
	}
}

void Stage::Update() {
	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			if (!block) {
				continue;
			}
			block->matWorld_ = MakeAffineMatrix(block->scale_, block->rotation_, block->translation_);
			block->TransferMatrix();
		}
	}
}

void Stage::Draw(const Camera& camera) {
	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			if (!block) {
				continue;
			}
			blockModel_->Draw(*block, camera, blockTextureHandle_);
		}
	}
}

Vector3 Stage::GetSpawnPosition(MapChipType type, uint32_t defaultX, uint32_t defaultY) {
	MapChipField::IndexSet index{defaultX, defaultY};
	// 見つからなければ、index は既定の値のまま
	mapChipField_->FindMapChipIndex(type, index);
	return mapChipField_->GetMapChipPositionByIndex(static_cast<int32_t>(index.xIndex), static_cast<int32_t>(index.yIndex));
}

Vector3 Stage::GetPlayerSpawnPosition() { return GetSpawnPosition(MapChipType::kPlayerSpawn, kDefaultPlayerSpawnX, kDefaultPlayerSpawnY); }

Vector3 Stage::GetBossSpawnPosition() { return GetSpawnPosition(MapChipType::kBossSpawn, kDefaultBossSpawnX, kDefaultBossSpawnY); }

CameraController::Rect Stage::GetStageCameraArea() const {
	const float w = MapChipField::kBlockWidth;
	CameraController::Rect area;
	area.left = -w / 2.0f + kCameraHalfWidth;                      // 左端の壁の外側が、画面の左端になる位置
	area.right = kDividerColumn * w + w / 2.0f - kCameraHalfWidth; // 分けている壁の外側が、画面の右端になる位置
	area.bottom = kCameraBottom;
	area.top = kCameraTop;
	return area;
}

CameraController::Rect Stage::GetBossRoomCameraArea() const {
	const float w = MapChipField::kBlockWidth;
	const float lastColumn = static_cast<float>(MapChipField::kNumBlockHorizontal - 1);
	CameraController::Rect area;
	area.left = kDividerColumn * w - w / 2.0f + kCameraHalfWidth; // 分けている壁の外側が、画面の左端になる位置
	area.right = lastColumn * w + w / 2.0f - kCameraHalfWidth;    // 右端の壁の外側が、画面の右端になる位置
	area.bottom = kCameraBottom;
	area.top = kCameraTop;
	return area;
}

float Stage::GetBossMinX() const { return kDividerColumn * MapChipField::kBlockWidth + 1.5f * MapChipField::kBlockWidth; }

float Stage::GetBossMaxX() const { return static_cast<float>(MapChipField::kNumBlockHorizontal - 1) * MapChipField::kBlockWidth - 1.5f * MapChipField::kBlockWidth; }

Vector3 Stage::GetTeleportDestination() {
	Vector3 position = GetBossSpawnPosition();
	position.x = GetBossMinX() + kTeleportEntranceOffset;
	return position;
}