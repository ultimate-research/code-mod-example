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

int get_command_flag_cat_replace(u64 module_accessor, int category) {
    u64 control_module = load_module(module_accessor, 0x48);
    int (*get_command_flag_cat)(u64, int) = (int (*)(u64, int)) load_module_impl(control_module, 0x350);
    int flag = get_command_flag_cat(control_module, category);

    int fighter_entry = WorkModule::get_int(module_accessor, FIGHTER_INSTANCE_WORK_ID_INT_ENTRY_ID);
    L2CAgent* l2c_agent = fighter_agents[fighter_entry];

    if (category == 0 && fighter_module_accessors[fighter_entry] == module_accessor && l2c_agent && l2c_agent->lua_state_agent && app::sv_system::battle_object_module_accessor(l2c_agent->lua_state_agent) == module_accessor) {
        for (ACMD acmd_obj : acmd_objs)
            acmd_obj.run(l2c_agent);
    }

    return flag;
}

void sv_replace_status_func(u64 l2c_agentbase, int status_kind, u64 key, void* func);

u64 end_shieldbreakfly_replace(u64 l2c_fighter, u64 l2c_agent);

u64 suicide_bomb_acmd_game = 0;

void replace_scripts(L2CAgent* l2c_agent, u8 category, int kind) {
    // fighter
    if (category == BATTLE_OBJECT_CATEGORY_FIGHTER) {
        u64 module_accessor = app::sv_system::battle_object_module_accessor(l2c_agent->lua_state_agent);
        int fighter_entry = WorkModule::get_int(module_accessor, FIGHTER_INSTANCE_WORK_ID_INT_ENTRY_ID);
        fighter_module_accessors[fighter_entry] = module_accessor;
        fighter_agents[fighter_entry] = l2c_agent;

        for (ACMD acmd_obj : acmd_objs)
            acmd_obj.nullify_original(l2c_agent);

        // squirtle
        if (kind == FIGHTER_KIND_PZENIGAME) {
            l2c_agent->sv_set_function_hash(
                (u64(*)(L2CAgent*, void*))suicide_bomb_acmd_game,
                hash40("game_attacks3"));
        }

        // peach
        if (kind == FIGHTER_KIND_PEACH) {
            sv_replace_status_func((u64)l2c_agent,
                                   FIGHTER_STATUS_KIND_SHIELD_BREAK_FLY,
                                   LUA_SCRIPT_STATUS_FUNC_STATUS_END,
                                   (void*)&end_shieldbreakfly_replace);
        }
    }
}

// with l2cvalue* in x8
u64 end_shieldbreakfly_replace(u64 l2c_fighter, u64 l2c_agent) {
    L2CValue* l2c_ret;
    asm("mov %x0, x8" : : "r"(l2c_ret) : "x8");

    u64 lua_state = LOAD64(l2c_fighter + 8);
    u64 module_accessor = LOAD64(LOAD64(lua_state - 8) + 416LL);
    void (*is_enable_passive)(u64) = (void (*)(u64))SaltySDCore_FindSymbol(
        "_ZN7lua2cpp16L2CFighterCommon17is_enable_passiveEv");
    L2CValue passive_enabled;
    asm("mov x8, %x0" : : "r"(&passive_enabled));
    is_enable_passive(l2c_fighter);

    if (passive_enabled.raw)
        print_string(module_accessor, "true");
    else
        print_string(module_accessor, "false");

    l2c_ret->type = L2C_integer;
    l2c_ret->raw = 0;
    return 0;
}

void* sv_get_status_func(u64 l2c_agentbase, int status_kind, u64 key) {
    u64 unk48 = LOAD64(l2c_agentbase + 0x48);
    u64 unk50 = LOAD64(l2c_agentbase + 0x50);
    if (0x2E8BA2E8BA2E8BA3LL * ((unk50 - unk48) >> 4) > (u64)status_kind)
        return *(void**)(unk48 + 0xB0LL * status_kind + (key << 32 >> 29));

    return 0;
}

void sv_replace_status_func(u64 l2c_agentbase, int status_kind, u64 key,
                            void* func) {
    u64 unk48 = LOAD64(l2c_agentbase + 0x48);
    u64 unk50 = LOAD64(l2c_agentbase + 0x50);
    if (0x2E8BA2E8BA2E8BA3LL * ((unk50 - unk48) >> 4) > (u64)status_kind) {
        *(void**)(unk48 + 0xB0LL * status_kind + (key << 32 >> 29)) = func;
    }
}

u64 clear_lua_stack_replace(L2CAgent* l2c_agent) {
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

int LoadModule_intercept(nn::ro::Module* module, void const* unk1, void* unk2,
                         unsigned long unk3, int unk4) {
    int ret = nn::ro::LoadModule(module, unk1, unk2, unk3, unk4);

    SaltySDCore_RegisterModule((void*)(module->module.module->module_base));
    suicide_bomb_acmd_game = SaltySDCore_FindSymbol(
        "_ZN7lua2cpp27L2CFighterAnimcmdGameCommon31bind_hash_call_game_SuicideBombEPN3lib8L2CAgentERNS1_7utility8VariadicEPKcSt9__va_list");

    return ret;
}

void script_replacement() {
    SaltySD_function_replace_sym(
        "_ZN3app8lua_bind40ControlModule__get_command_flag_cat_implEPNS_26BattleObjectModuleAccessorEi",
        (u64)&get_command_flag_cat_replace);
    SaltySD_function_replace_sym("_ZN3lib8L2CAgent15clear_lua_stackEv", (u64)&clear_lua_stack_replace);
    SaltySDCore_ReplaceImport("_ZN2nn2ro10LoadModuleEPNS0_6ModuleEPKvPvmi", (void*)LoadModule_intercept);
}
