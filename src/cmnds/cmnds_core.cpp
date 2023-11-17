// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_WARN;

#define CMNDS_BIT_FOR_ACTIVE_COMMAND (1 << 7)

// NOTE & READ !!!!
// Basic idea behind this module:
// 1. slots represent logical devices. Below can be any (BIN, PWM, etc)
// 2. wheres channels, represent physical devices
// 3. this is to have common logic, timers handling for BIN, PWM etc

typedef struct {
    u8 timer_id;
    u8 state;
    // actions are duplicated, use those from TIMERs!
    actions_t m_actions;
} CMNDS_channel_state_t;
static CMNDS_channel_state_t SLOT_States[CMNDS_NUM_OF_AVAIL_SLOTS] = { 0 };
static u8 g_CountCmnds = 0;



typedef struct {
    char module_id;
    u8 m_first_slot;
} CMNDS_module_t;
static CMNDS_module_t MODULES_States[CMNDS_NUM_OF_AVAIL_MODULES] = { 0 };
static u8 CountModulesRegistered = 0;
static u8 FirstAvailSlot = 0;

void CMNDS_ModuleInit(void) {

    // structure initialization
    _FOR(i, 0, CMNDS_NUM_OF_AVAIL_SLOTS) {
        SLOT_States[i].timer_id = TIMER_NULL;
        SLOT_States[i].state = 0;
        SLOT_States[i].m_actions.fun_start = NULL;
        SLOT_States[i].m_actions.fun_stop = NULL;
    }

    _FOR(i, 0, CMNDS_NUM_OF_AVAIL_MODULES) {
        MODULES_States[i].module_id = 0;
        MODULES_States[i].m_first_slot = 0;
    }

    u8 AllModulesCount = CMNDS_GetNumOfAvailSubModules();

    char ModuleId;
    _FOR(i, 0, AllModulesCount)
        if ('?' != (ModuleId = CMNDS_GetModuleID(i)))
            if (false == CMNDS_RegisterSubModule(ModuleId)) {
                IF_DEB_W() {
                    String str(F("CMNDS: can't register sub module: '"));
                    str += ModuleId;
                    str += F("'!");
                    SERIAL_publish(str.c_str());
                    MSG_Publish_Debug(str.c_str());
                }

                THROW_ERROR();
            }
}

static bool cmnds_GetFirstSlot4Module(char i_cModule, u8& o_FirstSlotNumber) {

    _FOR(i, 0, CountModulesRegistered)
        if (i_cModule == MODULES_States[i].module_id) {
            o_FirstSlotNumber = MODULES_States[i].m_first_slot;
            return true;
        }

    o_FirstSlotNumber = CMNDS_NULL;

    IF_DEB_W() {
        String str(F("CMNDS: requested module: '"));
        str += i_cModule;
        str += F("', hasn't been registered, yet!");
        MSG_Publish_Debug(str.c_str());
    }

    return false;
}

bool CMNDS_RegisterSubModule(char i_cModule) {

    if (FirstAvailSlot >= CMNDS_NUM_OF_AVAIL_SLOTS) {
        IF_DEB_L() {
            String str(F("CMNDS: no more free slots available!"));
            SERIAL_publish(str.c_str());
            MSG_Publish_Debug(str.c_str());
        }
        THROW_ERROR();
        return false;
    }

    _FOR(i, 0, CountModulesRegistered)
        if (i_cModule == MODULES_States[i].module_id) {
            IF_DEB_L() {
                String str(F("CMNDS: module already registered!"));
                SERIAL_publish(str.c_str());
                MSG_Publish_Debug(str.c_str());
            }
            THROW_ERROR();
            return false;
        }

    MODULES_States[CountModulesRegistered].module_id = i_cModule;
    MODULES_States[CountModulesRegistered].m_first_slot = FirstAvailSlot;

    CountModulesRegistered++;

    FirstAvailSlot += CMNDS_GetSlotsCount(i_cModule);

    return true;
}

bool CMNDS_isSlotActive(u8 slot) {
    if (slot < CMNDS_NUM_OF_AVAIL_SLOTS)
        return (0x0 != (SLOT_States[slot].state & CMNDS_BIT_FOR_ACTIVE_COMMAND));
    else
        return(false); // error
}

