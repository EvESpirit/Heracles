#pragma once
#include "Statescript.h"
#include <MinHook.h>
#include <mutex>
#include <functional>
#include "stringhash_library.h"

class statescript_logger {
public:
    struct LogStruct {
        std::vector<std::string> logs;
        std::vector<StatescriptNotification_Base*> e_log;
    };

    struct ScriptState {
        __int64 change_timestamp = 0;
        bool is_finish_state = true;
    };

    static void Initialize() {
        MH_CreateHook((PVOID)(globals::gameBase + 0xf4f600), TickScript, (PVOID*)&TickScript_orig);
        MH_EnableHook((PVOID)(globals::gameBase + 0xf4f600));

        MH_CreateHook((PVOID)(globals::gameBase + 0xd146a0), OnNodeStateEntry, (PVOID*)&StateEntry_orig);
        MH_EnableHook((PVOID)(globals::gameBase + 0xd146a0));

        MH_CreateHook((PVOID)(globals::gameBase + 0xd14850), OnNodeStateFinished, (PVOID*)&StateFinish_orig);
        MH_EnableHook((PVOID)(globals::gameBase + 0xd14850));

        MH_CreateHook((PVOID)(globals::gameBase + 0xd14a20), OnNodeActionEntry, (PVOID*)&ActionEntry_orig);
        MH_EnableHook((PVOID)(globals::gameBase + 0xd14a20));

        MH_CreateHook((PVOID)(globals::gameBase + 0xf47800), OnScriptEntry, (PVOID*)&ScriptEntry_orig);
        MH_EnableHook((PVOID)(globals::gameBase + 0xf47800));

        MH_CreateHook((PVOID)(globals::gameBase + 0xd73a80), Deallocate, (PVOID*)&Deallocate_orig);
        MH_EnableHook((PVOID)(globals::gameBase + 0xd73a80));
    }

    static const LogStruct& GetScriptLogs(StatescriptInstance* ss) {
        if (s_subscriptions.find(ss) != s_subscriptions.end())
            return s_subscriptions[ss];
        return EmptyLog;
    }
    static const std::map<int, ScriptState>& GetScriptState(StatescriptInstance* ss) {
        if (s_subscriptions_or_states_valid(ss) && s_scriptStates.find(ss) != s_scriptStates.end())
            return s_scriptStates[ss];
        return EmptyScriptState;
    }
    static const ScriptState& GetScriptNodeState(StatescriptInstance* ss, int m_nodex_idx) {
        if (s_scriptStates.find(ss) != s_scriptStates.end())
            return s_scriptStates[ss][m_nodex_idx];
        return EmptyNodeState;
    }

    static void Subscribe(StatescriptInstance* ss) {
        s_subscriptions.emplace(ss, LogStruct{});
    }

    static void Unsubscribe(StatescriptInstance* ss) {
        s_subscriptions.erase(ss);
    }

    static void SubscribeDeallocation(std::weak_ptr<std::function<void(StatescriptInstance*)>> func) {
        s_deletionSubs.push_back(func);
    }

    // Compact trace: prints node entries / logical button notifications and records them
    // into s_trace_history for inclusion in dummy_combat_diag.log.
    static constexpr int kTraceMaxLines = 250;
    static inline ULONGLONG s_trace_until = 0;
    static inline int s_trace_lines = 0;
    static inline std::vector<std::string> s_trace_history{};
    static bool TraceActive() {
        return s_trace_until != 0 && s_trace_lines < kTraceMaxLines && GetTickCount64() < s_trace_until;
    }
    static void StartTrace(ULONGLONG ms, const char* reason = "H_KEY") {
        s_trace_until = GetTickCount64() + ms;
        s_trace_lines = 0;
        char hdr[160];
        snprintf(hdr, sizeof(hdr), "[SSTrace] ==== Trace Start (%s, %llums) ====", reason ? reason : "Manual", ms);
        printf("%s\n", hdr);
        if (s_trace_history.size() < 600) s_trace_history.emplace_back(hdr);
    }
    static const char* TraceNodeName(STUStatescriptBase* stu, char* buf, size_t sz) {
        if (!stu || IsBadReadPtr(stu, sizeof(STUStatescriptBase)) || !stu->graph_node.base.vfptr || IsBadReadPtr(stu->graph_node.base.vfptr, 8)) return "?";
        auto info = stu->graph_node.base.vfptr->GetSTUInfo();
        if (!info || IsBadReadPtr(info, sizeof(STUInfo))) return "?";
        auto it = allmighty_hash_lib::hashes.find(info->name_hash);
        if (it != allmighty_hash_lib::hashes.end()) return it->second.c_str();
        snprintf(buf, sz, "0x%08x", (uint32)info->name_hash);
        return buf;
    }

