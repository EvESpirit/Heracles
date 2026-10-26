#pragma once
#include "../window_manager/window_manager.h"
#include "../entity_admin.h"

#include <map>
#include <string>
#include <vector>

class statescript_list : public window {
	WINDOW_DEFINE(statescript_list, "Statescript", "Statescript List", false);

	void render() override;
	void display_compo(Component_23_Statescript* ss);

	Component_23_Statescript* _ss = nullptr;
private:
	int _curr_inst_id = 0;
	__int64 _load_script = 0;

	void list_script(StatescriptInstance* script);
};

WINDOW_REGISTER(statescript_list);

class statescript_diag_window : public window {
	WINDOW_DEFINE(statescript_diag_window, "Diagnostics", "Statescript & Input Tracer", true);

	void render() override;

	void select_local_model_0x23();
	void subscribe_all_instances();
	void take_snapshot();
	void dump_trace_to_file();
	void dump_weapon_graph_to_file();
	void dump_weapon_nodes_forensics_to_file();
	void dump_horizon1_audit_to_file();

	// Gate B & C Test Functions
	void call_init_ss();
	void toggle_ontick_updates();
	void force_server_authority_weapon_nodes();
	void fixup_weapon_varbags();
	void patch_server_only_plugs();

	// Phase 3D Functions
	void test_enable_0x2f_aim_and_rebind();

	Component_23_Statescript* _ss = nullptr;
private:
	bool _has_snapshot = false;
	bool _auto_subscribe_scripts = true;
	bool _force_server_authority = false;
	std::string _dump_status_msg = "";
	std::string _experiment_status_msg = "";
	std::map<std::pair<short, int>, __int64> _snapshot_node_timestamps;
	std::map<std::pair<short, __int64>, std::string> _snapshot_var_values;

	void render_diagnostic_panel();
};

WINDOW_REGISTER(statescript_diag_window);
