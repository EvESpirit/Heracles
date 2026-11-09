#include "player_spawner.h"
#include "window_manager/window_manager.h"
#include "stu_resources.h"
#include "Statescript.h"
#include "StatescriptVar.h"
#include "MovementState.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <memory>
#include <mutex>

// ---- Delayed native lock-on filter bit (0x1) & Dummy Auto-Respawn State ----
// Bit 0x1 must be applied AFTER the overhead nameplate/healthbar UI has been created for the dummy (see modify_ent).
namespace {
	struct PendingLockonBit { uint32 entid; uint64_t due_ms; };
	std::mutex s_lockon_bit_mtx;
	std::vector<PendingLockonBit> s_pending_lockon_bits;
	constexpr uint64_t LOCKON_BIT_DELAY_MS = 1000;

	std::mutex s_dummy_records_mtx;
	std::vector<player_spawner::DummySpawnRecord> s_dummy_records;

	std::mutex s_bg_spawners_mtx;
	std::vector<std::shared_ptr<player_spawner>> s_background_deferred_spawners;
	bool s_in_bg_spawner_tick = false;

	std::mutex s_pending_dummy_ops_mtx;
	std::vector<CombatReplication::ServerCorpseCleanupCommand> s_queued_corpse_cleanups;
	std::vector<CombatReplication::ServerEntityRespawnCommand> s_queued_dummy_respawns;

	int ExtractTeamIndexFromFilterBits(Component_10_FilterBits* c10, int fallback_team) {
		if (!c10) return fallback_team;
		uint64_t fb = (uint64_t)(c10->filter_bits ? c10->filter_bits : c10->pre_filter_bits);
		uint64_t tbits = fb & 0x3C0000ULL;
		if (tbits == 0x40000ULL) return 0;
		if (tbits == 0x80000ULL) return 1;
		if (tbits == 0x100000ULL) return 2;
		if (tbits == 0x200000ULL) return 3;
		return fallback_team;
	}

	bool HasValidSceneNode148(Entity* ent) {
		if (!ent) return false;
		auto* c1 = ent->getById<Component_1_SceneRendering>(1);
		if (!c1 || IsBadReadPtr(c1, sizeof(Component_1_SceneRendering))) return true;
		uintptr_t node_148 = *(uintptr_t*)((uintptr_t)c1 + 0x148);
		return (node_148 != 0 && !IsBadReadPtr((const void*)node_148, 0x10));
	}

	uint32 ResolveGlobalIdToEntityId(EntityAdminBase* ea, uint32_t global_id) {
		if (!ea || global_id == 0 || global_id == 0xFFFFFFFFu) return 0;
		for (Entity* e : *ea) {
			if (e && (uint32_t)e->entity_global_id == global_id) {
				return e->entity_id;
			}
		}
		return 0;
	}

	void HideNullSceneNodeEntity(Entity* ent) {
		if (!ent) return;
		if (ComponentBase* c1d = ent->getById(0x1D)) {
			if (!IsBadReadPtr(c1d, 0x48)) {
				// Disarm +0xCA2520..+0xCA29AA Component_1D evaluator on hidden null-scene-node entities
				*(uintptr_t*)((uintptr_t)c1d + 0x40) = 0;
			}
		}
		const Vector4 hidden_pos(0.0f, -5000.0f, 0.0f, 0.0f);
		const Vector4 hidden_rot(0.0f, 0.0f, 0.0f, 1.0f);
		if (auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent)) {
			slot->has_nonzero_pos = true;
			slot->latest_nonzero_pos = hidden_pos;
			slot->latest_nonzero_rot = hidden_rot;
		}
		if (ComponentBase* c12 = ent->getById(0x12)) {
			if (!IsBadReadPtr(c12, 0x40)) {
				*(Vector4*)((uintptr_t)c12 + 0x20) = hidden_pos;
			}
		}
		if (auto* c15 = ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
			if (!IsBadReadPtr(c15, sizeof(Component_15_STUCharacterMoverComponent))) {
				c15->movement_state_1.absolute_position = hidden_pos;
				c15->movement_state_2.absolute_position = hidden_pos;
			}
		}
		if (auto* c1 = ent->getById<Component_1_SceneRendering>(1)) {
			if (!IsBadReadPtr(c1, sizeof(Component_1_SceneRendering))) {
				uintptr_t c1_ptr = (uintptr_t)c1;
				*(uint32_t*)(c1_ptr + 0x1B8) &= ~0x10000u;
				*(uint16_t*)(c1_ptr + 0x172) &= (uint16_t)~0x3000u;
				uintptr_t node_148 = *(uintptr_t*)(c1_ptr + 0x148);
				if (node_148 != 0 && !IsBadReadPtr((const void*)node_148, 0x10)) {
					c1->SetScale(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
					c1->SetInvisible();
					c1->SetPosRotation(hidden_pos, hidden_rot);
				} else {
					c1->scaling = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
					c1->position = hidden_pos;
				}
			}
		}
	}

	void SanitizeComponent1BeforeTeardown(Component_1_SceneRendering* c1) {
		if (!c1 || IsBadReadPtr(c1, sizeof(Component_1_SceneRendering))) return;
		uintptr_t c1_ptr = (uintptr_t)c1;
		*(uint32_t*)(c1_ptr + 0x1B8) &= ~0x10000u;

		// Avoid the +0xA453DB scene-node teardown trap:
		// +0x9F47DA checks word ptr [c1 + 0x172] & 0x3000 and calls +0xA453C0, which dereferences [c1 + 0x148] + 0x0D
		// at +0xA453DB without null-checking [c1 + 0x148].
		// (NOTE: Never call +0xA02150 — it is the mid-epilogue of +0xA01430, not a function entry point!)
		uintptr_t scene_node_148 = *(uintptr_t*)(c1_ptr + 0x148);
		if (!scene_node_148 || IsBadReadPtr((const void*)scene_node_148, 0x10)) {
			*(uint16_t*)(c1_ptr + 0x172) &= (uint16_t)~0x3000u;
		}
	}
}

void player_spawner::queue_lockon_bit(uint32 model_entid) {
	if (!model_entid) return;
	std::lock_guard<std::mutex> lk(s_lockon_bit_mtx);
	for (auto& p : s_pending_lockon_bits) {
		if (p.entid == model_entid) {
			p.due_ms = GetTickCount64() + LOCKON_BIT_DELAY_MS;
			return;
		}
	}
	s_pending_lockon_bits.push_back({ model_entid, GetTickCount64() + LOCKON_BIT_DELAY_MS });
}

