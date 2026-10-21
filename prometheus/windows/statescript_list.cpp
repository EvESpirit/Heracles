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
