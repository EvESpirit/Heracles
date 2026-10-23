#include "player_spawner.h"
#include "window_manager/window_manager.h"
#include "stu_resources.h"
#include "Statescript.h"
#include "StatescriptVar.h"
#include "MovementState.h"
#include <cstring>

void Component_28_STUHealthComponent::InitHeroHealthPools(uint64_t heroid, uint64_t model_res_id) {
	base.is_mirrored = 1;
	const char* hero_name = nullptr;
	if (heroid == 0 && model_res_id != 0) {
		for (auto& hero : stu_resources::GetIfAssignableTo(stringHash("STUHero"))) {
			auto stu = hero.second->to_editable();
			auto* ent_ref = stu.get_argument_resource("m_gameplayEntity");
			if (ent_ref && (uint64_t)ent_ref->resource_id == model_res_id) {
				heroid = (uint64_t)hero.first;
				break;
			}
		}
	}
	if (heroid != 0) {
		if (auto* hero_stu = stu_resources::GetByID((__int64)heroid)) {
			auto stu = hero_stu->to_editable();
			if (auto* name_ref = stu.get_argument_resource(0xcd16baa)) {
				hero_name = name_ref->get_UXDisplayString();
			}
			if (model_res_id == 0) {
				if (auto* ent_ref = stu.get_argument_resource("m_gameplayEntity")) {
					model_res_id = (uint64_t)ent_ref->resource_id;
				}
			}
		}
	}

	float norm_hp = 200.0f;
	float arm_hp = 0.0f;
	float shld_hp = 0.0f;

	// Overwatch 0.8.0 (Build 24919) Hero Health / Armor / Shield Pool Definitions
	if (heroid == 0x02E0000000000007ULL || model_res_id == 0x0400000000000044ULL || (hero_name && strstr(hero_name, "Reinhardt"))) {
		norm_hp = 300.0f; arm_hp = 200.0f;
	} else if (heroid == 0x02E0000000000003ULL || model_res_id == 0x0400000000000001ULL || (hero_name && strstr(hero_name, "Tracer"))) {
		norm_hp = 150.0f;
	} else if (heroid == 0x02E0000000000009ULL || (hero_name && strstr(hero_name, "Winston"))) {
		norm_hp = 400.0f; arm_hp = 100.0f;
	} else if (heroid == 0x02E000000000007AULL || (hero_name && strstr(hero_name, "D.Va"))) {
		norm_hp = 100.0f; arm_hp = 400.0f;
	} else if (heroid == 0x02E0000000000015ULL || (hero_name && strstr(hero_name, "Bastion"))) {
		norm_hp = 200.0f; arm_hp = 100.0f;
	} else if (heroid == 0x02E0000000000040ULL || (hero_name && strstr(hero_name, "Roadhog"))) {
		norm_hp = 600.0f;
	} else if (heroid == 0x02E0000000000068ULL || (hero_name && strstr(hero_name, "Zarya"))) {
		norm_hp = 200.0f; shld_hp = 200.0f;
	} else if (heroid == 0x02E0000000000020ULL || (hero_name && strstr(hero_name, "Zenyatta"))) {
		norm_hp = 50.0f; shld_hp = 100.0f;
	} else if (heroid == 0x02E0000000000016ULL || (hero_name && strstr(hero_name, "Symmetra"))) {
		norm_hp = 100.0f; shld_hp = 100.0f;
	} else if (heroid == 0x02E0000000000002ULL || heroid == 0x02E00000000000DDULL ||
	           (hero_name && (strstr(hero_name, "Reaper") || strstr(hero_name, "Mei")))) {
		norm_hp = 250.0f;
	}

	for (int i = 0; i < 0x10; i++) {
		normal_health[i].part_enabled = 0;
		normal_health[i].max_health = 0.0f;
		normal_health[i].curr_health = 0.0f;
		armour_health[i].part_enabled = 0;
		armour_health[i].max_health = 0.0f;
		armour_health[i].curr_health = 0.0f;
		shield_health[i].part_enabled = 0;
		shield_health[i].max_health = 0.0f;
		shield_health[i].curr_health = 0.0f;
	}

	if (norm_hp > 0.0f) {
		normal_health[0].part_enabled = 1;
		normal_health[0].max_health = norm_hp;
		normal_health[0].curr_health = norm_hp;
		normal_health[0].field_C = 0;
		normal_health[0].field_10 = 1;
		normal_health[0].some_other_flag = 1;
		normal_health[0].field_12 = 0;
	}
	if (arm_hp > 0.0f) {
		armour_health[0].part_enabled = 1;
		armour_health[0].max_health = arm_hp;
		armour_health[0].curr_health = arm_hp;
		armour_health[0].field_C = 0;
		armour_health[0].field_10 = 1;
		armour_health[0].some_other_flag = 1;
		armour_health[0].field_12 = 0;
	}
	if (shld_hp > 0.0f) {
		shield_health[0].part_enabled = 1;
		shield_health[0].max_health = shld_hp;
		shield_health[0].curr_health = shld_hp;
		shield_health[0].field_C = 0;
		shield_health[0].field_10 = 1;
		shield_health[0].some_other_flag = 1;
		shield_health[0].field_12 = 0;
	}
}

