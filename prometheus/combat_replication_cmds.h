#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include "game.h"

class EntityAdminBase;

// ============================================================================
// CombatReplication — Server-Authoritative Killfeed, Death, & Respawn Protocol
// ============================================================================
// Designed to be 100% ready to receive commands from a standalone Dedicated
// Server over the network (UDP/TCP/state_replicator), while also serving local
// offline sandbox simulation via the exact same command queue.
//
// All command structs (ServerKillfeedCommand, ServerEntityDeathCommand,
// ServerEntityRespawnCommand) are fixed-layout, trivially copyable POD packets
// with no pointers, ready for direct wire serialization.
// ============================================================================
namespace CombatReplication {

enum KillFlags : uint8_t {
    KILL_FLAG_NONE          = 0x00,
    KILL_FLAG_CRIT          = 0x01, // Critical hit / headshot (red crit icon & arrow)
    KILL_FLAG_SOLO_ELIM     = 0x02, // Solo elimination (100 fire points)
    KILL_FLAG_ENVIRONMENTAL = 0x04, // Environmental / fall hazard kill
    KILL_FLAG_SELF          = 0x08, // Self-elimination / suicide
    KILL_FLAG_ULTIMATE      = 0x10  // Ultimate ability kill (gold ability badge)
};

static constexpr uint8_t  TEAM_AUTO                     = 0xFF;
static constexpr uint32_t RESPAWN_WAIT_FOR_SERVER_CMD   = 0xFFFFFFFFu;
static constexpr uint32_t DEFAULT_KILLFEED_DURATION_MS  = 5500u;
static constexpr uint32_t DEFAULT_LOCAL_RESPAWN_MS      = 3500u;

#pragma pack(push, 1)

// 1. Server -> Client: Top-Right Killfeed + Optional Bottom-Center Elimination Banner
struct ServerKillfeedCommand {
    uint32_t event_seq              = 0;          // 0 = auto-assign locally; >0 = server sequence ID (deduplicated)
    uint32_t killer_entid           = 0;          // 0 or 0xFFFFFFFF = self / environmental kill
    uint64_t killer_heroid          = 0;          // 0x02E0... STUHero ID (0 = auto-resolve from killer_entid)
    uint8_t  killer_team            = TEAM_AUTO;  // 0..3 or TEAM_AUTO (auto-resolve from Component_10_FilterBits)
    char     killer_name[64]        = {};         // Explicit display name from server ("" = auto-resolve via +0x1027940)

    uint32_t victim_entid           = 0;          // Victim controller or model entity ID
    uint64_t victim_heroid          = 0;          // 0x02E0... STUHero ID (0 = auto-resolve from victim_entid)
    uint8_t  victim_team            = TEAM_AUTO;  // 0..3 or TEAM_AUTO (auto-resolve from Component_10_FilterBits)
    char     victim_name[64]        = {};         // Explicit display name from server ("" = auto-resolve via +0x1027940)

    uint64_t ability_script_id      = 0;          // 0x0580... ability Statescript ID (0 = primary weapon)
    char     ability_label[32]      = {};         // Explicit ability tag (e.g. "PULSE BOMB"; "" = auto-resolve)

