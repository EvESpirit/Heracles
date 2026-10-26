#pragma once
#include "statescript_list.h"
#include "statescript_window.h"
#include "statescript_node_search.h"
#include "statescript_fake_logicalbtn.h"
#include "CVD_VarbagSelector.h"
#include "../statescript_logger.h"
#include "../stringhash_library.h"
#include "../StatescriptVar.h"
#include "../STU_Editable.h"
#include "../stu_resources.h"
#include <fstream>
#include <format>
#include <set>
#include <unordered_set>

static bool is_valid_read_memory(const void* ptr, size_t size) {
	if (!ptr) return false;
	MEMORY_BASIC_INFORMATION mbi{};
	if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
	if (mbi.State != MEM_COMMIT) return false;
	if ((mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) == 0) return false;
	if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) return false;
	return ((uintptr_t)ptr + size <= (uintptr_t)mbi.BaseAddress + mbi.RegionSize);
}

static bool s_force_weapon_exec_flags = true;

static const char* get_rtti_mangled_raw(void* obj_with_vtable) {
	__try {
		if (!obj_with_vtable || !is_valid_read_memory(obj_with_vtable, sizeof(void*))) return nullptr;
		void** vfptr = *(void***)obj_with_vtable;
		if (!vfptr || !is_valid_read_memory(vfptr, sizeof(void*))) return nullptr;
		void* pColPtr = vfptr[-1];
		if (!pColPtr || !is_valid_read_memory(pColPtr, 32)) return nullptr;
		unsigned int* pCol = (unsigned int*)pColPtr;
		if (pCol[0] == 1) { // x64 MSVC
			uint32_t typeDescRva = pCol[3];
			uint32_t colSelfRva = pCol[5];
			uintptr_t moduleBase = (uintptr_t)pColPtr - colSelfRva;
			if (moduleBase != globals::gameBase) {
				moduleBase = globals::gameBase;
			}
			char* pTypeDesc = (char*)(moduleBase + typeDescRva);
			if (is_valid_read_memory(pTypeDesc, 32)) {
				char* mangled_name = pTypeDesc + 0x10;
				if (is_valid_read_memory(mangled_name, 8) && mangled_name[0] == '.' && mangled_name[1] == '?') {
					return mangled_name;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
	return nullptr;
}

static std::string get_rtti_classname(void* obj_with_vtable) {
	const char* raw = get_rtti_mangled_raw(obj_with_vtable);
	if (!raw) return "";
	std::string name(raw);
	if (name.starts_with(".?AV")) name = name.substr(4);
	if (name.ends_with("@@")) name = name.substr(0, name.size() - 2);
	return name;
}

static void ensure_known_stu_hashes() {
	static bool initialized = false;
	if (initialized) return;
	initialized = true;

	auto add = [](uint hash, const char* name) {
		allmighty_hash_lib::hashes[hash] = name;
	};

	add(0x3E490486, "STUStatescriptOutputPlug");
	add(0x4698e4b6, "STUStatescriptActionSetVar");
	add(0xe811ecbb, "STUStatescriptStateBooleanSwitch");
	add(0xbe49a4c2, "STUStatescriptEntryGameMessage");
	add(0xff7876f9, "STUConfigVarGameMessage");
	add(0xc739baac, "STUStatescriptStateUXScreen");
	add(0x8ba08edd, "STUStatescriptStateUXBase");
	add(0x3db862c4, "STUStatescriptStateSubScript");
	add(0x65f158da, "STUStatescriptStateWait");
	add(0x11d7f052, "STUStatescriptInputPlug");
	add(0x26367267, "STUStatescriptStateSwitch");
	add(0xffbfa88d, "STUStatescriptStateEffect");
	add(0x9b38211b, "STUStatescriptComponent");
	add(0x8F23FA30, "STUStatescriptStateUXHeroSelectScreen");
	add(0xff77458c, "STUStatescriptStateLogicalButton");
	add(0x5c7964ff, "STUConfigVarLogicalButton");
	add(0xcc3a08f4, "STUConfigVarExpression");
	add(0xa7213568, "STUStatescriptState");
	add(0x2ecefc8b, "STUStatescriptAction");
	add(0x66b5f581, "STUAction_PlayEffect_");
	add(0x2501a4db, "STUAction_RandomFromBucket_");
	add(0x5bea9b4b, "STUAction_SendGameMsgConditional_");
	add(0xb1032daf, "Simple_Condition_");
	add(0x48663e64, "Effect_To_Play_Var_");
	add(0x24ab1f0b, "Subgraph_Plug_");
	add(0xb8f35b20, "Subgraph_Plug_2_");
	add(0x5732771b, "ClientOnly_Subgraph_Plug_");
	add(0x26c93342, "ClientOnly_Subgraph_Plug_2_");
	add(0x4e5feee5, "ServerOnly_Subgraph_Plug_");
	add(0xbfd9aadd, "STUConfigVar");
	add(0xf8cc195d, "STUConfigVarDynamic");
	add(0x34D7F9EE, "STUIdentifier");
}

static void dump_varbag_entries(std::ofstream& out, StatescriptVar_VarBag* vb, const std::string& prefix) {
	if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) {
		out << prefix << "(null or invalid VarBag)\n";
		return;
	}
	int count = 0;
	for (int i = 0; i < 16; i++) {
		auto* item = &vb->sub_items[i];
		if (!item || !item->arr_ptr || IsBadReadPtr(item->arr_ptr, sizeof(void*))) continue;
		for (int j = 0; j < item->item_count; j++) {
			StatescriptVar_Base* rid = item->arr_ptr[j].ptr;
			if (!rid || IsBadReadPtr(rid, sizeof(StatescriptVar_Base))) continue;
			count++;
			out << prefix << "- Var `0x" << std::hex << rid->var_id << std::dec << "`";
			if (rid->is_cvd()) {
				StatescriptVar_Primitive* p = (StatescriptVar_Primitive*)rid;
				out << " [CVD Primitive type " << (int)p->global_var.type << "]: '" << p->global_var.get_value_str() << "'\n";
			}
			else if (rid->is_dictvar()) {
				StatescriptVar_Dictionary* d = (StatescriptVar_Dictionary*)rid;
				out << " [Dict/Array]: count=" << d->items_array.item_count << "\n";
			}
			else if (rid->is_varbag()) {
				out << " [Nested VarBag]\n";
			}
			else if (rid->is_siref()) {
				StatescriptVar_InstanceReference* r = (StatescriptVar_InstanceReference*)rid;
				out << " [InstanceRef]: target_inst=" << r->ss_inst_id << "\n";
			}
			else {
				out << " [Unknown]\n";
			}
		}
	}
	if (count == 0) {
		out << prefix << "(empty VarBag)\n";
	}
}

static void capture_varbag_values(StatescriptVar_VarBag* vb, short inst_id, std::map<std::pair<short, __int64>, std::string>& out_map) {
	if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) return;
	for (int i = 0; i < 16; i++) {
		auto* item = &vb->sub_items[i];
		if (!item || !item->arr_ptr || IsBadReadPtr(item->arr_ptr, sizeof(void*))) continue;
		for (int j = 0; j < item->item_count; j++) {
			StatescriptVar_Base* rid = item->arr_ptr[j].ptr;
			if (rid && !IsBadReadPtr(rid, sizeof(StatescriptVar_Base)) && rid->is_cvd()) {
				StatescriptVar_Primitive* p = (StatescriptVar_Primitive*)rid;
				out_map[{inst_id, rid->var_id}] = p->global_var.get_value_str();
			}
		}
	}
}

void statescript_diag_window::select_local_model_0x23() {
	auto* ea = GameEntityAdmin();
	if (!ea) return;
	Entity* local_controller = ea->getLocalEnt();
	if (!local_controller) return;
	auto* pet = local_controller->getById<Component_20_ModelReference>(0x20);
	if (!pet) return;
	Entity* model_ent = ea->getEntById(pet->cam_attach_entid);
	if (!model_ent && pet->aim_entid) model_ent = ea->getEntById(pet->aim_entid);
	if (!model_ent && pet->movement_attach_entid) model_ent = ea->getEntById(pet->movement_attach_entid);
	if (!model_ent) return;
	auto* c23 = model_ent->getById<Component_23_Statescript>(0x23);
	if (c23) {
		_ss = c23;
		if (_auto_subscribe_scripts) {
			subscribe_all_instances();
		}
	}
}

void statescript_diag_window::subscribe_all_instances() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) return;
	for (auto script : _ss->ss_inner.g1_instanceArr) {
		if (script && !IsBadReadPtr(script, sizeof(StatescriptInstance))) {
			statescript_logger::Subscribe(script);
		}
	}
}

void statescript_diag_window::take_snapshot() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) return;
	_snapshot_node_timestamps.clear();
	_snapshot_var_values.clear();

	for (auto script : _ss->ss_inner.g1_instanceArr) {
		if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
		statescript_logger::Subscribe(script);
		auto script_state = statescript_logger::GetScriptState(script);
		for (auto& pair : script_state) {
			_snapshot_node_timestamps[{script->instance_id, pair.first}] = pair.second.change_timestamp;
		}
		if (script->rid_instance_varbag) {
			capture_varbag_values(script->rid_instance_varbag, script->instance_id, _snapshot_var_values);
		}
	}
	if (_ss->ss_inner.rid_entity_varbag) {
		capture_varbag_values(_ss->ss_inner.rid_entity_varbag, -1, _snapshot_var_values);
	}
	_has_snapshot = true;
	_dump_status_msg = std::format("Snapshot taken! ({:d} nodes, {:d} vars recorded)",
		_snapshot_node_timestamps.size(), _snapshot_var_values.size());
}

void statescript_diag_window::dump_trace_to_file() {
	std::ofstream out("statescript_trace.log");
	if (!out.is_open()) {
		out.open("statescript_trace.log");
	}
	if (!out.is_open()) {
		_dump_status_msg = "ERROR: Failed to open statescript_trace.log for writing!";
		return;
	}

	out << "# STATESCRIPT LIVE TRACE REPORT\n\n";
	out << "Generated by `[Step 1 Diagnostic] Weapon & Input Tracer`\n\n";

	auto* ea = GameEntityAdmin();
	out << "## 1. Entity & Linkage Status\n";
	if (ea) {
		out << "- `GameEntityAdmin`: " << (void*)ea << "\n";
		out << "- `local_entid`: 0x" << std::hex << ea->local_entid << std::dec << "\n";
		Entity* local_controller = ea->getLocalEnt();
		if (local_controller) {
			out << "- Local Controller Entity: 0x" << std::hex << local_controller->entity_id << std::dec << "\n";
			auto* pet = local_controller->getById<Component_20_ModelReference>(0x20);
			if (pet) {
				out << "  - `aim_entid`: 0x" << std::hex << pet->aim_entid << "\n";
				out << "  - `cam_attach_entid`: 0x" << pet->cam_attach_entid << "\n";
				out << "  - `movement_attach_entid`: 0x" << pet->movement_attach_entid << std::dec << "\n";
			}
		}
	}

	if (_ss && !IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
		out << "\n## 2. Model StatescriptSyncMgr (+0xC0) Status\n";
		out << "- Model Entity ID: 0x" << std::hex << _ss->base.entity_backref->entity_id << std::dec << "\n";
		out << "- `has_player_entbackref`: " << _ss->ss_inner.has_player_entbackref << "\n";
		out << "- `is_aim_ent`: " << (int)_ss->ss_inner.is_aim_ent << "\n";
		out << "- `is_aim_ent_assigned`: " << (int)_ss->ss_inner.is_aim_ent_assigned << "\n";
		out << "- `entity_admin_scope`: " << _ss->ss_inner.entity_admin_scope << "\n";
		out << "- `ontick_updates_enabled`: " << _ss->ss_inner.ontick_updates_enabled << "\n";
		out << "- `is_instantiating`: " << (int)_ss->ss_inner.is_instantiating << "\n";
		out << "- `latest_logbtn`: " << _ss->ss_inner.latest_logbtn << "\n";
		out << "- `latest_logbtn_down`: " << (int)_ss->ss_inner.latest_logbtn_down << "\n";
		out << "- `logicalBtn_list.num`: " << _ss->ss_inner.logicalBtn_list.num << "\n";
		out << "- Active Instances (`g1_instanceArr.num`): " << _ss->ss_inner.g1_instanceArr.num << "\n";

		out << "\n## 3. Active Statescript Instances\n";
		for (auto script : _ss->ss_inner.g1_instanceArr) {
			if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
			out << "- Instance " << script->instance_id << ": Script ID `0x" << std::hex << script->script_id << std::dec << "`";
			if (script->parent_instance_id) {
				out << " (Child of " << script->parent_instance_id << ")";
			}
			out << "\n";
		}

		out << "\n## 4. Live Snapshot Diff\n";
		if (!_has_snapshot) {
			out << "No snapshot was captured before dumping.\n";
		}
		else {
			int changed_count = 0;
			for (auto script : _ss->ss_inner.g1_instanceArr) {
				if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
				auto script_state = statescript_logger::GetScriptState(script);
				for (auto& pair : script_state) {
					auto key = std::make_pair(script->instance_id, pair.first);
					auto it = _snapshot_node_timestamps.find(key);
					if (it != _snapshot_node_timestamps.end() && it->second != pair.second.change_timestamp) {
						changed_count++;
						out << "- [CHANGED NODE] Inst " << script->instance_id << ", Node " << pair.first
							<< ": ts " << it->second << " -> " << pair.second.change_timestamp << "\n";
					}
				}
				if (script->rid_instance_varbag) {
					std::map<std::pair<short, __int64>, std::string> cur_vars;
					capture_varbag_values(script->rid_instance_varbag, script->instance_id, cur_vars);
					for (auto& v : cur_vars) {
						auto it = _snapshot_var_values.find(v.first);
						if (it != _snapshot_var_values.end() && it->second != v.second) {
							changed_count++;
							out << "- [CHANGED VAR] Inst " << script->instance_id << ", Var 0x" << std::hex << v.first.second
								<< std::dec << ": '" << it->second << "' -> '" << v.second << "'\n";
						}
					}
				}
			}
			if (changed_count == 0) {
				out << "**NO CHANGES DETECTED (0 nodes / 0 vars changed)**\n";
				out << "Verdict: Primary Fire input did NOT reach Component 0x23 on Model Entity (Gate A Blocker)!\n";
			}
		}

		out << "\n## 5. Event & Notification Log (Last 100 entries)\n";
		int logged_lines = 0;
		for (auto script : _ss->ss_inner.g1_instanceArr) {
			if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
			auto& logs = statescript_logger::GetScriptLogs(script);
			for (auto& line : logs.logs) {
				out << "[Inst " << script->instance_id << "] " << line << "\n";
				logged_lines++;
				if (logged_lines >= 100) break;
			}
			if (logged_lines >= 100) break;
		}
		if (logged_lines == 0) {
			out << "No notifications captured by statescript_logger for subscribed instances.\n";
		}
	}
	else {
		out << "No Component 0x23 currently selected.\n";
	}

	out.close();
	_dump_status_msg = "Successfully dumped trace to statescript_trace.log!";
}

void statescript_diag_window::call_init_ss() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
		_experiment_status_msg = "Error: Invalid Statescript component!";
		return;
	}
	int before_ontick = _ss->ss_inner.ontick_updates_enabled;
	auto init_ss_fn = (void(*)(Component_23_Statescript*))(globals::gameBase + 0x103acb0);
	init_ss_fn(_ss);
	int after_ontick = _ss->ss_inner.ontick_updates_enabled;
	_experiment_status_msg = std::format("init_ss_fn (+0x103acb0) executed! ontick_updates_enabled: {:d} -> {:d}",
		before_ontick, after_ontick);
}

void statescript_diag_window::toggle_ontick_updates() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) return;
	_ss->ss_inner.ontick_updates_enabled = _ss->ss_inner.ontick_updates_enabled ? 0 : 1;
	_experiment_status_msg = std::format("ontick_updates_enabled manually set to {:d}", _ss->ss_inner.ontick_updates_enabled);
}

void statescript_diag_window::patch_server_only_plugs() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) return;
	ensure_known_stu_hashes();

	int total_plugs = 0;
	int server_only_plugs = 0;
	int client_only_plugs = 0;

	for (auto script : _ss->ss_inner.g1_instanceArr) {
		if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance)) || !script->graph) continue;
		for (auto node : script->graph->m_nodes) {
			if (!node || IsBadReadPtr(node, sizeof(STUStatescriptBase))) continue;
			auto plug_map = node->get_output_plugs();
			for (auto& entry : plug_map) {
				for (auto* plug : entry.second) {
					if (!plug || IsBadReadPtr(plug, sizeof(STUGraphPlug))) continue;
					total_plugs++;
					if (plug->base.vfptr && !IsBadReadPtr(plug->base.vfptr, sizeof(void*))) {
						auto* stu = plug->base.vfptr->GetSTUInfo();
						if (stu) {
							if (stu->name_hash == 0x4e5feee5) { // ServerOnly_Subgraph_Plug_
								server_only_plugs++;
							}
							else if (stu->name_hash == 0x5732771b) { // ClientOnly_Subgraph_Plug_
								client_only_plugs++;
							}
						}
					}
				}
			}
		}
	}

	_experiment_status_msg = std::format("Plugs scanned (Read-Only): {:d} total. {:d} ServerOnly (0x4e5feee5), {:d} ClientOnly (0x5732771b).",
		total_plugs, server_only_plugs, client_only_plugs);
}

void statescript_diag_window::force_server_authority_weapon_nodes() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) return;
	ensure_known_stu_hashes();

	// 1. Force ontick and clientonly flags on SyncMgr
	_ss->ss_inner.ontick_updates_enabled = 1;
	_ss->ss_inner.has_clientonly_scripts = 1;

	// 2. Patch node authority flags
	int cleared_server = 0;
	int forced_client = 0;
	for (auto script : _ss->ss_inner.g1_instanceArr) {
		if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance)) || !script->graph) continue;
		for (auto node : script->graph->m_nodes) {
			if (!node || IsBadReadPtr(node, sizeof(STUStatescriptBase))) continue;
			if (node->m_serverOnly) {
				node->m_serverOnly = 0;
				cleared_server++;
			}
			if (!node->m_clientOnly) {
				node->m_clientOnly = 1;
				forced_client++;
			}
		}
	}

	_experiment_status_msg = std::format("Authority patched! Cleared m_serverOnly on {:d} nodes, forced m_clientOnly on {:d} nodes. ontick_updates_enabled=1, has_clientonly=1.",
		cleared_server, forced_client);
}