void player_spawner::modify_ent(spawn_info info, Entity* ent) {
	auto game_ea = GameEntityAdmin();
	if (!ent) {
		imgui_helpers::messageBox("Failed to modify ent");
		return;
	}
	if (info.set_localent) {
		game_ea->local_entid = ent->entity_id;
	}

	Vector4 valid_rot = info.spawn_rotation;
	if (valid_rot.X == 0.0f && valid_rot.Y == 0.0f && valid_rot.Z == 0.0f && valid_rot.W == 0.0f) {
		valid_rot.W = 1.0f;
	}

	Component_1_SceneRendering* scene_comp = ent->getById<Component_1_SceneRendering>(1);
	if (scene_comp) {
		if (info.set_position) {
			if (auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent)) {
				float abs_sum = fabsf(info.spawn_position.X) + fabsf(info.spawn_position.Y) + fabsf(info.spawn_position.Z);
				if (abs_sum > 0.5f && info.spawn_position.Y > -4000.0f) {
					slot->has_nonzero_pos = true;
					slot->latest_nonzero_pos = info.spawn_position;
					slot->latest_nonzero_rot = valid_rot;
				}
			}
			scene_comp->SetPosRotation(info.spawn_position, valid_rot);
		}
		if (!info.set_filterbits_controller && ent == model_ent) {
			scene_comp->SetScale(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			scene_comp->SetVisible();
		}
	}

	if (info.set_position && ent == model_ent) {
		if (auto* c15 = ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
			c15->movement_state_1.absolute_position = info.spawn_position;
			c15->movement_state_1.rotation_quaternion = valid_rot;
			c15->movement_state_1.movement_flags = (c15->movement_state_1.movement_flags & ~0x2000) | 0x200;
			c15->movement_state_2.absolute_position = info.spawn_position;
			c15->movement_state_2.rotation_quaternion = valid_rot;
			c15->movement_state_2.movement_flags = (c15->movement_state_2.movement_flags & ~0x2000) | 0x200;
		}
		if (auto* c12 = ent->getById<Component_12_STUMovementStateComponent>(0x12)) {
			c12->mov_state.absolute_position = info.spawn_position;
			c12->mov_state.rotation_quaternion = valid_rot;
			c12->mov_state.movement_flags = (c12->mov_state.movement_flags & ~0x2000) | 0x200;
			if (c12->mov_state.command_frame <= 0) {
				c12->mov_state.command_frame = (game_ea && game_ea->GameEA_timing) ? game_ea->GameEA_timing->tick_count : 1;
			}
			auto CreateRotMatrix = (void(__fastcall*)(MovementState*, Matrix4x4*))(globals::gameBase + 0x102a610);
			CreateRotMatrix(&c12->mov_state, &c12->mov_state.rotation_matrix);
			c12->PasteMovementState(&c12->mov_state);
		}
	}

	Component_20_ModelReference* pet_comp = ent->getById<Component_20_ModelReference>(0x20);
	if (pet_comp) {
		if (info.cam_attach > 0) {
			pet_comp->cam_attach_entid = info.cam_attach == 1 ? controller_ent->entity_id : model_ent->entity_id;
		}
		if (info.aim_attach > 0) {
			pet_comp->aim_entid = info.aim_attach == 1 ? controller_ent->entity_id : model_ent->entity_id;
		}
		if (info.movement_attach > 0) {
			pet_comp->movement_attach_entid = info.movement_attach == 1 ? controller_ent->entity_id : model_ent->entity_id;
		}
	}

	Component_1F_Backref* petref = ent->getById<Component_1F_Backref>(0x1F);
	if (petref && info.petbackref_attach > 0) {
		Entity* backref_ent = info.petbackref_attach == 1 ? controller_ent : model_ent;
		petref->backref_entid = backref_ent->entity_id;
		petref->backref_global_entid = backref_ent->entity_global_id;
	}

	Component_28_STUHealthComponent* health = ent->getById<Component_28_STUHealthComponent>(0x28);
	if (health && info.set_health) {
		health->InitHeroHealthPools((uint64_t)info.heroid, (uint64_t)info.resource_id);
	}

	Component_2F_LocalPlayer* lp = ent->getById<Component_2F_LocalPlayer>(0x2F);
	if (lp && info.set_2f_movable) {
		lp->is_able_to_aim = true;

		auto move = (void(__fastcall*)(__int64, int, int, char, char))(globals::gameBase + 0xc27560);
		auto get_sys36 = (__int64(__fastcall*)(EntityAdminBase*))(globals::gameBase + 0xc03750);
		__int64 sys = get_sys36(GameEntityAdmin());
		move(sys, 1000, 1000, 1, 1);
	}

	Component_3F_PlayerInfo* player_info = ent->getById<Component_3F_PlayerInfo>(0x3F);
	if (player_info) {
		player_info->hero_select_enabled = true;
		player_info->hero_was_selected = true;
		player_info->m_slot = 3;
		player_info->selected_heroid = info.heroid;
		player_info->unconformed_selected_heroid = info.heroid;
		player_info->localplayer_heroselected = true;
	}

	Component_10_FilterBits* filter_bits = ent->getById<Component_10_FilterBits>(0x10);
	if (filter_bits) {
		if (info.set_filterbits_controller) {
			filter_bits->set_filterbits_spectator();
		} else {
			filter_bits->pre_filter_bits |= 0x800000;
			filter_bits->filter_bits |= 0x800000;
		}
		if (info.team != -1) {
			filter_bits->set_team(info.team);
		}
		filter_bits->FilterBitsMakeFinal();
	}

	Component_23_Statescript* statescript = ent->getById<Component_23_Statescript>(0x23);
	if (statescript) {
		if (info.weapon_attach) {
			auto stu_res = stu_resources::GetByID(info.resource_id);
			if (stu_res) {
				auto entity_def = stu_res->to_editable();
				auto component_map = entity_def.get_argument_map("m_componentMap");
				auto weapon_entry = component_map.value ? component_map.value->get_by_key(stringHash("STUWeaponComponent")) : nullptr;
				if (weapon_entry && weapon_entry->value) {
					auto weapon_comp = ((STUBase<>*)weapon_entry->value)->to_editable();
					auto mgr_script = weapon_comp.get_argument_resource("m_managerScript");
					if (mgr_script && mgr_script->has_resource())
						statescript->loadScript(mgr_script->resource_id);
					for (auto weapon : weapon_comp.get_argument_objectlist("m_weapons")) {
						auto script = weapon.get_argument_resource("m_script");
						if (script && script->has_resource())
							statescript->loadScript(script->resource_id);
					}
				}
			}
		}

		if (info.fixup_player_statescript && statescript->ss_inner.rid_entity_varbag) {
			StatescriptPrimitive cv{};
			cv.type = StatescriptPrimitive_BYTE;
			cv.value = 1;
			statescript->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000000FD }, cv);
			statescript->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000000FD }, cv);

			StatescriptPrimitive wpn_1{};
			wpn_1.type = StatescriptPrimitive_INT;
			wpn_1.value = 1;
			statescript->ss_inner.rid_entity_varbag->SetArray({ 0x0D8000000000001D }, wpn_1, wpn_1, -1);

			if (health) {
				StatescriptPrimitive health_cv{};
				health_cv.type = StatescriptPrimitive_FLT;
				*(float*)&health_cv.value = health->CalcTotalCurrentHealth();
				statescript->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000006C4 }, health_cv);
				statescript->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C4 }, health_cv);

				*(float*)&health_cv.value = health->CalcTotalMaxHealth();
				statescript->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000006C9 }, health_cv);
				statescript->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C9 }, health_cv);
			}
		}

		bool is_local_model = (ent == model_ent && controller_ent && game_ea && game_ea->local_entid == controller_ent->entity_id);
		for (auto* inst : statescript->ss_inner.g1_instanceArr) {
			if (inst) {
				if (is_local_model) {
					inst->execution_flags = (inst->execution_flags | 0x400009u | 0x200000u) & ~0x1004u;
				} else {
					inst->execution_flags = (inst->execution_flags | 0x400009u) & ~(0x1004u | 0x200000u);
				}
				inst->vfptr->RedoCheckIsAimEnt(inst);
			}
		}
	}
}

