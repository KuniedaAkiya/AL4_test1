#include "Stage.h"
#include "MatrixFunction.h"
#include <algorithm>

Stage::~Stage() {
	for (std::vector<WorldTransform*>& line : worldTransformBlocks_) {
		for (WorldTransform* block : line) {
			delete block;
		}
	}
	worldTransformBlocks_.clear();
	delete blockModel_;
	delete spikeModel_;
	delete jumpPadModel_;
	delete teleportModel_;
	delete mapChipField_;
}

void Stage::Initialize(const std::string& csvPath) {
	blockModel_ = Model::CreateFromOBJ("block", true);
	blockTextureHandle_ = TextureManager::Load("block/block.png");

	spikeModel_ = Model::CreateFromOBJ("spike", true);
	spikeTextureHandle_ = TextureManager::Load("spike/spike.png");
	jumpPadModel_ = Model::CreateFromOBJ("jumpPad", true);
	jumpPadTextureHandle_ = TextureManager::Load("jumpPad/jumpPad.png");
	teleportModel_ = Model::CreateFromOBJ("teleport", true);
	teleportTextureHandle_ = TextureManager::Load("teleport/teleport.png");

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
			} else if (type == MapChipType::kSpike) {
				// 棘：マスの底（bottom）から高さ kSpikeHitHeight 分の AABB を作成
				const Vector3 center = mapChipField_->GetMapChipPositionByIndex(j, i);
				const float bottom = center.y - MapChipField::kBlockHeight / 2.0f;

				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = center;
				worldTransformBlocks_[i][j] = worldTransform;

				AABB hit;
				hit.min = {center.x - kSpikeHitHalfWidth, bottom, center.z - 0.5f};
				hit.max = {center.x + kSpikeHitHalfWidth, bottom + kSpikeHitHeight, center.z + 0.5f};
				spikeAABBs_.push_back(hit);
			} else if (type == MapChipType::kJumpPad) {
				// ジャンプ台：薄い板を、マスの底に置く
				const Vector3 center = mapChipField_->GetMapChipPositionByIndex(j, i);
				const float bottom = center.y - MapChipField::kBlockHeight / 2.0f;

				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = center;

				// 初期化パラメータの回転角を代入
				worldTransform->rotation_ = kJumpPadRotation;

				worldTransformBlocks_[i][j] = worldTransform;

				AABB hit;
				hit.min = {center.x - kJumpPadHitHalfWidth, bottom, center.z - 0.5f};
				hit.max = {center.x + kJumpPadHitHalfWidth, bottom + kJumpPadHeight, center.z + 0.5f};
				jumpPadAABBs_.push_back(hit);
			}
		}
	}
	// ブロックは動かないので、行列は最初に1回だけ作る
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

void Stage::Update() {}

void Stage::Draw(const Camera& camera) {
	// 画面に映るブロックだけ描画
	const int32_t center = static_cast<int32_t>(camera.translation_.x / MapChipField::kBlockWidth + 0.5f);
	const int32_t first = (std::max)(0, center - kDrawColumnRange);
	const int32_t last = (std::min)(static_cast<int32_t>(MapChipField::kNumBlockHorizontal) - 1, center + kDrawColumnRange);

	for (uint32_t i = 0; i < worldTransformBlocks_.size(); i++) {
		for (int32_t j = first; j <= last; j++) {
			WorldTransform* block = worldTransformBlocks_[i][j];
			if (!block) {
				continue;
			}

			// チップの種類によって、モデルを使い分ける
			const MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kSpike) {
				spikeModel_->Draw(*block, camera, spikeTextureHandle_);
			} else if (type == MapChipType::kJumpPad) {
				jumpPadModel_->Draw(*block, camera, jumpPadTextureHandle_);
			} else if (type == MapChipType::kTeleport) {
				teleportModel_->Draw(*block, camera, teleportTextureHandle_);
			} else {
				blockModel_->Draw(*block, camera, blockTextureHandle_);
			}
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

bool Stage::IsTouchingSpike(const AABB& aabb) const {
	for (const AABB& spike : spikeAABBs_) {
		if (IsCollisionAABB(aabb, spike)) {
			return true;
		}
	}
	return false;
}

bool Stage::IsTouchingJumpPad(const AABB& aabb) const {
	for (const AABB& pad : jumpPadAABBs_) {
		if (IsCollisionAABB(aabb, pad)) {
			return true;
		}
	}
	return false;
}