void statescript_diag_window::fixup_weapon_varbags() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) return;
	auto* vb = _ss->ss_inner.rid_entity_varbag;
	if (!vb) {
		_experiment_status_msg = "Error: Entity VarBag is null!";
		return;
	}

	// 1. Health fixup (0x6C4 curr, 0x6C9 max)
	StatescriptPrimitive health_cv{};
	health_cv.type = StatescriptPrimitive_FLT;
	*(float*)&health_cv.value = 150.0f;
	vb->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C4 }, health_cv);
	vb->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C9 }, health_cv);

	// 2. Weapon equipped flag (0x0FD = 1)
	StatescriptPrimitive cv{};
	cv.type = StatescriptPrimitive_BYTE;
	cv.value = 1;
	vb->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000000FD }, cv);

	// 3. Active weapons array at index 0 (0x01D)
	StatescriptPrimitive wpn_1{};
	wpn_1.type = StatescriptPrimitive_INT;
	wpn_1.value = 1;
	vb->SetArray({ 0x0D8000000000001D }, wpn_1, wpn_1, 0);
	vb->SetArray({ StatescriptVar_ENTITY_VARBAG, 0x0D8000000000001D }, wpn_1, wpn_1, 0);

	_experiment_status_msg = "Weapon VarBags fixed up! Set Health (150/150), WeaponFlag (0xFD=1), ActiveWeapon array [0]=1.";
}

void statescript_diag_window::dump_weapon_graph_to_file() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
		_experiment_status_msg = "Error: No Statescript component selected!";
		return;
	}

	std::ofstream out("weapon_graph_trace.log");
	if (!out.is_open()) {
		_experiment_status_msg = "Error: Failed to open weapon_graph_trace.log for writing!";
		return;
	}

	out << "# WEAPON GRAPH & VARBAG DEEP TRACE REPORT\n\n";
	out << "Generated by `[Step 1 Diagnostic] Weapon & Input Tracer`\n\n";

	ensure_known_stu_hashes();

	out << "## 1. StatescriptSyncMgr (+0xC0) Details\n";
	out << "- `ontick_updates_enabled`: " << _ss->ss_inner.ontick_updates_enabled << "\n";
	out << "- `has_clientonly_scripts`: " << (int)_ss->ss_inner.has_clientonly_scripts << "\n";
	out << "- `entity_admin_scope`: " << _ss->ss_inner.entity_admin_scope << "\n";
	out << "- `has_player_entbackref`: " << _ss->ss_inner.has_player_entbackref << "\n";
	out << "- `is_aim_ent`: " << (int)_ss->ss_inner.is_aim_ent << "\n";
	out << "- `is_aim_ent_assigned`: " << (int)_ss->ss_inner.is_aim_ent_assigned << "\n";
	out << "- `is_instantiating`: " << (int)_ss->ss_inner.is_instantiating << "\n";
	out << "- `latest_logbtn`: " << _ss->ss_inner.latest_logbtn << "\n";
	out << "- `latest_logbtn_down`: " << (int)_ss->ss_inner.latest_logbtn_down << "\n";
	out << "- `g1_instanceArr.num`: " << _ss->ss_inner.g1_instanceArr.num << "\n\n";

	out << "## 2. Entity VarBag (`rid_entity_varbag`)\n";
	dump_varbag_entries(out, _ss->ss_inner.rid_entity_varbag, "  ");
	out << "\n";

	out << "## 3. Weapon & Hero Script Instances\n";
	for (auto script : _ss->ss_inner.g1_instanceArr) {
		if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;

		out << "### Instance " << script->instance_id << " (Script ID: `0x" << std::hex << script->script_id << std::dec << "`)\n";
		if (script->parent_instance_id) {
			out << "- Parent Instance ID: " << script->parent_instance_id << "\n";
		}

		if (script->graph && !IsBadReadPtr(script->graph, sizeof(STUStatescriptGraph))) {
			out << "- Nodes count: " << script->graph->m_nodes.count() << "\n";
			out << "- States count: " << script->graph->m_states.count() << "\n";

			auto script_state = statescript_logger::GetScriptState(script);

			out << "\n#### Nodes Listing:\n";
			for (auto node : script->graph->m_nodes) {
				if (!node || IsBadReadPtr(node, sizeof(STUStatescriptBase))) continue;
				std::string stu_name = "Unknown";
				std::string rtti_name = get_rtti_classname(node);
				if (node->graph_node.base.vfptr && !IsBadReadPtr(node->graph_node.base.vfptr, sizeof(void*))) {
					auto stu = node->graph_node.base.vfptr->GetSTUInfo();
					if (stu) {
						auto it = allmighty_hash_lib::hashes.find(stu->name_hash);
						if (it != allmighty_hash_lib::hashes.end()) {
							stu_name = it->second;
						}
						else if (!rtti_name.empty()) {
							stu_name = rtti_name;
							allmighty_hash_lib::hashes[stu->name_hash] = rtti_name;
						}
						else {
							stu_name = std::format("0x{:08x}", (uint32)stu->name_hash);
						}
					}
				}
				if (stu_name == "Unknown" && !rtti_name.empty()) {
					stu_name = rtti_name;
				}

				auto state_it = script_state.find(node->idx_in_nodes);
				__int64 last_ts = (state_it != script_state.end()) ? state_it->second.change_timestamp : 0;
				bool finished = (state_it != script_state.end()) ? state_it->second.is_finish_state : false;

				out << "- Node " << node->idx_in_nodes << ": `[" << stu_name << "]`";
				if (!rtti_name.empty() && rtti_name != stu_name) out << " (" << rtti_name << ")";
				out << " (isState=" << (int)node->m_isState
					<< ", CO=" << (int)node->m_clientOnly
					<< ", SERVER=" << (int)node->m_serverOnly << ")"
					<< " | last_ts=" << last_ts
					<< (finished ? " (FINISHED)" : " (ACTIVE)")
					<< "\n";

				// 1. Output plugs & links
				auto plug_map = node->get_output_plugs();
				for (auto& plug_entry : plug_map) {
					for (auto* plug : plug_entry.second) {
						if (!plug || IsBadReadPtr(plug, sizeof(STUGraphPlug))) continue;
						std::string plug_rtti = get_rtti_classname(plug);
						uint plug_hash = 0;
						if (plug->base.vfptr && !IsBadReadPtr(plug->base.vfptr, sizeof(void*))) {
							auto p_stu = plug->base.vfptr->GetSTUInfo();
							if (p_stu) plug_hash = p_stu->name_hash;
						}
						bool is_server_plug = (plug_hash == 0x4e5feee5);
						out << "    -> OutPlug 0x" << std::hex << plug_hash << std::dec;
						if (!plug_rtti.empty()) out << " [" << plug_rtti << "]";
						if (is_server_plug) out << " **[SERVER-ONLY PLUG]**";
						out << " (links: " << plug->m_links.count() << "):\n";
						for (auto* link : plug->m_links) {
							if (!link || IsBadReadPtr(link, sizeof(STUGraphLink))) continue;
							if (link->m_inputPlug && !IsBadReadPtr(link->m_inputPlug, sizeof(STUStatescriptInputPlug))) {
								auto* target = link->m_inputPlug->plug_base.m_parentNode;
								if (target && !IsBadReadPtr(target, sizeof(STUStatescriptBase))) {
									std::string target_rtti = get_rtti_classname(target);
									out << "       ==> Link to Node " << target->idx_in_nodes;
									if (!target_rtti.empty()) out << " [" << target_rtti << "]";
									out << "\n";
								}
							}
						}
					}
				}

				// 2. STU Arguments (resources)
				if (node->graph_node.base.vfptr && !IsBadReadPtr(node->graph_node.base.vfptr, sizeof(void*))) {
					auto editable = node->graph_node.base.to_editable();
					for (auto arg_it : *node->graph_node.base.vfptr->GetSTUInfo()) {
						auto* arg = arg_it.second;
						if (!arg || !arg->constraint) continue;
						auto type_flag = arg->constraint->get_type_flag();
						if (type_flag == STU_ConstraintType_ResourceRef || type_flag == STU_ConstraintType_NonSTUResourceRef) {
							auto res = editable.get_argument_resource(arg);
							if (res && res->has_resource()) {
								out << "    arg [0x" << std::hex << arg->name_hash << std::dec << "]: Resource `0x"
									<< std::hex << res->resource_id << std::dec << "`\n";
							}
						}
					}
				}
			}
		}

		out << "\n#### Instance VarBag 1 (`rid_instance_varbag`):\n";
		dump_varbag_entries(out, script->rid_instance_varbag, "  ");

		out << "\n#### Instance VarBag 2 (`second_inst_varbag`):\n";
		dump_varbag_entries(out, script->second_inst_varbag, "  ");
		out << "\n---\n\n";
	}

	out.close();
	_experiment_status_msg = "Successfully dumped deep weapon graph & VarBags to weapon_graph_trace.log!";
}

