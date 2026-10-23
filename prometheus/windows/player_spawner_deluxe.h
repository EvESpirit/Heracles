#pragma once
#include "../window_manager/window_manager.h"
#include <vector>
#include "entitydef_list.h"
#include "../stringhash_library.h"
#include "entity_window.h"
#include "../stu_resources.h"
#include "../player_spawner.h"
#include "../MovementState.h"
#include "../Statescript.h"
#include "../StatescriptVar.h"
#include <cmath>

//2f und 33 sind bei controller
//-> ist prob local player
//3f ist wsl bei model NEIN -> bei controller, controller ist local entity
//1f backref ist zu controller
//aim ent ist model
//mov attach ist model
//local entity ist das mit 3f
//local entity muss team haben in filter bits
//mov attach == local ent??? maybe, crasht nicht
//local ent muss camera stuff (c20) haben
//Component mit 2f muss c20 haben und wird benuetzt

//SPECTATE:
//Entity mit C20 muss filterbits 0x400000 haben
//Ein Spectator kann alles spectaten wenn kein team in filterbits ist
//LocalEnt muss flag 0x400000 haben

//set pos: 2f <- 12 <- 15 (adds grav?)
//GameClientApp.exe+102B000
//GameClientApp.exe+CCEDDD

class player_spawner_deluxe : public window {
	WINDOW_DEFINE(player_spawner_deluxe, "Game", "Player Spawner DeLuxe", true);

	void render_ent_radios(std::string label, int* input) {
		ImGui::PushID(label.c_str());
		ImGui::Text("%s:", label.c_str());
		ImGui::SameLine();
		ImGui::RadioButton("None", input, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Controller", input, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Model", input, 2);
		ImGui::PopID();
	}

	void render_ent_options(player_spawner::spawn_info* inf) {
		imgui_helpers::InputHex("Resource ID", &inf->resource_id);
		/*if (ImGui::Button("Choose")) {
			window_manager::add_window(new entitydef_list(true, [inf](__int64 ent_out) {
				inf->resource_id = ent_out;
			}), this);
		}*/
		if (allmighty_hash_lib::comments.find(inf->resource_id) != allmighty_hash_lib::comments.end()) {
			ImGui::SameLine();
			ImGui::Text("(%s)", allmighty_hash_lib::comments[inf->resource_id].c_str());
		}
		/*if (inf->latest_entid) {
			display_addr(inf->latest_entid & 0xFFFFFFFF, "EntID");
			auto ent = GameEntityAdmin()->getEntById(inf->latest_entid);
			if (ent) {
				ImGui::SameLine();
				if (ImGui::Button("Delete")) {
					GameEntityAdmin()->delEnt(ent);
				}
				ImGui::SameLine();
				if (ImGui::Button(EMOJI_SHARE)) {
					entity_window::get_latest_or_create(this)->nav_to_ent(ent);
				}
			}
		}*/
		ImGui::NewLine();
		ImGui::Checkbox("Load 2F & 33", &inf->load_2f_33);
		if (inf->load_2f_33) {
			ImGui::SameLine();
			ImGui::Checkbox("Movable", &inf->set_2f_movable);
		}
		ImGui::Checkbox("Load 3F", &inf->load_3f);
		ImGui::Checkbox("Set Health", &inf->set_health);
		ImGui::Checkbox("Set Local", &inf->set_localent);
		ImGui::Checkbox("Controller FilterBits", &inf->set_filterbits_controller);
		//ImGui::Checkbox("call_component2_vodoo", &inf->call_component2_vodoo);
		ImGui::Checkbox("Weapon Attach", &inf->weapon_attach);

		ImGui::NewLine();
		render_ent_radios("Cam Attach", &inf->cam_attach);
		render_ent_radios("Aim Attach", &inf->aim_attach);
		render_ent_radios("Movement Attach", &inf->movement_attach);
		render_ent_radios("Backref Attach", &inf->petbackref_attach);
		ImGui::TextUnformatted("Team");
		ImGui::SameLine();
		ImGui::RadioButton("None", &inf->team, -1);
		ImGui::SameLine();
		ImGui::RadioButton("0", &inf->team, 0);
		ImGui::SameLine();
		ImGui::RadioButton("1", &inf->team, 1);
		ImGui::SameLine();
		ImGui::RadioButton("2", &inf->team, 2);
		ImGui::SameLine();
		ImGui::RadioButton("3", &inf->team, 3);
	}

