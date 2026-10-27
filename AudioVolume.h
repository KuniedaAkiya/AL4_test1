#pragma once
// ============================================================
// 音量の設定（ここだけ見れば全部わかる）
//
// 各値は「音源ファイルの実際の大きさ」を測って決めた補正値です。
//   1.0 = 音源そのまま / 1.0より大きい = 増幅 / 小さい = 絞る
// 目標: BGM は約 -14 LUFS、SE は約 -11 LUFS（SEがBGMより少し前に出る）
//       ただし波形の頭が割れないよう、SEのピークは -2dBFS までに制限
//
// 全体を大きく/小さくしたい時は、下の kBgmMaster / kSeMaster だけ変えてください。
// ============================================================
namespace AudioVolume {

inline constexpr float kBgmMaster = 1.0f; // BGM全体の倍率
inline constexpr float kSeMaster = 1.0f;  // SE全体の倍率

// ---- BGM ----
inline constexpr float kBgmTitle = 0.97f * kBgmMaster;    // BGM_Title
inline constexpr float kBgmStage = 0.55f * kBgmMaster;    // BGM_GameScene（道中）
inline constexpr float kBgmBoss = 0.49f * kBgmMaster;     // BGM_Boss_Phase1
inline constexpr float kBgmGameOver = 0.50f * kBgmMaster; // BGM_GameOver
inline constexpr float kBgmClear = 0.86f * kBgmMaster;    // BGM_Clear

// ---- SE：メニュー ----
inline constexpr float kSeSelect = 1.38f * kSeMaster;   // SE_Select
inline constexpr float kSeDecision = 2.63f * kSeMaster; // SE_Decision

// ---- SE：プレイヤー ----
inline constexpr float kSePlayerJump = 1.70f * kSeMaster;      // SE_Player_Jump（ジャンプ台も同じ）
inline constexpr float kSePlayerLanding = 1.60f * kSeMaster;   // SE_Player_Landing
inline constexpr float kSePlayerAttack = 1.68f * kSeMaster;    // SE_Player_attack（剣を振る音）
inline constexpr float kSePlayerAttackHit = 0.79f * kSeMaster; // SE_Player_Attack_hit（ボスに当たった音）
inline constexpr float kSePlayerCounter = 0.82f * kSeMaster;   // SE_Player_Counter
inline constexpr float kSeJustGuard = 0.79f * kSeMaster;       // SE_Parry_Success
inline constexpr float kSePlayerDamage = 1.76f * kSeMaster;    // SE_Player_Damage
inline constexpr float kSePlayerDead = 2.04f * kSeMaster;      // SE_Player_Dead

// ---- SE：敵・ボス・ステージ ----
inline constexpr float kSeEnemyHit = 1.29f * kSeMaster;   // SE_Enemy_Hit_Damage
inline constexpr float kSeBossLand = 0.68f * kSeMaster;   // SE_Boss_Land
inline constexpr float kSeBossStart = 2.02f * kSeMaster;  // SE_Boss_Start_Sound（登場の咆哮）
inline constexpr float kSeFormChange = 0.81f * kSeMaster; // SE_FormChange（復活）
inline constexpr float kSeHeal = 1.48f * kSeMaster;       // SE_Heal
inline constexpr float kSeTeleport = 2.51f * kSeMaster;   // SE_TP

} // namespace AudioVolume