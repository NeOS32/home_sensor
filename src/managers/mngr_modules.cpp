// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#define MY_DEBUG 1
#if 1 == MY_DEBUG
#define DEBUG_LOCAL 1
#endif

static module_funs_t MODS[] =
{
        {.m_module_letter = 'I', .m_module_name = 0, .m_get_caps = BIN_IN_getCapabilities },
        {.m_module_letter = 'B', .m_module_name = 0, .m_get_caps = BIN_OUT_getCapabilities },
#if 1==N32_CFG_PWM_ENABLED
        {.m_module_letter = 'P', .m_module_name = 0, .m_get_caps = PWM_getCapabilities },
#endif // N32_CFG_PWM_ENABLED
        {.m_module_letter = 'L', .m_module_name = 0, .m_get_caps = LED_getCapabilities },
        {.m_module_letter = 'T', .m_module_name = 0, .m_get_caps = TEMP_getCapabilities },
#if 1==N32_CFG_HISTERESIS_ENABLED
        {.m_module_letter = 'H', .m_module_name = 0, .m_get_caps = HYST_getCapabilities },
#endif // N32_CFG_HISTERESIS_ENABLED
#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED
        {.m_module_letter = 'Q', .m_module_name = 0, .m_get_caps = QA_getCapabilities },
#endif // N32_CFG_QUICK_ACTIONS_ENABLED
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
