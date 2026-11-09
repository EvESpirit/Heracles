#pragma once
#include <vector>
#include "entity_admin.h"
#include "combat_replication_cmds.h"

class player_spawner {
public:
	enum class DeferredSpawnState : int {
		IDLE = 0,
		SPAWN_REQUESTED = 1,
		WAITING_FOR_COMPONENTS = 2,
		APPLYING_POST_SPAWN_INIT = 3,
		SPAWN_COMPLETE = 4,
		SPAWN_FAILED_TIMEOUT = 5
	};

	struct spawn_info {
		//Load
		__int64 resource_id = 0;
		__int64 skin_theme_id = 0;
		__int64 heroid = 0;
		//uint32 latest_entid;
		bool load_2f_33 = false;
		bool load_3f = false;
		//value: initData ptr
		std::vector<EntityLoader_Entry> additional_compids{};

		//Modify
		int cam_attach = 0;
		int aim_attach = 0;
		int movement_attach = 0;
		int petbackref_attach = 0;
		int team = -1;
		bool set_health = false;
		bool set_2f_movable = false;
		bool set_localent = false;
		bool set_filterbits_controller = false;
		bool weapon_attach = false;
		bool fixup_player_statescript = false;
		bool set_position = false;
		Vector4 spawn_position{};
		Vector4 spawn_rotation{};
		//bool call_component2_vodoo;
	};

private:
	void modify_ent(spawn_info info, Entity* ent);
	Entity* spawn_ent(spawn_info& info);

public:
	spawn_info controller_info{};
	spawn_info model_info{};
	Entity* controller_ent;
	Entity* model_ent;

	bool use_deferred_spawn = true;
	DeferredSpawnState spawn_state = DeferredSpawnState::IDLE;
	uint32 pending_controller_entid = 0;
	uint32 pending_model_entid = 0;
	spawn_info pending_controller_info{};
	spawn_info pending_model_info{};
	int wait_ticks = 0;
	int ready_after_ticks = 0;
	int pre_init_script_count = 0;
	int post_init_script_count = 0;
	int post_init_has_backref = 0;
	int post_init_is_aim_ent = 0;
	bool diag_c20 = false;
	bool diag_c1f = false;
	bool diag_c12 = false;
	bool diag_c23 = false;
	bool diag_not_instantiating = false;
	bool diag_varbag_ready = false;
	int diag_instance_count = 0;
	static constexpr int MAX_WAIT_TICKS = 300; // ~5 seconds safety timeout

	void template_localPlayer();

	player_spawner() {
		template_localPlayer();
	}

	player_spawner(__int64 model_entid) {
		template_localPlayer();
		model_info.resource_id = model_entid;
	}

	//returns key: controller - value: model
	std::pair<Entity*, Entity*> spawn();
	void tick_deferred_spawn();

	// Applies native lock-on filter bit 0x1 to queued non-local dummy models ~1s after spawn (after overhead UI creation).
	// Called every game tick from UpdateHealthbarVisibility_hook and from tick_deferred_spawn().
	static void queue_lockon_bit(uint32 model_entid);
	static void tick_pending_lockon_bits();

	struct DummySpawnRecord {
		uint32 controller_entid     = 0;
		uint32 model_entid          = 0;
		uint64_t heroid             = 0;
		uint64_t model_res_id       = 0;
		uint64_t skin_theme_id      = 0;
		int team                    = 1;
		Vector4 spawn_position      = {};
		Vector4 spawn_rotation      = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool is_dead                = false;
		bool killfeed_emitted       = false;
		uint64_t death_timestamp_ms = 0;
		uint64_t respawn_due_ms     = 0;
		bool respawn_queued         = false;
	};

	// Dummy Corpse Cleanup & Timed Auto-Respawn API (Server-Command Ready, Phase 7 Stage 4A)
	static inline bool&  s_auto_respawn_dummies    = CombatReplication::s_enable_dummy_auto_respawn;
	static inline float& s_dummy_respawn_delay_sec = CombatReplication::s_dummy_respawn_delay_sec;

	static void register_or_update_dummy_record(const DummySpawnRecord& rec);
	static bool get_dummy_record(uint32 model_or_ctrl_entid, DummySpawnRecord& out_rec);
	static void update_dummy_saved_transform(uint32 model_entid, const Vector4& pos, const Vector4& rot);
	static void update_dummy_saved_team(uint32 model_entid, int team);
	static void mark_dummy_killfeed_emitted(uint32 model_entid);
	static bool notify_dummy_death(uint32 ctrl_entid, uint32 model_entid, uint32_t respawn_delay_ms);
	static bool safe_cleanup_dummy_corpse(EntityAdminBase* ea, uint32 ctrl_entid, uint32 model_entid, bool remove_record);
	static std::pair<Entity*, Entity*> respawn_dummy_from_record(EntityAdminBase* ea, const DummySpawnRecord& rec, bool cleanup_corpse_first = true, bool queue_lockon = true);
	static bool execute_dummy_respawn_command(EntityAdminBase* ea, const CombatReplication::ServerEntityRespawnCommand& rcmd);
	static void queue_corpse_cleanup_command(const CombatReplication::ServerCorpseCleanupCommand& ccmd);
	static void process_pending_dummy_operations(EntityAdminBase* ea);
	static void tick_dummy_respawns(EntityAdminBase* ea);
	static void respawn_all_dead_dummies_now(EntityAdminBase* ea);

	// Hero & World Entity Skin Theme Registry (Dedicated Server & F1 Skin Switcher Compatible)
	static uint64_t get_classic_hero_skin_theme(uint64_t heroid);
	static uint64_t get_hero_skin_theme_by_slot(uint64_t heroid, int slot_index);
	static void set_preferred_hero_skin_theme(uint64_t heroid, uint64_t skin_theme_id);
	static uint64_t get_preferred_hero_skin_theme(uint64_t heroid);
	static void set_entity_skin_theme(uint32 entid, uint64_t skin_theme_id);
	static uint64_t get_entity_skin_theme(uint32 ctrl_entid, uint32 mdl_entid, uint64_t fallback_heroid = 0);
	static void cleanup_model_child_slots(EntityAdminBase* ea, Entity* m_ent);
};
