// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static module_funs_t MODS[] =
{
#if 1==N32_CFG_BIN_IN_ENABLED
        {.m_module_letter = 'I', .m_module_name = 0, .m_mod_init = BIN_IN_ModuleInit, .m_get_caps = BIN_IN_getCapabilities },
#endif // N32_CFG_BIN_IN_ENABLED
#if 1==N32_CFG_BIN_OUT_ENABLED
        {.m_module_letter = 'B', .m_module_name = 0, .m_mod_init = BIN_OUT_ModuleInit, .m_get_caps = BIN_OUT_getCapabilities },
#endif // N32_CFG_BIN_OUT_ENABLED
#if 1==N32_CFG_PWM_ENABLED
        {.m_module_letter = 'P', .m_module_name = 0, .m_mod_init = PWM_ModuleInit, .m_get_caps = PWM_getCapabilities },
#endif // N32_CFG_PWM_ENABLED
#if 1==N32_CFG_LED_W2918_ENABLED
        {.m_module_letter = 'L', .m_module_name = 0, .m_get_caps = LED_getCapabilities },
#endif // N32_CFG_LED_W2918_ENABLED
#if 1==N32_CFG_TEMP_ENABLED
        {.m_module_letter = 'T', .m_module_name = 0, .m_mod_init = TEMP_ModuleInit, .m_get_caps = TEMP_getCapabilities },
#endif // N32_CFG_TEMP_ENABLED
#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED
        {.m_module_letter = 'Q', .m_module_name = 0, .m_mod_init = QA_ModuleInit, .m_get_caps = QA_getCapabilities },
#endif // N32_CFG_QUICK_ACTIONS_ENABLED
#if 1==N32_CFG_ANALOG_IN_ENABLED
    {.m_module_letter = 'A', .m_module_name = 0, .m_mod_init = ANALOG_ModuleInit, .m_get_caps = ANALOG_getCapabilities },
#endif // N32_CFG_ANALOG_IN_ENABLED
#if 1==N32_CFG_HISTERESIS_ENABLED
    {.m_module_letter = 'H', .m_module_name = 0, .m_mod_init = HYSTERESIS_ModuleInit, .m_get_caps = HYST_getCapabilities },
#endif // N32_CFG_HISTERESIS_ENABLED
};

bool MOD_getModuleIndex(char i_mModuleId, mIndex& o_ModuleIndex) {
    _FOR(i, 0, int(sizeof(MODS) / sizeof(module_funs_t)))
        if (i_mModuleId == MODS[i].m_module_letter) {
            o_ModuleIndex = i;
            return true;
        }

    return false;
}

const module_funs_t& MOD_getModuleSlot(mIndex i_Index) {
    return MODS[i_Index];
}

const modCapabilities_f MOD_getModuleCapsFunc(mIndex i_Index) {
    return MODS[i_Index].m_get_caps;
}

void MOD_callAllInitFuns() {
    _FOR(i, 0, int(sizeof(MODS) / sizeof(module_funs_t)))
        (MODS[i].m_mod_init)();
}
