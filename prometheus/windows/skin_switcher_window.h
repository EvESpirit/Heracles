#pragma once
#include "../window_manager/window_manager.h"
#include "../entity_admin.h"
#include "../player_spawner.h"
#include "../STU.h"
#include "../STU_Editable.h"
#include "../stringhash_library.h"
#include "../combat_replication_cmds.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <format>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace SkinSwitcherUI {

struct SkinThemeEntry {
	int         slot_index          = 0;      // 0..3
	uint32_t    tier_enum           = 1;      // +0xA0 (1 = Classic, 2..4 = Recolor)
	uint64_t    theme_id            = 0;      // 0x0A50...
	uint64_t    display_str_id      = 0;      // 0x0DE0... (+0x60)
	std::string display_name;                 // Localized UX string (e.g. "Classic", "Cobalt", "Royal")
	float       swatch_rgba[4]      = { 0.5f, 0.5f, 0.5f, 1.0f }; // +0x90 m_color
	bool        raw_color_was_zero  = false;
	uint64_t    weapon_skin_0       = 0;      // 0x0350... (1P weapon override)
	uint64_t    weapon_skin_1       = 0;      // 0x0350... (3P weapon override)
	int         override_map_count  = 0;      // +0x30 STUBullshitMapFull entry count
};

struct HeroSkinEntry {
	uint64_t                    heroid             = 0; // 0x02E0...
	std::string                 hero_name;
	uint64_t                    gameplay_entity_id = 0; // 0x0400...
	uint64_t                    skin_id            = 0; // 0x0AE0...
	std::vector<SkinThemeEntry> themes;                 // 4 themes per hero
};

enum class WorldAllegiance : int {
	LOCAL_YOU = 0,
	FRIENDLY  = 1,
	ENEMY     = 2,
	NEUTRAL   = 3
};

struct WorldHeroInstance {
	uint32_t        ctrl_entid      = 0;
	uint32_t        mdl_entid       = 0;
	uint64_t        heroid          = 0;
	uint64_t        model_res_id    = 0;
	uint64_t        active_skin_id  = 0;
	int             team            = 1;
	bool            is_local        = false;
	bool            is_dead         = false;
	float           cur_hp          = 0.0f;
	float           max_hp          = 0.0f;
	Vector4         position        = {};
	Vector4         rotation        = { 0.0f, 0.0f, 0.0f, 1.0f };
	float           dist_to_local_m = 0.0f;
	WorldAllegiance allegiance      = WorldAllegiance::ENEMY;
};

inline std::vector<HeroSkinEntry> s_catalog;
inline bool                       s_catalog_loaded       = false;
inline int                        s_allegiance_filter    = -1; // -1 = All, 0 = You, 1 = Friendly, 2 = Enemy, 3 = Neutral
inline int                        s_quick_spawn_hero_idx = 0;
inline int                        s_quick_spawn_slot_idx = 0;
inline int                        s_quick_spawn_team     = 1;  // 0 = Friendly, 1 = Enemy, 2 = Neutral A, 3 = Neutral B
inline float                      s_quick_spawn_dist_m   = 4.5f;

inline std::string ResolveDisplayString(uint64_t str_res_id) {
	if (str_res_id == 0 || str_res_id == 0xFFFFFFFFFFFFFFFFULL) return "";
	const char* s = GetUXDisplayString((__int64)str_res_id);
	if (s && s[0] != '\0') return std::string(s);
	return "";
}

