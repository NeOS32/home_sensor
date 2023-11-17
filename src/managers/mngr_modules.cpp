// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

/// @brief a static table that is used to store all modules references and other data in the system
const static module_funs_t MODS[] =
{
#if 1==N32_CFG_BIN_IN_ENABLED
        {.m_module_letter = 'I', .m_module_name = 0, .m_get_caps = BIN_IN_getCapabilities },
#endif // N32_CFG_BIN_IN_ENABLED
#if 1==N32_CFG_BIN_OUT_ENABLED
        {.m_module_letter = 'B', .m_module_name = 0, .m_get_caps = BIN_OUT_getCapabilities },
#endif // N32_CFG_BIN_OUT_ENABLED
#if 1==N32_CFG_PWM_ENABLED
        {.m_module_letter = 'P', .m_module_name = 0, .m_get_caps = PWM_getCapabilities },
#endif // N32_CFG_PWM_ENABLED
#if 1==N32_CFG_LED_W2918_ENABLED
        {.m_module_letter = 'L', .m_module_name = 0, .m_get_caps = LED_getCapabilities },
#endif // N32_CFG_LED_W2918_ENABLED
#if 1==N32_CFG_TEMP_ENABLED
        {.m_module_letter = 'T', .m_module_name = 0, .m_get_caps = TEMP_getCapabilities },
#endif // N32_CFG_TEMP_ENABLED
#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED
        {.m_module_letter = 'Q', .m_module_name = 0, .m_get_caps = QA_getCapabilities },
#endif // N32_CFG_QUICK_ACTIONS_ENABLED
#if 1==N32_CFG_ANALOG_IN_ENABLED
    {.m_module_letter = 'A', .m_module_name = 0, .m_get_caps = ANALOG_getCapabilities },
#endif // N32_CFG_ANALOG_IN_ENABLED
#if 1==N32_CFG_HISTERESIS_ENABLED
    {.m_module_letter = 'H', .m_module_name = 0, .m_get_caps = HYST_getCapabilities },
#endif // N32_CFG_HISTERESIS_ENABLED
};

/// @brief Returns a module index
/// @param i_mModuleId a module identification letter
/// @param o_ModuleIndex is used to store index of found module
/// @return operation result
bool MOD_getModuleIndex(char i_mModuleId, mIndex& o_ModuleIndex) {
    _FOR(i, 0, int(sizeof(MODS) / sizeof(module_funs_t)))
        if (i_mModuleId == MODS[i].m_module_letter) {
            o_ModuleIndex = i;
            return true;
        }

    return false;
}

/// @brief Returns a module slot
/// @param i_Index index of given modules
/// @return a pointer to the modules slot
const module_funs_t& MOD_getModuleSlot(mIndex i_Index) {
    return MODS[i_Index];
}

/// @brief Returns a module capabilities retrievel function
/// @param i_Index index of requested modules
/// @return function pointer
const modCapabilities_f MOD_getModuleCapsFunc(mIndex i_Index) {
    return MODS[i_Index].m_get_caps;
}

/// @brief Initializes all registered modules in the modules table
/// @return true if all modules have been sucessfully initialized
bool MOD_callAllInitFuns() {
    bool bAllInitiated = true;

    _FOR(i, 0, int(sizeof(MODS) / sizeof(module_funs_t))) {

        const modCapabilities_f modCaps_f = MOD_getModuleCapsFunc(i);
        if (modCaps_f) {
            struct module_caps_s s = modCaps_f();
            if (s.m_mod_init) {
                (s.m_mod_init)();
                continue;
            }
        }
        bAllInitiated = false;
    }

    return bAllInitiated;
}