void player_spawner::tick_pending_lockon_bits() {
	{
		std::lock_guard<std::mutex> lk(s_lockon_bit_mtx);
		if (!s_pending_lockon_bits.empty()) {
			if (auto* ea = GameEntityAdmin()) {
				uint64_t now = GetTickCount64();
				for (size_t i = 0; i < s_pending_lockon_bits.size();) {
					const PendingLockonBit p = s_pending_lockon_bits[i];
					if (now < p.due_ms) { ++i; continue; }
					if (Entity* ent = ea->getEntById(p.entid)) {
						if (auto* c10 = ent->getById<Component_10_FilterBits>(0x10)) {
							c10->pre_filter_bits |= 0x1;
							c10->FilterBitsMakeFinal();
						}
						if (auto* c1 = ent->getById<Component_1_SceneRendering>(1)) {
							c1->SetVisible();
						}
					}
					s_pending_lockon_bits.erase(s_pending_lockon_bits.begin() + i);
				}
			}
		}
	}

	// Also tick any background deferred dummy spawners (from multi-spawn or timed auto-respawn)
	if (!s_in_bg_spawner_tick) {
		std::vector<std::shared_ptr<player_spawner>> active_jobs;
		{
			std::lock_guard<std::mutex> bg_lk(s_bg_spawners_mtx);
			if (!s_background_deferred_spawners.empty()) {
				active_jobs = s_background_deferred_spawners;
			}
		}
		if (!active_jobs.empty()) {
			s_in_bg_spawner_tick = true;
			for (auto& sp : active_jobs) {
				if (sp) sp->tick_deferred_spawn();
			}
			s_in_bg_spawner_tick = false;
			std::lock_guard<std::mutex> bg_lk(s_bg_spawners_mtx);
			for (auto it = s_background_deferred_spawners.begin(); it != s_background_deferred_spawners.end();) {
				if (!*it ||
					(*it)->spawn_state == DeferredSpawnState::SPAWN_COMPLETE ||
					(*it)->spawn_state == DeferredSpawnState::SPAWN_FAILED_TIMEOUT ||
					(*it)->spawn_state == DeferredSpawnState::IDLE) {
					it = s_background_deferred_spawners.erase(it);
				} else {
					++it;
				}
			}
		}
	}
}

static std::mutex s_skin_registry_mtx;
static std::map<uint64_t, uint64_t> s_preferred_hero_skin_themes;
static std::map<uint32, uint64_t>   s_entity_skin_themes;

uint64_t player_spawner::get_hero_skin_theme_by_slot(uint64_t heroid, int slot_index) {
	if (heroid == 0 || slot_index < 0) return 0;
	auto* hero_base = stu_resources::GetByID((__int64)heroid);
	if (!hero_base || !hero_base->valid()) return 0;
	auto hero_obj = hero_base->to_editable();
	auto skins_list = hero_obj.get_argument_resreflist(0xADF43464u); // m_skins
	if (!skins_list.valid()) return 0;
	for (STUResourceReference* skin_ref : skins_list) {
		if (!skin_ref || !skin_ref->has_resource()) continue;
		STUBase<>* skin_base = skin_ref->get_STU();
		if (!skin_base) skin_base = stu_resources::GetByID(skin_ref->resource_id);
		if (!skin_base || !skin_base->valid()) continue;
		auto skin_obj = skin_base->to_editable();
		auto themes_list = skin_obj.get_argument_resreflist(0x558BBCDCu); // m_skinThemes
		if (!themes_list.valid()) continue;
		int idx = 0;
		for (STUResourceReference* theme_ref : themes_list) {
			if (theme_ref && theme_ref->resource_id != 0) {
				if (idx == slot_index) {
					return (uint64_t)theme_ref->resource_id;
				}
				idx++;
			}
		}
	}
	return 0;
}

uint64_t player_spawner::get_classic_hero_skin_theme(uint64_t heroid) {
	return get_hero_skin_theme_by_slot(heroid, 0);
}

void player_spawner::set_preferred_hero_skin_theme(uint64_t heroid, uint64_t skin_theme_id) {
	if (heroid == 0) return;
	std::lock_guard<std::mutex> lk(s_skin_registry_mtx);
	if (skin_theme_id == 0) {
		s_preferred_hero_skin_themes.erase(heroid);
	} else {
		s_preferred_hero_skin_themes[heroid] = skin_theme_id;
	}
}

uint64_t player_spawner::get_preferred_hero_skin_theme(uint64_t heroid) {
	if (heroid == 0) return 0;
	{
		std::lock_guard<std::mutex> lk(s_skin_registry_mtx);
		auto it = s_preferred_hero_skin_themes.find(heroid);
		if (it != s_preferred_hero_skin_themes.end() && it->second != 0) {
			return it->second;
		}
	}
	return get_classic_hero_skin_theme(heroid);
}

void player_spawner::set_entity_skin_theme(uint32 entid, uint64_t skin_theme_id) {
	if (entid == 0 || entid == 0xFFFFFFFFu) return;
	std::lock_guard<std::mutex> lk(s_skin_registry_mtx);
	if (skin_theme_id == 0) {
		s_entity_skin_themes.erase(entid);
	} else {
		s_entity_skin_themes[entid] = skin_theme_id;
	}
}

uint64_t player_spawner::get_entity_skin_theme(uint32 ctrl_entid, uint32 mdl_entid, uint64_t fallback_heroid) {
	{
		std::lock_guard<std::mutex> lk(s_skin_registry_mtx);
		if (mdl_entid != 0) {
			auto it = s_entity_skin_themes.find(mdl_entid);
			if (it != s_entity_skin_themes.end() && it->second != 0) return it->second;
		}
		if (ctrl_entid != 0) {
			auto it = s_entity_skin_themes.find(ctrl_entid);
			if (it != s_entity_skin_themes.end() && it->second != 0) return it->second;
		}
	}
	DummySpawnRecord rec{};
	if (mdl_entid != 0 && get_dummy_record(mdl_entid, rec) && rec.skin_theme_id != 0) {
		return rec.skin_theme_id;
	}
	if (ctrl_entid != 0 && get_dummy_record(ctrl_entid, rec) && rec.skin_theme_id != 0) {
		return rec.skin_theme_id;
	}
	if (fallback_heroid != 0) {
		return get_classic_hero_skin_theme(fallback_heroid);
	}
	return 0;
}