inline void EnsureSkinCatalogLoaded(bool force_reload = false) {
	if (s_catalog_loaded && !force_reload && !s_catalog.empty()) return;
	auto all_heroes = stu_resources::GetIfAssignableTo(stringHash("STUHero"));
	if (all_heroes.empty()) return;

	s_catalog.clear();
	for (auto& kv : all_heroes) {
		uint64_t heroid = (uint64_t)kv.first;
		STUBase<>* hero_base = kv.second;
		if (!hero_base || !hero_base->valid()) continue;

		HeroSkinEntry h_entry{};
		h_entry.heroid = heroid;
		auto hero_obj = hero_base->to_editable();

		if (auto* name_ref = hero_obj.get_argument_resource(0x0CD16BAAu)) {
			if (const char* s = name_ref->get_UXDisplayString()) {
				if (s[0] != '\0') h_entry.hero_name = s;
			}
		}
		if (h_entry.hero_name.empty()) {
			h_entry.hero_name = std::format("Hero_0x{:X}", heroid & 0xFFFFULL);
		}

		if (auto* gp_ref = hero_obj.get_argument_resource(0x3DEFF5B4u)) { // m_gameplayEntity
			h_entry.gameplay_entity_id = (uint64_t)gp_ref->resource_id;
			if (heroid == 0x02E000000000007AULL && h_entry.gameplay_entity_id == 0x040000000000040AULL) {
				h_entry.gameplay_entity_id = 0x0400000000000407ULL; // D.Va Mech
			}
		}

		auto skins_list = hero_obj.get_argument_resreflist(0xADF43464u); // m_skins
		if (skins_list.valid()) {
			for (STUResourceReference* skin_ref : skins_list) {
				if (!skin_ref || !skin_ref->has_resource()) continue;
				h_entry.skin_id = (uint64_t)skin_ref->resource_id;
				STUBase<>* skin_base = skin_ref->get_STU();
				if (!skin_base) skin_base = stu_resources::GetByID(skin_ref->resource_id);
				if (!skin_base || !skin_base->valid()) continue;

				auto skin_obj = skin_base->to_editable();
				auto themes_list = skin_obj.get_argument_resreflist(0x558BBCDCu); // m_skinThemes
				if (!themes_list.valid()) continue;

				int slot = 0;
				for (STUResourceReference* theme_ref : themes_list) {
					if (!theme_ref || theme_ref->resource_id == 0) continue;

					SkinThemeEntry t_entry{};
					t_entry.slot_index = slot;
					t_entry.theme_id   = (uint64_t)theme_ref->resource_id;

					STUBase<>* theme_base = theme_ref->get_STU();
					if (!theme_base) theme_base = stu_resources::GetByID(theme_ref->resource_id);
					if (theme_base && theme_base->valid() && !IsBadReadPtr(theme_base, 0xA8)) {
						uintptr_t raw_ptr = (uintptr_t)theme_base;

						// +0x30 (0x9B68B58A): STUBullshitMapFull* override map
						uintptr_t map_ptr = *(uintptr_t*)(raw_ptr + 0x30);
						if (map_ptr && !IsBadReadPtr((const void*)map_ptr, 0x10)) {
							int32_t cnt = *(int32_t*)(map_ptr + 0x08);
							if (cnt >= 0 && cnt < 10000) t_entry.override_map_count = cnt;
						}

						// +0x60 (0x0CD16BAA): UX display string reference
						t_entry.display_str_id = *(uint64_t*)(raw_ptr + 0x68);
						auto theme_obj = theme_base->to_editable();
						if (auto* dref = theme_obj.get_argument_resource(0x0CD16BAAu)) {
							if (const char* ds = dref->get_UXDisplayString()) {
								if (ds[0] != '\0') t_entry.display_name = ds;
							}
						}
						if (t_entry.display_name.empty()) {
							t_entry.display_name = ResolveDisplayString(t_entry.display_str_id);
						}

						// +0x50 (0xCD61BC4D): 2x 0x0350... STUWeaponSkin list
						auto weap_list = theme_obj.get_argument_resreflist(0xCD61BC4Du);
						if (weap_list.valid()) {
							int w_idx = 0;
							for (STUResourceReference* w_ref : weap_list) {
								if (w_ref) {
									if (w_idx == 0) t_entry.weapon_skin_0 = (uint64_t)w_ref->resource_id;
									else if (w_idx == 1) t_entry.weapon_skin_1 = (uint64_t)w_ref->resource_id;
									w_idx++;
								}
							}
						}

						// +0x90 (0x2AA1F78D): m_color (float[4] RGBA)
						const float* rgba = (const float*)(raw_ptr + 0x90);
						if (std::isfinite(rgba[0]) && std::isfinite(rgba[1]) && std::isfinite(rgba[2])) {
							float sum = fabsf(rgba[0]) + fabsf(rgba[1]) + fabsf(rgba[2]);
							if (sum > 0.001f) {
								t_entry.swatch_rgba[0] = std::clamp(rgba[0], 0.0f, 1.0f);
								t_entry.swatch_rgba[1] = std::clamp(rgba[1], 0.0f, 1.0f);
								t_entry.swatch_rgba[2] = std::clamp(rgba[2], 0.0f, 1.0f);
								t_entry.swatch_rgba[3] = (std::isfinite(rgba[3]) && rgba[3] > 0.05f) ? std::clamp(rgba[3], 0.0f, 1.0f) : 1.0f;
							} else {
								t_entry.raw_color_was_zero = true;
								t_entry.swatch_rgba[0] = 0.22f;
								t_entry.swatch_rgba[1] = 0.22f;
								t_entry.swatch_rgba[2] = 0.26f;
								t_entry.swatch_rgba[3] = 1.0f;
							}
						}

						// +0xA0 (0x32A11A46): tier enum (1..4)
						t_entry.tier_enum = *(uint32_t*)(raw_ptr + 0xA0);
					}

					if (t_entry.display_name.empty()) {
						t_entry.display_name = (slot == 0) ? "Classic" : std::format("Theme {}", slot + 1);
					}
					h_entry.themes.push_back(t_entry);
					slot++;
				}
			}
		}

		if (!h_entry.themes.empty()) {
			s_catalog.push_back(std::move(h_entry));
		}
	}

	std::sort(s_catalog.begin(), s_catalog.end(), [](const HeroSkinEntry& a, const HeroSkinEntry& b) {
		return a.heroid < b.heroid;
	});
	s_catalog_loaded = !s_catalog.empty();
}

inline const HeroSkinEntry* FindHeroEntry(uint64_t heroid, uint64_t model_res_id = 0) {
	EnsureSkinCatalogLoaded();
	for (const auto& h : s_catalog) {
		if (heroid != 0 && h.heroid == heroid) return &h;
	}
	if (model_res_id != 0) {
		for (const auto& h : s_catalog) {
			if (h.gameplay_entity_id == model_res_id) return &h;
			if (h.heroid == 0x02E000000000007AULL && (model_res_id == 0x040000000000040AULL || model_res_id == 0x0400000000000407ULL)) {
				return &h;
			}
		}
	}
	return nullptr;
}

inline const SkinThemeEntry* FindThemeEntry(const HeroSkinEntry* h, uint64_t theme_id) {
	if (!h || h->themes.empty()) return nullptr;
	for (const auto& t : h->themes) {
		if (t.theme_id == theme_id) return &t;
	}
	return &h->themes[0];
}

inline int ExtractEntityTeam(Entity* m_ent, Entity* c_ent, int fallback_team = 1) {
	auto decode_c10 = [](Component_10_FilterBits* c10, int& out_team) -> bool {
		if (!c10 || IsBadReadPtr(c10, sizeof(Component_10_FilterBits))) return false;
		uint64_t tbits = c10->filter_bits & 0x3C0000ULL;
		if (tbits == 0x40000ULL)  { out_team = 0; return true; }
		if (tbits == 0x80000ULL)  { out_team = 1; return true; }
		if (tbits == 0x100000ULL) { out_team = 2; return true; }
		if (tbits == 0x200000ULL) { out_team = 3; return true; }
		return false;
	};
	int t = fallback_team;
	if (m_ent && decode_c10(m_ent->getById<Component_10_FilterBits>(0x10), t)) return t;
	if (c_ent && decode_c10(c_ent->getById<Component_10_FilterBits>(0x10), t)) return t;
	return fallback_team;
}