static STUInfo* safe_get_stu_info(void* stu_base_ptr) {
	__try {
		if (!stu_base_ptr || !is_valid_read_memory(stu_base_ptr, sizeof(void*))) return nullptr;
		void** vfptr = *(void***)stu_base_ptr;
		if (!vfptr || !is_valid_read_memory(vfptr, sizeof(void*))) return nullptr;
		STUBase<STUBase_vt>* b = (STUBase<STUBase_vt>*)stu_base_ptr;
		if (b->vfptr && b->vfptr->GetSTUInfo) {
			return b->vfptr->GetSTUInfo();
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return nullptr;
}

static void dump_stu_object_fields(std::ofstream& out, void* obj_ptr, STUInfo* sinfo, int depth, int max_depth, std::unordered_set<uintptr_t>& visited_ptrs) {
	if (!obj_ptr || !sinfo || depth > max_depth) return;
	if (!is_valid_read_memory(obj_ptr, 8) || !is_valid_read_memory(sinfo, sizeof(STUInfo))) return;

	std::string indent(depth * 2, ' ');
	for (auto arg_pair : *sinfo) {
		STUArgumentInfo* arg = arg_pair.second;
		if (!arg || !is_valid_read_memory(arg, sizeof(STUArgumentInfo))) continue;
		uint field_hash = arg->name_hash;
		int offset = arg->offset;
		if (offset < 0 || offset > 0x1000) continue;

		// 1. Prevent Parent-Node / Subgraph Recursion Bloat
		if (field_hash == 0x365378c7 || field_hash == 0xa2eca5de) {
			out << indent << "- Field `0x" << std::hex << field_hash << std::dec << "`, offset `+0x" << std::hex << offset << std::dec << "`: [Parent Node / Subgraph Backref - Skipped Recursion]\n";
			continue;
		}

		// Skip redundant base plug fields on each node
		if (field_hash == 0xbec2beae || field_hash == 0xfd24b86e || field_hash == 0x25c665a || field_hash == 0xd2cd7c9f || field_hash == 0x3f6ffa05) {
			out << indent << "- Field `0x" << std::hex << field_hash << std::dec << "`: [Base Plug - Skipped Recursion]\n";
			continue;
		}

		std::string field_name = "Unknown";
		auto it = allmighty_hash_lib::hashes.find(field_hash);
		if (it != allmighty_hash_lib::hashes.end()) field_name = it->second;

		STUConstraintType ctype = (arg->constraint && is_valid_read_memory(arg->constraint, sizeof(void*))) ? arg->constraint->get_type_flag() : (STUConstraintType)-1;
		std::string type_str = STU_ConstraintType_ToString(ctype);

		void* field_ptr = (void*)((uintptr_t)obj_ptr + offset);
		uint64_t raw_val = 0;
		if (is_valid_read_memory(field_ptr, 8)) {
			raw_val = *(uint64_t*)field_ptr;
		}

		out << indent << "- Field `0x" << std::hex << field_hash << std::dec << "` (`" << field_name
			<< "`), offset `+0x" << std::hex << offset << std::dec << "`, type `" << type_str
			<< "`, raw: `0x" << std::hex << raw_val << std::dec << "`";

		if (ctype == STU_ConstraintType_ResourceRef || ctype == STU_ConstraintType_NonSTUResourceRef) {
			STUResourceReference* ref = (STUResourceReference*)field_ptr;
			if (is_valid_read_memory(ref, sizeof(STUResourceReference)) && ref->has_resource()) {
				out << " [Resource: `0x" << std::hex << ref->resource_id << std::dec << "`]";
			}
		}
		out << "\n";

		if (depth >= max_depth) continue;

		// A. Object (pointer to STUBase)
		if (ctype == STU_ConstraintType_Object && raw_val > 0x10000 && raw_val < 0x7FFFFFFFFFFFull) {
			void* child_ptr = (void*)raw_val;
			if (visited_ptrs.count((uintptr_t)child_ptr)) {
				out << indent << "  * Child Object at `0x" << std::hex << (uintptr_t)child_ptr << std::dec << "` [Already Visited - Skipped]\n";
				continue;
			}
			visited_ptrs.insert((uintptr_t)child_ptr);

			if (is_valid_read_memory(child_ptr, 8)) {
				void** child_vfptr = *(void***)child_ptr;
				if (child_vfptr && is_valid_read_memory(child_vfptr, 8)) {
					STUInfo* child_info = safe_get_stu_info(child_ptr);
					if (!child_info && arg->constraint) {
						child_info = GetSTUInfoByHash(arg->constraint->get_stu_type());
					}

					std::string child_rtti = get_rtti_classname(child_ptr);
					uint child_hash = child_info ? child_info->name_hash : 0;
					std::string child_name = "Unknown";
					auto it_c = allmighty_hash_lib::hashes.find(child_hash);
					if (it_c != allmighty_hash_lib::hashes.end()) child_name = it_c->second;
					else if (child_hash) child_name = std::format("0x{:08x}", child_hash);

					out << indent << "  * Child Object: STU `0x" << std::hex << child_hash << std::dec
						<< "` (`" << child_name << "`), RTTI: `" << child_rtti << "` at `0x"
						<< std::hex << (uintptr_t)child_ptr << std::dec << "`\n";

					// If ConfigVarDynamic
					if (child_hash == 0xf8cc195d || child_rtti.find("ConfigVarDynamic") != std::string::npos) {
						STUConfigVarDynamic* cvd = (STUConfigVarDynamic*)child_ptr;
						if (is_valid_read_memory(cvd, sizeof(STUConfigVarDynamic))) {
							int path_count = cvd->item_location.count();
							out << indent << "    Path (" << path_count << " items): ";
							for (int k = 0; k < path_count; k++) {
								auto item = cvd->item_location.list()[k];
								out << "`0x" << std::hex << item.resource_id << std::dec << "` ";
							}
							out << "\n";
						}
					}

					// Special bytecode dump & operand iteration for STUConfigVarExpression (0xcc3a08f4)
					if (child_hash == 0xcc3a08f4) {
						void* expr_code_ptr = (void*)((uintptr_t)child_ptr + 0x10);
						if (is_valid_read_memory(expr_code_ptr, 32)) {
							out << indent << "    STUConfigVarExpression Bytecode/Data (+0x10, 32 bytes): ";
							uint8_t* b = (uint8_t*)expr_code_ptr;
							for (int bi = 0; bi < 32; bi++) {
								out << std::format("{:02X} ", b[bi]);
							}
							out << "\n";
						}

						void* operands_list_ptr = (void*)((uintptr_t)child_ptr + 0x58);
						STUBullshitListFull<__int64>* bs_op = (STUBullshitListFull<__int64>*)operands_list_ptr;
						if (is_valid_read_memory(bs_op, sizeof(STUBullshitListFull<__int64>)) && bs_op->list_ptr && is_valid_read_memory(bs_op->list_ptr, sizeof(STUBullshitListPtr))) {
							int op_cnt = bs_op->count();
							out << indent << "    Expression Operands List (Field 0x65ce2851 at +0x58, " << op_cnt << " items):\n";
							int op_limit = (op_cnt < 16) ? op_cnt : 16;
							for (int oi = 0; oi < op_limit; oi++) {
								void* op_obj = (void*)bs_op->list()[oi];
								if (op_obj && is_valid_read_memory(op_obj, 8)) {
									STUInfo* op_info = safe_get_stu_info(op_obj);
									uint op_hash = op_info ? op_info->name_hash : 0;
									std::string op_name = "Unknown";
									auto it_op = allmighty_hash_lib::hashes.find(op_hash);
									if (it_op != allmighty_hash_lib::hashes.end()) op_name = it_op->second;
									else if (op_hash) op_name = std::format("0x{:08x}", op_hash);

									out << indent << "      [" << oi << "] Operand STU `0x" << std::hex << op_hash << std::dec
										<< "` (`" << op_name << "`) at `0x" << std::hex << (uintptr_t)op_obj << std::dec << "`\n";

									if (op_hash == 0xf8cc195d) {
										STUConfigVarDynamic* cvd = (STUConfigVarDynamic*)op_obj;
										if (is_valid_read_memory(cvd, sizeof(STUConfigVarDynamic))) {
											int path_count = cvd->item_location.count();
											out << indent << "        Path (" << path_count << " items): ";
											for (int p = 0; p < path_count; p++) {
												auto item = cvd->item_location.list()[p];
												out << "`0x" << std::hex << item.resource_id << std::dec << "` ";
											}
											out << "\n";
										}
									}
								}
							}
						}
					}

					bool is_plug_obj = (child_hash == 0x11d7f052 || child_hash == 0x3e490486 || child_hash == 0x24ab1f0b || child_hash == 0xe51f3e8d || child_hash == 0xf1f0a36d || child_hash == 0xb8f35b20 || field_hash == 0xa2eca5de);
					if (is_plug_obj) {
						out << indent << "    [Plug Object - Recursion Blocked]\n";
					}
					else if (child_info) {
						dump_stu_object_fields(out, child_ptr, child_info, depth + 1, max_depth, visited_ptrs);
					}
				}
			}
		}

		// B. InlinedObject (struct inlined directly at field_ptr)
		else if (ctype == STU_ConstraintType_InlinedObject) {
			void* inlined_ptr = field_ptr;
			if (visited_ptrs.count((uintptr_t)inlined_ptr)) {
				out << indent << "  * Inlined Object at `0x" << std::hex << (uintptr_t)inlined_ptr << std::dec << "` [Already Visited - Skipped]\n";
				continue;
			}
			visited_ptrs.insert((uintptr_t)inlined_ptr);

			if (is_valid_read_memory(inlined_ptr, 8)) {
				STUInfo* child_info = safe_get_stu_info(inlined_ptr);
				if (!child_info && arg->constraint) {
					child_info = GetSTUInfoByHash(arg->constraint->get_stu_type());
				}
				uint child_hash = child_info ? child_info->name_hash : (arg->constraint ? arg->constraint->get_stu_type() : 0);
				std::string child_name = "Unknown";
				auto it_c = allmighty_hash_lib::hashes.find(child_hash);
				if (it_c != allmighty_hash_lib::hashes.end()) child_name = it_c->second;
				else if (child_hash) child_name = std::format("0x{:08x}", child_hash);

				out << indent << "  * Inlined Object: STU `0x" << std::hex << child_hash << std::dec
					<< "` (`" << child_name << "`) at `0x" << std::hex << (uintptr_t)inlined_ptr << std::dec << "`\n";

				bool is_plug_inlined = (child_hash == 0x11d7f052 || child_hash == 0x3e490486 || child_hash == 0x24ab1f0b || child_hash == 0xe51f3e8d || child_hash == 0xf1f0a36d || child_hash == 0xb8f35b20 || field_hash == 0xa2eca5de);
				if (is_plug_inlined) {
					out << indent << "    [Plug Object - Recursion Blocked]\n";
				}
				else if (child_info) {
					dump_stu_object_fields(out, inlined_ptr, child_info, depth + 1, max_depth, visited_ptrs);
				}
			}
		}

		// C. List: Object or List: InlinedObject
		else if (ctype == STU_ConstraintType_BSList_Object || ctype == STU_ConstraintType_BSList_InlinedObject) {
			STUBullshitListFull<__int64>* bs = (STUBullshitListFull<__int64>*)field_ptr;
			if (is_valid_read_memory(bs, sizeof(STUBullshitListFull<__int64>)) && bs->list_ptr && is_valid_read_memory(bs->list_ptr, sizeof(STUBullshitListPtr))) {
				STU_ObjectList obj_list(arg, field_ptr);
				if (obj_list.valid()) {
					int cnt = obj_list.count();
					out << indent << "  * List Count: " << cnt << "\n";
					int limit = (cnt < 16) ? cnt : 16;
					for (int k = 0; k < limit; k++) {
						STU_Object elem = obj_list[k];
						if (elem.valid() && elem.value && is_valid_read_memory(elem.value, 8)) {
							STU_Object root_elem = elem.get_runtime_root();
							uint elem_hash = root_elem.struct_info ? root_elem.struct_info->name_hash : 0;
							std::string elem_name = "Unknown";
							auto it_e = allmighty_hash_lib::hashes.find(elem_hash);
							if (it_e != allmighty_hash_lib::hashes.end()) elem_name = it_e->second;
							else if (elem_hash) elem_name = std::format("0x{:08x}", elem_hash);

							out << indent << "    [" << k << "] Element STU `0x" << std::hex << elem_hash << std::dec
								<< "` (`" << elem_name << "`) at `0x" << std::hex << (uintptr_t)root_elem.value << std::dec << "`\n";

							// If ConfigVarDynamic, dump CVD path
							if (elem_hash == 0xf8cc195d) {
								STUConfigVarDynamic* cvd = (STUConfigVarDynamic*)root_elem.value;
								if (is_valid_read_memory(cvd, sizeof(STUConfigVarDynamic))) {
									int path_count = cvd->item_location.count();
									out << indent << "      Path (" << path_count << " items): ";
									for (int p = 0; p < path_count; p++) {
										auto item = cvd->item_location.list()[p];
										out << "`0x" << std::hex << item.resource_id << std::dec << "` ";
									}
									out << "\n";
								}
							}

							bool is_plug_elem = (elem_hash == 0x11d7f052 || elem_hash == 0x3e490486 || elem_hash == 0x24ab1f0b || elem_hash == 0xe51f3e8d || elem_hash == 0xf1f0a36d || elem_hash == 0xb8f35b20 || field_hash == 0xa2eca5de);
							if (is_plug_elem) {
								out << indent << "      [Plug Element - Recursion Blocked]\n";
							}
							else if (root_elem.struct_info && !visited_ptrs.count((uintptr_t)root_elem.value)) {
								visited_ptrs.insert((uintptr_t)root_elem.value);
								dump_stu_object_fields(out, root_elem.value, root_elem.struct_info, depth + 1, max_depth, visited_ptrs);
							}
						}
					}
				}
			}
		}

		// D. List: ResourceRef or List: NonSTUResourceRef
		else if (ctype == STU_ConstraintType_BSList_ResourceRef || ctype == STU_ConstraintType_BSList_NonSTUResourceRef) {
			STUBullshitListFull<__int64>* bs = (STUBullshitListFull<__int64>*)field_ptr;
			if (is_valid_read_memory(bs, sizeof(STUBullshitListFull<__int64>)) && bs->list_ptr && is_valid_read_memory(bs->list_ptr, sizeof(STUBullshitListPtr))) {
				STU_ResourceRefList reflist(arg, field_ptr);
				if (reflist.valid()) {
					int cnt = reflist.count();
					out << indent << "  * ResourceRef List (" << cnt << " items): ";
					int limit = (cnt < 16) ? cnt : 16;
					for (int k = 0; k < limit; k++) {
						auto* r = reflist[k];
						if (r && is_valid_read_memory(r, sizeof(STUResourceReference)) && r->has_resource()) {
							out << "`0x" << std::hex << r->resource_id << std::dec << "` ";
						}
					}
					out << "\n";
				}
			}
		}
	}
}

static void dump_node_raw_hex(std::ofstream& out, STUStatescriptBase* node, int node_idx) {
	out << "\n##### Raw 8-Byte Hex Dump (first 0x100 bytes) of Node " << node_idx << " (`0x"
		<< std::hex << (uintptr_t)node << std::dec << "`):\n```\n";
	uint64_t* qwords = (uint64_t*)node;
	for (int off = 0; off < 0x100; off += 8) {
		uint64_t val = 0;
		if (is_valid_read_memory(&qwords[off / 8], 8)) {
			val = qwords[off / 8];
		}
		out << std::format("+0x{:02X}: {:016X}", off, val);

		if (val >= (uint64_t)globals::gameBase && val < (uint64_t)globals::gameBase + 0x3000000) {
			out << std::format("  [Game RVA: +0x{:X}]", val - (uint64_t)globals::gameBase);
		}
		uint64_t hi = (val >> 48) & 0xFFFF;
		if (hi == 0x0D80 || hi == 0x0580 || hi == 0x0400 || hi == 0x0800 || hi == 0x0760) {
			out << std::format("  [Resource/Var ID: 0x{:016X}]", val);
		}
		if (val > 0x10000 && val < 0x7FFFFFFFFFFFull && is_valid_read_memory((void*)val, 8)) {
			std::string rtti = get_rtti_classname((void*)val);
			if (!rtti.empty()) {
				out << "  [RTTI: " << rtti << "]";
			}
		}
		out << "\n";
	}
	out << "```\n\n";
}

static void dump_raw_mem_block(std::ofstream& out, const char* label, void* base_ptr, int byte_len) {
	out << "\n##### " << label << " (`0x" << std::hex << (uintptr_t)base_ptr << std::dec << "`, `0x" << std::hex << byte_len << std::dec << "` bytes):\n```\n";
	if (!base_ptr || !is_valid_read_memory(base_ptr, 8)) {
		out << "(null or invalid pointer)\n```\n";
		return;
	}
	uint64_t* qwords = (uint64_t*)base_ptr;
	for (int off = 0; off < byte_len; off += 8) {
		uint64_t val = 0;
		if (is_valid_read_memory(&qwords[off / 8], 8)) {
			val = qwords[off / 8];
		}
		uint32_t lo_dw = (uint32_t)(val & 0xFFFFFFFFull);
		uint32_t hi_dw = (uint32_t)(val >> 32);
		float lo_f = *(float*)&lo_dw;
		float hi_f = *(float*)&hi_dw;
		out << std::format("+0x{:03X}: {:016X}  (dw: {:08X} {:08X} | f: {:.3f}, {:.3f})", off, val, lo_dw, hi_dw, lo_f, hi_f);

		if (val >= (uint64_t)globals::gameBase && val < (uint64_t)globals::gameBase + 0x3000000) {
			out << std::format("  [Game RVA: +0x{:X}]", val - (uint64_t)globals::gameBase);
		}
		uint64_t hi = (val >> 48) & 0xFFFF;
		if (hi == 0x0D80 || hi == 0x0580 || hi == 0x0400 || hi == 0x0800 || hi == 0x0760 || hi == 0x0A50 || hi == 0x0DC0) {
			out << std::format("  [Resource/Var ID: 0x{:016X}]", val);
		}
		if (val > 0x10000 && val < 0x7FFFFFFFFFFFull && is_valid_read_memory((void*)val, 8)) {
			std::string rtti = get_rtti_classname((void*)val);
			if (!rtti.empty()) {
				out << "  [RTTI: " << rtti << "]";
			}
		}
		out << "\n";
	}
	out << "```\n";
}

static void dump_entity_door_forensics(std::ofstream& out, Entity* ent, const char* admin_label) {
	if (!ent || !is_valid_read_memory(ent, sizeof(Entity))) return;
	auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
	__int64 stu_id = slot ? slot->stu_id : 0;
	__int64 res_id = (ent->resload_entry && is_valid_read_memory(ent->resload_entry, sizeof(void*)) && ent->resload_entry->valid())
		? ent->resload_entry->align()->resource_id : 0;
	if (stu_id == 0) stu_id = res_id;

	out << "## [" << admin_label << "] Door Entity `0x" << std::hex << ent->entity_id
		<< "` (STU `0x" << stu_id << "`, Resload `0x" << res_id
		<< "`, SpawnCaller `+0x" << (slot ? slot->spawn_caller_rva : 0) << "`)" << std::dec << "\n";

	auto* c1 = ent->getById<Component_1_SceneRendering>(1);
	if (c1 && is_valid_read_memory(c1, sizeof(Component_1_SceneRendering))) {
		uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)c1 + 0x1D0);
		uintptr_t phys_ptr = *(uintptr_t*)((uintptr_t)c1 + 0x198);
		out << "- **Component_1_SceneRendering** (`0x" << std::hex << (uintptr_t)c1 << std::dec << "`):\n";
		out << std::format("  - Position: `({:.3f}, {:.3f}, {:.3f})`\n", c1->position.X, c1->position.Y, c1->position.Z);
		out << std::format("  - Rotation: `({:.4f}, {:.4f}, {:.4f}, {:.4f})`\n", c1->rotation.X, c1->rotation.Y, c1->rotation.Z, c1->rotation.W);
		out << std::format("  - Scale: `({:.3f}, {:.3f}, {:.3f})`\n", c1->scaling.X, c1->scaling.Y, c1->scaling.Z);
		out << std::format("  - `rendering_flags` (+0x180): `0x{:016X}` (`IsVisible() = {}`)\n",
			(unsigned long long)c1->rendering_flags, c1->IsVisible() ? "true" : "false");
		out << std::format("  - `phys_actor_ptr` (+0x198): `0x{:016X}` | `parent_c1` (+0x1D0): `0x{:016X}`\n",
			(unsigned long long)phys_ptr, (unsigned long long)parent_c1);
	}

	out << "\n### 1. Component List & Mirror STU Data\n";
	for (int i = 0; i < ent->component_list.num - 1; i++) {
		ComponentBase* comp = ent->component_list.ptr[i];
		if (!comp || !is_valid_read_memory(comp, sizeof(ComponentBase))) continue;
		uintptr_t vfptr_val = *(uintptr_t*)comp;
		uintptr_t vfptr_rva = (vfptr_val >= (uintptr_t)globals::gameBase) ? (vfptr_val - (uintptr_t)globals::gameBase) : 0;
		std::string comp_rtti = get_rtti_classname(comp);
		out << std::format("- **Component `0x{:02X}`** at `0x{:X}` (vfptr `+0x{:X}`, RTTI `{}`)\n",
			(int)comp->component_id, (uintptr_t)comp, vfptr_rva, comp_rtti);

		STUBase<STUBase_vt>* mirror = (STUBase<STUBase_vt>*)comp->GetMirrorData();
		if (mirror && is_valid_read_memory(mirror, sizeof(void*))) {
			STUInfo* minfo = safe_get_stu_info(mirror);
			uint mhash = minfo ? minfo->name_hash : 0;
			std::string mname = "Unknown";
			auto it = allmighty_hash_lib::hashes.find(mhash);
			if (it != allmighty_hash_lib::hashes.end()) mname = it->second;
			std::string mrtti = get_rtti_classname(mirror);
			out << std::format("  - MirrorData: `0x{:X}` STU `0x{:08X}` (`{}`), RTTI `{}`\n",
				(uintptr_t)mirror, mhash, mname, mrtti);
			if (minfo) {
				std::unordered_set<uintptr_t> visited;
				visited.insert((uintptr_t)mirror);
				dump_stu_object_fields(out, mirror, minfo, 2, 4, visited);
			}
		}
	}

	out << "\n### 2. Key Door Components Raw Memory\n";
	if (auto* c4 = ent->getById<ComponentBase>(0x04)) {
		dump_raw_mem_block(out, "Component_04 (Animation / Skeleton)", c4, 0x120);
	}
	if (auto* c12 = ent->getById<ComponentBase>(0x12)) {
		dump_raw_mem_block(out, "Component_12 (STUMovementStateComponent Header +0x00..+0x80)", c12, 0x80);
		dump_raw_mem_block(out, "Component_12 (Footer +0xA030..+0xA068)", (void*)((uintptr_t)c12 + 0xA030), 0x38);
	}
	if (auto* c29 = ent->getById<ComponentBase>(0x29)) {
		dump_raw_mem_block(out, "Component_29 (Door Mover / Portal A)", c29, 0x60);
	}
	if (auto* c2a = ent->getById<ComponentBase>(0x2A)) {
		dump_raw_mem_block(out, "Component_2A (Door Mover / NavMesh Obstacle)", c2a, 0x100);
	}
	if (auto* c68 = ent->getById<ComponentBase>(0x68)) {
		dump_raw_mem_block(out, "Component_68", c68, 0x80);
	}

	auto* c23 = ent->getById<Component_23_Statescript>(0x23);
	out << "\n### 3. `Component_23_Statescript` Deep Inspection\n";
	if (!c23 || !is_valid_read_memory(c23, sizeof(Component_23_Statescript))) {
		out << "(No Component_23_Statescript on this entity)\n\n---\n\n";
		return;
	}

	uint16_t next_cli_id = *(uint16_t*)((uintptr_t)c23 + 0xA8);
	uint16_t next_srv_id = *(uint16_t*)((uintptr_t)c23 + 0xAA);
	uint8_t init_byte_bc = *(uint8_t*)((uintptr_t)c23 + 0xBC);
	out << std::format("- `c23` ptr: `0x{:X}` | `+0xA8` (next_cli_id): `0x{:04X}` ({}) | `+0xAA` (next_srv_id): `0x{:04X}` ({}) | `+0xBC` (init_done): `{}`\n",
		(uintptr_t)c23, next_cli_id, (int16_t)next_cli_id, next_srv_id, (int16_t)next_srv_id, (int)init_byte_bc);
	out << std::format("- `ss_inner.is_instantiating`: `{}` | `ontick_updates_enabled`: `{}` | `g1_instanceArr.num`: `{}` | `g3_arrWithIds.num`: `{}`\n",
		(int)c23->ss_inner.is_instantiating, (int)c23->ss_inner.ontick_updates_enabled,
		c23->ss_inner.g1_instanceArr.num, c23->ss_inner.g3_arrWithIds.num);

	dump_raw_mem_block(out, "Component_23 Header (+0x00..+0x100)", c23, 0x100);

	out << "\n#### Entity VarBag (`c23->ss_inner.rid_entity_varbag`):\n";
	dump_varbag_entries(out, c23->ss_inner.rid_entity_varbag, "  ");

	out << "\n#### `g3_arrWithIds` Entries (" << c23->ss_inner.g3_arrWithIds.num << "):\n";
	for (int i = 0; i < c23->ss_inner.g3_arrWithIds.num; i++) {
		auto& item = c23->ss_inner.g3_arrWithIds.ptr[i];
		StatescriptInstance* inst_ptr = *(StatescriptInstance**)&item;
		out << std::format("  - [{}] inst_id: `{}` (`0x{:04X}`), inst_ptr: `0x{:X}`, script_id: `0x{:016X}`\n",
			i, (int)item.instance_id, (unsigned int)(uint16_t)item.instance_id, (uintptr_t)inst_ptr,
			(inst_ptr && is_valid_read_memory(inst_ptr, sizeof(StatescriptInstance))) ? (unsigned long long)inst_ptr->script_id : 0ULL);
	}

	out << "\n#### `g1_instanceArr` Script Instances (" << c23->ss_inner.g1_instanceArr.num << "):\n";
	for (int s_idx = 0; s_idx < c23->ss_inner.g1_instanceArr.num; s_idx++) {
		StatescriptInstance* script = c23->ss_inner.g1_instanceArr.ptr[s_idx];
		if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance))) continue;

		uint32_t exec_flags = *(uint32_t*)((uintptr_t)script + 0x258);
		out << std::format("\n##### Instance [{}] id={} (`0x{:04X}`), Script Resource `0x{:016X}`, parent_id={}, exec_flags(+0x258)=`0x{:08X}`, graph=`0x{:X}`\n",
			s_idx, script->instance_id, (uint16_t)script->instance_id,
			(unsigned long long)script->script_id, script->parent_instance_id,
			exec_flags, (uintptr_t)script->graph);

		out << "- **Instance VarBag 1 (`rid_instance_varbag`)**:\n";
		dump_varbag_entries(out, script->rid_instance_varbag, "    ");
		out << "- **Instance VarBag 2 (`second_inst_varbag`)**:\n";
		dump_varbag_entries(out, script->second_inst_varbag, "    ");

		if (script->state_impl_list.ptr && script->state_impl_list.num > 0 &&
			is_valid_read_memory(script->state_impl_list.ptr, script->state_impl_list.num * sizeof(void*))) {
			out << std::format("- **Runtime `state_impl_list`** (`ptr = 0x{:X}`, `num = {}`):\n",
				(uintptr_t)script->state_impl_list.ptr, script->state_impl_list.num);
			int state_cnt = script->state_impl_list.num;
			if (state_cnt > 16) state_cnt = 16;
			for (int si = 0; si < state_cnt; si++) {
				auto* st = script->state_impl_list.ptr[si];
				if (!st || !is_valid_read_memory(st, 0x80)) continue;
				uintptr_t vf = *(uintptr_t*)st;
				uintptr_t vf_rva = (vf >= globals::gameBase && vf < globals::gameBase + 0x3000000) ? (vf - globals::gameBase) : vf;
				uint32_t st_flags = *(uint32_t*)((uintptr_t)st + 0x40);
				std::string lbl = std::format("state_impl[{}] (idx_in_m_states={}, vfptr=+0x{:X}, flags=0x{:08X})",
					si, st->idx_in_m_states, vf_rva, st_flags);
				dump_raw_mem_block(out, lbl.c_str(), st, 0x100);
			}
		}

		if (script->graph && is_valid_read_memory(script->graph, sizeof(STUStatescriptGraph))) {
			out << std::format("- **Graph Counts**: `m_nodes = {}`, `m_states = {}`, `m_entries = {}`\n",
				script->graph->m_nodes.count(), script->graph->m_states.count(),
				script->graph->m_entries.count());

			auto script_state = statescript_logger::GetScriptState(script);

			for (auto* node : script->graph->m_nodes) {
				if (!node || !is_valid_read_memory(node, sizeof(STUStatescriptBase))) continue;
				STUInfo* n_info = safe_get_stu_info(node);
				uint n_hash = n_info ? n_info->name_hash : 0;
				std::string stu_name = "Unknown";
				std::string rtti_name = get_rtti_classname(node);
				auto it = allmighty_hash_lib::hashes.find(n_hash);
				if (it != allmighty_hash_lib::hashes.end()) stu_name = it->second;
				else if (!rtti_name.empty()) stu_name = rtti_name;
				else if (n_hash) stu_name = std::format("0x{:08X}", n_hash);

				auto state_it = script_state.find(node->idx_in_nodes);
				__int64 last_ts = (state_it != script_state.end()) ? state_it->second.change_timestamp : 0;
				bool finished = (state_it != script_state.end()) ? state_it->second.is_finish_state : false;

				out << std::format("\n  * **Node {}**: `[{}]` (STU `0x{:08X}`, RTTI `{}`) | isState={}, CO={}, SERVER={} | last_ts={} ({})\n",
					node->idx_in_nodes, stu_name, n_hash, rtti_name,
					(int)node->m_isState, (int)node->m_clientOnly, (int)node->m_serverOnly,
					last_ts, (state_it == script_state.end()) ? "NEVER_ENTERED" : (finished ? "FINISHED" : "ACTIVE"));

				// Output plugs & links
				auto plug_map = node->get_output_plugs();
				for (auto& plug_entry : plug_map) {
					for (auto* plug : plug_entry.second) {
						if (!plug || !is_valid_read_memory(plug, sizeof(STUGraphPlug))) continue;
						std::string plug_rtti = get_rtti_classname(plug);
						STUInfo* p_stu = safe_get_stu_info(plug);
						uint plug_hash = p_stu ? p_stu->name_hash : 0;
						out << std::format("    -> OutPlug `0x{:08X}` [{}] (links: {}):\n", plug_hash, plug_rtti, plug->m_links.count());
						for (auto* link : plug->m_links) {
							if (!link || !is_valid_read_memory(link, sizeof(STUGraphLink))) continue;
							if (link->m_inputPlug && is_valid_read_memory(link->m_inputPlug, sizeof(STUStatescriptInputPlug))) {
								auto* target = link->m_inputPlug->plug_base.m_parentNode;
								if (target && is_valid_read_memory(target, sizeof(STUStatescriptBase))) {
									out << std::format("       ==> Link to Node {} [{}]\n", target->idx_in_nodes, get_rtti_classname(target));
								}
							}
						}
					}
				}

				// Deep STU fields of the node
				if (n_info) {
					std::unordered_set<uintptr_t> visited_ptrs;
					visited_ptrs.insert((uintptr_t)node);
					dump_stu_object_fields(out, node, n_info, 2, 4, visited_ptrs);
				}
			}
		}
	}
	out << "\n---\n\n";
}