static u8 cmnds_getTimerIdForSlot(u8 slot) {
    if (slot < CMNDS_NUM_OF_AVAIL_SLOTS)
        return (SLOT_States[slot].timer_id);
    else
        return (CMNDS_NULL);
}

static bool cmnds_setSlotState(u8 slot, bool isActive) {
    if (slot < CMNDS_NUM_OF_AVAIL_SLOTS) {

        IF_DEB_L() {
            String str(F(" cmnds: setting slot: "));
            str += slot;
            str += F(", state=");
            str += isActive;
            str += F(", prev_state=");
            str += SLOT_States[slot].state;
            // DEBLN(str);
            MSG_Publish_Debug(str.c_str());
        }

        // do we want to set slot active?
        if (true == isActive) {
            SLOT_States[slot].state |= CMNDS_BIT_FOR_ACTIVE_COMMAND;
        }
        else { // isActive == false - slot disabling
            SLOT_States[slot].state &= (0xFF - CMNDS_BIT_FOR_ACTIVE_COMMAND);
            SLOT_States[slot].timer_id = CMNDS_NULL;
        }
        return (true);
    }
    else
        return (false);
}

void CMNDS_DisplayAssignments(void) {
    {
        String str1(F("\nCMNDS modules assignments:\n-=-=-=-=-="));
        MSG_Publish_Debug(str1.c_str());
    }

    String str;
    u8 SlotsRegistered = 0;
    _FOR(i, 0, CountModulesRegistered) {
        str = F(" ");
        str += i;
        str += F(": Module='");
        str += MODULES_States[i].module_id;
        str += F("', Slots=");
        str += MODULES_States[i].m_first_slot;
        str += F("..");
        str += MODULES_States[i].m_first_slot +
            CMNDS_GetSlotsCount(MODULES_States[i].module_id) - 1;
        str += F(", Count=");
        str += CMNDS_GetSlotsCount(MODULES_States[i].module_id);
        // DEBLN(str);
        MSG_Publish_Debug(str.c_str());

        SlotsRegistered += CMNDS_GetSlotsCount(MODULES_States[i].module_id);
    }
    str = F("Total: slots reserved: ");
    str += SlotsRegistered;
    str += F(", modules registered: ");
    str += CountModulesRegistered;
    MSG_Publish_Debug(str.c_str());

    {
        String str1(F("\nCMNDS slots:\n-=-=-=-=-="));
        // DEBLN(str1);
        MSG_Publish_Debug(str1.c_str());
    }

    _FOR(i, 0, CMNDS_NUM_OF_AVAIL_SLOTS) {
        str = F(" ");
        str += i;
        str += F(": state=");
        str += SLOT_States[i].state;
        str += F(", timer_id=");
        str += SLOT_States[i].timer_id;
        str += F(", isTimerActive=");
        str += TIMER_IsActive(SLOT_States[i].timer_id);

        // DEBLN(str);
        MSG_Publish_Debug(str.c_str());
    }

    str = F("Total: slots=");
    str += CMNDS_NUM_OF_AVAIL_SLOTS;
    str += F(", timers=");
    str += MAX_TIMERS;
    // DEBLN(str);
    MSG_Publish_Debug(str.c_str());
}

bool CMNDS_ResetSlotState(u8 slot) {
    // first, lets reset physical channel
    u8 timer_id;

    do {
        if (TIMER_NULL != (timer_id = cmnds_getTimerIdForSlot(slot))) {
            // ok, so there is a timer for the slot
            if (true == TIMER_IsActive(timer_id)) { //
                if (false == TIMER_Stop(timer_id))
                    break; // timer active, but can't stop it? Errror!
            }
            else
                break; // timer present but not active? error!
        };

        return (cmnds_setSlotState(slot, false));

    } while (0);

    cmnds_setSlotState(slot, false);
    THROW_ERROR();

    return (false);
}

bool CMNDS_ResetAllSlotStates(const state_t& s) {
    u8 slot, slot_max;
    bool ret = true;

    if (CMNDS_NULL == (slot = CMNDS_GetFirstSlotNumber(s.action)))
        return (false);

    slot_max = slot + CMNDS_GetSlotsCount(s.action);
    for (; slot < slot_max; slot++)
        if (false == CMNDS_ResetSlotState(slot))
            ret = false;

    return (ret);
}