Entity* player_spawner::spawn_ent(spawn_info& info) {
	auto res_ptr = try_load_resource(info.resource_id);
	if (!res_ptr) {
		printf("PlayerSpawner failed: No Resource! (Resource ID: %p)\n", info.resource_id);
		return nullptr;
	}
	auto spawner = EntityLoader::Create(info.resource_id, res_ptr, info.load_2f_33, info.load_3f);
	if (!spawner) {
		return nullptr;
	}
	for (auto additional : info.additional_compids) {
		auto spawner_elem = &spawner->loader_entries[additional.component_id];
		spawner_elem->component_id = additional.component_id;
		if (additional.init_data)
			spawner_elem->init_data = additional.init_data;
		if (additional.stu_component_data)
			spawner_elem->stu_component_data = additional.stu_component_data;
		if (additional.stu_instance_data)
			spawner_elem->stu_instance_data = additional.stu_instance_data;
	}
	Component_1_SceneRendering::InitData init_data{};
	if (info.set_position) {
		Vector4 valid_rot = info.spawn_rotation;
		if (valid_rot.X == 0.0f && valid_rot.Y == 0.0f && valid_rot.Z == 0.0f && valid_rot.W == 0.0f) {
			valid_rot.W = 1.0f;
		}
		init_data.position = info.spawn_position;
		init_data.scale = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		init_data.rotation = valid_rot;
		spawner->loader_entries[1].init_data = (__int64)&init_data;
	}
	spawner->aplist_extra = info.skin_theme_id;
	return spawner->Spawn(GameEntityAdmin());
}