void player_spawner::cleanup_model_child_slots(EntityAdminBase* ea, Entity* m_ent) {
	if (!ea || !m_ent) return;
	uint32 model_entid = m_ent->entity_id;
	std::vector<uint32> child_entids;
	auto push_unique_child = [&](uint32 cid) -> bool {
		if (cid == 0 || cid == 0xFFFFFFFFu || cid == model_entid || cid == ea->local_entid) return false;
		if (std::find(child_entids.begin(), child_entids.end(), cid) == child_entids.end()) {
			child_entids.push_back(cid);
			return true;
		}
		return false;
	};

	if (auto* mc23 = m_ent->getById<Component_23_Statescript>(0x23)) {
		if (!IsBadReadPtr(mc23, sizeof(Component_23_Statescript))) {
			uintptr_t slot_mgr = (uintptr_t)mc23 + 0x1458;
			int32_t rec_cnt = *(int32_t*)(slot_mgr + 0x10);
			uintptr_t recs  = *(uintptr_t*)(slot_mgr + 0x08);
			std::vector<uint64_t> slot_keys;
			if (rec_cnt > 0 && recs && !IsBadReadPtr((const void*)recs, 0x250)) {
				for (int idx = 0; idx < rec_cnt && idx < 16; ++idx) {
					uintptr_t r = recs + (size_t)idx * 0x250;
					if (IsBadReadPtr((const void*)r, 0x250)) break;
					// slot0 (+0x00) and slot1 (+0x04) store Entity::entity_global_id (resolved via +0x80E6C0 in +0xD55AA0)
					uint32_t gid0 = *(uint32_t*)(r + 0x00);
					uint32_t gid1 = *(uint32_t*)(r + 0x04);
					push_unique_child(ResolveGlobalIdToEntityId(ea, gid0));
					push_unique_child(ResolveGlobalIdToEntityId(ea, gid1));
					*(uint32_t*)(r + 0x00) = 0xFFFFFFFFu;
					*(uint32_t*)(r + 0x04) = 0xFFFFFFFFu;
					uint64_t key = *(uint64_t*)(r + 0x230);
					if (key != 0) slot_keys.push_back(key);
				}
			}
			auto cleanup_slot_fn = (void(__fastcall*)(uintptr_t, uint64_t))(globals::gameBase + 0xD55AA0);
			for (auto it = slot_keys.rbegin(); it != slot_keys.rend(); ++it) {
				cleanup_slot_fn(slot_mgr, *it);
			}
		}
	}

	// Collect direct and transitive children of m_ent (3P weapons -> barrel muzzles / sights)
	for (int pass = 0; pass < 3; ++pass) {
		bool added_any = false;
		std::vector<Entity*> parent_ents;
		std::vector<uintptr_t> parent_c1s;
		parent_ents.push_back(m_ent);
		if (auto* m_c1 = m_ent->getById<Component_1_SceneRendering>(1)) {
			parent_c1s.push_back((uintptr_t)m_c1);
		}
		for (uint32 cid : child_entids) {
			if (Entity* ce = ea->getEntById(cid)) {
				parent_ents.push_back(ce);
				if (auto* cc1 = ce->getById<Component_1_SceneRendering>(1)) {
					parent_c1s.push_back((uintptr_t)cc1);
				}
			}
		}
		for (Entity* ent : *ea) {
			if (!ent || ent == m_ent || ent->entity_id == ea->local_entid) continue;
			if (ent->hasComponent(0x15)) continue;
			if (ent->parent && std::find(parent_ents.begin(), parent_ents.end(), ent->parent) != parent_ents.end()) {
				if (push_unique_child(ent->entity_id)) added_any = true;
				continue;
			}
			if (auto* cc1 = ent->getById<Component_1_SceneRendering>(1)) {
				if (!IsBadReadPtr(cc1, sizeof(Component_1_SceneRendering))) {
					uintptr_t pc1 = *(uintptr_t*)((uintptr_t)cc1 + 0x1D0);
					if (pc1 != 0 && std::find(parent_c1s.begin(), parent_c1s.end(), pc1) != parent_c1s.end()) {
						if (push_unique_child(ent->entity_id)) added_any = true;
					}
				}
			}
		}
		if (!added_any) break;
	}

	// Clean up leaf descendants first, then 3P weapons
	for (auto it = child_entids.rbegin(); it != child_entids.rend(); ++it) {
		Entity* ce = ea->getEntById(*it);
		if (!ce) continue;
		if (!HasValidSceneNode148(ce)) {
			HideNullSceneNodeEntity(ce);
			continue;
		}
		if (auto* cc1 = ce->getById<Component_1_SceneRendering>(1)) {
			SanitizeComponent1BeforeTeardown(cc1);
		}
		ea->delEnt(ce);
	}
}

void player_spawner::register_or_update_dummy_record(const DummySpawnRecord& rec) {
	if (rec.model_entid == 0 && rec.controller_entid == 0) return;
	std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
	for (auto& existing : s_dummy_records) {
		if ((rec.model_entid != 0 && existing.model_entid == rec.model_entid) ||
			(rec.controller_entid != 0 && existing.controller_entid == rec.controller_entid)) {
			existing = rec;
			return;
		}
	}
	s_dummy_records.push_back(rec);
}

bool player_spawner::get_dummy_record(uint32 model_or_ctrl_entid, DummySpawnRecord& out_rec) {
	if (model_or_ctrl_entid == 0) return false;
	std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
	for (const auto& rec : s_dummy_records) {
		if (rec.model_entid == model_or_ctrl_entid || rec.controller_entid == model_or_ctrl_entid) {
			out_rec = rec;
			return true;
		}
	}
	return false;
}

void player_spawner::update_dummy_saved_transform(uint32 model_entid, const Vector4& pos, const Vector4& rot) {
	if (model_entid == 0) return;
	Vector4 valid_rot = rot;
	if (valid_rot.X == 0.0f && valid_rot.Y == 0.0f && valid_rot.Z == 0.0f && valid_rot.W == 0.0f) {
		valid_rot.W = 1.0f;
	}
	std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
	for (auto& rec : s_dummy_records) {
		if (rec.model_entid == model_entid || rec.controller_entid == model_entid) {
			rec.spawn_position = pos;
			rec.spawn_rotation = valid_rot;
			return;
		}
	}
}

void player_spawner::update_dummy_saved_team(uint32 model_entid, int team) {
	if (model_entid == 0 || team < 0) return;
	std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
	for (auto& rec : s_dummy_records) {
		if (rec.model_entid == model_entid || rec.controller_entid == model_entid) {
			rec.team = team;
			return;
		}
	}
}

void player_spawner::mark_dummy_killfeed_emitted(uint32 model_entid) {
	if (model_entid == 0) return;
	std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
	for (auto& rec : s_dummy_records) {
		if (rec.model_entid == model_entid || rec.controller_entid == model_entid) {
			rec.killfeed_emitted = true;
			return;
		}
	}
}

bool player_spawner::notify_dummy_death(uint32 ctrl_entid, uint32 model_entid, uint32_t respawn_delay_ms) {
	uint64_t now_ms = GetTickCount64();
	std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
	for (auto& rec : s_dummy_records) {
		if ((model_entid != 0 && rec.model_entid == model_entid) ||
			(ctrl_entid != 0 && rec.controller_entid == ctrl_entid)) {
			if (!rec.is_dead) {
				rec.is_dead = true;
				rec.death_timestamp_ms = now_ms;
				rec.respawn_queued = false;
				CombatReplication::s_total_dummy_deaths++;
			}
			if (respawn_delay_ms != CombatReplication::RESPAWN_WAIT_FOR_SERVER_CMD) {
				rec.respawn_due_ms = rec.death_timestamp_ms + respawn_delay_ms;
			} else {
				rec.respawn_due_ms = 0;
			}
			return true;
		}
	}
	return false;
}