u8 CMNDS_GetSlotsCount(char i_cAction) {
    switch (i_cAction) {
    case 'B': // BIN OUTPUT
        return (BIN_OUT_NUM_OF_AVAIL_CHANNELS);

    case 'I': // BIN INPUT
        return (BIN_IN_NUM_OF_AVAIL_CHANNELS);

    case 'P': // PWM
        return (PWM_NUM_OF_AVAIL_CHANNELS);

    case 'L': // LED
        return (LED_NUM_OF_AVAIL_CHANNELS);

    case 'T': // TEMP
        return (DS18B20_NUM_OF_AVAIL_CHANNELS);

    case 'H': // HISTERESIS
        return (HIST_NUM_OF_AVAIL_CHANNELS);

    case 'Q': // QUICK ACTIONS
        return (0); // QA is not handled here. The QUICK_ACTIONS_NUM_OF_AVAIL_CHANNELS is the number of QA items in eeprom

    case 'Z':       // Valve
        return (0); // TODO: finish
    }

    DEBLN(F("ERR: Couldn't get slots number!"));
    THROW_ERROR();

    return (CMNDS_NULL);
}

#define PIECE_OF_CODE(MAX_CHANNELS, MODULE_ID)                                 \
    if (MAX_CHANNELS > 0) {                                                    \
        if (i_uLogicalNumberOfModule > 0)                                      \
            i_uLogicalNumberOfModule--;                                        \
        else                                                                   \
            return (MODULE_ID);                                                \
    }

char CMNDS_GetModuleID(u8 i_uLogicalNumberOfModule) {
    PIECE_OF_CODE(BIN_OUT_NUM_OF_AVAIL_CHANNELS, 'B');
    PIECE_OF_CODE(BIN_IN_NUM_OF_AVAIL_CHANNELS, 'I');
    PIECE_OF_CODE(PWM_NUM_OF_AVAIL_CHANNELS, 'P');
    PIECE_OF_CODE(LED_NUM_OF_AVAIL_CHANNELS, 'L');
    PIECE_OF_CODE(DS18B20_NUM_OF_AVAIL_CHANNELS, 'T');
    PIECE_OF_CODE(HIST_NUM_OF_AVAIL_CHANNELS, 'H');
#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED
    PIECE_OF_CODE(QUICKACTIONS_NUM_OF_AVAIL_CHANNELS, 'Q');
#endif // N32_CFG_QUICK_ACTIONS_ENABLED

    return ('?');
}