void OriginClutterTracker::DumpSpawnDoorsToFile(EntityAdminBase* game_ea, const char* trigger_reason) {
	ensure_known_stu_hashes();
	if (!game_ea) {
		s_last_door_dump_status = "Error: GameEntityAdmin is null.";
		return;
	}

	std::ofstream out("spawn_doors_diag.log", std::ios::out | std::ios::trunc);
	if (!out.is_open()) {
		s_last_door_dump_status = "Error: Failed to open spawn_doors_diag.log for writing.";
		return;
	}

	auto* world = get_system27_WorldEngineSystem(game_ea);
	__int64 map_id = world ? world->loaded_map_id : 0;
	int wstate = world ? (int)world->world_state : -1;

	out << "# Spawn Doors & Map Statescript Forensic Dump\n";
	out << "- **Trigger Reason**: `" << (trigger_reason ? trigger_reason : "Manual") << "`\n";
	out << std::format("- **Loaded Map ID**: `0x{:016X}` (`world_state = {}`)\n", (unsigned long long)map_id, wstate);
	out << std::format("- **`GameEntityAdmin`**: `0x{:X}` (vfptr `+0x{:X}`)\n\n",
		(uintptr_t)game_ea, *(uintptr_t*)game_ea - (uintptr_t)globals::gameBase);

	uint32 local_ctrl_id = game_ea->local_entid;
	uint32 local_mdl_id = 0;
	if (Entity* lc = game_ea->getLocalEnt()) {
		if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
			local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
		}
	}

	int dumped_doors = 0;
	for (Entity* ent : *game_ea) {
		if (!ent) continue;
		if (ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
		if (ent->hasComponent(0x15) || ent->hasComponent(0x2F) || ent->hasComponent(0x3F) || ent->hasComponent(0x1D)) continue;
		if (IsSpawnDoorEntity(ent, local_ctrl_id, local_mdl_id) || (ent->hasComponent(0x12) && ent->hasComponent(0x2A))) {
			dump_entity_door_forensics(out, ent, "GameEntityAdmin");
			dumped_doors++;
		}
	}

	// Also check LobbyEntityAdmin if it exists and has any door/mover entities
	EntityAdminBase* lobby_ea = LobbyEntityAdmin();
	if (lobby_ea && is_valid_read_memory(lobby_ea, sizeof(EntityAdminBase))) {
		for (Entity* ent : *lobby_ea) {
			if (!ent) continue;
			if (ent->hasComponent(0x12) || ent->hasComponent(0x29) || ent->hasComponent(0x2A)) {
				dump_entity_door_forensics(out, ent, "LobbyEntityAdmin");
			}
		}
	}

	// Section: Summary of all other non-player entities in GameEntityAdmin with Component_23_Statescript
	out << "## Non-Player Map/System Entities With `Component_23_Statescript` in `GameEntityAdmin`\n";
	out << "| EntID | STU ID | Resload ID | Pos | `[c23+0xBC]` | `g1_num` | Script IDs (`0x580...`) | Components |\n";
	out << "|---|---|---|---|---|---|---|---|\n";
	for (Entity* ent : *game_ea) {
		if (!ent) continue;
		if (ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
		if (ent->hasComponent(0x15) || ent->hasComponent(0x2F) || ent->hasComponent(0x3F) || ent->hasComponent(0x1D)) continue;
		auto* c23 = ent->getById<Component_23_Statescript>(0x23);
		if (!c23 || !is_valid_read_memory(c23, sizeof(Component_23_Statescript))) continue;

		auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
		__int64 stu_id = slot ? slot->stu_id : 0;
		__int64 res_id = (ent->resload_entry && is_valid_read_memory(ent->resload_entry, sizeof(void*)) && ent->resload_entry->valid())
			? ent->resload_entry->align()->resource_id : 0;
		auto* c1 = ent->getById<Component_1_SceneRendering>(1);
		Vector4 pos = (c1 && is_valid_read_memory(c1, sizeof(Component_1_SceneRendering))) ? c1->position : Vector4{};
		uint8_t init_bc = *(uint8_t*)((uintptr_t)c23 + 0xBC);

		std::string scripts_str;
		for (int i = 0; i < c23->ss_inner.g1_instanceArr.num; i++) {
			auto* inst = c23->ss_inner.g1_instanceArr.ptr[i];
			if (inst && is_valid_read_memory(inst, sizeof(StatescriptInstance))) {
				if (!scripts_str.empty()) scripts_str += ", ";
				scripts_str += std::format("0x{:X}(id={})", (unsigned long long)inst->script_id, inst->instance_id);
			}
		}

		std::string comps_str;
		for (int i = 0; i < ent->component_list.num - 1; i++) {
			if (ent->component_list.ptr[i] && is_valid_read_memory(ent->component_list.ptr[i], sizeof(ComponentBase))) {
				if (!comps_str.empty()) comps_str += ",";
				comps_str += std::format("{:02X}", (int)ent->component_list.ptr[i]->component_id);
			}
		}

		out << std::format("| `0x{:X}` | `0x{:X}` | `0x{:X}` | `({:.1f},{:.1f},{:.1f})` | `{}` | `{}` | `{}` | `[{}]` |\n",
			ent->entity_id, (unsigned long long)stu_id, (unsigned long long)res_id,
			pos.X, pos.Y, pos.Z, (int)init_bc, c23->ss_inner.g1_instanceArr.num,
			scripts_str.empty() ? "none" : scripts_str, comps_str);
	}

	out.close();
	s_last_door_dump_status = std::format("Dumped {} door entities to spawn_doors_diag.log ({})",
		dumped_doors, trigger_reason ? trigger_reason : "Manual");
	printf("[OriginClutterTracker] %s\n", s_last_door_dump_status.c_str());
}

void statescript_diag_window::dump_weapon_nodes_forensics_to_file() {
	if (!_ss || !is_valid_read_memory(_ss, sizeof(Component_23_Statescript))) {
		_experiment_status_msg = "Error: No Statescript component selected!";
		return;
	}

	std::ofstream out("weapon_nodes_forensics.log");
	if (!out.is_open()) {
		_experiment_status_msg = "Error: Failed to open weapon_nodes_forensics.log for writing!";
		return;
	}

	ensure_known_stu_hashes();

	out << "# WEAPON NODES & ENTITY STATE FORENSICS REPORT\n\n";
	out << "Generated by `[Step 1 Forensics] Dump Weapon Nodes & Entity State`\n\n";

	auto* ea = GameEntityAdmin();
	Entity* local_controller = ea ? ea->getLocalEnt() : nullptr;
	Entity* local_model = nullptr;
	if (local_controller && is_valid_read_memory(local_controller, sizeof(Entity))) {
		auto* pet = local_controller->getById<Component_20_ModelReference>(0x20);
		if (pet && is_valid_read_memory(pet, sizeof(Component_20_ModelReference))) {
			local_model = ea->getEntById(pet->cam_attach_entid);
			if (!local_model && pet->aim_entid) local_model = ea->getEntById(pet->aim_entid);
		}
	}
	if (!local_model && _ss->base.entity_backref) {
		local_model = _ss->base.entity_backref;
	}

	// =========================================================================
	// SECTION A: Safe MSVC x64 RTTI Class Name Resolver Summary
	// =========================================================================
	out << "## Section A: MSVC x64 RTTI Class Name Resolver Status\n";
	out << "- Game Base: `0x" << std::hex << (uintptr_t)globals::gameBase << std::dec << "`\n";
	out << "- Resolver: Reading `_RTTICompleteObjectLocator` at `vfptr[-1]` with `VirtualQuery` guard\n";
	out << "- Verified on `Component_23_Statescript`: `" << get_rtti_classname(_ss) << "`\n\n";

	// =========================================================================
	// SECTION B: Controller & Model Entity Component & Linkage Audit
	// =========================================================================
	out << "## Section B: Controller & Model Entity Component & Linkage Audit\n\n";

	auto dump_components = [&](Entity* ent, const char* label) {
		out << "### Attached Components on " << label << " (EntID: `0x"
			<< std::hex << (ent ? ent->entity_id : 0) << std::dec << "`)\n\n";
		if (!ent || !is_valid_read_memory(ent, sizeof(Entity))) {
			out << "(Entity null or unreadable)\n\n";
			return;
		}
		out << "| Slot / ID | Component Ptr | vfptr RVA | MSVC RTTI Class Name |\n";
		out << "|---|---|---|---|\n";
		int count = 0;
		for (int i = 0; i < ent->component_list.num - 1; i++) {
			auto* comp = ent->component_list.ptr[i];
			if (!comp || !is_valid_read_memory(comp, sizeof(ComponentBase))) continue;
			uint8_t slot = comp->component_id;
			uintptr_t vfptr_rva = comp->vfptr ? ((uintptr_t)comp->vfptr - globals::gameBase) : 0;
			std::string rtti_name = get_rtti_classname(comp);
			if (rtti_name.empty()) rtti_name = "(no RTTI)";
			out << "| `0x" << std::hex << (int)slot << std::dec << "` | `0x"
				<< std::hex << (uintptr_t)comp << std::dec << "` | `+0x"
				<< std::hex << vfptr_rva << std::dec << "` | `"
				<< rtti_name << "` |\n";
			count++;
		}
		out << "\nTotal attached components: " << count << "\n\n";
	};

	dump_components(local_controller, "Local Controller Entity");
	dump_components(local_model, "Local Model Entity");

	out << "### Detailed Field Audits of Key Components\n\n";

	// Component 0x20 ModelReference on Controller
	if (local_controller && is_valid_read_memory(local_controller, sizeof(Entity))) {
		auto* pet_comp = local_controller->getById<Component_20_ModelReference>(0x20);
		if (pet_comp && is_valid_read_memory(pet_comp, sizeof(Component_20_ModelReference))) {
			out << "#### Component 0x20 (ModelReference) on Controller (`0x"
				<< std::hex << (uintptr_t)pet_comp << std::dec << "`):\n";
			out << "- `aim_entid` (+0x90): `0x" << std::hex << pet_comp->aim_entid << std::dec << "`\n";
			out << "- `cam_attach_entid` (+0x94): `0x" << std::hex << pet_comp->cam_attach_entid << std::dec << "`\n";
			out << "- `movement_attach_entid` (+0x98): `0x" << std::hex << pet_comp->movement_attach_entid << std::dec << "`\n";
			out << "\nRaw 64-bit Hex Dump of Component 0x20 (offsets 0x00 to 0xC0):\n```\n";
			uint64_t* raw = (uint64_t*)pet_comp;
			for (int off = 0; off < 0xC0; off += 8) {
				uint64_t val = is_valid_read_memory(&raw[off / 8], 8) ? raw[off / 8] : 0;
				out << std::format("+0x{:02X}: {:016X}\n", off, val);
			}
			out << "```\n\n";
		}
		else {
			out << "#### Component 0x20 on Controller: NULL or invalid\n\n";
		}
	}

	// Component 0x20 ModelReference on Model Entity (local_model)
	if (local_model && is_valid_read_memory(local_model, sizeof(Entity))) {
		auto* model_pet = local_model->getById<Component_20_ModelReference>(0x20);
		if (model_pet && is_valid_read_memory(model_pet, sizeof(Component_20_ModelReference))) {
			out << "#### Component 0x20 (ModelReference) on Model (`0x"
				<< std::hex << (uintptr_t)model_pet << std::dec << "`):\n";
			out << "- `aim_entid` (+0x90): `0x" << std::hex << model_pet->aim_entid << std::dec << "`\n";
			out << "- `cam_attach_entid` (+0x94): `0x" << std::hex << model_pet->cam_attach_entid << std::dec << "`\n";
			out << "- `movement_attach_entid` (+0x98): `0x" << std::hex << model_pet->movement_attach_entid << std::dec << "`\n";
			out << "\nRaw 64-bit Hex Dump of Component 0x20 on Model (offsets 0x00 to 0xC0):\n```\n";
			uint64_t* raw = (uint64_t*)model_pet;
			for (int off = 0; off < 0xC0; off += 8) {
				uint64_t val = is_valid_read_memory(&raw[off / 8], 8) ? raw[off / 8] : 0;
				out << std::format("+0x{:02X}: {:016X}\n", off, val);
			}
			out << "```\n\n";
		}
		else {
			out << "#### Component 0x20 on Model: NULL or invalid\n\n";
		}
	}

	// Component 0x1F Backref on Model
	if (local_model && is_valid_read_memory(local_model, sizeof(Entity))) {
		auto* backref = local_model->getById<Component_1F_Backref>(0x1F);
		if (backref && is_valid_read_memory(backref, sizeof(Component_1F_Backref))) {
			out << "#### Component 0x1F (Backref) on Model (`0x"
				<< std::hex << (uintptr_t)backref << std::dec << "`):\n";
			out << "- `backref_entid` (+0x18): `0x" << std::hex << backref->backref_entid << std::dec << "`\n";
			out << "- `backref_global_entid` (+0x1C): `0x" << std::hex << backref->backref_global_entid << std::dec << "`\n";
			out << "\nRaw 64-bit Hex Dump of Component 0x1F (offsets 0x00 to 0x40):\n```\n";
			uint64_t* raw = (uint64_t*)backref;
			for (int off = 0; off < 0x40; off += 8) {
				uint64_t val = is_valid_read_memory(&raw[off / 8], 8) ? raw[off / 8] : 0;
				out << std::format("+0x{:02X}: {:016X}\n", off, val);
			}
			out << "```\n\n";
		}
		else {
			out << "#### Component 0x1F on Model: NULL or invalid\n\n";
		}
	}

	// Component 0x23 StatescriptSyncMgr
	auto& mgr = _ss->ss_inner;
	out << "#### Component 0x23 (StatescriptSyncMgr at +0xC0):\n";
	out << "- `ontick_updates_enabled` (+0x368): " << mgr.ontick_updates_enabled << "\n";
	out << "- `field_36C` (+0x36C): " << mgr.field_36C << "\n";
	out << "- `field_370` (+0x370): " << mgr.field_370 << "\n";
	out << "- `flag_374` (+0x374): " << (int)mgr.flag_374 << "\n";
	out << "- `is_instantiating` (+0x375): " << (int)mgr.is_instantiating << "\n";
	out << "- `has_clientonly_scripts` (+0x376): " << (int)mgr.has_clientonly_scripts << "\n";
	out << "- `latest_logbtn_down` (+0x377): " << (int)mgr.latest_logbtn_down << "\n";
	out << "- `latest_logbtn` (+0x378): " << mgr.latest_logbtn << "\n";
	out << "- `entity_admin_scope` (+0x3D0): " << mgr.entity_admin_scope << "\n";
	out << "- `field_444` (+0x444): " << (int)mgr.field_444 << "\n";
	out << "- `has_player_entbackref` (+0x448): " << mgr.has_player_entbackref << "\n";
	out << "- `field_4D0` (+0x4D0): " << (int)mgr.field_4D0 << "\n";
	out << "- `field_4D1` (+0x4D1): " << (int)mgr.field_4D1 << "\n";
	out << "- `is_aim_ent` (+0x4D2): " << (int)mgr.is_aim_ent << "\n";
	out << "- `is_aim_ent_assigned` (+0x4D3): " << (int)mgr.is_aim_ent_assigned << "\n\n";

	// 3. STU Entity Definition (0x400000000000001) Component Map
	out << "#### STU Entity Definition (`0x400000000000001` - Tracer Model Entity) Component Map:\n";
	auto* model_res = stu_resources::GetByID(0x400000000000001);
	if (model_res) {
		auto entity_def = model_res->to_editable();
		auto comp_map = entity_def.get_argument_map("m_componentMap");
		if (comp_map.valid()) {
			out << "| Map Key (Hash) | Component Key Name | STU Type Hash | STU Class Name | MSVC RTTI Class Name |\n";
			out << "|---|---|---|---|---|\n";
			for (auto entry : comp_map) {
				int key = entry.first;
				std::string key_name = "Unknown";
				auto it_k = allmighty_hash_lib::hashes.find(key);
				if (it_k != allmighty_hash_lib::hashes.end()) key_name = it_k->second;

				STU_Object comp_obj = entry.second;
				uint stu_hash = comp_obj.struct_info ? comp_obj.struct_info->name_hash : 0;
				std::string stu_name = "Unknown";
				auto it_s = allmighty_hash_lib::hashes.find(stu_hash);
				if (it_s != allmighty_hash_lib::hashes.end()) stu_name = it_s->second;
				else if (stu_hash) stu_name = std::format("0x{:08x}", stu_hash);

				std::string rtti_name = comp_obj.value ? get_rtti_classname(comp_obj.value) : "";
				if (rtti_name.empty()) rtti_name = "(no RTTI)";

				out << "| `0x" << std::hex << key << std::dec << "` | `" << key_name
					<< "` | `0x" << std::hex << stu_hash << std::dec << "` | `" << stu_name
					<< "` | `" << rtti_name << "` |\n";
			}
			out << "\n";
		}
		else {
			out << "(m_componentMap invalid or empty)\n\n";
		}
	}
	else {
		out << "(Model resource 0x400000000000001 not loaded in stu_resources)\n\n";
	}

	// =========================================================================
	// SECTION C: Runtime StatescriptState Implementation & STU Node Inspection
	// =========================================================================
	out << "## Section C: Runtime StatescriptState Implementation & STU Node Inspection\n\n";

	StatescriptInstance* inst_weapon = nullptr;
	StatescriptInstance* inst_controller = nullptr;
	for (auto script : _ss->ss_inner.g1_instanceArr) {
		if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance))) continue;
		if (script->script_id == 0x580000000000016 || script->instance_id == -24) inst_weapon = script;
		if (script->script_id == 0x58000000000001a || script->instance_id == -1) inst_controller = script;
	}

	// 1. Runtime StatescriptState Implementation Objects (Top of Section C)
	auto dump_state_impl_list = [&](StatescriptInstance* inst, const char* label, bool is_weapon_script) {
		out << "### " << label << " (`state_impl_list` at +0xA0, Script ID: `0x"
			<< std::hex << (inst ? inst->script_id : 0) << std::dec << "`)\n\n";
		if (!inst || !is_valid_read_memory(inst, sizeof(StatescriptInstance))) {
			out << "(Instance null or unreadable)\n\n";
			return;
		}

		// Dump states_int_list (+0xB8)
		out << "- `states_int_list` (+0xB8, count: " << inst->states_int_list.num << "): [ ";
		for (int i = 0; i < inst->states_int_list.num; i++) {
			out << inst->states_int_list.ptr[i] << " ";
		}
		out << "]\n\n";

		out << "| State Idx (k) | State Ptr | Node Idx (+0x18) | Last TS (+0x20) | Active (+0x28) | vfptr RVA | Virtual Functions (vfn[0..9]) |\n";
		out << "|---|---|---|---|---|---|---|\n";

		std::set<int> state_hex_targets = { 1, 2, 12, 13, 26, 40, 43, 44, 45, 47 };
		std::vector<std::pair<int, StatescriptState*>> target_states_for_hex;

		for (int k = 0; k < inst->state_impl_list.num; k++) {
			StatescriptState* s = inst->state_impl_list.ptr[k];
			if (!s || !is_valid_read_memory(s, sizeof(void*))) continue;

			int node_idx = is_valid_read_memory((void*)((uintptr_t)s + 0x18), 4) ? *(int*)((uintptr_t)s + 0x18) : -1;
			uint32_t last_ts = is_valid_read_memory((void*)((uintptr_t)s + 0x20), 4) ? *(uint32_t*)((uintptr_t)s + 0x20) : 0;
			uint8_t is_active = is_valid_read_memory((void*)((uintptr_t)s + 0x28), 1) ? *(uint8_t*)((uintptr_t)s + 0x28) : 0;
			uintptr_t vfptr = is_valid_read_memory(s, sizeof(void*)) ? *(uintptr_t*)s : 0;
			uintptr_t vfptr_rva = vfptr ? (vfptr - (uintptr_t)globals::gameBase) : 0;

			std::string vfn_str;
			if (vfptr && is_valid_read_memory((void*)vfptr, 10 * sizeof(uintptr_t))) {
				uintptr_t* vt = (uintptr_t*)vfptr;
				for (int m = 0; m < 10; m++) {
					uintptr_t fn_rva = vt[m] ? (vt[m] - (uintptr_t)globals::gameBase) : 0;
					vfn_str += std::format("+0x{:X} ", fn_rva);
				}
			}

			out << "| `" << k << "` | `0x" << std::hex << (uintptr_t)s << std::dec
				<< "` | `" << node_idx << "` | `" << last_ts << "` | `" << (int)is_active
				<< "` | `+0x" << std::hex << vfptr_rva << std::dec << "` | `" << vfn_str << "` |\n";

			if (is_weapon_script && state_hex_targets.count(node_idx)) {
				target_states_for_hex.push_back({ node_idx, s });
			}
		}
		out << "\n";

		if (!target_states_for_hex.empty()) {
			out << "#### Detailed 0x80 Hex Dumps of Target StatescriptState Objects:\n\n";
			for (auto& item : target_states_for_hex) {
				int n_idx = item.first;
				StatescriptState* s = item.second;
				out << "##### StatescriptState for Node " << n_idx << " (`0x" << std::hex << (uintptr_t)s << std::dec << "`):\n```\n";
				uint64_t* raw_s = (uint64_t*)s;
				for (int off = 0; off < 0x80; off += 8) {
					uint64_t val = is_valid_read_memory(&raw_s[off / 8], 8) ? raw_s[off / 8] : 0;
					out << std::format("+0x{:02X}: {:016X}", off, val);
					if (val >= (uint64_t)globals::gameBase && val < (uint64_t)globals::gameBase + 0x3000000) {
						out << std::format("  [Game RVA: +0x{:X}]", val - (uint64_t)globals::gameBase);
					}
					out << "\n";
				}
				out << "```\n\n";
			}
		}
	};

	dump_state_impl_list(inst_weapon, "Instance -24 (Weapon Script)", true);
	dump_state_impl_list(inst_controller, "Instance -1 (Controller Script)", false);

	// 2. Node STU Field & ConfigVar Inspections
	out << "### Node STU Field & ConfigVar Inspections\n\n";

	auto dump_instance_nodes_forensics = [&](StatescriptInstance* script, bool is_weapon_script) {
		if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance)) || !script->graph) return;
		out << "### Script Instance " << script->instance_id << " (Script ID: `0x"
			<< std::hex << script->script_id << std::dec << "`)\n";
		out << "- Total Nodes: " << script->graph->m_nodes.count() << "\n";
		out << "- Total States: " << script->graph->m_states.count() << "\n\n";

		std::set<int> raw_hex_target_nodes = { 13, 15, 26, 32, 33, 40, 43, 45, 47, 48 };
		std::set<int> weapon_allowed_nodes = { 1, 2, 12, 13, 15, 26, 32, 33, 40, 43, 45, 47, 48 };

		for (auto node : script->graph->m_nodes) {
			if (!node || !is_valid_read_memory(node, sizeof(STUStatescriptBase))) continue;
			int idx = node->idx_in_nodes;

			if (is_weapon_script) {
				if (weapon_allowed_nodes.find(idx) == weapon_allowed_nodes.end()) continue;
			}
			else {
				if (idx != 18) continue;
			}

			uint stu_hash = 0;
			STUInfo* sinfo = safe_get_stu_info(node);
			if (sinfo) stu_hash = sinfo->name_hash;

			std::string stu_name = "Unknown";
			auto it_h = allmighty_hash_lib::hashes.find(stu_hash);
			if (it_h != allmighty_hash_lib::hashes.end()) stu_name = it_h->second;
			else if (stu_hash) stu_name = std::format("0x{:08x}", stu_hash);

			std::string rtti_name = get_rtti_classname(node);
			if (rtti_name.empty()) rtti_name = "(no RTTI)";

			out << "#### Node [" << idx << "]: Address `0x" << std::hex << (uintptr_t)node << std::dec
				<< "`, STU `0x" << std::hex << stu_hash << std::dec << "` (`" << stu_name << "`)"
				<< ", RTTI: `" << rtti_name << "`\n";
			out << "- State Flags: `m_isState=" << (int)node->m_isState
				<< "`, `m_clientOnly=" << (int)node->m_clientOnly
				<< "`, `m_serverOnly=" << (int)node->m_serverOnly
				<< "`, `idx_in_nodes=" << node->idx_in_nodes
				<< "`, `idx_in_states=" << node->idx_in_states << "`\n";

			// Dump STU fields recursively (compact, with visited_ptrs tracking)
			if (sinfo) {
				out << "- **STU Fields / Arguments:**\n";
				std::unordered_set<uintptr_t> visited_ptrs;
				visited_ptrs.insert((uintptr_t)node);
				dump_stu_object_fields(out, node, sinfo, 1, 4, visited_ptrs);
			}

			// Output Plugs & Links
			auto plug_map = node->get_output_plugs();
			if (!plug_map.empty()) {
				out << "- **Output Plugs & Links:**\n";
				for (auto& entry : plug_map) {
					for (auto* plug : entry.second) {
						if (!plug || !is_valid_read_memory(plug, sizeof(STUGraphPlug))) continue;
						uint p_hash = 0;
						auto p_stu = safe_get_stu_info(plug);
						if (p_stu) p_hash = p_stu->name_hash;

						std::string p_rtti = get_rtti_classname(plug);
						std::string p_name = "Unknown";
						auto it_p = allmighty_hash_lib::hashes.find(p_hash);
						if (it_p != allmighty_hash_lib::hashes.end()) p_name = it_p->second;
						else if (p_hash) p_name = std::format("0x{:08x}", p_hash);

						out << "  -> Plug `0x" << std::hex << p_hash << std::dec << "` (`" << p_name
							<< "`, RTTI: `" << p_rtti << "`), links: " << plug->m_links.count() << "\n";
						for (auto* link : plug->m_links) {
							if (!link || !is_valid_read_memory(link, sizeof(STUGraphLink))) continue;
							if (link->m_inputPlug && is_valid_read_memory(link->m_inputPlug, sizeof(STUStatescriptInputPlug))) {
								auto* target = link->m_inputPlug->plug_base.m_parentNode;
								if (target && is_valid_read_memory(target, sizeof(STUStatescriptBase))) {
									std::string target_rtti = get_rtti_classname(target);
									out << "     ==> Link to Node " << target->idx_in_nodes;
									if (!target_rtti.empty()) out << " [" << target_rtti << "]";
									out << "\n";
								}
							}
						}
					}
				}
			}

			// Raw 8-byte aligned hex dump for target nodes in weapon script (0x580000000000016)
			if (is_weapon_script && raw_hex_target_nodes.find(idx) != raw_hex_target_nodes.end()) {
				dump_node_raw_hex(out, node, idx);
			}
			out << "\n";
		}
		out << "\n---\n\n";
	};

	if (inst_weapon) {
		dump_instance_nodes_forensics(inst_weapon, true);
	}
	else {
		out << "### Warning: Instance -24 (0x580000000000016) not found!\n\n";
	}

	if (inst_controller) {
		dump_instance_nodes_forensics(inst_controller, false);
	}
	else {
		out << "### Warning: Instance -1 (0x58000000000001a) not found!\n\n";
	}

	// =========================================================================
	// SECTION D: Current VarBag State After Task 1 Fix
	// =========================================================================
	out << "## Section D: Current VarBag State After Task 1 Fix\n\n";

	out << "### Entity VarBag (`ss_inner.rid_entity_varbag`)\n";
	dump_varbag_entries(out, _ss->ss_inner.rid_entity_varbag, "  ");
	out << "\n";

	auto dump_inst_varbags = [&](StatescriptInstance* script, const char* label) {
		if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance))) return;
		out << "### " << label << " (Instance " << script->instance_id
			<< ", Script ID `0x" << std::hex << script->script_id << std::dec << "`)\n";
		out << "#### `rid_instance_varbag` (+0x98):\n";
		dump_varbag_entries(out, script->rid_instance_varbag, "  ");
		out << "#### `second_inst_varbag` (+0x270):\n";
		dump_varbag_entries(out, script->second_inst_varbag, "  ");
		out << "#### `third_inst_varbag` (+0x280):\n";
		dump_varbag_entries(out, script->third_inst_varbag, "  ");
		out << "\n";
	};

	dump_inst_varbags(inst_weapon, "Instance -24 (Weapon Script)");
	dump_inst_varbags(inst_controller, "Instance -1 (Controller Script)");

	out.close();
	_experiment_status_msg = "Successfully dumped weapon_nodes_forensics.log!";
}