    static inline __int64(__fastcall* TickScript_orig)(StatescriptInstance*, StatescriptNotification_Base*);
    static __int64 __fastcall TickScript(StatescriptInstance* ss, StatescriptNotification_Base* ss_e) {
        if (ss_e && ss_e->type != ETYPE_TIMER && ss_e->type != ETYPE_LIFETIME && TraceActive()) {
            char tbuf[128]{};
            ss_e->vfptr->ToString(ss_e, tbuf, 128);
            s_trace_lines++;
            char lbuf[256];
            snprintf(lbuf, sizeof(lbuf), "[SSTrace] NOTIFY t=%d script=0x%llx inst=%hd exec=0x%X : %s",
                (int)ss_e->type, (unsigned long long)ss->script_id, ss->instance_id, ss->execution_flags, tbuf);
            printf("%s\n", lbuf);
            if (s_trace_history.size() < 600) s_trace_history.emplace_back(lbuf);
        }
        //printf("Tick [%hd %p] ", ss->instance_id, ss->script_id);
        if (s_subscriptions.find(ss) != s_subscriptions.end()) {
            char buf[128];
            ss_e->vfptr->ToString(ss_e, buf, 128);
            StatescriptNotification_Base* cloned = nullptr;
            switch (ss_e->type) {
            case ETYPE_LIFETIME:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_Lifetime;
                break;
            case ETYPE_TIMER:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_Timer;
                break;
            case ETYPE_EVALEXITS:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_EvaluateExits;
                break;
            case ETYPE_TRAVERSEFROM:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_TraverseFrom;
                break;
            case ETYPE_SETVAR:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_SetVar;
                break;
            case ETYPE_SETVARARRAY:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_SetVarArray;
                break;
            case ETYPE_STOPINST:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_Base;
                break;
            case ETYPE_LOGICALBUTTON:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_LogicalButton;
                break;
            case ETYPE_ONATTRDEPCHANGE:
                cloned = (StatescriptNotification_Base*)new StatescriptNotification_AttrDependencyChanged;
                break;
            default:
                printf("Failed to clone statescript type %d.\n");
                break;
            }
            auto& sub = s_subscriptions[ss];
            sub.logs.push_back(buf);
            if (cloned) {
                cloned->vfptr = ss_e->vfptr;
                sub.e_log.push_back(cloned);
            }
            if (sub.logs.size() > 1200) {
                sub.logs.erase(sub.logs.begin(), sub.logs.begin() + 200);
            }
            if (sub.e_log.size() > 1200) {
                for (size_t i = 0; i < 200; ++i) {
                    delete sub.e_log[i];
                }
                sub.e_log.erase(sub.e_log.begin(), sub.e_log.begin() + 200);
            }
        }
        char result = TickScript_orig(ss, ss_e);
        return result;
    }
private:
    static bool s_subscriptions_or_states_valid(StatescriptInstance* ss) {
        return ss != nullptr;
    }
    static inline void(__fastcall* StateEntry_orig)(StatescriptSyncMgr*, StatescriptInstance*, StatescriptState*);
    static inline void(__fastcall* StateFinish_orig)(StatescriptSyncMgr*, StatescriptInstance*, StatescriptState*);
    static inline void(__fastcall* ActionEntry_orig)(StatescriptSyncMgr*, StatescriptInstance*, StatescriptState*, int);
    static inline char(__fastcall* ScriptEntry_orig)(__int64, StatescriptInstance*, STUStatescriptBase*);
    static inline void(__fastcall* Deallocate_orig)(StatescriptInstance*, char);
public:
    static inline void(*s_universal_state_entry_cb)(StatescriptSyncMgr*, StatescriptInstance*, StatescriptState*) = nullptr;
    static inline void(*s_universal_state_exit_cb)(StatescriptSyncMgr*, StatescriptInstance*, StatescriptState*) = nullptr;
private:

    static void __fastcall OnNodeStateEntry(StatescriptSyncMgr* inner, StatescriptInstance* ss, StatescriptState* istate) {
        if (!ss->graph) {
            printf("Invalid state update from %p/%p/%p\n", inner, ss, istate);
            return;
        }
        auto& state = s_scriptStates[ss][ss->graph->STUFromState(istate)->ss_base.idx_in_nodes];
        state.change_timestamp = inner->cf_timestamp;
        state.is_finish_state = false;
        if (TraceActive()) {
            auto stu_state = ss->graph->STUFromState(istate);
            uintptr_t vt = (istate && !IsBadReadPtr(istate, 8)) ? *(uintptr_t*)istate : 0;
            uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + 0x3000000) ? (vt - globals::gameBase) : vt;
            uint32_t stu_hash = (stu_state && stu_state->ss_base.graph_node.base.vfptr && stu_state->ss_base.graph_node.base.vfptr->GetSTUInfo())
                ? stu_state->ss_base.graph_node.base.vfptr->GetSTUInfo()->name_hash : 0;
            char nbuf[32]{};
            s_trace_lines++;
            char lbuf[256];
            snprintf(lbuf, sizeof(lbuf), "[SSTrace] STATE+ script=0x%llx inst=%hd exec=0x%X node=%d(m_st=%d) %s(0x%08X,cO=%d,sO=%d) vt=+0x%llX flg=0x%X",
                (unsigned long long)ss->script_id, ss->instance_id, ss->execution_flags,
                stu_state ? stu_state->ss_base.idx_in_nodes : -1, istate ? istate->idx_in_m_states : -1,
                stu_state ? TraceNodeName(&stu_state->ss_base, nbuf, sizeof(nbuf)) : "?", stu_hash,
                stu_state ? (int)stu_state->ss_base.m_clientOnly : -1, stu_state ? (int)stu_state->ss_base.m_serverOnly : -1,
                (unsigned long long)vt_rva, istate ? (uint32_t)istate->state_flags : 0);
            printf("%s\n", lbuf);
            if (s_trace_history.size() < 600) s_trace_history.emplace_back(lbuf);
        }
        if (s_universal_state_entry_cb) {
            s_universal_state_entry_cb(inner, ss, istate);
        }
        StateEntry_orig(inner, ss, istate);
    }
    static void __fastcall OnNodeStateFinished(StatescriptSyncMgr* inner, StatescriptInstance* ss, StatescriptState* istate) {
        if (!ss->graph) {
            printf("Invalid state update from %p/%p/%p\n", inner, ss, istate);
            return;
        }
        auto& state = s_scriptStates[ss][ss->graph->STUFromState(istate)->ss_base.idx_in_nodes];
        state.change_timestamp = inner->cf_timestamp;
        state.is_finish_state = true;
        if (TraceActive()) {
            auto stu_state = ss->graph->STUFromState(istate);
            uintptr_t vt = (istate && !IsBadReadPtr(istate, 8)) ? *(uintptr_t*)istate : 0;
            uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + 0x3000000) ? (vt - globals::gameBase) : vt;
            uint32_t stu_hash = (stu_state && stu_state->ss_base.graph_node.base.vfptr && stu_state->ss_base.graph_node.base.vfptr->GetSTUInfo())
                ? stu_state->ss_base.graph_node.base.vfptr->GetSTUInfo()->name_hash : 0;
            char nbuf[32]{};
            s_trace_lines++;
            char lbuf[256];
            snprintf(lbuf, sizeof(lbuf), "[SSTrace] STATE- script=0x%llx inst=%hd exec=0x%X node=%d(m_st=%d) %s(0x%08X,cO=%d,sO=%d) vt=+0x%llX flg=0x%X",
                (unsigned long long)ss->script_id, ss->instance_id, ss->execution_flags,
                stu_state ? stu_state->ss_base.idx_in_nodes : -1, istate ? istate->idx_in_m_states : -1,
                stu_state ? TraceNodeName(&stu_state->ss_base, nbuf, sizeof(nbuf)) : "?", stu_hash,
                stu_state ? (int)stu_state->ss_base.m_clientOnly : -1, stu_state ? (int)stu_state->ss_base.m_serverOnly : -1,
                (unsigned long long)vt_rva, istate ? (uint32_t)istate->state_flags : 0);
            printf("%s\n", lbuf);
            if (s_trace_history.size() < 600) s_trace_history.emplace_back(lbuf);
        }
        if (s_universal_state_exit_cb) {
            s_universal_state_exit_cb(inner, ss, istate);
        }
        StateFinish_orig(inner, ss, istate);
    }
    static void __fastcall OnNodeActionEntry(StatescriptSyncMgr* inner, StatescriptInstance* ss, StatescriptState* istate, int idx_in_m_nodes) {
        auto& state = s_scriptStates[ss][idx_in_m_nodes];
        state.change_timestamp = inner->cf_timestamp;
        state.is_finish_state = true;
        if (TraceActive() && ss->graph) {
            STUStatescriptBase* node = (idx_in_m_nodes >= 0 && idx_in_m_nodes < (int)ss->graph->m_nodes.count()) ? ss->graph->m_nodes.list()[idx_in_m_nodes] : nullptr;
            StatescriptAction* act = (ss->action_impl_list.ptr && idx_in_m_nodes >= 0 && idx_in_m_nodes < ss->action_impl_list.num)
                ? ss->action_impl_list.ptr[idx_in_m_nodes] : nullptr;
            uintptr_t avt = (act && !IsBadReadPtr(act, 8)) ? *(uintptr_t*)act : 0;
            uintptr_t avt_rva = (avt >= globals::gameBase && avt < globals::gameBase + 0x3000000) ? (avt - globals::gameBase) : avt;
            uintptr_t fnF8 = (avt && !IsBadReadPtr((const void*)(avt + 0xF8), 8)) ? *(uintptr_t*)(avt + 0xF8) : 0;
            uintptr_t fnF8_rva = (fnF8 >= globals::gameBase && fnF8 < globals::gameBase + 0x3000000) ? (fnF8 - globals::gameBase) : fnF8;
            uintptr_t fn100 = (avt && !IsBadReadPtr((const void*)(avt + 0x100), 8)) ? *(uintptr_t*)(avt + 0x100) : 0;
            uintptr_t fn100_rva = (fn100 >= globals::gameBase && fn100 < globals::gameBase + 0x3000000) ? (fn100 - globals::gameBase) : fn100;
            uint32_t stu_hash = (node && node->graph_node.base.vfptr && node->graph_node.base.vfptr->GetSTUInfo())
                ? node->graph_node.base.vfptr->GetSTUInfo()->name_hash : 0;
            char nbuf[32]{};
            s_trace_lines++;
            char lbuf[256];
            snprintf(lbuf, sizeof(lbuf), "[SSTrace] ACTION script=0x%llx inst=%hd exec=0x%X node=%d %s(0x%08X,cO=%d,sO=%d) act_vt=+0x%llX(fnF8=+0x%llX,fn100=+0x%llX)",
                (unsigned long long)ss->script_id, ss->instance_id, ss->execution_flags, idx_in_m_nodes,
                TraceNodeName(node, nbuf, sizeof(nbuf)), stu_hash,
                node ? (int)node->m_clientOnly : -1, node ? (int)node->m_serverOnly : -1,
                (unsigned long long)avt_rva, (unsigned long long)fnF8_rva, (unsigned long long)fn100_rva);
            printf("%s\n", lbuf);
            if (s_trace_history.size() < 600) s_trace_history.emplace_back(lbuf);
        }
        ActionEntry_orig(inner, ss, istate, idx_in_m_nodes);
    }
    static char __fastcall OnScriptEntry(__int64 a1, StatescriptInstance* ss, STUStatescriptBase* stu) {
        if (TraceActive() && ss && stu) {
            static StatescriptInstance* s_last_ss = nullptr;
            static int s_last_idx = -1;
            if (ss != s_last_ss || stu->idx_in_nodes != s_last_idx) {
                s_last_ss = ss;
                s_last_idx = stu->idx_in_nodes;
                uint32_t stu_hash = (stu->graph_node.base.vfptr && stu->graph_node.base.vfptr->GetSTUInfo())
                    ? stu->graph_node.base.vfptr->GetSTUInfo()->name_hash : 0;
                char nbuf[32]{};
                s_trace_lines++;
                char lbuf[256];
                snprintf(lbuf, sizeof(lbuf), "[SSTrace] ENTRY script=0x%llx inst=%hd exec=0x%X node=%d %s(0x%08X,cO=%d,sO=%d)",
                    (unsigned long long)ss->script_id, ss->instance_id, ss->execution_flags, stu->idx_in_nodes,
                    TraceNodeName(stu, nbuf, sizeof(nbuf)), stu_hash,
                    (int)stu->m_clientOnly, (int)stu->m_serverOnly);
                printf("%s\n", lbuf);
                if (s_trace_history.size() < 600) s_trace_history.emplace_back(lbuf);
            }
        }
        auto& state = s_scriptStates[ss][stu->idx_in_nodes];
        state.change_timestamp = ss->ss_inner->cf_timestamp;
        state.is_finish_state = true;
        return ScriptEntry_orig(a1, ss, stu);
    }
    static void __fastcall Deallocate(StatescriptInstance* script, char a2) {
        s_subscriptions.erase(script);
        s_scriptStates.erase(script);
        for (auto it = s_deletionSubs.begin(); it != s_deletionSubs.end(); ) {
            auto func = it->lock();
            if (func) {
                (*func)(script);
                it++;
            }
            else {
                s_deletionSubs.erase(it);
            }
        }
        Deallocate_orig(script, a2);
    }

    static inline std::map<StatescriptInstance*, LogStruct> s_subscriptions{};
    static inline std::vector<std::weak_ptr<std::function<void(StatescriptInstance*)>>> s_deletionSubs{};
    static inline std::map<StatescriptInstance*, std::map<int, ScriptState>> s_scriptStates{};
    static inline const LogStruct const EmptyLog{};
    static inline const std::map<int, ScriptState> const EmptyScriptState{};
    static inline const ScriptState const EmptyNodeState{};
};