	void template_teamplayer() {
		_spawner.controller_info.resource_id = 0x40000000000000E;
		_spawner.controller_info.aim_attach = 2;
		_spawner.controller_info.cam_attach = 2;
		_spawner.controller_info.set_localent = false;
		_spawner.controller_info.set_2f_movable = false;
		_spawner.controller_info.load_2f_33 = false;
		_spawner.controller_info.load_3f = true;
		_spawner.controller_info.movement_attach = 2;
		_spawner.controller_info.set_filterbits_controller = false;
		_spawner.controller_info.team = 1;
		if (_spawner.model_info.resource_id == 0) {
			_spawner.model_info.resource_id = 0x400000000000001;
		}
		if (_spawner.model_info.heroid == 0 && _spawner.model_info.resource_id == 0x400000000000001) {
			_spawner.model_info.heroid = 0x02E0000000000003ULL;
			_spawner.controller_info.heroid = 0x02E0000000000003ULL;
		}
		_spawner.model_info.petbackref_attach = 1;
		_spawner.model_info.set_health = true;
		_spawner.model_info.weapon_attach = true;
		_spawner.model_info.fixup_player_statescript = true;
		_spawner.model_info.team = 1;
	}

	static bool compute_local_player_spawn_transform(float forward_dist, Vector4& out_pos, Vector4& out_rot, bool face_towards_player) {
		auto* ea = GameEntityAdmin();
		if (!ea) return false;
		Entity* local = ea->getLocalEnt();
		if (!local) return false;
		auto* pet = local->getById<Component_20_ModelReference>(0x20);
		if (!pet) return false;
		uint32 mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : (pet->aim_entid ? pet->aim_entid : pet->movement_attach_entid);
		Entity* model = ea->getEntById(mdl_id);
		if (!model) return false;

		Vector4 base_pos{};
		Vector4 base_rot(0.0f, 0.0f, 0.0f, 1.0f);
		bool found_pos = false;

		if (auto* c1 = model->getById<Component_1_SceneRendering>(1)) {
			base_pos = c1->position;
			base_rot = c1->rotation;
			found_pos = true;
		}
		if (auto* c15 = model->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
			float abs15 = fabsf(c15->movement_state_2.absolute_position.X) +
			              fabsf(c15->movement_state_2.absolute_position.Y) +
			              fabsf(c15->movement_state_2.absolute_position.Z);
			if (abs15 > 0.5f) {
				base_pos = c15->movement_state_2.absolute_position;
				base_rot = c15->movement_state_2.rotation_quaternion;
				found_pos = true;
			}
		}
		if (!found_pos) return false;

		if (base_rot.X == 0.0f && base_rot.Y == 0.0f && base_rot.Z == 0.0f && base_rot.W == 0.0f) {
			base_rot.W = 1.0f;
		}

		// Compute horizontal forward vector on XZ plane from quaternion q = (x, y, z, w) rotating +Z (0, 0, 1)
		float qx = base_rot.X, qy = base_rot.Y, qz = base_rot.Z, qw = base_rot.W;
		float fx = 2.0f * (qx * qz + qw * qy);
		float fz = 1.0f - 2.0f * (qx * qx + qy * qy);
		float flen = sqrtf(fx * fx + fz * fz);
		if (flen > 0.001f) {
			fx /= flen;
			fz /= flen;
		} else {
			fx = 0.0f;
			fz = 1.0f;
		}

		out_pos = base_pos;
		out_pos.X += fx * forward_dist;
		out_pos.Z += fz * forward_dist;

		if (face_towards_player) {
			// Rotate 180 degrees around Y axis: q_180_y * q = (0, 1, 0, 0) * (qx, qy, qz, qw) = (-qz, qw, qx, -qy)
			out_rot = Vector4(-qz, qw, qx, -qy);
		} else {
			out_rot = base_rot;
		}
		return true;
	}