static void collect_stu_resource_refs_recursive(
	void* obj_ptr,
	STUInfo* sinfo,
	int depth,
	int max_depth,
	std::unordered_set<uintptr_t>& visited_ptrs,
	std::vector<std::pair<uint32_t, uint64_t>>& out_field_and_resid)
{
	if (!obj_ptr || !sinfo || depth > max_depth) return;
	if (!is_valid_read_memory(obj_ptr, 8) || !is_valid_read_memory(sinfo, sizeof(STUInfo))) return;

	for (auto arg_pair : *sinfo) {
		STUArgumentInfo* arg = arg_pair.second;
		if (!arg || !is_valid_read_memory(arg, sizeof(STUArgumentInfo))) continue;
		uint32_t field_hash = arg->name_hash;
		int offset = arg->offset;
		if (offset < 0 || offset > 0x1000) continue;

		// Skip parent-node/subgraph backrefs and base plugs
		if (field_hash == 0x365378c7 || field_hash == 0xa2eca5de ||
			field_hash == 0xbec2beae || field_hash == 0xfd24b86e ||
			field_hash == 0x25c665a || field_hash == 0xd2cd7c9f || field_hash == 0x3f6ffa05) {
			continue;
		}

		if (!arg->constraint || !is_valid_read_memory(arg->constraint, sizeof(void*))) continue;
		STUConstraintType ctype = arg->constraint->get_type_flag();
		void* field_ptr = (void*)((uintptr_t)obj_ptr + offset);

		if (ctype == STU_ConstraintType_ResourceRef || ctype == STU_ConstraintType_NonSTUResourceRef) {
			STUResourceReference* ref = (STUResourceReference*)field_ptr;
			if (is_valid_read_memory(ref, sizeof(STUResourceReference)) && ref->resource_id != 0) {
				out_field_and_resid.push_back({ field_hash, (uint64_t)ref->resource_id });
			}
		}
		else if (ctype == STU_ConstraintType_BSList_ResourceRef || ctype == STU_ConstraintType_BSList_NonSTUResourceRef) {
			auto* bs = (STUBullshitListFull<__int64>*)field_ptr;
			if (is_valid_read_memory(bs, sizeof(STUBullshitListFull<__int64>)) &&
				bs->list_ptr && is_valid_read_memory(bs->list_ptr, sizeof(STUBullshitListPtr))) {
				int cnt = bs->count();
				if (cnt > 0 && cnt < 256 && is_valid_read_memory(bs->list(), cnt * sizeof(STUResourceReference))) {
					auto* arr = (STUResourceReference*)bs->list();
					for (int k = 0; k < cnt; k++) {
						if (arr[k].resource_id != 0) {
							out_field_and_resid.push_back({ field_hash, (uint64_t)arr[k].resource_id });
						}
					}
				}
			}
		}
		else if (ctype == STU_ConstraintType_Object) {
			if (!is_valid_read_memory(field_ptr, 8)) continue;
			uint64_t raw_val = *(uint64_t*)field_ptr;
			if (raw_val > 0x10000 && raw_val < 0x7FFFFFFFFFFFull) {
				void* child_ptr = (void*)raw_val;
				if (!visited_ptrs.count((uintptr_t)child_ptr) && is_valid_read_memory(child_ptr, 8)) {
					visited_ptrs.insert((uintptr_t)child_ptr);
					STUInfo* child_info = safe_get_stu_info(child_ptr);
					if (!child_info) child_info = GetSTUInfoByHash(arg->constraint->get_stu_type());
					uint32_t ch = child_info ? child_info->name_hash : 0;
					bool is_plug = (ch == 0x11d7f052 || ch == 0x3e490486 || ch == 0x24ab1f0b || ch == 0xe51f3e8d || ch == 0xf1f0a36d || ch == 0xb8f35b20);
					if (child_info && !is_plug) {
						collect_stu_resource_refs_recursive(child_ptr, child_info, depth + 1, max_depth, visited_ptrs, out_field_and_resid);
					}
				}
			}
		}
		else if (ctype == STU_ConstraintType_InlinedObject) {
			if (!visited_ptrs.count((uintptr_t)field_ptr) && is_valid_read_memory(field_ptr, 8)) {
				visited_ptrs.insert((uintptr_t)field_ptr);
				STUInfo* child_info = safe_get_stu_info(field_ptr);
				if (!child_info) child_info = GetSTUInfoByHash(arg->constraint->get_stu_type());
				uint32_t ch = child_info ? child_info->name_hash : 0;
				bool is_plug = (ch == 0x11d7f052 || ch == 0x3e490486 || ch == 0x24ab1f0b || ch == 0xe51f3e8d || ch == 0xf1f0a36d || ch == 0xb8f35b20);
				if (child_info && !is_plug) {
					collect_stu_resource_refs_recursive(field_ptr, child_info, depth + 1, max_depth, visited_ptrs, out_field_and_resid);
				}
			}
		}
		else if (ctype == STU_ConstraintType_BSList_Object || ctype == STU_ConstraintType_BSList_InlinedObject) {
			auto* bs = (STUBullshitListFull<__int64>*)field_ptr;
			if (is_valid_read_memory(bs, sizeof(STUBullshitListFull<__int64>)) &&
				bs->list_ptr && is_valid_read_memory(bs->list_ptr, sizeof(STUBullshitListPtr))) {
				STUInfo* list_type = GetSTUInfoByHash(arg->constraint->get_stu_type());
				if (list_type) {
					STU_ObjectList obj_list(arg, field_ptr);
					if (obj_list.valid()) {
						int cnt = obj_list.count();
						int lim = (cnt < 64) ? cnt : 64;
						for (int k = 0; k < lim; k++) {
							STU_Object elem = obj_list[k];
							if (elem.valid() && elem.value && is_valid_read_memory(elem.value, 8)) {
								STU_Object root_elem = elem.get_runtime_root();
								uint32_t eh = root_elem.struct_info ? root_elem.struct_info->name_hash : 0;
								bool is_plug = (eh == 0x11d7f052 || eh == 0x3e490486 || eh == 0x24ab1f0b || eh == 0xe51f3e8d || eh == 0xf1f0a36d || eh == 0xb8f35b20);
								if (root_elem.struct_info && !is_plug && !visited_ptrs.count((uintptr_t)root_elem.value)) {
									visited_ptrs.insert((uintptr_t)root_elem.value);
									collect_stu_resource_refs_recursive(root_elem.value, root_elem.struct_info, depth + 1, max_depth, visited_ptrs, out_field_and_resid);
								}
							}
						}
					}
				}
			}
		}
	}
}