inline std::vector<WorldHeroInstance> CollectWorldHeroes(EntityAdminBase* ea, int& out_local_team) {
	std::vector<WorldHeroInstance> result;
	out_local_team = 0;
	if (!ea || !ea->entity_list_array) return result;
	EnsureSkinCatalogLoaded();

	uint32_t local_ctrl_id = ea->local_entid;
	uint32_t local_mdl_id  = 0;
	Vector4  local_pos{};
	if (local_ctrl_id != 0) {
		if (Entity* lc = ea->getEntById(local_ctrl_id)) {
			if (auto* c20 = lc->getById<Component_20_ModelReference>(0x20)) {
				local_mdl_id = c20->cam_attach_entid ? c20->cam_attach_entid : (c20->aim_entid ? c20->aim_entid : c20->movement_attach_entid);
			}
			Entity* lm = local_mdl_id ? ea->getEntById(local_mdl_id) : nullptr;
			out_local_team = ExtractEntityTeam(lm, lc, 0);
			if (lm) {
				if (auto* c15 = lm->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
					local_pos = c15->movement_state_2.absolute_position;
				} else if (auto* c1 = lm->getById<Component_1_SceneRendering>(1)) {
					local_pos = c1->position;
				}
			}
		}
	}

	auto already_added = [&](uint32_t cid, uint32_t mid) {
		for (const auto& w : result) {
			if ((mid != 0 && w.mdl_entid == mid) || (cid != 0 && w.ctrl_entid == cid)) return true;
		}
		return false;
	};

	for (Entity* ent : *ea) {
		if (!ent) continue;
		Entity* m_ent = nullptr;
		Entity* c_ent = nullptr;

		if (ent->hasComponent(0x15) && ent->hasComponent(0x28)) {
			m_ent = ent;
			if (auto* c1f = m_ent->getById<Component_1F_Backref>(0x1F)) {
				if (c1f->backref_entid != 0 && c1f->backref_entid != m_ent->entity_id) {
					c_ent = ea->getEntById(c1f->backref_entid);
				}
			}
			if (!c_ent) {
				player_spawner::DummySpawnRecord drec{};
				if (player_spawner::get_dummy_record(m_ent->entity_id, drec) && drec.controller_entid != 0) {
					c_ent = ea->getEntById(drec.controller_entid);
				}
			}
		} else if (ent->hasComponent(0x20) && ent->hasComponent(0x3F)) {
			c_ent = ent;
			if (auto* c20 = c_ent->getById<Component_20_ModelReference>(0x20)) {
				uint32_t mid = c20->cam_attach_entid ? c20->cam_attach_entid : (c20->aim_entid ? c20->aim_entid : c20->movement_attach_entid);
				if (mid != 0) m_ent = ea->getEntById(mid);
			}
		} else {
			continue;
		}

		uint32_t cid = c_ent ? c_ent->entity_id : 0;
		uint32_t mid = m_ent ? m_ent->entity_id : 0;
		if (cid == 0 && mid == 0) continue;
		if (already_added(cid, mid)) continue;

		uint64_t heroid = 0;
		uint64_t model_res_id = 0;
		if (c_ent) {
			if (auto* c3f = c_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
				heroid = (uint64_t)c3f->selected_heroid;
			}
		}
		if (m_ent && m_ent->resload_entry && m_ent->resload_entry->valid()) {
			model_res_id = (uint64_t)m_ent->resload_entry->align()->resource_id;
		}

		player_spawner::DummySpawnRecord rec{};
		if (player_spawner::get_dummy_record(mid ? mid : cid, rec)) {
			if (heroid == 0) heroid = rec.heroid;
			if (model_res_id == 0) model_res_id = rec.model_res_id;
		}

		const HeroSkinEntry* h_entry = FindHeroEntry(heroid, model_res_id);
		if (!h_entry) continue; // Skip non-hero entities (e.g. map props or deployables)
		heroid = h_entry->heroid;
		if (model_res_id == 0) model_res_id = h_entry->gameplay_entity_id;

		WorldHeroInstance inst{};
		inst.ctrl_entid   = cid;
		inst.mdl_entid    = mid;
		inst.heroid       = heroid;
		inst.model_res_id = model_res_id;
		inst.is_local     = ((local_ctrl_id != 0 && cid == local_ctrl_id) || (local_mdl_id != 0 && mid == local_mdl_id));
		inst.team         = ExtractEntityTeam(m_ent, c_ent, inst.is_local ? 0 : (rec.team >= 0 ? rec.team : 1));
		inst.active_skin_id = player_spawner::get_entity_skin_theme(cid, mid, heroid);

		if (m_ent) {
			if (auto* c28 = m_ent->getById<Component_28_STUHealthComponent>(0x28)) {
				inst.cur_hp  = c28->CalcTotalCurrentHealth();
				inst.max_hp  = c28->CalcTotalMaxHealth();
				inst.is_dead = (inst.max_hp > 0.0f && inst.cur_hp <= 0.0f);
			}
			auto* c15 = m_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
			auto* c1  = m_ent->getById<Component_1_SceneRendering>(1);
			if (c15 && (fabsf(c15->movement_state_2.absolute_position.X) + fabsf(c15->movement_state_2.absolute_position.Y) + fabsf(c15->movement_state_2.absolute_position.Z) > 0.01f)) {
				inst.position = c15->movement_state_2.absolute_position;
			} else if (c1) {
				inst.position = c1->position;
			}
			if (c1) {
				inst.rotation = c1->rotation;
			}
		}
		if (rec.is_dead) inst.is_dead = true;

		float dx = inst.position.X - local_pos.X;
		float dy = inst.position.Y - local_pos.Y;
		float dz = inst.position.Z - local_pos.Z;
		inst.dist_to_local_m = inst.is_local ? 0.0f : std::sqrt(dx * dx + dy * dy + dz * dz);

		if (inst.is_local) {
			inst.allegiance = WorldAllegiance::LOCAL_YOU;
		} else if (inst.team >= 2 || inst.team < 0) {
			inst.allegiance = WorldAllegiance::NEUTRAL;
		} else if (inst.team == out_local_team) {
			inst.allegiance = WorldAllegiance::FRIENDLY;
		} else {
			inst.allegiance = WorldAllegiance::ENEMY;
		}

		result.push_back(inst);
	}

	std::sort(result.begin(), result.end(), [](const WorldHeroInstance& a, const WorldHeroInstance& b) {
		if (a.is_local != b.is_local) return a.is_local > b.is_local;
		if ((int)a.allegiance != (int)b.allegiance) return (int)a.allegiance < (int)b.allegiance;
		return a.mdl_entid < b.mdl_entid;
	});
	return result;
}