	static void apply_dummy_transform(Entity* model_ent, const Vector4& pos, const Vector4& rot) {
		if (!model_ent) return;
		Vector4 valid_rot = rot;
		if (valid_rot.X == 0.0f && valid_rot.Y == 0.0f && valid_rot.Z == 0.0f && valid_rot.W == 0.0f) {
			valid_rot.W = 1.0f;
		}
		if (auto* slot = OriginClutterTracker::GetSlot(model_ent->entity_id, model_ent)) {
			slot->culled_by_heracles = false;
			slot->has_nonzero_pos = true;
			slot->latest_nonzero_pos = pos;
			slot->latest_nonzero_rot = valid_rot;
		}
		if (auto* c1 = model_ent->getById<Component_1_SceneRendering>(1)) {
			c1->SetScale(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			c1->SetPosRotation(pos, valid_rot);
			c1->SetVisible();
		}
		if (auto* c15 = model_ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
			c15->movement_state_1.absolute_position = pos;
			c15->movement_state_1.rotation_quaternion = valid_rot;
			c15->movement_state_2.absolute_position = pos;
			c15->movement_state_2.rotation_quaternion = valid_rot;
		}
		if (auto* c12 = model_ent->getById<Component_12_STUMovementStateComponent>(0x12)) {
			auto* ea = GameEntityAdmin();
			c12->mov_state.absolute_position = pos;
			c12->mov_state.rotation_quaternion = valid_rot;
			if (c12->mov_state.command_frame <= 0) {
				c12->mov_state.command_frame = (ea && ea->GameEA_timing) ? ea->GameEA_timing->tick_count : 1;
			}
			auto CreateRotMatrix = (void(__fastcall*)(MovementState*, Matrix4x4*))(globals::gameBase + 0x102a610);
			CreateRotMatrix(&c12->mov_state, &c12->mov_state.rotation_matrix);
			c12->PasteMovementState(&c12->mov_state);
		}
	}

	static void sync_entity_health_varbag(Entity* model_ent) {
		if (!model_ent) return;
		auto* ss = model_ent->getById<Component_23_Statescript>(0x23);
		auto* health_stru = model_ent->getById<Component_28_STUHealthComponent>(0x28);
		if (ss && health_stru && ss->ss_inner.rid_entity_varbag) {
			StatescriptPrimitive health_cv{};
			health_cv.type = StatescriptPrimitive_FLT;

			*(float*)&health_cv.value = health_stru->CalcTotalCurrentHealth();
			ss->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000006C4 }, health_cv);
			ss->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C4 }, health_cv);

			*(float*)&health_cv.value = health_stru->CalcTotalMaxHealth();
			ss->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000006C9 }, health_cv);
			ss->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C9 }, health_cv);
		}
	}

	inline void render() override {
		if (open_window(nullptr, 0, ImVec2(1050, 550))) {
			ImGui::Text("Player Spawner DeLuxe at your service.");
			if (ImGui::BeginChild("replay entity", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
				ImGui::Text("Controller Entity");
				render_ent_options(&_spawner.controller_info);
			}
			ImGui::EndChild();
			ImGui::SameLine();
			if (ImGui::BeginChild("model entity", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
				ImGui::Text("Model Entity");
				STU_Object _selected_hero = STU_Object(nullptr, nullptr);
				if (ImGui::BeginListBox("H", ImVec2(0, 150))) {
					for (auto hero : stu_resources::GetIfAssignableTo(stringHash("STUHero"))) {
						auto stu = hero.second->to_editable();
						auto ent_id = stu.get_argument_resource("m_gameplayEntity")->resource_id;
						auto name = stu.get_argument_resource(0xcd16baa)->get_UXDisplayString();
						auto selected = _spawner.model_info.heroid == hero.first;

						if (selected)
							_selected_hero = stu;
						if (ImGui::Selectable(name, selected)) {
							_spawner.model_info.resource_id = ent_id;
							_spawner.model_info.heroid = hero.first;
							_spawner.controller_info.heroid = hero.first;
						}
					}
					ImGui::EndListBox();
				}

				if (_selected_hero.valid()) {
					for (auto skin_id : _selected_hero.get_argument_resreflist("m_skins")) {
						auto skin = skin_id->get_STU()->to_editable();
						std::string name = std::format("{:x}", skin_id->resource_id);
						for (auto skin_theme_id : skin.get_argument_resreflist("m_skinThemes")) {
							auto skin_theme = skin_theme_id->get_STU()->to_editable();
							if (ImGui::RadioButton(std::format("##{:x}", skin_theme_id->resource_id).c_str(), _spawner.model_info.skin_theme_id == skin_theme_id->resource_id)) {
								_spawner.model_info.skin_theme_id = skin_theme_id->resource_id;
							}
							ImGui::SameLine();
							ImGui::TextDisabled(name.c_str());
							ImGui::SameLine();
							imgui_helpers::display_type(skin_theme_id->resource_id, true, true, false);
						}
					}
				}

				render_ent_options(&_spawner.model_info);
			}
			ImGui::EndChild();

			_spawner.tick_deferred_spawn();

			ImGui::Separator();
			ImGui::Checkbox("Use Deferred Post-Spawn Init (Fix 0x23 Race)", &_spawner.use_deferred_spawn);

			const char* state_str = "UNKNOWN";
			ImVec4 state_col = ImVec4(1.f, 1.f, 1.f, 1.f);
			switch (_spawner.spawn_state) {
			case player_spawner::DeferredSpawnState::IDLE:
				state_str = "IDLE";
				state_col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
				break;
			case player_spawner::DeferredSpawnState::SPAWN_REQUESTED:
				state_str = "SPAWN_REQUESTED";
				state_col = ImVec4(1.f, 0.8f, 0.2f, 1.f);
				break;
			case player_spawner::DeferredSpawnState::WAITING_FOR_COMPONENTS:
				state_str = "WAITING_FOR_COMPONENTS";
				state_col = ImVec4(1.f, 0.8f, 0.2f, 1.f);
				break;
			case player_spawner::DeferredSpawnState::APPLYING_POST_SPAWN_INIT:
				state_str = "APPLYING_POST_SPAWN_INIT";
				state_col = ImVec4(0.3f, 0.7f, 1.f, 1.f);
				break;
			case player_spawner::DeferredSpawnState::SPAWN_COMPLETE:
				state_str = "SPAWN_COMPLETE";
				state_col = ImVec4(0.2f, 1.f, 0.2f, 1.f);
				break;
			case player_spawner::DeferredSpawnState::SPAWN_FAILED_TIMEOUT:
				state_str = "SPAWN_FAILED_TIMEOUT";
				state_col = ImVec4(1.f, 0.2f, 0.2f, 1.f);
				break;
			}

			ImGui::Text("Deferred State: ");
			ImGui::SameLine();
			ImGui::TextColored(state_col, "%s", state_str);
			if (_spawner.spawn_state != player_spawner::DeferredSpawnState::IDLE) {
				ImGui::Text("EntIDs: Controller = 0x%x, Model = 0x%x", _spawner.pending_controller_entid, _spawner.pending_model_entid);
				ImGui::Text("Wait Ticks: %d (Ready at: %d)", _spawner.wait_ticks, _spawner.ready_after_ticks);
				ImGui::Text("0x23 Scripts: %d -> %d", _spawner.pre_init_script_count, _spawner.post_init_script_count);
				ImGui::Text("SyncMgr Flags: has_player_entbackref = %d, is_aim_ent = %d", _spawner.post_init_has_backref, _spawner.post_init_is_aim_ent);
				ImGui::Text("Live Checks: C20:%s C23:%s (C12:%s C1F:%s) | Inst:%s VarBag:%s (g1:%d)",
					_spawner.diag_c20 ? "OK" : "NULL",
					_spawner.diag_c23 ? "OK" : "NULL",
					_spawner.diag_c12 ? "OK" : "NULL",
					_spawner.diag_c1f ? "OK" : "NULL",
					_spawner.diag_not_instantiating ? "DONE" : "BUSY",
					_spawner.diag_varbag_ready ? "OK" : "NULL",
					_spawner.diag_instance_count);
			}

			if (ImGui::Button("Set Spawn Pos from Current Player")) {
				auto ea = GameEntityAdmin();
				if (ea) {
					auto local = ea->getLocalEnt();
					if (local) {
						auto pet = local->getById<Component_20_ModelReference>(0x20);
						if (pet) {
							auto model = ea->getEntById(pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid);
							if (model) {
								auto c1 = model->getById<Component_1_SceneRendering>(1);
								if (c1) {
									_spawner.model_info.set_position = true;
									_spawner.model_info.spawn_position = c1->position;
									_spawner.model_info.spawn_rotation = c1->rotation;
								}
								if (auto* c15 = model->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
									float abs15 = fabsf(c15->movement_state_2.absolute_position.X) + fabsf(c15->movement_state_2.absolute_position.Y) + fabsf(c15->movement_state_2.absolute_position.Z);
									if (abs15 > 0.5f) {
										_spawner.model_info.set_position = true;
										_spawner.model_info.spawn_position = c15->movement_state_2.absolute_position;
										_spawner.model_info.spawn_rotation = c15->movement_state_2.rotation_quaternion;
									}
								}
							}
						}
					}
				}
			}
			if (_spawner.model_info.set_position) {
				ImGui::SameLine();
				ImGui::Text("(Pos: %.1f, %.1f, %.1f)", _spawner.model_info.spawn_position.X, _spawner.model_info.spawn_position.Y, _spawner.model_info.spawn_position.Z);
				ImGui::SameLine();
				if (ImGui::Button("Clear Pos")) {
					_spawner.model_info.set_position = false;
				}
			}
			ImGui::Separator();

			if (ImGui::Button("Spawn")) {
				_spawner.spawn();
			}
			ImGui::SameLine();
			if (ImGui::Button("Preset: Local Player (Team 0)")) {
				_spawner.template_localPlayer();
			}
			ImGui::SameLine();
			if (ImGui::Button("Preset: Enemy Bot/Dummy (Team 1)")) {
				template_teamplayer();
			}

			ImGui::Separator();
			ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "One-Click Enemy Target Dummy Spawner (Team 1):");
			ImGui::SetNextItemWidth(140.0f);
			ImGui::SliderFloat("Forward Dist (m)", &_dummy_forward_dist, 0.0f, 12.0f, "%.1f m");
			ImGui::SameLine();
			ImGui::Checkbox("Face Player", &_dummy_face_player);
			ImGui::SameLine();
			if (ImGui::Button("Spawn Jeff Kaplan")) {
				Vector4 spawn_pos{}, spawn_rot{};
				if (compute_local_player_spawn_transform(_dummy_forward_dist, spawn_pos, spawn_rot, _dummy_face_player)) {
					template_teamplayer();
					_spawner.model_info.set_position = true;
					_spawner.model_info.spawn_position = spawn_pos;
					_spawner.model_info.spawn_rotation = spawn_rot;
					_spawner.spawn();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Spawn Jeff Kaplan At Exact Player Pos")) {
				Vector4 spawn_pos{}, spawn_rot{};
				if (compute_local_player_spawn_transform(0.0f, spawn_pos, spawn_rot, _dummy_face_player)) {
					template_teamplayer();
					_spawner.model_info.set_position = true;
					_spawner.model_info.spawn_position = spawn_pos;
					_spawner.model_info.spawn_rotation = spawn_rot;
					_spawner.spawn();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Dump Dummy & Shot Forensics (dummy_combat_diag.log)")) {
				WeaponHardpointDiag::DumpDummyCombatForensics(GameEntityAdmin(), "Manual Spawner Button");
			}
			if (WeaponHardpointDiag::s_shot_raycast_calls > 0 || WeaponHardpointDiag::s_last_dummy_dump_status != "Not dumped yet.") {
				ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Shot Raycasts(+0xD354B0): %d | %s",
					WeaponHardpointDiag::s_shot_raycast_calls, WeaponHardpointDiag::s_last_dummy_dump_status.c_str());
			}
			ImGui::Checkbox("Offline Dummy Damage (local hitscan authority)", &WeaponHardpointDiag::s_dummy_damage_enabled);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::SliderFloat("Dmg/pellet", &WeaponHardpointDiag::s_dummy_damage_per_pellet, 1.0f, 200.0f, "%.0f");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(90.0f);
			ImGui::SliderFloat("Crit x", &WeaponHardpointDiag::s_dummy_crit_mult, 1.0f, 3.0f, "%.1f");
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", WeaponHardpointDiag::s_last_dummy_hit.c_str());

			// Compact Live Enemy Dummies / Non-Local Hero Models Status List
			if (EntityAdminBase* ea = GameEntityAdmin()) {
				Entity* local_ctrl = ea->getLocalEnt();
				uint32 local_ctrl_id = ea->local_entid;
				uint32 local_mdl_id = 0;
				if (local_ctrl) {
					if (auto* pet = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
						local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : (pet->aim_entid ? pet->aim_entid : pet->movement_attach_entid);
					}
				}

				int dummy_count = 0;
				for (Entity* ent : *ea) {
					if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
					if (ent->hasComponent(0x15) && ent->hasComponent(0x28)) {
						dummy_count++;
					}
				}

				if (dummy_count > 0) {
					ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Active Non-Local Hero Dummies (%d):", dummy_count);
					if (ImGui::BeginChild("EnemyDummiesList", ImVec2(0, 125), true)) {
						uint32 del_ctrl_id = 0;
						uint32 del_mdl_id = 0;
						for (Entity* ent : *ea) {
							if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
							if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;

							uint32 ctrl_id = 0;
							if (auto* c1f = ent->getById<Component_1F_Backref>(0x1F)) {
								ctrl_id = c1f->backref_entid;
							}
							Entity* ctrl_ent = ctrl_id ? ea->getEntById(ctrl_id) : nullptr;
							auto* c1 = ent->getById<Component_1_SceneRendering>(1);
							auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
							auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
							auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);

							float cur_hp = c28 ? c28->CalcTotalCurrentHealth() : 0.0f;
							float max_hp = c28 ? c28->CalcTotalMaxHealth() : 0.0f;
							sync_entity_health_varbag(ent);

							__int64 fb = c10 ? c10->filter_bits : 0;
							const char* team_str = (fb & 0x80000) ? "Team 1 (Enemy)" : ((fb & 0x40000) ? "Team 0 (Ally)" : "NoTeam");
							Vector4 pos = c1 ? c1->position : Vector4{};
							bool is_vis = c1 ? c1->IsVisible() : false;
							int zero_blk = slot ? slot->zero_overwrite_blocked_count : 0;

							ImGui::PushID((int)ent->entity_id);
							if (ImGui::SmallButton("TP In Front")) {
								Vector4 tp_pos{}, tp_rot{};
								if (compute_local_player_spawn_transform(_dummy_forward_dist, tp_pos, tp_rot, _dummy_face_player)) {
									apply_dummy_transform(ent, tp_pos, tp_rot);
								}
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("-50 HP") && c28) {
								float rem = 50.0f;
								HealthPart* pools[3] = { c28->shield_health, c28->armour_health, c28->normal_health };
								for (int p = 0; p < 3 && rem > 0.0f; p++) {
									for (int i = 0; i < 0x10 && rem > 0.0f; i++) {
										if (pools[p][i].part_enabled && pools[p][i].curr_health > 0.0f) {
											if (pools[p][i].curr_health >= rem) {
												pools[p][i].curr_health -= rem;
												rem = 0.0f;
											} else {
												rem -= pools[p][i].curr_health;
												pools[p][i].curr_health = 0.0f;
											}
										}
									}
								}
								sync_entity_health_varbag(ent);
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("Reset HP") && c28) {
								uint64_t h_id = 0;
								if (ctrl_ent) {
									if (auto* c3f = ctrl_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
										h_id = (uint64_t)c3f->selected_heroid;
									}
								}
								c28->InitHeroHealthPools(h_id, 0);
								sync_entity_health_varbag(ent);
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("Team 1")) {
								if (c10) {
									c10->pre_filter_bits |= 0x800000;
									c10->set_team(1);
									c10->FilterBitsMakeFinal();
								}
								if (ctrl_ent) {
									if (auto* cc10 = ctrl_ent->getById<Component_10_FilterBits>(0x10)) {
										cc10->set_team(1);
										cc10->FilterBitsMakeFinal();
									}
								}
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("Inspect")) {
								entity_window::get_latest_or_create(this)->nav_to_ent(ent);
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("Del")) {
								del_ctrl_id = ctrl_id;
								del_mdl_id = ent->entity_id;
							}
							ImGui::SameLine();
							ImGui::Text("M:0x%x C:0x%x | HP:%.0f/%.0f | %s (0x%llx) | Pos(%.1f,%.1f,%.1f) Vis:%s Blk:%d",
								ent->entity_id, ctrl_id, cur_hp, max_hp, team_str, (unsigned long long)fb,
								pos.X, pos.Y, pos.Z, is_vis ? "Y" : "N", zero_blk);
							ImGui::PopID();
						}
						if (del_mdl_id != 0) {
							if (Entity* m = ea->getEntById(del_mdl_id)) ea->delEnt(m);
							if (del_ctrl_id != 0 && del_ctrl_id != local_ctrl_id) {
								if (Entity* c = ea->getEntById(del_ctrl_id)) ea->delEnt(c);
							}
						}
					}
					ImGui::EndChild();
				}
			}
			ImGui::Separator();

			int total_spawn_doors = 0, open_spawn_doors = 0;
			OriginClutterTracker::GetSpawnDoorCounts(GameEntityAdmin(), total_spawn_doors, open_spawn_doors);
			if (ImGui::Button("Open Spawn Doors")) {
				OriginClutterTracker::OpenSpawnDoors(GameEntityAdmin());
				OriginClutterTracker::GetSpawnDoorCounts(GameEntityAdmin(), total_spawn_doors, open_spawn_doors);
			}
			ImGui::SameLine();
			if (ImGui::Button("Close / Restore Spawn Doors")) {
				OriginClutterTracker::RestoreSpawnDoors(GameEntityAdmin());
				OriginClutterTracker::GetSpawnDoorCounts(GameEntityAdmin(), total_spawn_doors, open_spawn_doors);
			}
			ImGui::SameLine();
			ImGui::Checkbox("Auto-Open Doors", &OriginClutterTracker::s_auto_open_spawn_doors);
			ImGui::SameLine();
			ImGui::TextColored(
				(total_spawn_doors > 0 && open_spawn_doors == total_spawn_doors)
					? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
					: ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
				"Doors: %d/%d Open", open_spawn_doors, total_spawn_doors);

			ImGui::NewLine();
			if (ImGui::RadioButton("0x400000000000013 (PvPGame 1)", _spawn_entid == 0x400000000000013)) {
				_spawn_entid = 0x400000000000013;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("0x40000000000012c (PvPGame 2)", _spawn_entid == 0x40000000000012c)) {
				_spawn_entid = 0x40000000000012c;
			}
			ImGui::SameLine();
			ImGui::Checkbox("Spawn in LobbyEA", &_spawn_lobby);
			ImGui::SameLine();
			ImGui::Checkbox("Set Location", &_spawn_setloc);
			imgui_helpers::InputHex("Ent", &_spawn_entid);
			ImGui::SameLine();
			if (ImGui::Button("Spawn Entity Simple")) {
				auto simple_res = try_load_resource(_spawn_entid);
				if (!simple_res) {
					imgui_helpers::messageBox("Failed to load entity " + std::format("{:x}", _spawn_entid), this);
					return;
				}
				auto loader = EntityLoader::Create(_spawn_entid, simple_res, false, false);
				Component_1_SceneRendering::InitData data{};
				if (_spawn_setloc) {
					Entity* controller = GameEntityAdmin()->getLocalEnt();
					Entity* model = GameEntityAdmin()->getEntById(controller->getById<Component_20_ModelReference>(0x20)->cam_attach_entid);
					auto comp1 = model->getById<Component_1_SceneRendering>(1);
					if (comp1) {
						data.position = comp1->position;
						data.rotation = comp1->rotation;
						data.scale = comp1->scaling;
						loader->loader_entries[1].init_data = (__int64)&data;
					}
				}
				auto created_ent = loader->Spawn(_spawn_lobby ? LobbyEntityAdmin() : GameEntityAdmin());
				if (created_ent) {
					_pvpgame_entid = created_ent->entity_id;
					printf("Spawned Entity Simple: %x\n", _pvpgame_entid);
				}
			}
			auto pvpgame_ent = GameEntityAdmin()->getEntById(_pvpgame_entid);
			if (pvpgame_ent) {
				ImGui::Text("%x", _pvpgame_entid);
				ImGui::SameLine();
				if (ImGui::Button("Delete")) {
					GameEntityAdmin()->delEnt(pvpgame_ent);
				}
				ImGui::SameLine();
				if (ImGui::Button(EMOJI_SHARE)) {
					entity_window::get_latest_or_create(this)->nav_to_ent(pvpgame_ent);
				}
			}

			ImGui::NewLine();
			auto sys = PrometheusSystem::instance();
			if (_dealloc_freelookview && sys->local_freeLookView) {
				sys->local_freeLookView = nullptr;
				ow_dealloc((__int64)sys->local_freeLookView);
				_dealloc_freelookview = false;
			}
			bool freelook = !!sys->local_freeLookView;
			if (ImGui::Checkbox("FreeLookView", &freelook)) {
				if (sys->local_freeLookView) {
					_dealloc_freelookview = true;

					auto cam = GameEntityAdmin()->getSingletonComponent<Component_4F_Camera>(0x4F);
					cam->override_views.num = 0;
				}
				else {
					sys->local_freeLookView = teFreeLookView::create();

					auto cam = GameEntityAdmin()->getSingletonComponent<Component_4F_Camera>(0x4F);
					cam->override_views.emplace_item(OverrideView{ &sys->local_freeLookView->base, 1 });
				}
			}
			if (freelook) {
				ImGui::SliderFloat("Movement Delta", &sys->freeLookView_movDelta, 10, 50, "%.0f", ImGuiSliderFlags_AlwaysClamp);
			}

			if (_dealloc_thirdpers && _thirdpers_view) {
				ow_dealloc((__int64)_thirdpers_view);
				_thirdpers_view = nullptr;
				_dealloc_thirdpers = false;
			}
		}
		ImGui::End();
	}

	inline void initialize() override {
		_spawner.template_localPlayer();
	}

private:
	bool _dealloc_freelookview = false;
	bool _dealloc_thirdpers = false;
	View* _thirdpers_view = 0;
	player_spawner _spawner;

	__int64 _spawn_entid = 0x400000000000013;
	bool _spawn_lobby = false;
	bool _spawn_setloc = false;
	uint _pvpgame_entid = 0;
	float _dummy_forward_dist = 3.5f;
	bool _dummy_face_player = true;
};