void statescript_diag_window::dump_horizon1_audit_to_file() {
	ensure_known_stu_hashes();

	std::ofstream out("horizon1_stu_audit.log", std::ios::out | std::ios::trunc);
	if (!out.is_open()) {
		_dump_status_msg = "ERROR: Failed to open horizon1_stu_audit.log for writing!";
		_experiment_status_msg = _dump_status_msg;
		return;
	}

	out << "# HORIZON 1: LIVE STU & STATESCRIPT CENSUS AUDIT (`horizon1_stu_audit.log`)\n";
	out << "- **Game Base**: `0x" << std::hex << (uintptr_t)globals::gameBase << std::dec << "`\n\n";

	// =========================================================================
	// PRE-PASS: Ensure all 21 STUHero resources, their m_gameplayEntity (0x0400...),
	// their weapon/component scripts (0x0580...), and child sub-scripts/entities
	// are constructed in stu_resources before running the census!
	// =========================================================================
	for (uint64_t hid = 0x02E0000000000001ULL; hid <= 0x02E0000000000020ULL; hid++) {
		if (!stu_resources::GetByID((__int64)hid)) {
			try_load_resource((__int64)hid);
		}
	}

	struct GraphOrEntLink {
		uint64_t parent_id;      // 0x02E..., 0x0400..., or 0x0580...
		int node_idx;            // -1 if from entity/component
		uint32_t owner_stu_hash; // component hash or node STU hash
		uint32_t field_hash;
		uint64_t child_id;       // 0x0580..., 0x0400..., 0x09A0..., etc.
	};
	std::vector<GraphOrEntLink> discovered_links;
	std::unordered_set<uint64_t> queued_res_ids;
	std::vector<uint64_t> bfs_queue;
	std::map<uint64_t, std::string> res_owner_hero_map;

	auto enqueue_res = [&](uint64_t rid, const std::string& hero_tag) {
		if (rid == 0) return;
		if (!hero_tag.empty()) {
			if (res_owner_hero_map[rid].empty()) res_owner_hero_map[rid] = hero_tag;
			else if (res_owner_hero_map[rid].find(hero_tag) == std::string::npos) {
				res_owner_hero_map[rid] += ", " + hero_tag;
			}
		}
		uint64_t hi = (rid >> 48) & 0xFFFF;
		if (hi != 0x0400 && hi != 0x0580 && hi != 0x09A0 && hi != 0x0330) return;
		if (queued_res_ids.insert(rid).second) {
			bfs_queue.push_back(rid);
		}
	};

	auto heroes_map = stu_resources::GetIfAssignableTo(stringHash("STUHero"));
	for (auto& hpair : heroes_map) {
		uint64_t hero_id = (uint64_t)hpair.first;
		STUBase<>* hero_base = hpair.second;
		if (!hero_base || !hero_base->valid()) continue;
		STU_Object hero_obj = hero_base->to_editable();

		std::string hero_name = std::format("Hero_0x{:X}", hero_id & 0xFFFF);
		if (auto* name_ref = hero_obj.get_argument_resource(0xcd16baa)) {
			if (is_valid_read_memory(name_ref, sizeof(STUResourceReference)) &&
				name_ref->resource_id != 0 &&
				(name_ref->resource_id & 0xDE0000000000000ULL) != 0) {
				const char* s = GetUXDisplayString(name_ref->resource_id);
				if (s && s[0] != '\0') hero_name = s;
			}
		}

		uint64_t gp_ent_id = 0;
		if (auto* gp_ref = hero_obj.get_argument_resource("m_gameplayEntity")) {
			if (is_valid_read_memory(gp_ref, sizeof(STUResourceReference))) {
				gp_ent_id = (uint64_t)gp_ref->resource_id;
			}
		}
		if (gp_ent_id == 0 && is_valid_read_memory((void*)((uintptr_t)hero_base + 0x28), 8)) {
			uint64_t cand = *(uint64_t*)((uintptr_t)hero_base + 0x28);
			if (((cand >> 48) & 0xFFFF) == 0x0400) gp_ent_id = cand;
		}
		if (gp_ent_id != 0) {
			discovered_links.push_back({ hero_id, -1, stringHash("STUHero"), stringHash("m_gameplayEntity"), gp_ent_id });
			enqueue_res(gp_ent_id, hero_name);
		}
	}

	for (size_t qi = 0; qi < bfs_queue.size() && qi < 2500; qi++) {
		uint64_t cur_id = bfs_queue[qi];
		STUBase<>* res_ptr = stu_resources::GetByID((__int64)cur_id);
		if (!res_ptr) {
			try_load_resource((__int64)cur_id);
			res_ptr = stu_resources::GetByID((__int64)cur_id);
		}
		if (!res_ptr || !res_ptr->valid()) continue;
		STUInfo* rinfo = safe_get_stu_info(res_ptr);
		if (!rinfo) continue;
		std::string cur_hero_tag = res_owner_hero_map[cur_id];

		uint64_t hi = (cur_id >> 48) & 0xFFFF;
		if (hi == 0x0400) {
			STU_Object ent_obj(rinfo, res_ptr);
			// Scan m_childEntityData
			std::unordered_set<uintptr_t> vis_ent;
			vis_ent.insert((uintptr_t)res_ptr);
			std::vector<std::pair<uint32_t, uint64_t>> ent_refs;
			collect_stu_resource_refs_recursive(res_ptr, rinfo, 0, 4, vis_ent, ent_refs);
			for (auto& er : ent_refs) {
				discovered_links.push_back({ cur_id, -1, rinfo->name_hash, er.first, er.second });
				enqueue_res(er.second, cur_hero_tag);
			}
			// Scan m_componentMap
			STUArgumentInfo* cmap_arg = rinfo->argumentByHash(stringHash("m_componentMap"));
			if (cmap_arg) {
				auto* bs_map = (STUBullshitMapFull<void*>*)((uintptr_t)res_ptr + cmap_arg->offset);
				if (is_valid_read_memory(bs_map, sizeof(STUBullshitMapFull<void*>)) &&
					bs_map->list_ptr && is_valid_read_memory(bs_map->list_ptr, sizeof(STUBullshitMapPtr<void*>)) &&
					bs_map->flag == 2) {
					STU_Map comp_map(cmap_arg, bs_map);
					if (comp_map.valid()) {
						for (auto entry : comp_map) {
							STU_Object comp_obj = entry.second;
							if (!comp_obj.value || !is_valid_read_memory(comp_obj.value, 8)) continue;
							STUInfo* cinfo = safe_get_stu_info(comp_obj.value);
							if (!cinfo) cinfo = comp_obj.struct_info;
							if (!cinfo) continue;
							std::unordered_set<uintptr_t> vis_c;
							vis_c.insert((uintptr_t)comp_obj.value);
							std::vector<std::pair<uint32_t, uint64_t>> crefs;
							collect_stu_resource_refs_recursive(comp_obj.value, cinfo, 0, 5, vis_c, crefs);
							for (auto& cr : crefs) {
								discovered_links.push_back({ cur_id, -1, cinfo->name_hash, cr.first, cr.second });
								enqueue_res(cr.second, cur_hero_tag);
							}
						}
					}
				}
			}
		}
		else if (hi == 0x0580 && rinfo->assignable_to_hash(stringHash("STUStatescriptGraph"))) {
			STUStatescriptGraph* graph = (STUStatescriptGraph*)res_ptr;
			if (graph->m_nodes.list_ptr && is_valid_read_memory(graph->m_nodes.list_ptr, sizeof(STUBullshitListPtr))) {
				for (auto* node : graph->m_nodes) {
					if (!node || !is_valid_read_memory(node, sizeof(STUStatescriptBase))) continue;
					STUInfo* ninfo = safe_get_stu_info(node);
					if (!ninfo) continue;
					std::unordered_set<uintptr_t> vis_n;
					vis_n.insert((uintptr_t)node);
					std::vector<std::pair<uint32_t, uint64_t>> nrefs;
					collect_stu_resource_refs_recursive(node, ninfo, 0, 5, vis_n, nrefs);
					for (auto& nr : nrefs) {
						uint64_t ref_hi = (nr.second >> 48) & 0xFFFF;
						if (ref_hi == 0x0580 || ref_hi == 0x0400 || ref_hi == 0x09A0 || ref_hi == 0x0330) {
							discovered_links.push_back({ cur_id, node->idx_in_nodes, ninfo->name_hash, nr.first, nr.second });
							enqueue_res(nr.second, cur_hero_tag);
						}
					}
				}
			}
		}
	}

	// =========================================================================
	// SECTION 1: Complete StatescriptRTTI & STURegistry Implementation Matrix
	// =========================================================================
	out << "## Section 1: Complete `StatescriptRTTI` & `STURegistry` Implementation Matrix\n\n";

	std::unordered_map<InheritanceInfo*, StatescriptRTTI*> rtti_map;
	int total_rtti_nodes = 0;
	StatescriptRTTI* rtti_head = *(StatescriptRTTI**)(globals::gameBase + 0x18f8e88);
	for (StatescriptRTTI* cur = rtti_head; cur && is_valid_read_memory(cur, sizeof(StatescriptRTTI)); cur = cur->next_instance) {
		total_rtti_nodes++;
		if (cur->stuclass_inherit) {
			rtti_map[cur->stuclass_inherit] = cur;
		}
	}
	out << "- **`StatescriptRTTI` List Head (`+0x18F8E88`)**: `" << total_rtti_nodes << "` registered entries (`" << rtti_map.size() << "` unique `stuclass_inherit` pointers)\n\n";

	struct STUClassAuditEntry {
		STUInfo* info;
		uint32_t hash;
		std::string name;
		std::string category;
		int cat_order;
		int inst_size;
		bool has_direct_rtti;
		bool has_inherited_rtti;
		uint32_t inherited_from_hash;
		uintptr_t factory_rva;
		uintptr_t cv_impl_rva;
	};
	std::vector<STUClassAuditEntry> class_entries;
	std::unordered_set<uint32_t> stripped_node_hashes;
	std::unordered_map<uint32_t, STUClassAuditEntry> class_entry_by_hash;

	uint32_t hash_ss_base = stringHash("STUStatescriptBase");
	for (STURegistry* reg = GetSTURegistry(); reg && is_valid_read_memory(reg, sizeof(STURegistry)); reg = reg->next) {
		STUInfo* info = reg->info;
		if (!info || !is_valid_read_memory(info, sizeof(STUInfo))) continue;

		std::string category;
		int cat_order = 99;
		if (info->assignable_to_rtti(STU_RTTI::STUStatescriptState) || info->assignable_to_hash(STU_NAME::STUStatescriptState)) {
			category = "State";
			cat_order = 1;
		}
		else if (info->assignable_to_rtti(STU_RTTI::STUStatescriptActionTerminator) || info->assignable_to_hash(STU_NAME::STUStatescriptAction)) {
			category = "Action";
			cat_order = 2;
		}
		else if (info->assignable_to_rtti(STU_RTTI::STUStatescriptCondition)) {
			category = "Condition";
			cat_order = 3;
		}
		else if (info->assignable_to_rtti(STU_RTTI::STUStatescriptGameEntry)) {
			category = "Entry";
			cat_order = 4;
		}
		else if (info->assignable_to_hash( hash_ss_base )) {
			category = "StatescriptBase";
			cat_order = 5;
		}
		else if (info->assignable_to_hash(STU_NAME::STUConfigVar)) {
			category = "ConfigVar";
			cat_order = 6;
		}
		else {
			continue;
		}

		StatescriptRTTI* direct_rtti = nullptr;
		auto it_r = rtti_map.find(info->rtti_info);
		if (it_r != rtti_map.end()) {
			direct_rtti = it_r->second;
		}

		StatescriptRTTI* inherited_rtti = nullptr;
		uint32_t inherited_hash = 0;
		for (STUInfo* p = info->base_stu; p && is_valid_read_memory(p, sizeof(STUInfo)); p = p->base_stu) {
			auto it_p = rtti_map.find(p->rtti_info);
			if (it_p != rtti_map.end()) {
				inherited_rtti = it_p->second;
				inherited_hash = p->name_hash;
				break;
			}
		}

		StatescriptRTTI* eff_rtti = direct_rtti ? direct_rtti : inherited_rtti;
		uintptr_t fn_ptr = (eff_rtti && eff_rtti->field_8 && is_valid_read_memory((void*)eff_rtti->field_8, 8))
			? *(uintptr_t*)eff_rtti->field_8
			: (eff_rtti ? (uintptr_t)eff_rtti->factory_func : 0);
		uintptr_t cv_ptr = (eff_rtti && eff_rtti->field_8 && is_valid_read_memory((void*)(eff_rtti->field_8 + 8), 8))
			? *(uintptr_t*)(eff_rtti->field_8 + 8)
			: 0;

		uintptr_t fn_rva = (fn_ptr >= (uintptr_t)globals::gameBase && fn_ptr < (uintptr_t)globals::gameBase + 0x3000000)
			? (fn_ptr - (uintptr_t)globals::gameBase) : fn_ptr;
		uintptr_t cv_rva = (cv_ptr >= (uintptr_t)globals::gameBase && cv_ptr < (uintptr_t)globals::gameBase + 0x3000000)
			? (cv_ptr - (uintptr_t)globals::gameBase) : cv_ptr;

		bool has_direct = (direct_rtti != nullptr) && ((cat_order == 6) ? (cv_ptr != 0 || fn_ptr != 0) : (fn_ptr != 0));
		bool has_inh = (!has_direct && inherited_rtti != nullptr) && ((cat_order == 6) ? (cv_ptr != 0 || fn_ptr != 0) : (fn_ptr != 0));

		std::string kname = "Unknown";
		auto it_n = allmighty_hash_lib::hashes.find(info->name_hash);
		if (it_n != allmighty_hash_lib::hashes.end()) kname = it_n->second;

		STUClassAuditEntry e{
			info,
			info->name_hash,
			kname,
			category,
			cat_order,
			info->instance_size,
			has_direct,
			has_inh,
			inherited_hash,
			fn_rva,
			cv_rva
		};
		class_entries.push_back(e);
		class_entry_by_hash[info->name_hash] = e;
		if (!has_direct && cat_order <= 5) {
			stripped_node_hashes.insert(info->name_hash);
		}
	}

	std::sort(class_entries.begin(), class_entries.end(), [](const STUClassAuditEntry& a, const STUClassAuditEntry& b) {
		if (a.cat_order != b.cat_order) return a.cat_order < b.cat_order;
		if (a.has_direct_rtti != b.has_direct_rtti) return a.has_direct_rtti < b.has_direct_rtti; // Stripped first!
		return a.hash < b.hash;
	});

	// Summary counts per category
	out << "### 1.1 Category Summary\n";
	out << "| Category | Total STU Classes | With Direct RTTI | Stripped (No Direct RTTI) | With Base Fallback RTTI |\n";
	out << "|---|---|---|---|---|\n";
	for (int co = 1; co <= 6; co++) {
		std::string cname;
		int tot = 0, dir = 0, strp = 0, inh = 0;
		for (auto& ce : class_entries) {
			if (ce.cat_order != co) continue;
			cname = ce.category;
			tot++;
			if (ce.has_direct_rtti) dir++;
			else {
				strp++;
				if (ce.has_inherited_rtti) inh++;
			}
		}
		if (tot > 0) {
			out << std::format("| **{}** | {} | {} | **{}** | {} |\n", cname, tot, dir, strp, inh);
		}
	}
	out << "\n";

	// Full Matrix for States, Actions, Conditions, Entries (and stripped ConfigVars)
	out << "### 1.2 Statescript Node & Stripped ConfigVar Implementation Matrix\n";
	out << "| STU Hash | Known Name | Base STU Hash | Category | `inst_size` | Has Direct RTTI? | Fallback RTTI | `factory_func` RVA | `cv_impl` RVA |\n";
	out << "|---|---|---|---|---|---|---|---|---|\n";
	for (auto& ce : class_entries) {
		// Include all States/Actions/Conditions/Entries/StatescriptBase, and stripped ConfigVars
		if (ce.cat_order == 6 && ce.has_direct_rtti) continue;
		uint32_t base_h = (ce.info->base_stu && is_valid_read_memory(ce.info->base_stu, sizeof(STUInfo))) ? ce.info->base_stu->name_hash : 0;
		std::string fb_str = ce.has_inherited_rtti ? std::format("Yes (`0x{:08X}`)", ce.inherited_from_hash) : "NO";
		out << std::format("| `0x{:08X}` | `{}` | `0x{:08X}` | {} | `0x{:X}` | **{}** | {} | `+0x{:X}` | `+0x{:X}` |\n",
			ce.hash, ce.name, base_h, ce.category, ce.inst_size,
			ce.has_direct_rtti ? "YES" : "NO", fb_str, ce.factory_rva, ce.cv_impl_rva);
	}
	out << "\n";

	// Detailed Schema Dump of Every Stripped Node Class (and Stripped ConfigVars)
	out << "### 1.3 Complete `.bss`-Populated `STUInfo` Schemas for All Stripped Node & ConfigVar Classes\n\n";
	for (auto& ce : class_entries) {
		if (ce.has_direct_rtti) continue;
		STUInfo* info = ce.info;
		out << std::format("#### Stripped {} `0x{:08X}` (`{}`), `instance_size = 0x{:X}`\n",
			ce.category, ce.hash, ce.name, ce.inst_size);

		// Inheritance chain
		out << "- **Inheritance Chain**: ";
		for (STUInfo* p = info; p && is_valid_read_memory(p, sizeof(STUInfo)); p = p->base_stu) {
			std::string pname = "Unknown";
			auto it_p = allmighty_hash_lib::hashes.find(p->name_hash);
			if (it_p != allmighty_hash_lib::hashes.end()) pname = it_p->second;
			out << std::format("`0x{:08X}` (`{}`, sz=`0x{:X}`, args={})", p->name_hash, pname, p->instance_size, p->argument_count);
			if (p->base_stu) out << " -> ";
		}
		out << "\n";

		out << "- **Arguments (`STUArgumentInfo`)**:\n";
		int arg_idx = 0;
		for (auto arg_pair : *info) {
			STUInfo* def_cls = arg_pair.first;
			STUArgumentInfo* arg = arg_pair.second;
			if (!arg || !is_valid_read_memory(arg, sizeof(STUArgumentInfo))) continue;
			std::string aname = "Unknown";
			auto it_a = allmighty_hash_lib::hashes.find(arg->name_hash);
			if (it_a != allmighty_hash_lib::hashes.end()) aname = it_a->second;

			STUConstraintType ctype = (arg->constraint && is_valid_read_memory(arg->constraint, sizeof(void*)))
				? arg->constraint->get_type_flag() : (STUConstraintType)-1;
			uint32_t target_stu = (arg->constraint && is_valid_read_memory(arg->constraint, sizeof(void*)))
				? (uint32_t)arg->constraint->get_stu_type() : 0;
			std::string target_name = "Unknown";
			auto it_t = allmighty_hash_lib::hashes.find(target_stu);
			if (it_t != allmighty_hash_lib::hashes.end()) target_name = it_t->second;

			out << std::format("  - [{}] `+0x{:03X}`: Field `0x{:08X}` (`{}`) | Constraint: `{}` | TargetType: `0x{:08X}` (`{}`) | DefinedIn: `0x{:08X}`\n",
				arg_idx++, arg->offset, arg->name_hash, aname,
				STU_ConstraintType_ToString(ctype), target_stu, target_name,
				def_cls ? def_cls->name_hash : 0);

			if ((ctype == STU_ConstraintType_Enum || ctype == STU_ConstraintType_BSList_Enum) && target_stu != 0) {
				if (STUEnumDefinition* edef = STUFindEnum(target_stu)) {
					if (is_valid_read_memory(edef, sizeof(STUEnumDefinition)) && edef->values && edef->values_count > 0 && edef->values_count < 64) {
						out << "    * Enum Values: ";
						for (int ev = 0; ev < edef->values_count; ev++) {
							out << std::format("`{}=0x{:X}` ", edef->values[ev].value, edef->values[ev].hash);
						}
						out << "\n";
					}
				}
			}
		}
		out << "\n";
	}

	// =========================================================================
	// SECTION 2: Corpus-Wide Usage Census Across All Loaded STUStatescriptGraphs
	// =========================================================================
	out << "## Section 2: Corpus-Wide Usage Census Across All Loaded `STUStatescriptGraph`s (`0x0580...`)\n\n";

	struct NodeRepresentative {
		uint64_t script_id;
		STUStatescriptBase* node;
		STUInfo* node_info;
	};
	struct StrippedNodeUsage {
		uint32_t stu_hash = 0;
		std::string stu_name;
		std::string category;
		bool is_stripped = false;
		int total_nodes = 0;
		int server_only_nodes = 0;
		std::set<uint64_t> script_ids;
		std::vector<NodeRepresentative> reps;
	};
	std::map<uint32_t, StrippedNodeUsage> usage_by_hash;

	auto all_graphs = stu_resources::GetIfAssignableTo(stringHash("STUStatescriptGraph"));
	out << "- **Total Loaded `STUStatescriptGraph` Resources Scanned**: `" << all_graphs.size() << "`\n\n";

	for (auto& gpair : all_graphs) {
		uint64_t script_id = (uint64_t)gpair.first;
		STUStatescriptGraph* graph = (STUStatescriptGraph*)gpair.second;
		if (!graph || !is_valid_read_memory(graph, sizeof(STUStatescriptGraph))) continue;
		if (!graph->m_nodes.list_ptr || !is_valid_read_memory(graph->m_nodes.list_ptr, sizeof(STUBullshitListPtr))) continue;

		for (auto* node : graph->m_nodes) {
			if (!node || !is_valid_read_memory(node, sizeof(STUStatescriptBase))) continue;
			STUInfo* ninfo = safe_get_stu_info(node);
			if (!ninfo) continue;
			uint32_t nhash = ninfo->name_hash;
			bool is_strp = (stripped_node_hashes.count(nhash) > 0);
			bool is_srv = (node->m_serverOnly != 0);
			if (!is_strp && !is_srv) continue;

			auto& u = usage_by_hash[nhash];
			if (u.stu_hash == 0) {
				u.stu_hash = nhash;
				auto it_ce = class_entry_by_hash.find(nhash);
				u.stu_name = (it_ce != class_entry_by_hash.end()) ? it_ce->second.name : "Unknown";
				u.category = (it_ce != class_entry_by_hash.end()) ? it_ce->second.category : (node->m_isState ? "State" : "Action/Node");
				u.is_stripped = is_strp;
			}
			u.total_nodes++;
			if (is_srv) u.server_only_nodes++;
			u.script_ids.insert(script_id);

			if (u.reps.size() < 5) {
				bool already_from_script = false;
				for (auto& r : u.reps) {
					if (r.script_id == script_id) { already_from_script = true; break; }
				}
				if (!already_from_script || u.reps.size() < 2) {
					u.reps.push_back({ script_id, node, ninfo });
				}
			}
		}
	}

	std::vector<StrippedNodeUsage> sorted_usage;
	for (auto& kv : usage_by_hash) sorted_usage.push_back(kv.second);
	std::sort(sorted_usage.begin(), sorted_usage.end(), [](const StrippedNodeUsage& a, const StrippedNodeUsage& b) {
		if (a.is_stripped != b.is_stripped) return a.is_stripped > b.is_stripped;
		return a.total_nodes > b.total_nodes;
	});

	out << "### 2.1 Ranked Usage Table of Stripped & `m_serverOnly` Node Classes\n";
	out << "| STU Hash | Known Name | Category | Stripped (`act_vt==NULL`)? | Total Nodes | `m_serverOnly` Nodes | Graphs Count | Sample `0x0580...` Script IDs (with Hero Owner) |\n";
	out << "|---|---|---|---|---|---|---|---|\n";
	for (auto& u : sorted_usage) {
		std::string scripts_sample;
		int sc = 0;
		for (uint64_t sid : u.script_ids) {
			if (sc++ >= 8) { scripts_sample += "..."; break; }
			if (!scripts_sample.empty()) scripts_sample += ", ";
			std::string htag = res_owner_hero_map[sid];
			if (!htag.empty()) scripts_sample += std::format("`0x{:X}` ({})", sid, htag);
			else scripts_sample += std::format("`0x{:X}`", sid);
		}
		out << std::format("| `0x{:08X}` | `{}` | {} | **{}** | **{}** | {} | {} | {} |\n",
			u.stu_hash, u.stu_name, u.category, u.is_stripped ? "YES (STRIPPED)" : "Has RTTI",
			u.total_nodes, u.server_only_nodes, u.script_ids.size(), scripts_sample);
	}
	out << "\n";

	out << "### 2.2 Representative Live Node Instances (Up to 5 Per Stripped / `m_serverOnly` Class)\n\n";
	for (auto& u : sorted_usage) {
		out << std::format("#### Class `0x{:08X}` (`{}` — {}, Stripped={}, TotalNodes={}, Graphs={})\n\n",
			u.stu_hash, u.stu_name, u.category, u.is_stripped ? "YES" : "NO", u.total_nodes, u.script_ids.size());

		for (size_t ri = 0; ri < u.reps.size(); ri++) {
			auto& rep = u.reps[ri];
			std::string htag = res_owner_hero_map[rep.script_id];
			out << std::format("##### Representative [{}] in Graph `0x{:016X}` ({}) — `node[{}]` at `0x{:X}` (`m_isState={}`, `m_clientOnly={}`, `m_serverOnly={}`)\n",
				ri + 1, rep.script_id, htag.empty() ? "Shared/Map" : htag,
				rep.node->idx_in_nodes, (uintptr_t)rep.node,
				(int)rep.node->m_isState, (int)rep.node->m_clientOnly, (int)rep.node->m_serverOnly);

			// Outgoing plugs
			auto plug_map = rep.node->get_output_plugs();
			for (auto& pe : plug_map) {
				for (auto* plug : pe.second) {
					if (!plug || !is_valid_read_memory(plug, sizeof(STUGraphPlug))) continue;
					STUInfo* pinfo = safe_get_stu_info(plug);
					uint32_t phash = pinfo ? pinfo->name_hash : 0;
					out << std::format("  - OutPlug `0x{:08X}` [{}] (links: {}):", phash, get_rtti_classname(plug), plug->m_links.count());
					for (auto* link : plug->m_links) {
						if (link && is_valid_read_memory(link, sizeof(STUGraphLink)) &&
							link->m_inputPlug && is_valid_read_memory(link->m_inputPlug, sizeof(STUStatescriptInputPlug))) {
							auto* tgt = link->m_inputPlug->plug_base.m_parentNode;
							if (tgt && is_valid_read_memory(tgt, sizeof(STUStatescriptBase))) {
								STUInfo* tinfo = safe_get_stu_info(tgt);
								out << std::format(" ==> `node[{}]` (`0x{:08X}`)", tgt->idx_in_nodes, tinfo ? tinfo->name_hash : 0);
							}
						}
					}
					out << "\n";
				}
			}

			std::unordered_set<uintptr_t> visited;
			visited.insert((uintptr_t)rep.node);
			dump_stu_object_fields(out, rep.node, rep.node_info, 1, 4, visited);
			if (u.is_stripped || ri == 0) {
				dump_node_raw_hex(out, rep.node, rep.node->idx_in_nodes);
			}
		}
		out << "---\n\n";
	}

	// =========================================================================
	// SECTION 3: All 21 Launch Heroes' Gameplay Entities, Components, Weapons & Scripts Manifest
	// =========================================================================
	out << "## Section 3: All 21 Launch Heroes' Gameplay Entities, Components, Weapons & Scripts Manifest\n\n";

	std::set<uint64_t> dumped_full_topology_scripts;
	auto dump_graph_stripped_summary = [&](uint64_t script_id, const std::string& indent) {
		STUBase<>* gbase = stu_resources::GetByID((__int64)script_id);
		if (!gbase) {
			try_load_resource((__int64)script_id);
			gbase = stu_resources::GetByID((__int64)script_id);
		}
		if (!gbase || !gbase->valid()) {
			out << indent << std::format("- Script `0x{:016X}`: (not loaded)\n", script_id);
			return;
		}
		STUStatescriptGraph* graph = (STUStatescriptGraph*)gbase;
		if (!graph->m_nodes.list_ptr || !is_valid_read_memory(graph->m_nodes.list_ptr, sizeof(STUBullshitListPtr))) return;

		int total_n = graph->m_nodes.count();
		int total_s = graph->m_states.count();
		std::vector<std::string> notable_nodes;
		for (auto* node : graph->m_nodes) {
			if (!node || !is_valid_read_memory(node, sizeof(STUStatescriptBase))) continue;
			STUInfo* ninfo = safe_get_stu_info(node);
			uint32_t nh = ninfo ? ninfo->name_hash : 0;
			bool is_strp = (stripped_node_hashes.count(nh) > 0);
			bool is_srv = (node->m_serverOnly != 0);
			if (is_strp || is_srv || nh == 0x3db862c4 || nh == 0x860ee5fc || nh == 0xc2a01675 || nh == 0x6e81f6d4) {
				notable_nodes.push_back(std::format("node[{}]:`0x{:08X}`({}{}{})",
					node->idx_in_nodes, nh,
					is_strp ? "STRIPPED" : "ok",
					is_srv ? ",SRV" : "",
					node->m_isState ? ",State" : ",Act"));
			}
		}
		out << indent << std::format("- Script `0x{:016X}` (`nodes={}`, `states={}`): ", script_id, total_n, total_s);
		if (notable_nodes.empty()) {
			out << "No stripped or serverOnly nodes.\n";
		} else {
			for (size_t i = 0; i < notable_nodes.size(); i++) {
				if (i > 0) out << ", ";
				out << notable_nodes[i];
			}
			out << "\n";
		}
		if (dumped_full_topology_scripts.insert(script_id).second && total_n > 0 && total_n < 256) {
			for (auto* node : graph->m_nodes) {
				if (!node || !is_valid_read_memory(node, sizeof(STUStatescriptBase))) continue;
				STUInfo* ninfo = safe_get_stu_info(node);
				uint32_t nh = ninfo ? ninfo->name_hash : 0;
				bool is_strp = (stripped_node_hashes.count(nh) > 0);
				std::string links_s;
				auto pmap = node->get_output_plugs();
				for (auto& pe : pmap) {
					STUArgumentInfo* parg = ninfo ? ninfo->argumentByHash(pe.first) : nullptr;
					uint32_t poff = (parg && is_valid_read_memory(parg, sizeof(STUArgumentInfo))) ? parg->offset : 0;
					for (auto* plug : pe.second) {
						if (!plug || !is_valid_read_memory(plug, sizeof(STUGraphPlug))) continue;
						for (auto* lnk : plug->m_links) {
							if (lnk && is_valid_read_memory(lnk, sizeof(STUGraphLink)) &&
								lnk->m_inputPlug && is_valid_read_memory(lnk->m_inputPlug, sizeof(STUStatescriptInputPlug))) {
								auto* tgt = lnk->m_inputPlug->plug_base.m_parentNode;
								if (tgt && is_valid_read_memory(tgt, sizeof(STUStatescriptBase))) {
									links_s += std::format("+0x{:X}->{} ", poff, tgt->idx_in_nodes);
								}
							}
						}
					}
				}
				out << indent << std::format("  * `node[{}]`: `stu=0x{:08X}` `isSt={}` `cO={}` `sO={}` {} {}\n",
					node->idx_in_nodes, nh, (int)node->m_isState, (int)node->m_clientOnly, (int)node->m_serverOnly,
					is_strp ? "[STRIPPED]" : "", links_s);
			}
		}
	};

	uint32_t hash_ss_comp = 0x9b38211b; // STUStatescriptComponent
	uint32_t hash_wpn_comp = stringHash("STUWeaponComponent");
	uint32_t hash_hp_comp = 0xaebe9baa;  // STUHealthComponent

	for (auto& hpair : heroes_map) {
		uint64_t hero_id = (uint64_t)hpair.first;
		STUBase<>* hero_base = hpair.second;
		if (!hero_base || !hero_base->valid()) continue;
		STU_Object hero_obj = hero_base->to_editable();

		std::string hero_name = "Unknown";
		if (auto* name_ref = hero_obj.get_argument_resource(0xcd16baa)) {
			if (is_valid_read_memory(name_ref, sizeof(STUResourceReference)) &&
				name_ref->resource_id != 0 &&
				(name_ref->resource_id & 0xDE0000000000000ULL) != 0) {
				const char* s = GetUXDisplayString(name_ref->resource_id);
				if (s && s[0] != '\0') hero_name = s;
			}
		}

		uint64_t gp_ent_id = 0;
		if (auto* gp_ref = hero_obj.get_argument_resource("m_gameplayEntity")) {
			if (is_valid_read_memory(gp_ref, sizeof(STUResourceReference))) {
				gp_ent_id = (uint64_t)gp_ref->resource_id;
			}
		}
		if (gp_ent_id == 0 && is_valid_read_memory((void*)((uintptr_t)hero_base + 0x28), 8)) {
			uint64_t cand = *(uint64_t*)((uintptr_t)hero_base + 0x28);
			if (((cand >> 48) & 0xFFFF) == 0x0400) gp_ent_id = cand;
		}

		out << std::format("### Hero `{}` — `STUHero` `0x{:016X}` | `m_gameplayEntity` `0x{:016X}`\n\n",
			hero_name, hero_id, gp_ent_id);

		STUBase<>* ent_res = gp_ent_id ? stu_resources::GetByID((__int64)gp_ent_id) : nullptr;
		if (!ent_res || !ent_res->valid()) {
			out << "(Gameplay entity resource not loaded)\n\n---\n\n";
			continue;
		}

		STUInfo* ent_info = safe_get_stu_info(ent_res);
		STUArgumentInfo* cmap_arg = ent_info ? ent_info->argumentByHash(stringHash("m_componentMap")) : nullptr;
		if (!cmap_arg) {
			out << "(No m_componentMap argument on gameplay entity)\n\n---\n\n";
			continue;
		}

		auto* bs_map = (STUBullshitMapFull<void*>*)((uintptr_t)ent_res + cmap_arg->offset);
		if (!is_valid_read_memory(bs_map, sizeof(STUBullshitMapFull<void*>)) ||
			!bs_map->list_ptr || !is_valid_read_memory(bs_map->list_ptr, sizeof(STUBullshitMapPtr<void*>)) ||
			bs_map->flag != 2) {
			out << "(m_componentMap invalid)\n\n---\n\n";
			continue;
		}

		STU_Map comp_map(cmap_arg, bs_map);
		out << "#### Component Map (`m_componentMap`):\n";
		out << "| Key Hash | Component STU Hash | Known Name | MSVC RTTI |\n";
		out << "|---|---|---|---|\n";

		std::vector<std::pair<uint32_t, STU_Object>> key_comps;
		for (auto entry : comp_map) {
			uint32_t key = (uint32_t)entry.first;
			STU_Object comp_obj = entry.second;
			STUInfo* cinfo = comp_obj.value ? safe_get_stu_info(comp_obj.value) : comp_obj.struct_info;
			uint32_t chash = cinfo ? cinfo->name_hash : 0;
			std::string cname = "Unknown";
			auto it_c = allmighty_hash_lib::hashes.find(chash);
			if (it_c != allmighty_hash_lib::hashes.end()) cname = it_c->second;
			std::string crtti = comp_obj.value ? get_rtti_classname(comp_obj.value) : "";
			out << std::format("| `0x{:08X}` | `0x{:08X}` | `{}` | `{}` |\n", key, chash, cname, crtti);
			if (comp_obj.value && cinfo) {
				key_comps.push_back({ chash, STU_Object(cinfo, comp_obj.value) });
			}
		}
		out << "\n";

		for (auto& kc : key_comps) {
			uint32_t chash = kc.first;
			STU_Object& cobj = kc.second;
			if (chash == hash_ss_comp || chash == hash_wpn_comp || chash == hash_hp_comp) {
				std::string label = (chash == hash_ss_comp) ? "STUStatescriptComponent (0x9B38211B)"
					: (chash == hash_wpn_comp) ? "STUWeaponComponent" : "STUHealthComponent (0xAEBE9BAA)";
				out << "#### " << label << " (`0x" << std::hex << (uintptr_t)cobj.value << std::dec << "`):\n";
				std::unordered_set<uintptr_t> vis;
				vis.insert((uintptr_t)cobj.value);
				dump_stu_object_fields(out, cobj.value, cobj.struct_info, 1, 4, vis);

				std::unordered_set<uintptr_t> vis_r;
				vis_r.insert((uintptr_t)cobj.value);
				std::vector<std::pair<uint32_t, uint64_t>> crefs;
				collect_stu_resource_refs_recursive(cobj.value, cobj.struct_info, 0, 5, vis_r, crefs);
				bool printed_hdr = false;
				for (auto& cr : crefs) {
					if (((cr.second >> 48) & 0xFFFF) == 0x0580) {
						if (!printed_hdr) {
							out << "- **Referenced `0x0580...` Graphs & Stripped Node Audit**:\n";
							printed_hdr = true;
						}
						dump_graph_stripped_summary(cr.second, "  ");
					}
				}
				out << "\n";
			}
		}
		out << "---\n\n";
	}

	// =========================================================================
	// SECTION 4: Subscript & Projectile/Deployable Entity Graph Discovery
	// =========================================================================
	out << "## Section 4: Subscript (`0x3DB862C4`) & Projectile/Deployable Entity (`0x0400...` / `0x09A0...`) Call Graph\n\n";
	out << "| Parent Resource | Hero Owner | Node Idx | Owner STU Hash | Field Hash | Child Resource ID | Child Type |\n";
	out << "|---|---|---|---|---|---|---|\n";
	std::set<std::tuple<uint64_t, int, uint32_t, uint64_t>> seen_edges;
	std::set<uint64_t> child_entities_to_inspect;
	for (auto& lnk : discovered_links) {
		uint64_t chi_hi = (lnk.child_id >> 48) & 0xFFFF;
		if (chi_hi != 0x0580 && chi_hi != 0x0400 && chi_hi != 0x09A0 && chi_hi != 0x0330) continue;
		auto key = std::make_tuple(lnk.parent_id, lnk.node_idx, lnk.field_hash, lnk.child_id);
		if (!seen_edges.insert(key).second) continue;

		std::string htag = res_owner_hero_map[lnk.parent_id];
		if (htag.empty()) htag = res_owner_hero_map[lnk.child_id];
		std::string ctype_str = (chi_hi == 0x0580) ? "STUStatescriptGraph"
			: (chi_hi == 0x0400) ? "STUEntityDefinition"
			: (chi_hi == 0x09A0) ? "ProjectileConfig (0x09A0)" : "GameEffect (0x0330)";
		out << std::format("| `0x{:016X}` | {} | `{}` | `0x{:08X}` | `0x{:08X}` | `0x{:016X}` | {} |\n",
			lnk.parent_id, htag.empty() ? "-" : htag, lnk.node_idx, lnk.owner_stu_hash, lnk.field_hash, lnk.child_id, ctype_str);

		if (chi_hi == 0x0400 && ((lnk.parent_id >> 48) & 0xFFFF) != 0x02E0) {
			child_entities_to_inspect.insert(lnk.child_id);
		}
		if (chi_hi == 0x09A0 || chi_hi == 0x0330) {
			child_entities_to_inspect.insert(lnk.child_id);
		}
	}
	out << "\n";

	out << "### 4.1 Spawned Projectile / Deployable / Effect STU Resources (`0x0400...`, `0x09A0...`, `0x0330...`) Deep Dump\n\n";
	for (uint64_t cid : child_entities_to_inspect) {
		STUBase<>* cres = stu_resources::GetByID((__int64)cid);
		if (!cres || !cres->valid()) continue;
		STUInfo* cinfo = safe_get_stu_info(cres);
		if (!cinfo) continue;
		std::string htag = res_owner_hero_map[cid];
		out << std::format("#### Resource `0x{:016X}` ({}) — STU `0x{:08X}` (`{}`), RTTI `{}`\n",
			cid, htag.empty() ? "Shared" : htag, cinfo->name_hash,
			allmighty_hash_lib::hashes.count(cinfo->name_hash) ? allmighty_hash_lib::hashes[cinfo->name_hash] : "Unknown",
			get_rtti_classname(cres));

		uint64_t chi_hi = (cid >> 48) & 0xFFFF;
		if (chi_hi == 0x0400) {
			STUArgumentInfo* cmap_arg = cinfo->argumentByHash(stringHash("m_componentMap"));
			if (cmap_arg) {
				auto* bs_map = (STUBullshitMapFull<void*>*)((uintptr_t)cres + cmap_arg->offset);
				if (is_valid_read_memory(bs_map, sizeof(STUBullshitMapFull<void*>)) &&
					bs_map->list_ptr && is_valid_read_memory(bs_map->list_ptr, sizeof(STUBullshitMapPtr<void*>)) &&
					bs_map->flag == 2) {
					STU_Map comp_map(cmap_arg, bs_map);
					for (auto entry : comp_map) {
						STU_Object comp_obj = entry.second;
						if (!comp_obj.value || !is_valid_read_memory(comp_obj.value, 8)) continue;
						STUInfo* sub_info = safe_get_stu_info(comp_obj.value);
						if (!sub_info) sub_info = comp_obj.struct_info;
						if (!sub_info) continue;
						out << std::format("- **Component `0x{:08X}`** (`{}`):\n",
							sub_info->name_hash, get_rtti_classname(comp_obj.value));
						std::unordered_set<uintptr_t> vis;
						vis.insert((uintptr_t)comp_obj.value);
						dump_stu_object_fields(out, comp_obj.value, sub_info, 2, 4, vis);
						std::vector<std::pair<uint32_t, uint64_t>> child_crefs;
						std::unordered_set<uintptr_t> cvis;
						cvis.insert((uintptr_t)comp_obj.value);
						collect_stu_resource_refs_recursive(comp_obj.value, sub_info, 0, 5, cvis, child_crefs);
						for (auto& ccr : child_crefs) {
							if (((ccr.second >> 48) & 0xFFFF) == 0x0580) {
								dump_graph_stripped_summary(ccr.second, "    ");
							}
						}
					}
				}
			}
		} else {
			std::unordered_set<uintptr_t> vis;
			vis.insert((uintptr_t)cres);
			dump_stu_object_fields(out, cres, cinfo, 1, 4, vis);
		}
		out << "\n";
	}

	out.close();
	_dump_status_msg = std::format("Successfully dumped Horizon 1 STU & 21-Hero Audit to horizon1_stu_audit.log! ({} classes, {} graphs)",
		class_entries.size(), all_graphs.size());
	_experiment_status_msg = _dump_status_msg;
}