void player_spawner::template_localPlayer() {
	controller_info.resource_id = 0x40000000000000E;
	controller_info.aim_attach = 2;
	controller_info.cam_attach = 2;
	controller_info.set_localent = true;
	controller_info.set_2f_movable = true;
	controller_info.load_2f_33 = true;
	controller_info.load_3f = true;
	controller_info.movement_attach = 2;
	controller_info.set_filterbits_controller = false;
	controller_info.team = 0;
	if (model_info.resource_id == 0) {
		model_info.resource_id = 0x400000000000001;
	}
	if (model_info.heroid == 0 && model_info.resource_id == 0x400000000000001) {
		model_info.heroid = 0x02E0000000000003ULL;
		controller_info.heroid = 0x02E0000000000003ULL;
	}
	model_info.petbackref_attach = 1;
	model_info.set_health = true;
	model_info.weapon_attach = true;
	model_info.fixup_player_statescript = true;
	model_info.team = 0;
}

std::pair<Entity*, Entity*> player_spawner::spawn() {
	if (controller_info.heroid == 0 && model_info.heroid != 0) {
		controller_info.heroid = model_info.heroid;
	}
	if (controller_info.heroid == 0 && model_info.resource_id == 0x400000000000001) {
		controller_info.heroid = 0x02E0000000000003ULL;
		model_info.heroid = 0x02E0000000000003ULL;
	}

	controller_ent = spawn_ent(controller_info);
	model_ent = spawn_ent(model_info);
	if (!controller_ent || !model_ent) {
		return {};
	}

	if (!use_deferred_spawn) {
		modify_ent(controller_info, controller_ent);
		modify_ent(model_info, model_ent);
		return { controller_ent, model_ent };
	}

	pending_controller_entid = controller_ent->entity_id;
	pending_model_entid = model_ent->entity_id;
	pending_controller_info = controller_info;
	pending_model_info = model_info;
	wait_ticks = 0;
	ready_after_ticks = 0;
	pre_init_script_count = 0;
	post_init_script_count = 0;
	post_init_has_backref = 0;
	post_init_is_aim_ent = 0;
	diag_c20 = false;
	diag_c1f = false;
	diag_c12 = false;
	diag_c23 = false;
	diag_not_instantiating = false;
	diag_varbag_ready = false;
	diag_instance_count = 0;
	spawn_state = DeferredSpawnState::WAITING_FOR_COMPONENTS;

	return { controller_ent, model_ent };
}

