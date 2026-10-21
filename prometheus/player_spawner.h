#pragma once
#include <vector>
#include "entity_admin.h"

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
};