WINDOW_REGISTER(player_spawner_deluxe);

class weapon_hardpoint_diag_window : public window {
	WINDOW_DEFINE(weapon_hardpoint_diag_window, "Diagnostics", "Weapon & Hardpoint Diagnostics", true);

	inline void render() override {
		if (open_window(nullptr, 0, ImVec2(750, 220))) {
			ImGui::Text("Weapon, Effect & Hardpoint Ring-Buffer Tracer");
			ImGui::Separator();
			ImGui::Text("Logged Events: %d | FireShot(+0xD46060): %d | SSEffect(+0xD58110): %d | Attach(+0xA01430): %d | ResolveWeapon(+0xD560B0): %d | UpdateWeapons(+0xD563D0): %d",
				WeaponHardpointDiag::s_total_events_logged,
				WeaponHardpointDiag::s_fire_shot_calls,
				WeaponHardpointDiag::s_ss_effect_calls,
				WeaponHardpointDiag::s_attach_calls,
				WeaponHardpointDiag::s_resolve_weapon_calls,
				WeaponHardpointDiag::s_update_weapon_calls);

			if (ImGui::Button("Dump Weapon & Hardpoint Diagnostics (weapon_hardpoint_diag.log)")) {
				WeaponHardpointDiag::DumpReportToFile(GameEntityAdmin(), "Manual ImGui Button");
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear Trace Ring Buffer")) {
				WeaponHardpointDiag::ClearTrace();
			}
			ImGui::SameLine();
			if (ImGui::Button("Force Re-Attach Weapons (3P)")) {
				WeaponHardpointDiag::ForceUpdateWeapons(GameEntityAdmin(), 0);
			}
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Status: %s", WeaponHardpointDiag::s_last_dump_status.c_str());
		}
		ImGui::End();
	}
};