void player_spawner::tick_deferred_spawn() {
	if (spawn_state != DeferredSpawnState::WAITING_FOR_COMPONENTS && spawn_state != DeferredSpawnState::APPLYING_POST_SPAWN_INIT) {
		return;
	}

	auto* ea = GameEntityAdmin();
	if (!ea) {
		return;
	}

	Entity* c_ent = ea->getEntById(pending_controller_entid);
	Entity* m_ent = ea->getEntById(pending_model_entid);

	if (spawn_state == DeferredSpawnState::WAITING_FOR_COMPONENTS) {
		wait_ticks++;

		if (!c_ent || !m_ent) {
			if (wait_ticks >= MAX_WAIT_TICKS) {
				spawn_state = DeferredSpawnState::SPAWN_FAILED_TIMEOUT;
				printf("[DeferredSpawner] Timeout waiting for entities! (C: 0x%x, M: 0x%x, ticks: %d)\n", pending_controller_entid, pending_model_entid, wait_ticks);
			}
			return;
		}

		auto* c20 = c_ent->getById<Component_20_ModelReference>(0x20);
		auto* c1f = m_ent->getById<Component_1F_Backref>(0x1F);
		auto* c12 = m_ent->getById<ComponentBase>(0x12);
		auto* c23 = m_ent->getById<Component_23_Statescript>(0x23);

		diag_c20 = (c20 != nullptr);
		diag_c1f = (c1f != nullptr);
		diag_c12 = (c12 != nullptr);
		diag_c23 = (c23 != nullptr);

		bool basic_ready = (diag_c20 && diag_c23);
		bool statescript_ready = false;

		if (diag_c23) {
			diag_not_instantiating = (c23->ss_inner.is_instantiating == 0);
			diag_varbag_ready = (c23->ss_inner.rid_entity_varbag != nullptr);
			diag_instance_count = c23->ss_inner.g1_instanceArr.num;

			if (diag_not_instantiating && diag_varbag_ready) {
				statescript_ready = true;
			}
			else if (wait_ticks >= 15 && diag_varbag_ready) {
				statescript_ready = true;
			}
		}

		if (basic_ready && statescript_ready) {
			ready_after_ticks = wait_ticks;
			pre_init_script_count = c23->ss_inner.g1_instanceArr.num;
			spawn_state = DeferredSpawnState::APPLYING_POST_SPAWN_INIT;
			printf("[DeferredSpawner] Components ready after %d ticks! Pre-init scripts: %d\n", ready_after_ticks, pre_init_script_count);
		}
		else if (wait_ticks >= MAX_WAIT_TICKS) {
			spawn_state = DeferredSpawnState::SPAWN_FAILED_TIMEOUT;
			printf("[DeferredSpawner] Timeout waiting for components! (c20: %p, c1f: %p, c12: %p, c23: %p, ticks: %d)\n",
				c20, c1f, c12, c23, wait_ticks);
			return;
		}
		else {
			return;
		}
	}

	if (spawn_state == DeferredSpawnState::APPLYING_POST_SPAWN_INIT) {
		this->controller_ent = c_ent;
		this->model_ent = m_ent;

		modify_ent(pending_controller_info, c_ent);

		if (c_ent && m_ent) {
			Component_1F_Backref* petref = m_ent->getById<Component_1F_Backref>(0x1F);
			if (petref && pending_model_info.petbackref_attach > 0) {
				Entity* backref_ent = pending_model_info.petbackref_attach == 1 ? c_ent : m_ent;
				petref->backref_entid = backref_ent->entity_id;
				petref->backref_global_entid = backref_ent->entity_global_id;
			}
			if (pending_controller_info.set_localent && c_ent->getById<Component_2F_LocalPlayer>(0x2F) != nullptr) {
				auto tell_statescript_this_is_localent = (void(__fastcall*)(Entity*, uint*))(globals::gameBase + 0xd07830);
				tell_statescript_this_is_localent(m_ent, &c_ent->entity_id);
			}
		}

		modify_ent(pending_model_info, m_ent);

		auto* c23 = m_ent->getById<Component_23_Statescript>(0x23);
		if (c23) {
			bool is_local_model = (pending_controller_info.set_localent || (c_ent && ea->local_entid == c_ent->entity_id));
			for (auto* inst : c23->ss_inner.g1_instanceArr) {
				if (inst) {
					if (is_local_model) {
						inst->execution_flags = (inst->execution_flags | 0x400009u | 0x200000u) & ~0x1004u;
					} else {
						inst->execution_flags = (inst->execution_flags | 0x400009u) & ~(0x1004u | 0x200000u);
					}
					inst->vfptr->RedoCheckIsAimEnt(inst);
				}
			}
			post_init_script_count = c23->ss_inner.g1_instanceArr.num;
			post_init_has_backref = c23->ss_inner.has_player_entbackref;
			post_init_is_aim_ent = (int)c23->ss_inner.is_aim_ent;
		}

		spawn_state = DeferredSpawnState::SPAWN_COMPLETE;
		printf("[DeferredSpawner] Post-spawn init complete! Scripts: %d -> %d, backref: %d, aim_ent: %d\n",
			pre_init_script_count, post_init_script_count, post_init_has_backref, post_init_is_aim_ent);
	}
}