void statescript_diag_window::test_enable_0x2f_aim_and_rebind() {
	auto* ea = GameEntityAdmin();
	if (!ea) {
		_experiment_status_msg = "[Test 0x2F] GameEntityAdmin is null!";
		return;
	}
	Entity* local_controller = ea->getLocalEnt();
	if (!local_controller) {
		_experiment_status_msg = "[Test 0x2F] local_controller is null!";
		return;
	}
	auto* pet = local_controller->getById<Component_20_ModelReference>(0x20);
	Entity* local_model = nullptr;
	if (pet) {
		if (pet->aim_entid) local_model = ea->getEntById(pet->aim_entid);
		if (!local_model && pet->cam_attach_entid) local_model = ea->getEntById(pet->cam_attach_entid);
		if (!local_model && pet->movement_attach_entid) local_model = ea->getEntById(pet->movement_attach_entid);
	}
	if (!local_model && _ss && _ss->base.entity_backref) {
		local_model = _ss->base.entity_backref;
	}

	// 2. On local_controller
	Component_2F_LocalPlayer* lp = local_controller->getById<Component_2F_LocalPlayer>(0x2F);
	if (lp && is_valid_read_memory(lp, sizeof(Component_2F_LocalPlayer))) {
		lp->is_able_to_aim = 1;
		lp->field_7D1 = 1;

		auto move = (void(__fastcall*)(__int64, int, int, char, char))(globals::gameBase + 0xc27560);
		auto get_sys36 = (__int64(__fastcall*)(EntityAdminBase*))(globals::gameBase + 0xc03750);
		__int64 sys = get_sys36(GameEntityAdmin());
		if (sys) move(sys, 1000, 1000, 1, 1);
	}

	int notified_scripts = 0;
	int notified_states = 0;

	// 3. On local_model
	if (local_model && is_valid_read_memory(local_model, sizeof(Entity))) {
		auto tell_statescript_this_is_localent = (void(__fastcall*)(Entity*, uint*))(globals::gameBase + 0xd07830);
		tell_statescript_this_is_localent(local_model, &local_controller->entity_id);

		if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
			_ss = local_model->getById<Component_23_Statescript>(0x23);
		}

		if (_ss && is_valid_read_memory(_ss, sizeof(Component_23_Statescript))) {
			for (auto script : _ss->ss_inner.g1_instanceArr) {
				if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance))) continue;
				if ((script->execution_flags & 0x200008u) != 0x200008u) {
					script->execution_flags = (script->execution_flags | 0x400009u | 0x200000u) & ~0x1004u;
				}
				if (script->vfptr && is_valid_read_memory(script->vfptr, 0x198) && script->vfptr->RedoCheckIsAimEnt) {
					script->vfptr->RedoCheckIsAimEnt(script);
					notified_scripts++;
				}
				for (int k = 0; k < script->state_impl_list.num; k++) {
					StatescriptState* s = script->state_impl_list.ptr[k];
					if (!s || !is_valid_read_memory(s, sizeof(void*))) continue;
					if (s->vfptr && is_valid_read_memory(s->vfptr, 0x1C0) && s->vfptr->OnAimEntityChanged) {
						s->vfptr->OnAimEntityChanged(s, (DWORD*)&local_controller->entity_id);
						notified_states++;
					}
				}
			}
		}
	}

	int is_able = lp ? (int)lp->is_able_to_aim : -1;
	int f7d1 = lp ? (int)lp->field_7D1 : -1;
	int aim_ent = (_ss && is_valid_read_memory(_ss, sizeof(Component_23_Statescript))) ? (int)_ss->ss_inner.is_aim_ent : -1;

	uint32_t inst24_flags = 0;
	uint32_t inst1_flags = 0;
	if (_ss && is_valid_read_memory(_ss, sizeof(Component_23_Statescript))) {
		for (auto script : _ss->ss_inner.g1_instanceArr) {
			if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance))) continue;
			if (script->instance_id == -24 || script->script_id == 0x580000000000016) {
				inst24_flags = script->execution_flags;
			}
			if (script->instance_id == -1 || script->script_id == 0x58000000000001a) {
				inst1_flags = script->execution_flags;
			}
		}
	}

	_experiment_status_msg = std::format("[Test 0x2F] lp->aim={:d}, f7D1={:d}, is_aim_ent={:d}, scripts={:d}, states={:d} | inst-24: 0x{:X}, inst-1: 0x{:X}",
		is_able, f7d1, aim_ent, notified_scripts, notified_states, inst24_flags, inst1_flags);
}