inline void SpawnQuickWorldHero(EntityAdminBase* ea, const HeroSkinEntry& h_entry, uint64_t skin_theme_id, int team, float forward_dist_m) {
	if (!ea) return;
	Vector4 spawn_pos{ 0.0f, 2.0f, 0.0f, 0.0f };
	Vector4 spawn_rot{ 0.0f, 0.0f, 0.0f, 1.0f };
	if (Entity* lc = ea->getLocalEnt()) {
		Entity* lm = nullptr;
		if (auto* c20 = lc->getById<Component_20_ModelReference>(0x20)) {
			uint32_t mid = c20->cam_attach_entid ? c20->cam_attach_entid : c20->aim_entid;
			if (mid) lm = ea->getEntById(mid);
		}
		if (lm) {
			if (auto* c15 = lm->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
				spawn_pos = c15->movement_state_2.absolute_position;
			} else if (auto* c1 = lm->getById<Component_1_SceneRendering>(1)) {
				spawn_pos = c1->position;
			}
			if (auto* c1 = lm->getById<Component_1_SceneRendering>(1)) {
				spawn_rot = c1->rotation;
			}
		}
		if (auto* lp = lc->getById<Component_2F_LocalPlayer>(0x2F)) {
			float yaw = lp->view_angles_radian.X;
			spawn_pos.X += std::sinf(yaw) * forward_dist_m;
			spawn_pos.Z += std::cosf(yaw) * forward_dist_m;
		} else {
			spawn_pos.X += forward_dist_m;
		}
	}
	spawn_pos.W = 0.0f;

	player_spawner::DummySpawnRecord rec{};
	rec.heroid         = h_entry.heroid;
	rec.model_res_id   = h_entry.gameplay_entity_id;
	rec.skin_theme_id  = skin_theme_id;
	rec.team           = team;
	rec.spawn_position = spawn_pos;
	rec.spawn_rotation = spawn_rot;
	player_spawner::respawn_dummy_from_record(ea, rec, /*cleanup_corpse_first=*/false, /*queue_lockon=*/true);
}

inline void DrawSkinSwatchButton(
	const char* id_str,
	const HeroSkinEntry& h_entry,
	const SkinThemeEntry& t_entry,
	bool is_equipped,
	bool is_preferred,
	float btn_width,
	bool& out_clicked)
{
	out_clicked = false;
	ImGui::PushID(id_str);

	ImVec4 bg_col = is_equipped
		? ImVec4(0.16f, 0.36f, 0.22f, 0.95f)
		: ImVec4(0.14f, 0.16f, 0.20f, 0.90f);
	ImVec4 hover_col = is_equipped
		? ImVec4(0.22f, 0.46f, 0.28f, 1.00f)
		: ImVec4(0.22f, 0.26f, 0.34f, 0.98f);
	ImVec4 active_col = ImVec4(0.28f, 0.54f, 0.34f, 1.00f);

	ImGui::PushStyleColor(ImGuiCol_Button, bg_col);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover_col);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, active_col);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, is_equipped ? 2.0f : 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, is_equipped
		? ImVec4(0.35f, 0.95f, 0.45f, 1.0f)
		: (is_preferred ? ImVec4(0.95f, 0.78f, 0.25f, 0.85f) : ImVec4(0.32f, 0.35f, 0.42f, 0.70f)));

	std::string label = is_equipped
		? std::format("     [{}] {} *", t_entry.slot_index + 1, t_entry.display_name)
		: std::format("     [{}] {}", t_entry.slot_index + 1, t_entry.display_name);

	if (ImGui::Button(label.c_str(), ImVec2(btn_width, 28.0f))) {
		out_clicked = true;
	}

	ImVec2 rmin = ImGui::GetItemRectMin();
	ImVec2 rmax = ImGui::GetItemRectMax();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Draw color swatch square inside the left side of the button
	ImVec2 sw_min(rmin.x + 5.0f, rmin.y + 5.0f);
	ImVec2 sw_max(rmin.x + 23.0f, rmax.y - 5.0f);
	ImU32 sw_col = ImGui::ColorConvertFloat4ToU32(ImVec4(
		t_entry.swatch_rgba[0], t_entry.swatch_rgba[1], t_entry.swatch_rgba[2], 1.0f));
	dl->AddRectFilled(sw_min, sw_max, sw_col, 3.0f);
	dl->AddRect(sw_min, sw_max, IM_COL32(255, 255, 255, is_equipped ? 240 : 140), 3.0f, 0, 1.2f);

	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "%s — [%d] %s (%s)",
			h_entry.hero_name.c_str(),
			t_entry.slot_index + 1,
			t_entry.display_name.c_str(),
			t_entry.slot_index == 0 ? "Classic Base Skin" : "Recolor Skin Theme");
		ImGui::Separator();
		ImGui::ColorButton("##tt_swatch", ImVec4(
			t_entry.swatch_rgba[0], t_entry.swatch_rgba[1], t_entry.swatch_rgba[2], 1.0f),
			ImGuiColorEditFlags_NoTooltip, ImVec2(32.0f, 32.0f));
		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::Text("STUSkinTheme: 0x%016llX (Tier Enum=%u)", (unsigned long long)t_entry.theme_id, t_entry.tier_enum);
		ImGui::Text("Parent STUSkin: 0x%016llX | UX String: 0x%016llX",
			(unsigned long long)h_entry.skin_id, (unsigned long long)t_entry.display_str_id);
		ImGui::Text("Weapon Skins: 1P=0x%016llX | 3P=0x%016llX",
			(unsigned long long)t_entry.weapon_skin_0, (unsigned long long)t_entry.weapon_skin_1);
		ImGui::Text("Override Map (+0x30): %d overridden asset GUIDs | Swatch RGBA: (%.2f, %.2f, %.2f)%s",
			t_entry.override_map_count,
			t_entry.swatch_rgba[0], t_entry.swatch_rgba[1], t_entry.swatch_rgba[2],
			t_entry.raw_color_was_zero ? " [raw=0,0,0]" : "");
		ImGui::EndGroup();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.45f, 0.95f, 0.55f, 1.0f),
			"Click to hot-swap skin in-place via ServerEntityRespawnCommand!");
		ImGui::EndTooltip();
	}

	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(2);
	ImGui::PopID();
}