WINDOW_REGISTER(weapon_hardpoint_diag_window);

class origin_clutter_diag_window : public window {
	WINDOW_DEFINE(origin_clutter_diag_window, "Diagnostics", "Origin (0,0,0) Clutter Inspector", true);

	inline void render() override {
		if (open_window(nullptr, 0, ImVec2(800, 420))) {
			ImGui::Text("Origin (0,0,0) Clutter Inspector & Transform Guard");
			ImGui::Separator();
			ImGui::Checkbox("Block (0,0,0) Transform Overwrites (Case B)", &OriginClutterTracker::s_block_zero_overwrites);
			ImGui::SameLine();
			ImGui::Checkbox("Auto-Cull Unplaced (0,0,0) Props on Map Load (Case A)", &OriginClutterTracker::s_auto_cull_unplaced_origin_props);

			ImGui::Text("Blocked Overwrites: %d | Culled Props: %d | Ticks Since World State 6: %d",
				OriginClutterTracker::s_total_zero_overwrites_blocked,
				OriginClutterTracker::s_total_culled_props,
				OriginClutterTracker::s_ticks_since_world_ready);

			if (ImGui::Button("Open Spawn Doors")) {
				OriginClutterTracker::OpenSpawnDoors(GameEntityAdmin());
			}
			ImGui::SameLine();
			if (ImGui::Button("Close / Restore Spawn Doors")) {
				OriginClutterTracker::RestoreSpawnDoors(GameEntityAdmin());
			}
			ImGui::SameLine();
			if (ImGui::Button("Cull Unplaced (0,0,0) Dynamic Props Now")) {
				OriginClutterTracker::CullUnplacedOriginProps(GameEntityAdmin());
			}
			ImGui::SameLine();
			if (ImGui::Button("Restore Culled Props")) {
				OriginClutterTracker::RestoreCulledProps(GameEntityAdmin());
			}
			ImGui::SameLine();
			if (ImGui::Button("Dump Origin Report (origin_clutter_diag.log)")) {
				OriginClutterTracker::DumpReportToFile(GameEntityAdmin(), "Manual UI Button");
			}
			ImGui::SameLine();
			if (ImGui::Button("Dump Door Info (spawn_doors_diag.log)")) {
				OriginClutterTracker::DumpSpawnDoorsToFile(GameEntityAdmin(), "Manual Origin Inspector Button");
			}

			if (ImGui::BeginChild("OriginClutterList", ImVec2(0, 0), true)) {
				auto* ea = GameEntityAdmin();
				if (ea) {
					Entity* local_ctrl = ea->getLocalEnt();
					uint32 local_mdl_id = 0;
					if (local_ctrl) {
						if (auto* pet = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
							local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
						}
					}
					for (Entity* ent : *ea) {
						if (!ent || ent == local_ctrl || ent->entity_id == local_mdl_id)
							continue;
						if (ent->hasComponent(0x15) || ent->hasComponent(0x2F) || ent->hasComponent(0x3F) || ent->hasComponent(0x1D))
							continue;
						auto* c1 = ent->getById<Component_1_SceneRendering>(1);
						if (!c1)
							continue;
						bool has_render_or_mover = (ent->getById(0x1C) != nullptr) || (ent->getById(0x2A) != nullptr) || (ent->getById(0x2D) != nullptr) || (ent->getById(0x12) != nullptr);
						if (!has_render_or_mover)
							continue;
						float abs_pos = fabsf(c1->position.X) + fabsf(c1->position.Y) + fabsf(c1->position.Z);
						auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
						bool is_near_zero = (abs_pos < 1.0f);
						bool had_overwrite = slot && (slot->zero_overwrite_count > 0 || slot->direct_zero_overwrite_count > 0);
						bool is_culled = slot && slot->culled_by_heracles;
						if (!is_near_zero && !had_overwrite && !is_culled)
							continue;

						__int64 stu_id = (ent->resload_entry && ent->resload_entry->valid()) ? ent->resload_entry->align()->resource_id : (slot ? slot->stu_id : 0);
						ImGui::PushID((int)ent->entity_id);
						if (ImGui::SmallButton("Inspect")) {
							entity_window::get_latest_or_create(this)->nav_to_ent(ent);
						}
						ImGui::SameLine();
						if (!is_culled && ImGui::SmallButton("Hide")) {
							if (slot) {
								slot->pre_cull_pos = slot->has_nonzero_pos ? slot->latest_nonzero_pos : c1->position;
								slot->pre_cull_rot = slot->has_nonzero_pos ? slot->latest_nonzero_rot : c1->rotation;
								slot->culled_by_heracles = true;
							}
							Vector4 hidden_pos(0.f, -5000.f, 0.f, 0.f);
							Vector4 ident_rot(0.f, 0.f, 0.f, 1.f);
							c1->SetPosRotation(hidden_pos, ident_rot);
							c1->SetScale(Vector4(0.f, 0.f, 0.f, 0.f));
							c1->SetInvisible();
							OriginClutterTracker::s_total_culled_props++;
						} else if (is_culled && ImGui::SmallButton("Un-Hide")) {
							slot->culled_by_heracles = false;
							c1->SetScale(Vector4(1.f, 1.f, 1.f, 1.f));
							c1->SetPosRotation(slot->pre_cull_pos, slot->pre_cull_rot);
							c1->SetVisible();
						}
						if (slot && slot->has_nonzero_pos) {
							ImGui::SameLine();
							if (ImGui::SmallButton("Restore NonZero")) {
								slot->culled_by_heracles = false;
								c1->SetScale(Vector4(1.f, 1.f, 1.f, 1.f));
								c1->SetPosRotation(slot->latest_nonzero_pos, slot->latest_nonzero_rot);
								c1->SetVisible();
							}
						}
						ImGui::SameLine();
						ImGui::Text("Ent 0x%x | STU 0x%llx | Call +0x%llx | Pos(%.1f,%.1f,%.1f) | %s",
							ent->entity_id,
							(unsigned long long)stu_id,
							(unsigned long long)(slot ? slot->spawn_caller_rva : 0),
							c1->position.X, c1->position.Y, c1->position.Z,
							is_culled ? "CULLED" : (had_overwrite ? "OVERWRITE_BLOCKED" : "AT_ORIGIN"));
						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}
};

WINDOW_REGISTER(origin_clutter_diag_window);