bool player_spawner::safe_cleanup_dummy_corpse(EntityAdminBase* ea, uint32 ctrl_entid, uint32 model_entid, bool remove_record) {
	if (!ea) ea = GameEntityAdmin();
	if (!ea) return false;

	uint32 local_ctrl_id = ea->local_entid;
	uint32 local_mdl_id = 0;
	if (local_ctrl_id) {
		if (Entity* lctrl = ea->getEntById(local_ctrl_id)) {
			if (auto* lc20 = lctrl->getById<Component_20_ModelReference>(0x20)) {
				local_mdl_id = lc20->cam_attach_entid ? lc20->cam_attach_entid : (lc20->aim_entid ? lc20->aim_entid : lc20->movement_attach_entid);
			}
		}
	}

	// Resolve missing half of the (controller, model) pair if only one ID was passed
	if (model_entid == 0 && ctrl_entid != 0) {
		if (Entity* ce = ea->getEntById(ctrl_entid)) {
			if (auto* c20 = ce->getById<Component_20_ModelReference>(0x20)) {
				model_entid = c20->cam_attach_entid ? c20->cam_attach_entid : (c20->aim_entid ? c20->aim_entid : c20->movement_attach_entid);
			}
		}
	}
	if (ctrl_entid == 0 && model_entid != 0) {
		if (Entity* me = ea->getEntById(model_entid)) {
			if (auto* c1f = me->getById<Component_1F_Backref>(0x1F)) {
				ctrl_entid = c1f->backref_entid;
			}
		}
	}

	// Never delete the LocalPlayer via dummy corpse cleanup
	if ((local_ctrl_id != 0 && (ctrl_entid == local_ctrl_id || model_entid == local_ctrl_id)) ||
		(local_mdl_id != 0 && (ctrl_entid == local_mdl_id || model_entid == local_mdl_id))) {
		return false;
	}

	// 1. Cancel any pending delayed 0x1 lock-on bit for this model
	if (model_entid != 0) {
		std::lock_guard<std::mutex> lk(s_lockon_bit_mtx);
		for (auto it = s_pending_lockon_bits.begin(); it != s_pending_lockon_bits.end();) {
			if (it->entid == model_entid) {
				it = s_pending_lockon_bits.erase(it);
			} else {
				++it;
			}
		}
	}

	// 2. Cancel any pending background deferred spawner for these entity IDs
	{
		std::lock_guard<std::mutex> bg_lk(s_bg_spawners_mtx);
		for (auto it = s_background_deferred_spawners.begin(); it != s_background_deferred_spawners.end();) {
			if (*it && ((model_entid != 0 && (*it)->pending_model_entid == model_entid) ||
			            (ctrl_entid != 0 && (*it)->pending_controller_entid == ctrl_entid))) {
				it = s_background_deferred_spawners.erase(it);
			} else {
				++it;
			}
		}
	}

	// 3. Optionally remove from dummy spawn records
	if (remove_record) {
		std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
		for (auto it = s_dummy_records.begin(); it != s_dummy_records.end();) {
			if ((model_entid != 0 && it->model_entid == model_entid) ||
				(ctrl_entid != 0 && it->controller_entid == ctrl_entid)) {
				it = s_dummy_records.erase(it);
			} else {
				++it;
			}
		}
	}

	// 4. Notify CombatReplication to clear local_c2f + 0x15E0 target_entid (+0x00, preserving timestamp +0x04)
	//    and untrack any HealthbarViewModel sub-objects bound to this entity
	CombatReplication::NotifyEntityAboutToBeDeleted(ctrl_entid, model_entid);

	// 5. Stop any active voice lines on the dying/dead model entity (+0xC40950)
	if (model_entid != 0) {
		auto get_voicesys = (uintptr_t(__fastcall*)(EntityAdminBase*))(globals::gameBase + 0xC71B50);
		auto stop_voice = (void(__fastcall*)(uintptr_t, uint32_t, uint32_t))(globals::gameBase + 0xC40950);
		if (uintptr_t vs = get_voicesys(ea)) {
			stop_voice(vs, model_entid, 0);
		}
	}

	Entity* m_ent = model_entid ? ea->getEntById(model_entid) : nullptr;
	Entity* c_ent = ctrl_entid ? ea->getEntById(ctrl_entid) : nullptr;
	if (!m_ent && !c_ent) {
		return false;
	}

	// 6. Safely disarm Component_23 + 0x1458 child weapon/effect slots and transitive Component_1 + 0x1D0 children on m_ent BEFORE delEnt(m_ent)
	if (m_ent) {
		cleanup_model_child_slots(ea, m_ent);
	}

	// 7. Sanitize & delete model_ent and controller_ent
	if (m_ent) {
		if (auto* slot = OriginClutterTracker::GetSlot(m_ent->entity_id, m_ent)) {
			slot->entity_id = 0;
			slot->has_nonzero_pos = false;
			slot->culled_by_heracles = false;
		}
		if (HasValidSceneNode148(m_ent)) {
			if (auto* mc1 = m_ent->getById<Component_1_SceneRendering>(1)) {
				SanitizeComponent1BeforeTeardown(mc1);
			}
			ea->delEnt(m_ent);
		} else {
			HideNullSceneNodeEntity(m_ent);
		}
	}

	if (c_ent) {
		if (auto* cc1 = c_ent->getById<Component_1_SceneRendering>(1)) {
			SanitizeComponent1BeforeTeardown(cc1);
		}
		ea->delEnt(c_ent);
	}

	CombatReplication::s_total_corpses_cleaned++;
	return true;
}

