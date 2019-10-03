#include <switch_min.h>

#include <stdint.h>

#include "useful/const_value_table.h"
#include "useful/crc32.h"
#include "useful/raygun_printer.h"
#include "useful/useful.h"

#include "saltysd/nn_ro.h"

#include "acmd_wrapper.h"
#include "acmd_edits.h"

using namespace lib;
using namespace app::sv_animcmd;
using namespace app::lua_bind;

L2CAgent* fighter_agents[8];
u64 fighter_module_accessors[8];

u64 prev_get_command_flag_cat = 0;
u64 prev_clear_lua_stack = 0;

int get_command_flag_cat_replace(u64 module_accessor, int category) {
    int (*prev_replace)(u64, int) = (int (*)(u64, int)) prev_get_command_flag_cat;
    if (prev_replace)
        prev_replace(module_accessor, category);

    u64 control_module = load_module(module_accessor, 0x48);
    int (*get_command_flag_cat)(u64, int) = (int (*)(u64, int)) load_module_impl(control_module, 0x350);
    int flag = get_command_flag_cat(control_module, category);

    if (category == 0) {
        int fighter_entry = WorkModule::get_int(module_accessor, FIGHTER_INSTANCE_WORK_ID_INT_ENTRY_ID);
        L2CAgent* l2c_agent = fighter_agents[fighter_entry];
        if (fighter_module_accessors[fighter_entry] == module_accessor && l2c_agent && l2c_agent->lua_state_agent 
            && app::sv_system::battle_object_module_accessor(l2c_agent->lua_state_agent) == module_accessor) {
            for (ACMD& acmd_obj : acmd_objs)
                acmd_obj.run(l2c_agent);
        }
    }

    return flag;
}

void replace_scripts(L2CAgent* l2c_agent, u8 category, int kind) {
    // fighter
    if (category == BATTLE_OBJECT_CATEGORY_FIGHTER) {
        u64 module_accessor = app::sv_system::battle_object_module_accessor(l2c_agent->lua_state_agent);
        int fighter_entry = WorkModule::get_int(module_accessor, FIGHTER_INSTANCE_WORK_ID_INT_ENTRY_ID);
        fighter_module_accessors[fighter_entry] = module_accessor;
        fighter_agents[fighter_entry] = l2c_agent;

        for (ACMD& acmd_obj : acmd_objs)
            acmd_obj.nullify_original(l2c_agent);
    }
}

void* sv_get_status_func(u64 l2c_agentbase, int status_kind, u64 key) {
    u64 unk48 = LOAD64(l2c_agentbase + 0x48);
    u64 unk50 = LOAD64(l2c_agentbase + 0x50);
    if (0x2E8BA2E8BA2E8BA3LL * ((unk50 - unk48) >> 4) > (u64)status_kind)
        return *(void**)(unk48 + 0xB0LL * status_kind + (key << 32 >> 29));

    return 0;
}

void sv_replace_status_func(u64 l2c_agentbase, int status_kind, u64 key, void* func) {
    u64 unk48 = LOAD64(l2c_agentbase + 0x48);
    u64 unk50 = LOAD64(l2c_agentbase + 0x50);
    if (0x2E8BA2E8BA2E8BA3LL * ((unk50 - unk48) >> 4) > (u64)status_kind) {
        *(void**)(unk48 + 0xB0LL * status_kind + (key << 32 >> 29)) = func;
    }
}

u64 clear_lua_stack_replace(L2CAgent* l2c_agent) {
    u64 (*prev_replace)(L2CAgent*) = (u64 (*)(L2CAgent*)) prev_clear_lua_stack;
    if (prev_replace)
        prev_replace(l2c_agent);

    u64 lua_state = l2c_agent->lua_state_agent;
    if ((lua_state - 8) && 
        LOAD64(lua_state - 8) &&
        (LOAD64(LOAD64(lua_state - 8) + 416LL))) {
        u8 battle_object_category = *(u8*)(LOAD64(lua_state - 8) + 404LL);
        int battle_object_kind = *(int*)(LOAD64(lua_state - 8) + 408LL);
        replace_scripts(l2c_agent, battle_object_category, battle_object_kind);
    }

    return l2c_agent->_clear_lua_stack();
}

void script_replacement() {
    SaltySD_function_replace_sym_check_prev(
        "_ZN3app8lua_bind40ControlModule__get_command_flag_cat_implEPNS_26BattleObjectModuleAccessorEi",
        (u64)&get_command_flag_cat_replace,
        prev_get_command_flag_cat);
    SaltySD_function_replace_sym_check_prev(
        "_ZN3lib8L2CAgent15clear_lua_stackEv", 
        (u64)&clear_lua_stack_replace,
        prev_clear_lua_stack);
}