u8 CMNDS_GetNumOfAvailSubModules() {
    u8 SubModules = 0;
    if (BIN_OUT_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
    if (BIN_IN_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
    if (PWM_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
    if (LED_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
    if (DS18B20_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
    if (HIST_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED
    if (QUICKACTIONS_NUM_OF_AVAIL_CHANNELS > 0)
        SubModules++;
#endif // N32_CFG_QUICK_ACTIONS_ENABLED

    return (SubModules);
}

u8 CMNDS_GetFirstSlotNumber(char i_cModule) {

    u8 FirstSlotNumber = 0;
    if (true == cmnds_GetFirstSlot4Module(i_cModule, FirstSlotNumber))
        return FirstSlotNumber;

    THROW_ERROR();
    DEBLN(F("ERR: Couldn't get first slot number!"));

    return CMNDS_NULL;
}

u8 CMNDS_GetSlotNumber(const state_t& s) {

    switch (s.action) {
    case 'B': // BIN OUTPUT
        if (s.c.b.channel < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.b.channel);
        break;

    case 'I': // BINARY INPUT
        if (s.c.i.channel < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.i.channel);
        break;

    case 'P': // PWM
        if (s.c.p.channel < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.p.channel);
        break;

    case 'L': // LED
        if (s.c.l.channel < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.l.channel);
        break;

    case 'T': // TEMP
        if (s.c.t.channel < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.t.channel);
        break;

    case 'Z': // Valve
        if (s.c.v.v1 < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.v.v1);
        break;

    case 'H': // HIST
        if (s.c.h.channel < CMNDS_GetSlotsCount(s.action))
            return (CMNDS_GetFirstSlotNumber(s.action) + s.c.h.channel);
        break;
    }

    THROW_ERROR();
    DEBLN(F("ERR: Couldn't match slot number!"));

    return (CMNDS_NULL);
}

static void fun_start_wrapper(actions_context_t& i_rActionsContext) {

#if 1 == DEBUG_LOCAL
    String str(F("CMNDS: fun_start: v1="));
    str += i_rActionsContext.var1;
    str += F(", v2=");
    str += i_rActionsContext.var2;
    str += F(", Slot=");
    str += i_rActionsContext.slot;
    str += F(", timer_id=");
    str += i_rActionsContext.timer_id;
    /*str += ", f_Start= ";
    str += (u16)SLOT_States[i_rActionsContext.slot].m_actions.fun_start;
    str += ", f_Stop= ";
    str += (u16)SLOT_States[i_rActionsContext.slot].m_actions.fun_stop;*/
    DEBLN(str);
#endif // DEBUG_LOCAL

    // slot allocation
    cmnds_setSlotState(i_rActionsContext.slot, true);

    // We get here called by TIMER module and thus:
    // - Timer is already allocated
    // so only calling low-level function
    // low level
    u8 timer_id = i_rActionsContext.timer_id;
    if (CMNDS_NULL != timer_id) {
        if (SLOT_States[i_rActionsContext.slot].m_actions.fun_start)
            (SLOT_States[i_rActionsContext.slot].m_actions.fun_start)(*TIMER_getActionContext(timer_id));
        else
            DEBLN(F("ERR: called function that is null!"));
    }
    else
        DEBLN(F("ERR: empty timer_id!"));
}
static void fun_stop_wrapper(actions_context_t& i_rActionsContext) {

#if 1 == DEBUG_LOCAL
    String str(F("CMNDS: fun_stop: v1="));
    str += i_rActionsContext.var1;
    str += F(", v2=");
    str += i_rActionsContext.var2;
    str += F(", Slot=");
    str += i_rActionsContext.slot;
    str += F(", timer_id=");
    str += i_rActionsContext.timer_id;
    /*str += ", f_Start= ";
    str += (u16)SLOT_States[i_rActionsContext.slot].m_actions.fun_start;
    str += ", f_Stop= ";
    str += (u16)SLOT_States[i_rActionsContext.slot].m_actions.fun_stop;*/
    DEBLN(str);
#endif // DEBUG_LOCAL

    // low level
    u8 timer_id = i_rActionsContext.timer_id;
    bool doesFunWantToBeRestarted = false;

    // first, we need to get the timer to a well-known state, so shuting it down
    // silently
    TIMER_ResetTimer(timer_id);

    // calling for client's stop
    (SLOT_States[i_rActionsContext.slot].m_actions.fun_stop)(
        *TIMER_getActionContext(timer_id));

    // finally, does client want to be run again?
    doesFunWantToBeRestarted = TIMER_IsActive(timer_id);

    // CMNDS_ResetSlotState(i_rActionsContext.slot);;
    // slot release
    if (false == doesFunWantToBeRestarted)
        cmnds_setSlotState(i_rActionsContext.slot, false);

    // This fun is called by Timer, so TIMER will be cleared on its own
    // return( doesFunWantToBeRestarted );
}

bool CMNDS_ScheduleAction(const state_t& s, actions_t& i_rActions,
    actions_context_t& i_rActionsContext) {
    u8 slot = CMNDS_GetSlotNumber(s);

    IF_DEB_L() {
        String str(F("CMNDS: Sched: count="));
        str += s.count;
        str += F(", s.action=");
        str += s.action;
        str += F(", slot=");
        str += slot;
        MSG_Publish_Debug(str.c_str());
    }

    if (CMNDS_NULL == slot) {
        // someting wrong, ignorring
        DEBLN(F("ERR: CMNDS: wrong slot number!"));
    }
    else if (true == CMNDS_isSlotActive(slot)) {
        // when channel is already active, we want only to extend timer
        IF_DEB_L() {
            String str(F("CMNDS: slot already active: extended, count="));
            str += s.count;
            str += F(", action=");
            str += s.action;
            MSG_Publish_Debug(str.c_str());
        }

        do {
            u8 timer_id = cmnds_getTimerIdForSlot(slot);
            if (false == TIMER_ReStart(timer_id, s.count))
                break;

            return true; // success
        } while (0);
    }
    else {
        // slot hasn't been activated, let's keep the original actions & context
        SLOT_States[slot].m_actions = i_rActions;
        // SLOT_States[slot].m_actions_context = i_rActionsContext;
        i_rActionsContext.slot = slot;

        // wrappers initialisation
        // the idea is: TIMER calls WRAPPER which in turn calls MODULE
        actions_t WrappedActions{ fun_start_wrapper, fun_stop_wrapper };

        // timer when fires, is going to call wrappers
        if (TIMER_NULL != (SLOT_States[slot].timer_id = TIMER_Start(WrappedActions, i_rActionsContext, s.count))) {
            i_rActionsContext.timer_id = SLOT_States[slot].timer_id;
            g_CountCmnds++;

            return true; // success
        }
    }

    if (CMNDS_NULL != slot)
        CMNDS_ResetSlotState(slot); // TODO is it ok here?

    THROW_ERROR();
    return false; // error
}

// can be called recurrently?
bool CMNDS_decodeCmnd(const byte* payload, state_t& i_State, u8* o_uCmndLen) {
    module_funs_t module_slot;
    mIndex iModuleIndex = 255;

    if (0 == payload )
        return false;

    char ch = *payload++;
    if (false == MOD_getModuleIndex(ch, iModuleIndex)) {
        IF_DEB_L() {
            String str(F(" module not found - message ignored: '"));
            str += char(*(payload - 1));
            str += F("'");
            SERIAL_publish(str.c_str());
        }
        return false;
    }

    module_slot = MOD_getModuleSlot(iModuleIndex);
    if (0 == module_slot.m_get_caps) {
        IF_DEB_L() {
            String str1(F(" no caps for given module!"));
            SERIAL_publish(str1.c_str());
        }
        return false;
    }

    // let's get a module's caps
    struct module_caps_s caps_instance = (module_slot.m_get_caps)();
    if (0 == caps_instance.m_cmnd_decoder) {
        IF_DEB_L() {
            String str(F(" no decoder fun installed for given module!"));
            SERIAL_publish(str.c_str());
        }
        return false;
    }

    IF_DEB_L() {
        String str2(F("CMNDS: Module found: '"));
        str2 += char(ch);
        str2 += F("', slot: ");
        str2 += iModuleIndex;
        SERIAL_publish(str2.c_str());
    }

    // finally, let's ask a module to try to decode the command
    return caps_instance.m_cmnd_decoder(payload, i_State, o_uCmndLen);
}

/// @brief Executes given state by finding responsible module, checking capabilities and finally calling "executor" function
/// @param s a reference to a cmnd's state struct
/// @return a result of the operation
bool CMNDS_executeCmnd(state_t& s) {

    mIndex i = 255;
    if (false == MOD_getModuleIndex(s.action, i))
        return false;

    const module_funs_t module_slot = MOD_getModuleSlot(i);
    if (0 == module_slot.m_get_caps)
        return false;

    struct module_caps_s caps_instance = (module_slot.m_get_caps)();
    if (0 == caps_instance.m_cmnd_executor) {
        IF_DEB_L() {
            String str2(F("CMNDS: module doesn't have execution capabilities - ignoring"));
            SERIAL_publish(str2.c_str());
        }
        return true;
    }
    return caps_instance.m_cmnd_executor(s);
}

bool CMNDS_modInit(char i_cModule) {
    module_funs_t module_slot;
    mIndex i = 255;

    if (false == MOD_getModuleIndex(i_cModule, i))
        return false;

    module_slot = MOD_getModuleSlot(i);
    if (0 == module_slot.m_get_caps)
        return false;

    struct module_caps_s caps_instance = (module_slot.m_get_caps)();
    if (0 == caps_instance.m_mod_init)
        return false;

    caps_instance.m_mod_init();

    return true;
}

bool CMNDS_Launch(byte* payload) {
    state_t s;

    // command processing
    s.action = *payload; // ACTION

    if (false == CMNDS_decodeCmnd(payload, s)) {
        IF_DEB_L() {
            String str(F(" - failed, message ignored !"));
            SERIAL_publish(str.c_str());
        }
        return false;
    }

    if (false == CMNDS_executeCmnd(s)) {
        IF_DEB_L() {
            String str(F(" cmnd execution failed!"));
            SERIAL_publish(str.c_str());
        }
        return false;
    }

    return true;
}