std::pair<Entity*, Entity*> player_spawner::respawn_dummy_from_record(EntityAdminBase* ea, const DummySpawnRecord& rec, bool cleanup_corpse_first, bool queue_lockon) {
	if (!ea) ea = GameEntityAdmin();
	if (!ea) return {};

	if (cleanup_corpse_first && (rec.controller_entid != 0 || rec.model_entid != 0)) {
		safe_cleanup_dummy_corpse(ea, rec.controller_entid, rec.model_entid, /*remove_record=*/true);
	}

	uint64_t heroid = rec.heroid;
	uint64_t model_res_id = rec.model_res_id;
	if (model_res_id == 0 && heroid != 0) {
		if (auto* hero_stu = stu_resources::GetByID((__int64)heroid)) {
			auto stu = hero_stu->to_editable();
			if (auto* ent_ref = stu.get_argument_resource("m_gameplayEntity")) {
				model_res_id = (uint64_t)ent_ref->resource_id;
			}
		}
	}
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
	if (model_res_id == 0) {
		model_res_id = 0x0400000000000001ULL; // Default Tracer
		if (heroid == 0) heroid = 0x02E0000000000003ULL;
	}
	if (heroid == 0x02E000000000007AULL && model_res_id == 0x040000000000040AULL) {
		model_res_id = 0x0400000000000407ULL; // D.Va Mech
	}

	Vector4 pos = rec.spawn_position;
	Vector4 rot = rec.spawn_rotation;
	if (rot.X == 0.0f && rot.Y == 0.0f && rot.Z == 0.0f && rot.W == 0.0f) {
		rot.W = 1.0f;
	}
	int team = (rec.team >= 0 && rec.team <= 3) ? rec.team : 1;

	auto spawner = std::make_shared<player_spawner>((__int64)model_res_id);
	spawner->use_deferred_spawn = true;
	spawner->controller_info.resource_id = 0x40000000000000EULL;
	spawner->controller_info.aim_attach = 2;
	spawner->controller_info.cam_attach = 2;
	spawner->controller_info.movement_attach = 2;
	spawner->controller_info.set_localent = false;
	spawner->controller_info.set_2f_movable = false;
	spawner->controller_info.load_2f_33 = false;
	spawner->controller_info.load_3f = true;
	spawner->controller_info.set_filterbits_controller = false;
	spawner->controller_info.heroid = (__int64)heroid;
	spawner->controller_info.team = team;

	spawner->model_info.resource_id = (__int64)model_res_id;
	spawner->model_info.heroid = (__int64)heroid;
	spawner->model_info.skin_theme_id = (__int64)rec.skin_theme_id;
	spawner->model_info.petbackref_attach = 1;
	spawner->model_info.set_health = true;
	spawner->model_info.weapon_attach = true;
	spawner->model_info.fixup_player_statescript = true;
	spawner->model_info.team = team;
	spawner->model_info.set_position = true;
	spawner->model_info.spawn_position = pos;
	spawner->model_info.spawn_rotation = rot;

	auto ents = spawner->spawn();
	if (ents.first && ents.second) {
		if (!queue_lockon) {
			std::lock_guard<std::mutex> lk(s_lockon_bit_mtx);
			for (auto it = s_pending_lockon_bits.begin(); it != s_pending_lockon_bits.end();) {
				if (it->entid == ents.second->entity_id) {
					it = s_pending_lockon_bits.erase(it);
				} else {
					++it;
				}
			}
		}
		{
			std::lock_guard<std::mutex> bg_lk(s_bg_spawners_mtx);
			s_background_deferred_spawners.push_back(spawner);
		}
		CombatReplication::s_total_dummy_respawns++;
		CombatReplication::s_last_dummy_respawn_status = std::format(
			"DummyRespawn: M:0x{:X} C:0x{:X} hero=0x{:X} team={} at ({:.1f}, {:.1f}, {:.1f})",
			ents.second->entity_id, ents.first->entity_id, (unsigned long long)heroid, team, pos.X, pos.Y, pos.Z);
		printf("[DummyRespawn] %s\n", CombatReplication::s_last_dummy_respawn_status.c_str());
	}
	return ents;
}

bool player_spawner::execute_dummy_respawn_command(EntityAdminBase* ea, const CombatReplication::ServerEntityRespawnCommand& rcmd) {
	std::lock_guard<std::mutex> lk(s_pending_dummy_ops_mtx);
	s_queued_dummy_respawns.push_back(rcmd);
	return true;
}

void player_spawner::queue_corpse_cleanup_command(const CombatReplication::ServerCorpseCleanupCommand& ccmd) {
	std::lock_guard<std::mutex> lk(s_pending_dummy_ops_mtx);
	s_queued_corpse_cleanups.push_back(ccmd);
}

void player_spawner::process_pending_dummy_operations(EntityAdminBase* ea) {
	if (!ea) ea = GameEntityAdmin();
	if (!ea) return;

	std::vector<CombatReplication::ServerCorpseCleanupCommand> cleanups;
	std::vector<CombatReplication::ServerEntityRespawnCommand> respawns;
	{
		std::lock_guard<std::mutex> lk(s_pending_dummy_ops_mtx);
		if (!s_queued_corpse_cleanups.empty()) {
			cleanups.swap(s_queued_corpse_cleanups);
		}
		if (!s_queued_dummy_respawns.empty()) {
			respawns.swap(s_queued_dummy_respawns);
		}
	}

	for (const auto& ccmd : cleanups) {
		safe_cleanup_dummy_corpse(ea, ccmd.target_ctrl_entid, ccmd.target_mdl_entid, ccmd.remove_spawn_record != 0);
	}

	for (const auto& rcmd : respawns) {
		DummySpawnRecord rec{};
		uint32 lookup_id = rcmd.target_mdl_entid ? rcmd.target_mdl_entid : rcmd.target_ctrl_entid;
		if (!get_dummy_record(lookup_id, rec)) {
			rec.controller_entid = rcmd.target_ctrl_entid;
			rec.model_entid      = rcmd.target_mdl_entid;
			Entity* m_ent = rec.model_entid ? ea->getEntById(rec.model_entid) : nullptr;
			Entity* c_ent = rec.controller_entid ? ea->getEntById(rec.controller_entid) : nullptr;
			if (!m_ent && c_ent) {
				if (auto* c20 = c_ent->getById<Component_20_ModelReference>(0x20)) {
					rec.model_entid = c20->cam_attach_entid ? c20->cam_attach_entid : (c20->aim_entid ? c20->aim_entid : c20->movement_attach_entid);
					m_ent = ea->getEntById(rec.model_entid);
				}
			}
			if (!c_ent && m_ent) {
				if (auto* c1f = m_ent->getById<Component_1F_Backref>(0x1F)) {
					rec.controller_entid = c1f->backref_entid;
					c_ent = ea->getEntById(rec.controller_entid);
				}
			}
			if (c_ent) {
				if (auto* c3f = c_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
					rec.heroid = (uint64_t)c3f->selected_heroid;
				}
			}
			if (m_ent) {
				if (m_ent->resload_entry && m_ent->resload_entry->valid()) {
					rec.model_res_id = (uint64_t)m_ent->resload_entry->align()->resource_id;
				}
				if (auto* c10 = m_ent->getById<Component_10_FilterBits>(0x10)) {
					rec.team = ExtractTeamIndexFromFilterBits(c10, 1);
				}
				if (auto* c1 = m_ent->getById<Component_1_SceneRendering>(1)) {
					rec.spawn_position = c1->position;
					rec.spawn_rotation = c1->rotation;
				}
			}
		}

		if (rcmd.override_heroid != 0) {
			if (rcmd.override_heroid != rec.heroid && rcmd.override_model_resid == 0) {
				rec.model_res_id = 0; // Force m_gameplayEntity resolution for the new hero
				if (rcmd.override_skin_theme_id == 0) {
					rec.skin_theme_id = get_preferred_hero_skin_theme(rcmd.override_heroid);
				}
			}
			rec.heroid = rcmd.override_heroid;
		}
		if (rcmd.override_model_resid != 0)   rec.model_res_id  = rcmd.override_model_resid;
		if (rcmd.override_skin_theme_id != 0) rec.skin_theme_id = rcmd.override_skin_theme_id;
		if (rcmd.override_team != CombatReplication::TEAM_AUTO) {
			rec.team = (int)rcmd.override_team;
		}
		if (rcmd.has_explicit_spawn_transform) {
			rec.spawn_position = rcmd.spawn_position;
			rec.spawn_rotation = rcmd.spawn_rotation;
		}

		respawn_dummy_from_record(ea, rec, rcmd.cleanup_existing_corpse != 0, rcmd.apply_delayed_lockon_bit != 0);
	}
}

