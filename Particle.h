#pragma once
#include "KamataEngine.h"
#include <vector>

using namespace KamataEngine;

class Particle {
public:
	// パーティクルの初期化用パラメータ構造体
	struct Parameter {
		Vector3 position{};                         // 出現位置
		Vector3 velocity{};                         // 初速
		Vector3 accel{};                            // 加速度（重力など）
		Vector3 rotation{};                         // 初期回転角
		Vector3 angularVelocity{};                  // 回転速度
		Vector3 startScale{1.0f, 1.0f, 1.0f};       // 開始スケール
		Vector3 endScale{0.0f, 0.0f, 0.0f};         // 終了スケール
		Vector4 startColor{1.0f, 1.0f, 1.0f, 1.0f}; // 開始色
		Vector4 endColor{1.0f, 1.0f, 1.0f, 0.0f};   // 終了色
		int32_t lifeTime = 20;                      // 生存フレーム数

		// 個別モデル・テクスチャ指定（指定がない場合はデフォルトモデルを使用）
		Model* customModel = nullptr;
		uint32_t customTextureHandle = 0;
	};

	// ==== パーティクルの種類ごとのパラメータを作る（出す場所・色・数だけ渡す）====
	// 攻撃ヒット時の小さな光（白→黄色）
	static std::vector<Parameter> MakeHit(const Vector3& position);
	// 通常攻撃ヒットの火花（黄白→オレンジ）
	static std::vector<Parameter> MakeHitSpark(const Vector3& position);
	// プレイヤーの斬撃の光（青）
	static std::vector<Parameter> MakeSlash(const Vector3& position, bool isRightDirection);
	// ボスに当たった時の、少しだけ飛び散る粒（color で染める。軽い粒モデルを使う）
	static std::vector<Parameter> MakeBossHit(const Vector3& position, const Vector4& color);
	// 死亡時の飛散（敵・ボス・プレイヤー共通。色・大きさ・個数だけ変える）
	static std::vector<Parameter> MakeDeath(const Vector3& position, const Vector4& color, float baseScale, int32_t count, Model* model);

	void Initialize(const Parameter& param);
	void Update();
	void Draw();

	// 消滅判定
	bool IsDead() const { return isDead_; }

	static void SetModel(Model* model) { model_ = model; }
	static void SetCamera(Camera* camera) { camera_ = camera; }

private:
	static Model* model_;
	static Camera* camera_;

	WorldTransform worldTransform_;
	ObjectColor objectColor_;

	Parameter param_;
	int32_t currentFrame_ = 0;
	bool isDead_ = false;
};