    uint8_t  kill_flags             = KILL_FLAG_NONE; // Bitmask of KillFlags
    uint16_t fire_points            = 100;        // Elimination fire points (e.g. 100)
    uint8_t  trigger_native_banner  = 1;          // 1 = if killer is LocalPlayer, also fire +0xCA7EE0 banner & 2D sting
    uint32_t display_duration_ms    = DEFAULT_KILLFEED_DURATION_MS;
};
static_assert(std::is_trivially_copyable_v<ServerKillfeedCommand>, "ServerKillfeedCommand must be trivially copyable POD");

// 2. Server -> Client: Entity Death Transition + Respawn Scheduling
struct ServerEntityDeathCommand {
    uint32_t event_seq              = 0;          // 0 = auto-assign locally; >0 = server sequence ID (deduplicated)
    uint32_t victim_ctrl_entid      = 0;          // 0 = auto-resolve from victim_mdl_entid via Component_1F
    uint32_t victim_mdl_entid       = 0;          // 0 = auto-resolve from victim_ctrl_entid via Component_20
    uint32_t killer_entid           = 0;          // 0 or 0xFFFFFFFF = self / environmental
    uint64_t ability_script_id      = 0;          // 0x0580... ability script ID
    uint8_t  kill_flags             = KILL_FLAG_NONE;
    char     killer_name[64]        = {};         // Optional server override for "YOU WERE ELIMINATED BY [NAME]"
    uint32_t respawn_delay_ms       = DEFAULT_LOCAL_RESPAWN_MS; // RESPAWN_WAIT_FOR_SERVER_CMD = wait for explicit RespawnCmd
    uint8_t  trigger_death_scream   = 1;          // 1 = play 3D hero death scream (+0xC40350)
    uint8_t  trigger_status_banner  = 1;          // 1 = if LocalPlayer, fire +0xCA7B40 ("YOU HAVE DIED" / "YOU WERE ELIMINATED BY")
    uint8_t  emit_killfeed_entry    = 1;          // 1 = also push a top-right Killfeed entry for this death
};
static_assert(std::is_trivially_copyable_v<ServerEntityDeathCommand>, "ServerEntityDeathCommand must be trivially copyable POD");

// 3. Server -> Client: Spawn-Room / Explicit Transform Respawn Command
struct ServerEntityRespawnCommand {
    uint32_t event_seq                      = 0;  // 0 = auto-assign locally; >0 = server sequence ID (deduplicated)
    uint32_t target_ctrl_entid              = 0;  // 0 = LocalPlayer controller (GameEntityAdmin()->local_entid)
    uint32_t target_mdl_entid               = 0;  // 0 = resolve from controller Component_20
    uint64_t override_heroid                = 0;  // 0 = keep current Component_3F::selected_heroid
    uint64_t override_skin_theme_id         = 0;  // 0 = keep current skin
    uint8_t  has_explicit_spawn_transform   = 0;  // 1 = use spawn_position/spawn_rotation; 0 = resolve Component_2B SpawnRoom
    Vector4  spawn_position                 = {};
    Vector4  spawn_rotation                 = { 0.0f, 0.0f, 0.0f, 1.0f };
};
static_assert(std::is_trivially_copyable_v<ServerEntityRespawnCommand>, "ServerEntityRespawnCommand must be trivially copyable POD");

#pragma pack(pop)

// Runtime Configuration & Telemetry (accessible from ImGui & Network Handlers)
inline bool        s_enable_killfeed_overlay   = true;
inline bool        s_enable_local_auto_respawn = true;
inline float       s_local_respawn_delay_sec   = 3.5f;
inline bool        s_local_is_dead             = false;
inline uint64_t    s_local_death_start_ms      = 0;
inline uint64_t    s_local_respawn_at_ms       = 0;
inline uint32_t    s_total_killfeed_events     = 0;
inline uint32_t    s_total_local_deaths        = 0;
inline uint32_t    s_total_local_respawns      = 0;
inline std::string s_last_spawnroom_status     = "SpawnRoom: waiting for first spawn";

// Thread-safe Command Enqueue API (callable from network thread, raycast worker threads, or main thread)
void EnqueueKillfeedCommand(const ServerKillfeedCommand& cmd);
void EnqueueEntityDeathCommand(const ServerEntityDeathCommand& cmd);
void EnqueueEntityRespawnCommand(const ServerEntityRespawnCommand& cmd);
void ClearKillfeedOverlay();

// Spawn-Room (Component_2B) Transform Resolution & Recording
void RecordInitialLocalSpawnTransform(EntityAdminBase* game_ea, const Vector4& pos, const Vector4& rot, int team_idx);
bool ResolveTeamSpawnRoomTransform(EntityAdminBase* game_ea, int team_idx, Vector4& out_pos, Vector4& out_rot, std::string* out_diag = nullptr);

// Main-Thread Simulation Tick (called from UpdateHealthbarVisibility_hook / PrometheusSystem::OnTick)
void OnGameTick(EntityAdminBase* game_ea, uintptr_t sys46);

// DX11 Present Overlay Renderer (called every frame in PresentHook, even when debug UI is hidden)
void RenderOverlay();

} // namespace CombatReplication