void player_spawner::tick_dummy_respawns(EntityAdminBase* ea) {
	tick_pending_lockon_bits();
	if (!ea) ea = GameEntityAdmin();
	if (!ea) return;

	uint32 local_ctrl_id = ea->local_entid;
	uint32 local_mdl_id = 0;
	if (local_ctrl_id) {
		if (Entity* lctrl = ea->getEntById(local_ctrl_id)) {
			if (auto* lc20 = lctrl->getById<Component_20_ModelReference>(0x20)) {
				local_mdl_id = lc20->cam_attach_entid ? lc20->cam_attach_entid : (lc20->aim_entid ? lc20->aim_entid : lc20->movement_attach_entid);
			}
		}
	}

	uint64_t now_ms = GetTickCount64();
	std::vector<CombatReplication::ServerEntityDeathCommand>   to_enqueue_deaths;
	std::vector<CombatReplication::ServerEntityRespawnCommand> to_enqueue_respawns;

	{
		std::lock_guard<std::mutex> lk(s_dummy_records_mtx);

		// 1. Prune records whose entities were externally deleted (and aren't awaiting respawn)
		for (auto it = s_dummy_records.begin(); it != s_dummy_records.end();) {
			if (it->model_entid == local_mdl_id || it->controller_entid == local_ctrl_id) {
				it = s_dummy_records.erase(it);
				continue;
			}
			Entity* m_ent = it->model_entid ? ea->getEntById(it->model_entid) : nullptr;
			if (!m_ent && !it->respawn_queued) {
				it = s_dummy_records.erase(it);
			} else {
				++it;
			}
		}

		// 2. Scan live non-local hero dummies in GameEntityAdmin
		for (Entity* ent : *ea) {
			if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
			if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;

			uint32 mdl_id = ent->entity_id;
			uint32 ctrl_id = 0;
			if (auto* c1f = ent->getById<Component_1F_Backref>(0x1F)) {
				ctrl_id = c1f->backref_entid;
			}
			Entity* ctrl_ent = ctrl_id ? ea->getEntById(ctrl_id) : nullptr;
			auto* c1  = ent->getById<Component_1_SceneRendering>(1);
			auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
			auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
			if (!c28) continue;

			float cur_hp = c28->CalcTotalCurrentHealth();
			float max_hp = c28->CalcTotalMaxHealth();

			DummySpawnRecord* rec_ptr = nullptr;
			for (auto& r : s_dummy_records) {
				if (r.model_entid == mdl_id || (ctrl_id != 0 && r.controller_entid == ctrl_id)) {
					rec_ptr = &r;
					break;
				}
			}
			if (!rec_ptr) {
				DummySpawnRecord new_rec{};
				new_rec.controller_entid = ctrl_id;
				new_rec.model_entid      = mdl_id;
				if (ctrl_ent) {
					if (auto* c3f = ctrl_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
						new_rec.heroid = (uint64_t)c3f->selected_heroid;
					}
				}
				if (ent->resload_entry && ent->resload_entry->valid()) {
					new_rec.model_res_id = (uint64_t)ent->resload_entry->align()->resource_id;
				}
				new_rec.team = ExtractTeamIndexFromFilterBits(c10, 1);
				if (c1) {
					new_rec.spawn_position = c1->position;
					new_rec.spawn_rotation = c1->rotation;
				}
				s_dummy_records.push_back(new_rec);
				rec_ptr = &s_dummy_records.back();
			}

			rec_ptr->model_entid = mdl_id;
			if (ctrl_id != 0) rec_ptr->controller_entid = ctrl_id;
			if (c10) rec_ptr->team = ExtractTeamIndexFromFilterBits(c10, rec_ptr->team);
			if (rec_ptr->heroid == 0 && ctrl_ent) {
				if (auto* c3f = ctrl_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
					rec_ptr->heroid = (uint64_t)c3f->selected_heroid;
				}
			}
			if (rec_ptr->model_res_id == 0 && ent->resload_entry && ent->resload_entry->valid()) {
				rec_ptr->model_res_id = (uint64_t)ent->resload_entry->align()->resource_id;
			}
			if (c1) {
				float saved_mag = std::fabs(rec_ptr->spawn_position.X) + std::fabs(rec_ptr->spawn_position.Y) + std::fabs(rec_ptr->spawn_position.Z);
				float live_mag  = std::fabs(c1->position.X) + std::fabs(c1->position.Y) + std::fabs(c1->position.Z);
				if (saved_mag < 0.05f && live_mag >= 0.05f) {
					rec_ptr->spawn_position = c1->position;
					rec_ptr->spawn_rotation = c1->rotation;
				}
			}

			if (max_hp > 0.0f && cur_hp <= 0.0f) {
				// Dummy is at 0 HP (ragdoll corpse)
				uint64_t delay_ms = (uint64_t)((std::max)(0.5f, CombatReplication::s_dummy_respawn_delay_sec) * 1000.0f + 0.5f);
				if (!rec_ptr->is_dead) {
					rec_ptr->is_dead            = true;
					rec_ptr->death_timestamp_ms = now_ms;
					rec_ptr->respawn_queued     = false;
					rec_ptr->respawn_due_ms     = CombatReplication::s_enable_dummy_auto_respawn ? (now_ms + delay_ms) : 0;
					CombatReplication::s_total_dummy_deaths++;

					// Enqueue ServerEntityDeathCommand (if lethal damage packet already emitted killfeed & scream, suppress duplicate)
					CombatReplication::ServerEntityDeathCommand dcmd{};
					dcmd.victim_ctrl_entid     = rec_ptr->controller_entid;
					dcmd.victim_mdl_entid      = rec_ptr->model_entid;
					dcmd.killer_entid          = local_mdl_id ? local_mdl_id : local_ctrl_id;
					dcmd.kill_flags            = CombatReplication::KILL_FLAG_SOLO_ELIM;
					dcmd.respawn_delay_ms      = CombatReplication::s_enable_dummy_auto_respawn
						? (uint32_t)delay_ms
						: CombatReplication::RESPAWN_WAIT_FOR_SERVER_CMD;
					dcmd.trigger_death_scream  = rec_ptr->killfeed_emitted ? 0 : 1;
					dcmd.trigger_status_banner = 0;
					dcmd.emit_killfeed_entry   = rec_ptr->killfeed_emitted ? 0 : 1;
					rec_ptr->killfeed_emitted  = true;
					to_enqueue_deaths.push_back(dcmd);
				} else if (!rec_ptr->respawn_queued) {
					// Dynamically keep respawn_due_ms synced with live toggle & slider changes
					if (CombatReplication::s_enable_dummy_auto_respawn) {
						rec_ptr->respawn_due_ms = rec_ptr->death_timestamp_ms + delay_ms;
						if (now_ms >= rec_ptr->respawn_due_ms) {
							rec_ptr->respawn_queued = true;
							CombatReplication::ServerEntityRespawnCommand rcmd{};
							rcmd.target_ctrl_entid            = rec_ptr->controller_entid;
							rcmd.target_mdl_entid             = rec_ptr->model_entid;
							rcmd.override_heroid              = rec_ptr->heroid;
							rcmd.override_model_resid         = rec_ptr->model_res_id;
							rcmd.override_skin_theme_id       = rec_ptr->skin_theme_id;
							rcmd.override_team                = (uint8_t)rec_ptr->team;
							rcmd.apply_delayed_lockon_bit     = 1;
							rcmd.cleanup_existing_corpse      = 1;
							rcmd.has_explicit_spawn_transform = 1;
							rcmd.spawn_position               = rec_ptr->spawn_position;
							rcmd.spawn_rotation               = rec_ptr->spawn_rotation;
							to_enqueue_respawns.push_back(rcmd);
						}
					} else {
						rec_ptr->respawn_due_ms = 0;
					}
				}
			} else if (cur_hp > 0.0f && rec_ptr->is_dead) {
				// Dummy was healed back above 0 HP (e.g. via "Reset HP" button)
				rec_ptr->is_dead            = false;
				rec_ptr->killfeed_emitted   = false;
				rec_ptr->death_timestamp_ms = 0;
				rec_ptr->respawn_due_ms     = 0;
				rec_ptr->respawn_queued     = false;
			}
		}
	}

	for (const auto& dcmd : to_enqueue_deaths) {
		CombatReplication::EnqueueEntityDeathCommand(dcmd);
	}
	for (const auto& rcmd : to_enqueue_respawns) {
		CombatReplication::EnqueueEntityRespawnCommand(rcmd);
	}
}