void statescript_diag_window::render_diagnostic_panel() {
	if (ImGui::CollapsingHeader("[Step 1 Diagnostic] Weapon & Input Tracer", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("[Horizon 1] Dump All Stripped STU Nodes & 21 Hero Graphs (horizon1_stu_audit.log)")) {
			dump_horizon1_audit_to_file();
		}
		if (!_dump_status_msg.empty()) {
			ImGui::TextColored(ImVec4(0.3f, 1.f, 0.3f, 1.f), "%s", _dump_status_msg.c_str());
		}
		ImGui::Separator();

		if (ImGui::Button("Select Local Player Model (via 0x20)")) {
			select_local_model_0x23();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Auto-Subscribe Scripts", &_auto_subscribe_scripts);
		if (_ss && !IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
			ImGui::SameLine();
			if (ImGui::Button("Open Entity in Statescript List")) {
				statescript_list::get_latest_or_create(this)->display_compo(_ss);
			}
		}

		if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
			ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "No Model Component 0x23 selected. Click button above or spawn a hero first.");
			return;
		}

		auto* ea = GameEntityAdmin();
		uint local_ctrl_id = ea ? ea->local_entid : 0;
		uint model_id = _ss->base.entity_backref ? _ss->base.entity_backref->entity_id : 0;
		ImGui::Text("Target: Controller = 0x%x, Model = 0x%x (Scripts: %d)", local_ctrl_id, model_id, _ss->ss_inner.g1_instanceArr.num);
		ImGui::Text("SyncMgr: backref=%d, is_aim_ent=%d, scope=%d, logbtn_down=%d",
			_ss->ss_inner.has_player_entbackref, (int)_ss->ss_inner.is_aim_ent,
			_ss->ss_inner.entity_admin_scope, (int)_ss->ss_inner.latest_logbtn_down);

		ImGui::Separator();

		if (ImGui::Button("Snapshot Current Statescript State")) {
			take_snapshot();
		}
		ImGui::SameLine();
		if (ImGui::Button("Dump to statescript_trace.log")) {
			dump_trace_to_file();
		}

		if (!_dump_status_msg.empty()) {
			ImGui::TextColored(ImVec4(0.3f, 1.f, 0.3f, 1.f), "%s", _dump_status_msg.c_str());
		}

		// Compute live diff if snapshot exists
		if (_has_snapshot) {
			int changed_nodes = 0;
			int changed_vars = 0;
			std::vector<std::string> changes;

			for (auto script : _ss->ss_inner.g1_instanceArr) {
				if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
				auto script_state = statescript_logger::GetScriptState(script);
				for (auto& pair : script_state) {
					auto key = std::make_pair(script->instance_id, pair.first);
					auto it = _snapshot_node_timestamps.find(key);
					if (it != _snapshot_node_timestamps.end() && it->second != pair.second.change_timestamp) {
						changed_nodes++;
						if (changes.size() < 10) {
							changes.push_back(std::format("Node [Inst {:d}: {:d}] ts {:d}->{:d}",
								script->instance_id, pair.first, it->second, pair.second.change_timestamp));
						}
					}
				}
				if (script->rid_instance_varbag) {
					std::map<std::pair<short, __int64>, std::string> cur_vars;
					capture_varbag_values(script->rid_instance_varbag, script->instance_id, cur_vars);
					for (auto& v : cur_vars) {
						auto it = _snapshot_var_values.find(v.first);
						if (it != _snapshot_var_values.end() && it->second != v.second) {
							changed_vars++;
							if (changes.size() < 10) {
								changes.push_back(std::format("Var [Inst {:d}: {:x}] '{}'->'{}'",
									script->instance_id, v.first.second, it->second, v.second));
							}
						}
					}
				}
			}

			if (changed_nodes == 0 && changed_vars == 0) {
				ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "DIFF: NO ACTIVITY (0 nodes, 0 vars changed)");
				ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "-> If Left Click held, input is NOT reaching 0x23 (Gate A blocker)!");
			}
			else {
				ImGui::TextColored(ImVec4(0.2f, 1.f, 0.2f, 1.f), "DIFF: %d nodes, %d vars changed since snapshot!", changed_nodes, changed_vars);
				for (auto& c : changes) {
					ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "  * %s", c.c_str());
				}
			}
		}

		// Rolling Log Viewer
		ImGui::Separator();
		ImGui::Text("Recent Notifications (from statescript_logger):");
		if (ImGui::BeginChild("notif_log", ImVec2(0, 100), true)) {
			int displayed = 0;
			for (auto script : _ss->ss_inner.g1_instanceArr) {
				if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
				auto& logs = statescript_logger::GetScriptLogs(script);
				for (auto it = logs.logs.rbegin(); it != logs.logs.rend() && displayed < 20; ++it, ++displayed) {
					ImGui::TextUnformatted(it->c_str());
				}
			}
			if (displayed == 0) {
				ImGui::TextDisabled("No notifications logged yet.");
			}
		}
		ImGui::EndChild();

		// Phase 4 Gate B & C Experiments
		ImGui::Separator();
		if (ImGui::CollapsingHeader("[Phase 4 Experiments] Unblock Firing Gates (Gate B & C)", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Current State: ontick=%d, clientonly=%d, scope=%d, aim_ent=%d",
				_ss->ss_inner.ontick_updates_enabled, (int)_ss->ss_inner.has_clientonly_scripts, _ss->ss_inner.entity_admin_scope, (int)_ss->ss_inner.is_aim_ent);

			// Gate B: Call init_ss_fn (+0x103acb0)
			if (ImGui::Button("[Gate B] Call init_ss_fn (+0x103acb0)")) {
				call_init_ss();
			}
			ImGui::SameLine();
			// Gate B: ontick_updates_enabled toggle
			bool ontick = _ss->ss_inner.ontick_updates_enabled != 0;
			if (ImGui::Checkbox("[Gate B] ontick_updates_enabled (+0x368)", &ontick)) {
				toggle_ontick_updates();
			}

			// Gate B: Force Server Authority (clear m_serverOnly)
			if (ImGui::Button("[Gate B] Force Server Authority (Nodes m_serverOnly=0 & ontick=1)")) {
				force_server_authority_weapon_nodes();
			}
			ImGui::SameLine();
			if (ImGui::Button("[Gate B] Scan Plugs (ServerOnly vs ClientOnly)")) {
				patch_server_only_plugs();
			}

			// Gate C: Fixup Health & Weapon VarBags
			if (ImGui::Button("[Gate C] Fixup Weapon VarBags (Set Health 150/150, Weapon Array [0]=1)")) {
				fixup_weapon_varbags();
			}

			ImGui::Separator();
			// Deep Graph Dump
			if (ImGui::Button("Dump Weapon Graph & VarBags to weapon_graph_trace.log")) {
				dump_weapon_graph_to_file();
			}

			// Forensics Dump
			if (ImGui::Button("[Step 1 Forensics] Dump Weapon Nodes & Entity State -> weapon_nodes_forensics.log")) {
				dump_weapon_nodes_forensics_to_file();
			}

			if (ImGui::Button("[Horizon 1] Dump All Stripped STU Nodes & 21 Hero Graphs (horizon1_stu_audit.log)##exp")) {
				dump_horizon1_audit_to_file();
			}

			ImGui::Checkbox("[Gate B+] Force Authoritative Execution Flags (0x608009) on Model Scripts", &s_force_weapon_exec_flags);

			if (ImGui::Button("[Step 1 Test] Enable 0x2F Aim + Re-Notify Weapon Scripts")) {
				test_enable_0x2f_aim_and_rebind();
			}


			extern bool s_enable_ontick_varbag_spam;
			ImGui::Checkbox("Legacy OnTick VarBag Override (entity_admin.cpp:692)", &s_enable_ontick_varbag_spam);

			if (!_experiment_status_msg.empty()) {
				ImGui::TextColored(ImVec4(0.3f, 1.f, 0.8f, 1.f), "%s", _experiment_status_msg.c_str());
			}
		}
	}
}

void statescript_diag_window::render() {
	if (s_force_weapon_exec_flags && _ss && is_valid_read_memory(_ss, sizeof(Component_23_Statescript))) {
		for (auto* script : _ss->ss_inner.g1_instanceArr) {
			if (!script || !is_valid_read_memory(script, sizeof(StatescriptInstance))) continue;
			if ((script->execution_flags & 0x200008u) != 0x200008u) {
				script->execution_flags = (script->execution_flags | 0x400009u | 0x200000u) & ~0x1004u;
				if (script->vfptr && is_valid_read_memory(script->vfptr, 0x198) && script->vfptr->RedoCheckIsAimEnt) {
					script->vfptr->RedoCheckIsAimEnt(script);
				}
			}
		}
	}

	if (open_window(nullptr, 0, ImVec2(620, 680))) {
		render_diagnostic_panel();
	}
	ImGui::End();
}

void statescript_list::render() {
	if (!_ss || IsBadReadPtr(_ss, sizeof(Component_23_Statescript))) {
		queue_deletion();
		return;
	}
	char buf[64];
	sprintf_s(buf, "Entity %x (%d)##ss_list", _ss->base.entity_backref->entity_id, _ss->ss_inner.g1_instanceArr.num);
	if (open_window(buf, ImGuiWindowFlags_MenuBar, ImVec2(300, 600))) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Tools")) {
				if (ImGui::MenuItem("Search for Nodes")) {
					statescript_node_search::get_latest_or_create(this)->set(_ss);
				}
				if (ImGui::MenuItem("Fake LogicalButton")) {
					auto size = ImGui::GetWindowSize();
					size.x /= 0.7f;
					ImGui::SetWindowSize(size);
					dock_item_right(window_manager::get_latest_or_create<statescript_fake_logicalbtn>(this, true, true).get(), 0.7);
				}
				if (ImGui::MenuItem("ConfigVarDynamic Editor")) {
					CVD_VarbagSelector::get_latest_or_create(this)->set(&_ss->ss_inner);
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if (ImGui::BeginListBox("", ImVec2(-10, -30))) {
			for (auto script : _ss->ss_inner.g1_instanceArr) {
				if (!script->parent_instance_id) {
					list_script(script);
				}
			}
			ImGui::EndListBox();

			imgui_helpers::InputHex("", &_load_script);
			ImGui::SameLine();
			if (ImGui::Button("Load Script")) {
				_ss->vfptr->load_statescript_script(_ss, _load_script, 0, 0);
			}
		}
	}
	ImGui::End();
}

void statescript_list::display_compo(Component_23_Statescript* ss) {
	_ss = ss;
	_curr_inst_id = 0;
}

void statescript_list::list_script(StatescriptInstance* script) {
	ImGui::PushID(script);
	if (ImGui::Button(EMOJI_CROSS)) {
		script->vfptr->StopScript(script);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(std::format("({:d}) {:x}", script->instance_id, script->script_id).c_str(), _curr_inst_id == script->instance_id)) {
		_curr_inst_id = script->instance_id;
		statescript_window::get_latest_or_create(this)->display_instance(script);
	}
	imgui_helpers::display_type(script->script_id, true, true, false);
	if (!script->graph) {
		ImGui::Text("No Graph!");
	}
	ImGui::Separator();
	for (auto& other_script : _ss->ss_inner.g1_instanceArr) {
		if (other_script != script && other_script->parent_instance_id == script->instance_id) {
			ImGui::Indent();
			list_script(other_script);
			ImGui::Unindent();
		}
	}
	ImGui::PopID();
}