inline void RenderSkinSwitcherContents() {
	EnsureSkinCatalogLoaded();
	EntityAdminBase* ea = GameEntityAdmin();

	int local_team = 0;
	auto world_heroes = CollectWorldHeroes(ea, local_team);

	int cnt_you = 0, cnt_friend = 0, cnt_enemy = 0, cnt_neutral = 0;
	for (const auto& w : world_heroes) {
		if (w.allegiance == WorldAllegiance::LOCAL_YOU)     cnt_you++;
		else if (w.allegiance == WorldAllegiance::FRIENDLY) cnt_friend++;
		else if (w.allegiance == WorldAllegiance::ENEMY)    cnt_enemy++;
		else                                                cnt_neutral++;
	}

	// Top Status Banner
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.12f, 0.16f, 0.95f));
	ImGui::BeginChild("##skin_switcher_header", ImVec2(0, 82.0f), true);
	ImGui::TextColored(ImVec4(0.40f, 0.88f, 1.00f, 1.0f), "HERO & WORLD SKIN SWITCHER [F1]");
	ImGui::SameLine();
	ImGui::TextDisabled("| Build 0.8.0.24919: 21 Heroes x 4 STUSkinThemes = 84 Skins (168 Weapon Skins) | 100%% Dedicated-Server Compatible");
	ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
	if (ImGui::SmallButton("Reload STU Catalog")) {
		EnsureSkinCatalogLoaded(true);
	}

	ImGui::TextColored(ImVec4(0.55f, 0.95f, 0.65f, 1.0f), "Wire Status:");
	ImGui::SameLine();
	ImGui::Text("Swaps=%u | %s", CombatReplication::s_total_skin_swaps, CombatReplication::s_last_skin_swap_status.c_str());

	// Allegiance Filter Bar
	auto draw_filter_pill = [&](const char* label, int filter_val, const ImVec4& col) {
		bool active = (s_allegiance_filter == filter_val);
		ImGui::PushStyleColor(ImGuiCol_Button, active ? col : ImVec4(col.x * 0.35f, col.y * 0.35f, col.z * 0.35f, 0.80f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4((std::min)(col.x * 1.15f, 1.0f), (std::min)(col.y * 1.15f, 1.0f), (std::min)(col.z * 1.15f, 1.0f), 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, active ? 2.0f : 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, active ? ImVec4(1.0f, 1.0f, 1.0f, 0.95f) : ImVec4(0.3f, 0.3f, 0.35f, 0.6f));
		if (ImGui::Button(label, ImVec2(0, 24.0f))) {
			s_allegiance_filter = filter_val;
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(1);
	};

	draw_filter_pill(std::format("ALL IN WORLD ({})##flt_all", world_heroes.size()).c_str(), -1, ImVec4(0.32f, 0.45f, 0.68f, 0.95f));
	ImGui::SameLine();
	draw_filter_pill(std::format("YOU / LOCAL ({})##flt_you", cnt_you).c_str(), 0, ImVec4(0.18f, 0.62f, 0.32f, 0.95f));
	ImGui::SameLine();
	draw_filter_pill(std::format("FRIENDLY / ALLY ({})##flt_frd", cnt_friend).c_str(), 1, ImVec4(0.15f, 0.52f, 0.78f, 0.95f));
	ImGui::SameLine();
	draw_filter_pill(std::format("ENEMY / HOSTILE ({})##flt_enm", cnt_enemy).c_str(), 2, ImVec4(0.78f, 0.22f, 0.22f, 0.95f));
	ImGui::SameLine();
	draw_filter_pill(std::format("NEUTRAL / FACTION ({})##flt_neu", cnt_neutral).c_str(), 3, ImVec4(0.76f, 0.58f, 0.16f, 0.95f));

	// Batch Actions on right side of filter bar
	ImGui::SameLine();
	ImGui::TextDisabled("| Batch:");
	ImGui::SameLine();
	auto apply_batch_slot = [&](int slot_or_rand) {
		for (const auto& w : world_heroes) {
			if (s_allegiance_filter != -1 && (int)w.allegiance != s_allegiance_filter) continue;
			const HeroSkinEntry* h = FindHeroEntry(w.heroid, w.model_res_id);
			if (!h || h->themes.empty()) continue;
			int idx = (slot_or_rand < 0) ? (std::rand() % (int)h->themes.size()) : std::clamp(slot_or_rand, 0, (int)h->themes.size() - 1);
			CombatReplication::EnqueueWorldEntitySkinSwap(w.ctrl_entid, w.mdl_entid, h->themes[idx].theme_id);
		}
	};
	if (ImGui::SmallButton("Randomize Filtered")) apply_batch_slot(-1);
	ImGui::SameLine();
	if (ImGui::SmallButton("All -> [1] Classic")) apply_batch_slot(0);
	ImGui::SameLine();
	if (ImGui::SmallButton("All -> [2]")) apply_batch_slot(1);
	ImGui::SameLine();
	if (ImGui::SmallButton("All -> [3]")) apply_batch_slot(2);
	ImGui::SameLine();
	if (ImGui::SmallButton("All -> [4]")) apply_batch_slot(3);

	ImGui::EndChild();
	ImGui::PopStyleColor();

	// Quick Spawner Bar (spawn a Friend, Neutral, or Enemy with any skin right from F1!)
	if (!s_catalog.empty()) {
		s_quick_spawn_hero_idx = std::clamp(s_quick_spawn_hero_idx, 0, (int)s_catalog.size() - 1);
		const HeroSkinEntry& q_hero = s_catalog[s_quick_spawn_hero_idx];
		s_quick_spawn_slot_idx = std::clamp(s_quick_spawn_slot_idx, 0, (std::max)(0, (int)q_hero.themes.size() - 1));

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.13f, 0.17f, 0.90f));
		ImGui::BeginChild("##skin_quick_spawner", ImVec2(0, 42.0f), true);
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(ImVec4(0.95f, 0.80f, 0.30f, 1.0f), "Quick Spawn w/ Skin:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(145.0f);
		if (ImGui::BeginCombo("##qs_hero", q_hero.hero_name.c_str())) {
			for (int i = 0; i < (int)s_catalog.size(); ++i) {
				bool sel = (i == s_quick_spawn_hero_idx);
				if (ImGui::Selectable(s_catalog[i].hero_name.c_str(), sel)) {
					s_quick_spawn_hero_idx = i;
				}
				if (sel) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (!q_hero.themes.empty()) {
			const SkinThemeEntry& qt = q_hero.themes[s_quick_spawn_slot_idx];
			ImGui::ColorButton("##qs_sw", ImVec4(qt.swatch_rgba[0], qt.swatch_rgba[1], qt.swatch_rgba[2], 1.0f),
				ImGuiColorEditFlags_NoTooltip, ImVec2(22.0f, 22.0f));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(165.0f);
			std::string qt_preview = std::format("[{}] {}", qt.slot_index + 1, qt.display_name);
			if (ImGui::BeginCombo("##qs_theme", qt_preview.c_str())) {
				for (int ti = 0; ti < (int)q_hero.themes.size(); ++ti) {
					const auto& t = q_hero.themes[ti];
					ImGui::PushID(ti);
					ImGui::ColorButton("##c", ImVec4(t.swatch_rgba[0], t.swatch_rgba[1], t.swatch_rgba[2], 1.0f),
						ImGuiColorEditFlags_NoTooltip, ImVec2(16.0f, 16.0f));
					ImGui::SameLine();
					bool sel = (ti == s_quick_spawn_slot_idx);
					std::string lbl = std::format("[{}] {} (0x{:X})", t.slot_index + 1, t.display_name, t.theme_id & 0xFFFFULL);
					if (ImGui::Selectable(lbl.c_str(), sel)) {
						s_quick_spawn_slot_idx = ti;
					}
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
		}

		ImGui::SameLine();
		uint64_t qs_skin_id = !q_hero.themes.empty() ? q_hero.themes[s_quick_spawn_slot_idx].theme_id : 0;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.50f, 0.76f, 0.95f));
		if (ImGui::Button("+ Spawn FRIEND (Team 0)")) {
			SpawnQuickWorldHero(ea, q_hero, qs_skin_id, local_team, s_quick_spawn_dist_m);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.76f, 0.20f, 0.20f, 0.95f));
		if (ImGui::Button("+ Spawn ENEMY (Team 1)")) {
			int enemy_team = (local_team == 0) ? 1 : 0;
			SpawnQuickWorldHero(ea, q_hero, qs_skin_id, enemy_team, s_quick_spawn_dist_m);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.55f, 0.14f, 0.95f));
		if (ImGui::Button("+ Spawn NEUTRAL (Team 2)")) {
			SpawnQuickWorldHero(ea, q_hero, qs_skin_id, 2, s_quick_spawn_dist_m);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.60f, 0.30f, 0.95f));
		if (ImGui::Button("Swap YOU to This Hero+Skin")) {
			if (ea && ea->local_entid != 0) {
				CombatReplication::EnqueueWorldEntitySkinSwap(ea->local_entid, 0, qs_skin_id, q_hero.heroid);
			}
		}
		ImGui::PopStyleColor();

		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	// Main Tab Bar: [1] Live World Heroes Switcher | [2] All 21 Heroes / 84 Skins Wardrobe
	if (ImGui::BeginTabBar("##skin_switcher_tabs")) {
		if (ImGui::BeginTabItem("Live World Heroes — Switch Skins on You, Friends, Neutrals & Enemies")) {
			if (!ea || world_heroes.empty()) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.30f, 1.0f),
					"No active hero entities found in GameEntityAdmin yet. Load into a map or spawn a hero above!");
			} else {
				ImGuiTableFlags tflags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
					ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
				if (ImGui::BeginTable("##world_heroes_skin_table", 4, tflags, ImVec2(0, 0))) {
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("Allegiance & Team", ImGuiTableColumnFlags_WidthFixed, 175.0f);
					ImGui::TableSetupColumn("Hero (In-Place Swap)", ImGuiTableColumnFlags_WidthFixed, 155.0f);
					ImGui::TableSetupColumn("Vitals & Entity IDs", ImGuiTableColumnFlags_WidthFixed, 165.0f);
					ImGui::TableSetupColumn("Visual Skin Theme Switcher (Click Any Swatch to Hot-Swap In-Place)", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					for (size_t idx = 0; idx < world_heroes.size(); ++idx) {
						const auto& w = world_heroes[idx];
						if (s_allegiance_filter != -1 && (int)w.allegiance != s_allegiance_filter) continue;

						const HeroSkinEntry* h_entry = FindHeroEntry(w.heroid, w.model_res_id);
						if (!h_entry) continue;

						ImGui::PushID((int)(w.mdl_entid ? w.mdl_entid : (w.ctrl_entid + idx * 1000)));
						ImGui::TableNextRow(ImGuiTableRowFlags_None, 38.0f);

						// Column 0: Allegiance Badge + Live Team Switcher
						ImGui::TableSetColumnIndex(0);
						ImVec4 badge_col;
						const char* badge_txt = "";
						switch (w.allegiance) {
						case WorldAllegiance::LOCAL_YOU:
							badge_col = ImVec4(0.20f, 0.78f, 0.36f, 1.0f);
							badge_txt = "YOU (LOCAL)";
							break;
						case WorldAllegiance::FRIENDLY:
							badge_col = ImVec4(0.22f, 0.68f, 0.98f, 1.0f);
							badge_txt = "FRIENDLY (ALLY)";
							break;
						case WorldAllegiance::ENEMY:
							badge_col = ImVec4(0.96f, 0.28f, 0.28f, 1.0f);
							badge_txt = "ENEMY (HOSTILE)";
							break;
						case WorldAllegiance::NEUTRAL:
						default:
							badge_col = ImVec4(0.95f, 0.76f, 0.22f, 1.0f);
							badge_txt = "NEUTRAL (FACTION)";
							break;
						}
						ImGui::TextColored(badge_col, "[%s]", badge_txt);

						const char* team_labels[] = {
							"Team 0 (Blue/Friend)",
							"Team 1 (Red/Enemy)",
							"Team 2 (Neutral A)",
							"Team 3 (Neutral B)"
						};
						int cur_team_idx = std::clamp(w.team, 0, 3);
						ImGui::SetNextItemWidth(158.0f);
						if (ImGui::BeginCombo("##team_sw", team_labels[cur_team_idx])) {
							for (int t = 0; t < 4; ++t) {
								bool sel = (t == cur_team_idx);
								if (ImGui::Selectable(team_labels[t], sel) && t != cur_team_idx) {
									CombatReplication::EnqueueWorldEntitySkinSwap(
										w.ctrl_entid, w.mdl_entid, w.active_skin_id, w.heroid, (uint8_t)t);
								}
							}
							ImGui::EndCombo();
						}

						// Column 1: Hero Name + Live In-Place Hero Swap Dropdown
						ImGui::TableSetColumnIndex(1);
						ImGui::TextColored(ImVec4(1.0f, 0.96f, 0.84f, 1.0f), "%s", h_entry->hero_name.c_str());
						ImGui::SetNextItemWidth(145.0f);
						if (ImGui::BeginCombo("##hero_sw", "Change Hero...")) {
							for (const auto& cand_h : s_catalog) {
								bool sel = (cand_h.heroid == h_entry->heroid);
								if (ImGui::Selectable(cand_h.hero_name.c_str(), sel) && !sel) {
									uint64_t new_skin = player_spawner::get_preferred_hero_skin_theme(cand_h.heroid);
									CombatReplication::EnqueueWorldEntitySkinSwap(
										w.ctrl_entid, w.mdl_entid, new_skin, cand_h.heroid);
								}
							}
							ImGui::EndCombo();
						}

						// Column 2: Vitals & Entity IDs
						ImGui::TableSetColumnIndex(2);
						if (w.is_dead) {
							ImGui::TextColored(ImVec4(0.95f, 0.30f, 0.30f, 1.0f), "DEAD CORPSE (0 / %.0f)", w.max_hp);
						} else if (w.max_hp > 0.0f) {
							float frac = std::clamp(w.cur_hp / w.max_hp, 0.0f, 1.0f);
							ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
								w.is_local ? ImVec4(0.22f, 0.76f, 0.36f, 0.9f) :
								(w.allegiance == WorldAllegiance::FRIENDLY ? ImVec4(0.20f, 0.62f, 0.92f, 0.9f) :
								(w.allegiance == WorldAllegiance::NEUTRAL ? ImVec4(0.85f, 0.68f, 0.20f, 0.9f) :
								ImVec4(0.86f, 0.24f, 0.24f, 0.9f))));
							std::string hp_txt = std::format("{:.0f} / {:.0f} HP", w.cur_hp, w.max_hp);
							ImGui::ProgressBar(frac, ImVec2(148.0f, 16.0f), hp_txt.c_str());
							ImGui::PopStyleColor();
						}
						if (w.is_local) {
							ImGui::TextDisabled("M:0x%X C:0x%X (You)", w.mdl_entid, w.ctrl_entid);
						} else {
							ImGui::TextDisabled("M:0x%X C:0x%X (%.1fm)", w.mdl_entid, w.ctrl_entid, w.dist_to_local_m);
						}

						// Column 3: Visual 4-Skin Swatch Strip
						ImGui::TableSetColumnIndex(3);
						uint64_t pref_skin = player_spawner::get_preferred_hero_skin_theme(h_entry->heroid);
						float avail_w = ImGui::GetContentRegionAvail().x;
						int num_themes = (int)h_entry->themes.size();
						float btn_w = (num_themes > 0) ? (std::max)(115.0f, (avail_w - (num_themes - 1) * 6.0f) / (float)num_themes) : 130.0f;

						for (int ti = 0; ti < num_themes; ++ti) {
							if (ti > 0) ImGui::SameLine(0.0f, 6.0f);
							const auto& theme = h_entry->themes[ti];
							bool is_equipped = (w.active_skin_id == theme.theme_id) ||
								(w.active_skin_id == 0 && ti == 0);
							bool is_pref = (pref_skin == theme.theme_id);
							bool clicked = false;
							std::string btn_id = std::format("w_sw_{}_{}", w.mdl_entid ? w.mdl_entid : w.ctrl_entid, ti);
							DrawSkinSwatchButton(btn_id.c_str(), *h_entry, theme, is_equipped, is_pref, btn_w, clicked);
							if (clicked) {
								CombatReplication::EnqueueWorldEntitySkinSwap(w.ctrl_entid, w.mdl_entid, theme.theme_id);
							}
						}

						ImGui::PopID();
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("21-Hero Complete Skin Wardrobe (All 84 Skins & Default Loadouts)")) {
			ImGui::TextWrapped(
				"Click any skin swatch below to set that hero's Preferred Default Skin (used automatically when spawning or selecting heroes with H). "
				"If you or any world entities are currently playing that hero, use the quick action buttons on that row to switch them immediately.");

			ImGuiTableFlags wflags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
				ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
			if (ImGui::BeginTable("##wardrobe_84_table", 4, wflags, ImVec2(0, 0))) {
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("Hero (STUHero 0x02E0...)", ImGuiTableColumnFlags_WidthFixed, 175.0f);
				ImGui::TableSetupColumn("Base STUSkin (0x0AE0...)", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("4 Skin Themes (STUSkinTheme 0x0A50... — Click to Set Preferred & Apply to You)", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Quick World Spawn", ImGuiTableColumnFlags_WidthFixed, 205.0f);
				ImGui::TableHeadersRow();

				for (int hi = 0; hi < (int)s_catalog.size(); ++hi) {
					const auto& h = s_catalog[hi];
					uint64_t pref_skin = player_spawner::get_preferred_hero_skin_theme(h.heroid);

					ImGui::PushID(hi);
					ImGui::TableNextRow(ImGuiTableRowFlags_None, 34.0f);

					ImGui::TableSetColumnIndex(0);
					ImGui::TextColored(ImVec4(0.98f, 0.90f, 0.55f, 1.0f), "%s", h.hero_name.c_str());
					ImGui::TextDisabled("0x%016llX", (unsigned long long)h.heroid);

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("0x%016llX", (unsigned long long)h.skin_id);
					ImGui::TextDisabled("Model: 0x%llX", (unsigned long long)(h.gameplay_entity_id & 0xFFFFULL));

					ImGui::TableSetColumnIndex(2);
					float avail_w = ImGui::GetContentRegionAvail().x;
					int n_themes = (int)h.themes.size();
					float btn_w = (n_themes > 0) ? (std::max)(110.0f, (avail_w - (n_themes - 1) * 6.0f) / (float)n_themes) : 120.0f;

					for (int ti = 0; ti < n_themes; ++ti) {
						if (ti > 0) ImGui::SameLine(0.0f, 6.0f);
						const auto& theme = h.themes[ti];
						bool is_pref = (pref_skin == theme.theme_id);
						bool clicked = false;
						std::string bid = std::format("ward_{}_{}", hi, ti);
						DrawSkinSwatchButton(bid.c_str(), h, theme, is_pref, is_pref, btn_w, clicked);
						if (clicked) {
							player_spawner::set_preferred_hero_skin_theme(h.heroid, theme.theme_id);
							// Also immediately hot-swap LocalPlayer if LocalPlayer is currently playing this hero!
							for (const auto& w : world_heroes) {
								if (w.is_local && w.heroid == h.heroid) {
									CombatReplication::EnqueueWorldEntitySkinSwap(w.ctrl_entid, w.mdl_entid, theme.theme_id);
								}
							}
						}
					}

					ImGui::TableSetColumnIndex(3);
					if (ImGui::SmallButton("You")) {
						if (ea && ea->local_entid != 0) {
							CombatReplication::EnqueueWorldEntitySkinSwap(ea->local_entid, 0, pref_skin, h.heroid);
						}
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("+Friend")) {
						SpawnQuickWorldHero(ea, h, pref_skin, local_team, s_quick_spawn_dist_m);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("+Enemy")) {
						SpawnQuickWorldHero(ea, h, pref_skin, (local_team == 0) ? 1 : 0, s_quick_spawn_dist_m);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("+Neutral")) {
						SpawnQuickWorldHero(ea, h, pref_skin, 2, s_quick_spawn_dist_m);
					}

					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

} // namespace SkinSwitcherUI

namespace CombatReplication {

void RenderSkinSwitcherF1Window() {
	if (!s_show_skin_switcher_f1) return;
	ImGui::SetNextWindowSize(ImVec2(1080.0f, 640.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Hero & World Skin Switcher [F1] — Local, Friendly, Neutral & Enemy", &s_show_skin_switcher_f1)) {
		SkinSwitcherUI::RenderSkinSwitcherContents();
	}
	ImGui::End();
}

} // namespace CombatReplication

class skin_switcher_window : public window {
	WINDOW_DEFINE(skin_switcher_window, "Game", "Hero & World Skin Switcher (F1)", true);

	inline void render() override {
		CombatReplication::s_show_skin_switcher_f1 = true;
		queue_deletion();
	}
};

WINDOW_REGISTER(skin_switcher_window);