void player_spawner::respawn_all_dead_dummies_now(EntityAdminBase* ea) {
	if (!ea) ea = GameEntityAdmin();
	if (!ea) return;

	std::vector<CombatReplication::ServerEntityRespawnCommand> cmds;
	{
		std::lock_guard<std::mutex> lk(s_dummy_records_mtx);
		for (auto& rec : s_dummy_records) {
			Entity* m_ent = rec.model_entid ? ea->getEntById(rec.model_entid) : nullptr;
			auto* c28 = m_ent ? m_ent->getById<Component_28_STUHealthComponent>(0x28) : nullptr;
			bool dead_now = rec.is_dead || (c28 && c28->CalcTotalMaxHealth() > 0.0f && c28->CalcTotalCurrentHealth() <= 0.0f);
			if (dead_now && !rec.respawn_queued) {
				rec.respawn_queued = true;
				CombatReplication::ServerEntityRespawnCommand rcmd{};
				rcmd.target_ctrl_entid            = rec.controller_entid;
				rcmd.target_mdl_entid             = rec.model_entid;
				rcmd.override_heroid              = rec.heroid;
				rcmd.override_model_resid         = rec.model_res_id;
				rcmd.override_skin_theme_id       = rec.skin_theme_id;
				rcmd.override_team                = (uint8_t)rec.team;
				rcmd.apply_delayed_lockon_bit     = 1;
				rcmd.cleanup_existing_corpse      = 1;
				rcmd.has_explicit_spawn_transform = 1;
				rcmd.spawn_position               = rec.spawn_position;
				rcmd.spawn_rotation               = rec.spawn_rotation;
				cmds.push_back(rcmd);
			}
		}
	}
	for (const auto& rcmd : cmds) {
		CombatReplication::EnqueueEntityRespawnCommand(rcmd);
	}
}

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
	} else if (model_res_id == 0x040000000000040AULL) {
		norm_hp = 150.0f;
	} else if (heroid == 0x02E000000000007AULL || model_res_id == 0x0400000000000407ULL || (hero_name && strstr(hero_name, "D.Va"))) {
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
		if (info.set_localent || controller_info.set_localent) {
			player_info->m_slot = 3;
		} else {
			static int s_dummy_slot_counter = 0;
			static uint32 s_last_slot_ctrl_id = 0;
			uint32 cur_ctrl_id = controller_ent ? controller_ent->entity_id : ent->entity_id;
			if (cur_ctrl_id != 0 && cur_ctrl_id != s_last_slot_ctrl_id) {
				s_last_slot_ctrl_id = cur_ctrl_id;
				s_dummy_slot_counter++;
			}
			const uint8_t kDummySlots[5] = { 1, 2, 4, 5, 6 };
			player_info->m_slot = kDummySlots[s_dummy_slot_counter % 5];
		}
		player_info->selected_heroid = info.heroid;
		player_info->unconformed_selected_heroid = info.heroid;
		player_info->localplayer_heroselected = true;
	}

	Component_10_FilterBits* filter_bits = ent->getById<Component_10_FilterBits>(0x10);
	if (filter_bits) {
		if (info.set_filterbits_controller) {
			filter_bits->set_filterbits_spectator();
		} else {
			// Non-local dummy models get the same 0x800000 base as the LocalPlayer. Filter bit 0x1 (native lock-on
			// candidate bit for 0x725967AA / 0x498B421A, e.g. Zen Harmony/Discord) is applied ~1s LATER via
			// queue_lockon_bit(): if 0x1 is present while the overhead nameplate/healthbar UI is created, that UI is
			// never created; toggling 0x1 afterwards affects lock-on live without touching the UI (verified in-game).
			// NEVER set 0x400000 on dummies: it makes them non-hostile allies and disables shot collision (d91fb68 regression).
			filter_bits->pre_filter_bits |= 0x800000;
			filter_bits->filter_bits |= 0x800000;
			if (ent == model_ent && !controller_info.set_localent && !info.set_localent) {
				queue_lockon_bit(ent->entity_id);
			}
		}
		if (info.team != -1) {
			filter_bits->set_team(info.team);
		}
		filter_bits->FilterBitsMakeFinal();
		if (!info.set_filterbits_controller && ent == model_ent) {
			if (auto* c1 = ent->getById<Component_1_SceneRendering>(1)) {
				c1->SetVisible();
			}
		}
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
					inst->execution_flags = (inst->execution_flags | 0x400001u) & ~(0x100Cu | 0x200000u);
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
	// If this spawner instance already has an in-flight non-local dummy deferred spawn,
	// hand the previous job off to s_background_deferred_spawners so rapid multi-spawning never drops a dummy!
	if ((spawn_state == DeferredSpawnState::WAITING_FOR_COMPONENTS || spawn_state == DeferredSpawnState::APPLYING_POST_SPAWN_INIT) &&
		!pending_controller_info.set_localent && !pending_model_info.set_localent &&
		pending_controller_entid != 0 && pending_model_entid != 0) {
		auto prev_job = std::make_shared<player_spawner>(*this);
		std::lock_guard<std::mutex> bg_lk(s_bg_spawners_mtx);
		s_background_deferred_spawners.push_back(prev_job);
	}

	if (controller_info.heroid == 0 && model_info.heroid != 0) {
		controller_info.heroid = model_info.heroid;
	}
	if (controller_info.heroid == 0 && model_info.resource_id == 0x400000000000001) {
		controller_info.heroid = 0x02E0000000000003ULL;
		model_info.heroid = 0x02E0000000000003ULL;
	}
	// D.Va's STUHero::m_gameplayEntity points to Pilot D.Va (0x040000000000040A); spawn her in her Mech (0x0400000000000407)
	if (model_info.heroid == 0x02E000000000007AULL && model_info.resource_id == 0x040000000000040AULL) {
		model_info.resource_id = 0x0400000000000407ULL;
	}
	if (model_info.skin_theme_id == 0 && model_info.heroid != 0) {
		uint64_t pref_skin = get_preferred_hero_skin_theme((uint64_t)model_info.heroid);
		if (pref_skin != 0) {
			model_info.skin_theme_id = (__int64)pref_skin;
		}
	}

	controller_ent = spawn_ent(controller_info);
	model_ent = spawn_ent(model_info);
	if (!controller_ent || !model_ent) {
		return {};
	}

	uint64_t effective_skin = (model_info.skin_theme_id != 0)
		? (uint64_t)model_info.skin_theme_id
		: get_classic_hero_skin_theme((uint64_t)model_info.heroid);
	set_entity_skin_theme(controller_ent->entity_id, effective_skin);
	set_entity_skin_theme(model_ent->entity_id, effective_skin);

	if (model_info.set_health) {
		if (auto* health = model_ent->getById<Component_28_STUHealthComponent>(0x28)) {
			health->InitHeroHealthPools((uint64_t)model_info.heroid, (uint64_t)model_info.resource_id);
		}
	}

	// Register non-local dummy in s_dummy_records with its saved transform, hero, skin, and team
	if (!controller_info.set_localent && !model_info.set_localent) {
		DummySpawnRecord rec{};
		rec.controller_entid = controller_ent->entity_id;
		rec.model_entid      = model_ent->entity_id;
		rec.heroid           = (uint64_t)model_info.heroid;
		rec.model_res_id     = (uint64_t)model_info.resource_id;
		rec.skin_theme_id    = effective_skin;
		rec.team             = (model_info.team >= 0) ? model_info.team : ((controller_info.team >= 0) ? controller_info.team : 1);
		rec.spawn_position   = model_info.spawn_position;
		rec.spawn_rotation   = model_info.spawn_rotation;
		if (rec.spawn_rotation.X == 0.0f && rec.spawn_rotation.Y == 0.0f && rec.spawn_rotation.Z == 0.0f && rec.spawn_rotation.W == 0.0f) {
			rec.spawn_rotation.W = 1.0f;
		}
		register_or_update_dummy_record(rec);
	}

	if (!use_deferred_spawn) {
		modify_ent(controller_info, controller_ent);
		modify_ent(model_info, model_ent);
		auto on_backref_assigned = (void(__fastcall*)(Entity*, uint*))(globals::gameBase + 0xd07830);
		on_backref_assigned(model_ent, &controller_ent->entity_id);
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
	tick_deferred_spawn();

	return { controller_ent, model_ent };
}

void player_spawner::tick_deferred_spawn() {
	tick_pending_lockon_bits();
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
		modify_ent(pending_model_info, m_ent);

		if (c_ent && m_ent) {
			Component_1F_Backref* petref = m_ent->getById<Component_1F_Backref>(0x1F);
			if (petref && pending_model_info.petbackref_attach > 0) {
				Entity* backref_ent = pending_model_info.petbackref_attach == 1 ? c_ent : m_ent;
				petref->backref_entid = backref_ent->entity_id;
				petref->backref_global_entid = backref_ent->entity_global_id;
			}
			// +0xD07830 (OnEntityBackrefAssigned): checks Component_2F internally (+0xD078F6) for LocalPlayer,
			// and for all entities (including non-local dummies) calls +0x1070370(comp23, &c_ent->entity_id)
			// (Component_23::OnBackrefAssigned) at +0xD07C1B, setting has_player_entbackref = 1 and notifying
			// 0x058000000000004B via vt+0x270 to spawn child script 0x05800000000001C7 (WorldUI3D / HealthbarViewModel / Nameplate)!
			auto on_backref_assigned = (void(__fastcall*)(Entity*, uint*))(globals::gameBase + 0xd07830);
			on_backref_assigned(m_ent, &c_ent->entity_id);
		}

		auto* c23 = m_ent->getById<Component_23_Statescript>(0x23);
		if (c23) {
			bool has_1c7 = false;
			for (auto* inst : c23->ss_inner.g1_instanceArr) {
				if (inst && inst->script_id == 0x05800000000001C7ULL) {
					has_1c7 = true;
					break;
				}
			}
			if (!has_1c7) {
				c23->loadScript(0x05800000000001C7ULL);
			}
			bool is_local_model = (pending_controller_info.set_localent || (c_ent && ea->local_entid == c_ent->entity_id));
			for (auto* inst : c23->ss_inner.g1_instanceArr) {
				if (inst) {
					if (is_local_model) {
						inst->execution_flags = (inst->execution_flags | 0x400009u | 0x200000u) & ~0x1004u;
					} else {
						inst->execution_flags = (inst->execution_flags | 0x400001u) & ~(0x100Cu | 0x200000u);
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
