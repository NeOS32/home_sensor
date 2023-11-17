// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#if 1 == N32_CFG_BIN_OUT_ENABLED

static debug_level_t uDebugLevel = DEBUG_WARN;
static bool bModuleInitialised = false;

#define BIN_MAX (0xFF)

static bool binout_getPinFromChannelNum(u8 i_ChannelNumber, u8& o_PinNumber) {
    // Logical channels numbering DL and DH is linera, so DL is always after DH. However, physical pins may be placed anywhere thus BIN_OUT_LINE_DH_FIRST & BIN_OUT_LINE_DHLFIRST was introduced. 
    if (i_ChannelNumber < BIN_OUT_LINE_DH_COUNT) {
        o_PinNumber = BIN_OUT_LINE_DH_FIRST + i_ChannelNumber;
        return true;
    }
    else if (i_ChannelNumber < (BIN_OUT_LINE_DH_COUNT + BIN_OUT_LINE_DL_COUNT)) {
        o_PinNumber = BIN_OUT_LINE_DL_FIRST + ( i_ChannelNumber - BIN_OUT_LINE_DH_COUNT );
        return true;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

static void binout_SetupChannel(u8 i_ChannelNumber, u8 i_DefaultState = LOW) {
    u8 PhysicalPinNumber;

    if (true == binout_getPinFromChannelNum(i_ChannelNumber, PhysicalPinNumber)) {
        pinMode(PhysicalPinNumber, OUTPUT);
        digitalWrite(PhysicalPinNumber, i_DefaultState);
    }
}

static void binout_SetPhysicalPinInActiveState(u8 i_logSchannelNumber, u8 i_uPhysicalPinBum) {
    // for channels 0..BIN_NUM_OF_DEFAULT_HIGH_CHANNELS default is state
    // HIGH, so turning it ON means, setting pin "LOW". For channels >=
    // BIN_NUM_OF_DEFAULT_HIGH_CHANNELS, default is state LOW, so turning it
    // ON means, setting pin to "HIGH"
    u8 state = digitalRead(i_uPhysicalPinBum);
    if (i_logSchannelNumber < BIN_OUT_LINE_DH_COUNT) {
        if (LOW != state)
            digitalWrite(i_uPhysicalPinBum, LOW); // var2 == pin
    }
    else {
        if (HIGH != state)
            digitalWrite(i_uPhysicalPinBum, HIGH); // var2 == pin
    }
}

static void binout_SetPhysicalPinInInActiveState(u8 i_logSchannelNumber, u8 i_uPhysicalPinBum) {
    // for channels 0..BIN_NUM_OF_DEFAULT_HIGH_CHANNELS default is state
    // HIGH, so turning it ON means, setting pin "LOW". For channels >=
    // BIN_NUM_OF_DEFAULT_HIGH_CHANNELS, default is state LOW, so turning it
    // ON means, setting pin to "HIGH"
    u8 state = digitalRead(i_uPhysicalPinBum);
    if (i_logSchannelNumber < BIN_OUT_LINE_DH_COUNT) {
        if (HIGH != state)
            digitalWrite(i_uPhysicalPinBum, HIGH); // var2 == pin
    }
    else {
        if (LOW != state)
            digitalWrite(i_uPhysicalPinBum, LOW); // var2 == pin
    }
}

static void binout_ChannelTurnON(actions_context_t& i_rActionsContext) {
    IF_DEB_L() {
        String str(F("BIN: turn_on Channel="));
        str += i_rActionsContext.var1;
        str += F(", Pin=");
        str += i_rActionsContext.var2;
        str += F(", Slot=");
        str += i_rActionsContext.slot;
        //DEBLN(str);
        MSG_Publish_Debug(str.c_str());
    }

    u8 PhysicalPinBum;
    if (false == binout_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPinBum)) {
        DEB_W(F("BIN: wrong ch number!\n"));
        THROW_ERROR();
        return;
    }

    if ((i_rActionsContext.var1 >= BIN_OUT_NUM_OF_AVAIL_CHANNELS) ||
        (PhysicalPinBum != i_rActionsContext.var2)) {
        DEB_W(F("BIN: wrong ch number!\n"));
        THROW_ERROR();
        return;
    }

    binout_SetPhysicalPinInActiveState(i_rActionsContext.var1, PhysicalPinBum);
};

static void binout_ChannelTurnOFF(actions_context_t& i_rActionsContext) {
    IF_DEB_L() {
        String str(F("BIN: turn_off Channel="));
        str += i_rActionsContext.var1;
        str += F(", Pin=");
        str += i_rActionsContext.var2;
        str += F(", Slot=");
        str += i_rActionsContext.slot;
        //DEBLN(str);
        MSG_Publish_Debug(str.c_str());
    }

    u8 PhysicalPinBum;
    if (false == binout_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPinBum)) {
        DEB_W(F("BIN: wrong ch number!\n"));
        THROW_ERROR();
        return;
    }

    if ((i_rActionsContext.var1 >= BIN_OUT_NUM_OF_AVAIL_CHANNELS) ||
        (PhysicalPinBum != i_rActionsContext.var2)) {
        DEB_W(F("BIN: wrong ch number!\n"));
        THROW_ERROR();
        return;
    }

    binout_SetPhysicalPinInInActiveState(i_rActionsContext.var1, PhysicalPinBum);
}

void BIN_OUT_ModuleInit(void) {

    u8 uLogChannel;
    if (BIN_OUT_LINE_DH_COUNT > 0) {
        // DH is always first, so offset is "0" - see the last parameter
        PIN_RegisterPins(binout_getPinFromChannelNum, BIN_OUT_LINE_DH_COUNT, F("BIN_OUT_DH"), 0);
        for (uLogChannel = 0; uLogChannel < BIN_OUT_LINE_DH_COUNT; uLogChannel++)
            binout_SetupChannel(uLogChannel, HIGH);
    }

    if (BIN_OUT_LINE_DL_COUNT > 0) {
        // DL is always second, so offset is BIN_OUT_LINE_DH_COUNT - see the last parameter
        PIN_RegisterPins(binout_getPinFromChannelNum, BIN_OUT_LINE_DL_COUNT, F("BIN_OUT_DL"), BIN_OUT_LINE_DH_COUNT);
        for (uLogChannel = BIN_OUT_LINE_DH_COUNT;
            uLogChannel < BIN_OUT_NUM_OF_AVAIL_CHANNELS; uLogChannel++)
            binout_SetupChannel(uLogChannel, LOW);
    }

    bModuleInitialised = true;
}

bool BIN_OUT_ExecuteCommand(const state_t& s) {
    CHECK_MODULE_SANITY();

    actions_t Actions;
    Actions.fun_start = binout_ChannelTurnON;
    Actions.fun_stop = binout_ChannelTurnOFF;

    actions_context_t ActionsContext;//{s.c.b.channel, s.c.b.pin, 0, 0};
    ActionsContext.var1 = s.c.b.channel;
    ActionsContext.var2 = s.c.b.pin;
    ActionsContext.slot = 0;
    ActionsContext.timer_id = 0;

    u8 slot, slot_max;
    bool ret = true;

    if (CMNDS_NULL != (slot = CMNDS_GetSlotNumber(s))) {
        switch (s.command) {
        case CMND_BIN_OUT_B0_CHANNEL_ON_FOR_NS: // DONE
            IF_DEB_L() {
                String str(F("BIN:EXEC: CMND: "));
                str += '0' + CMND_BIN_OUT_B0_CHANNEL_ON_FOR_NS;
                str += F(", Channel: ");
                str += s.c.b.channel;
                str += F(", Pin: ");
                str += s.c.b.pin;
                DEBLN(str);
            }
            if ((u32)0x0 == s.count) { // is that a stopping request?
                if (CMNDS_isSlotActive(slot)) // is that slot still active?
                    CMNDS_ResetSlotState(slot);
                binout_SetPhysicalPinInInActiveState(s.c.b.channel, s.c.b.pin);
                return true;
            }
            return (CMNDS_ScheduleAction(s, Actions, ActionsContext));

        case CMND_BIN_OUT_B1_RESET_CHANNEL: // DONE
            return (CMNDS_ResetSlotState(slot));

        case CMND_BIN_OUT_B2_RESET_ALL_CHANNELS: // DONE
            slot = CMNDS_GetFirstSlotNumber(s.action);
            slot_max = slot + CMNDS_GetSlotsCount(s.action);
            for (; slot < slot_max; slot++)
                if (false == CMNDS_ResetSlotState(slot))
                    ret = false;
            return ret;
        }
    }

    THROW_ERROR();
    return false; // error
}

/**
 * B0ANS - Output "A" is set ACTIVE for NS seconds
 * B1ANS - Output "A" is reset (set LOW forced)
 * B2ANS - All outputs are reset (set NOT ACTIIVE forced)
 */
bool decode_CMND_B(const byte* payload, state_t& s, u8* o_CmndLen) {
    CHECK_MODULE_SANITY();

    const byte* cmndStart = payload;
    bool sanity_ok = false;

    s.command = (*payload++) - '0'; // [0..8] - command
    s.c.b.channel =
        getDecodedChannelNum((*payload++) - '0'); // [0..9] - Channel

    char number = (*payload++) - '0'; // [0..9] - number
    char scale = *payload++; // [SMTH] - Seconds/Minutes/TenMinutes/Hours
    s.count = getSecondsFromNumberAndScale(number, scale);

    if (false == binout_getPinFromChannelNum(s.c.b.channel, s.c.b.pin))
        goto SKIP;

    s.sum = (*payload++) - '0'; // sum = 1

    if (s.command >= 0 && s.command <= CMND_BIN_OUT_MAX_VALUE)
        if (s.c.b.channel >= 0 && s.c.b.channel < BIN_OUT_NUM_OF_AVAIL_CHANNELS)
            if (number >= 0 && number <= 9)
                if (true == isSumOk(s))
                    sanity_ok = true;

SKIP:
    // Info display
    IF_DEB_L() {
        u8 i = s.c.b.channel;
        String str(F("BIN:CMND: channel: "));
        str += i;
        str += F(", Cmd: ");
        i = s.command + '0';
        str += i;
        str += F(", Sanity: ");
        str += sanity_ok;
        str += F(", Secs: ");
        str += (s.count + '0');
        str += F(", Sum: ");
        i = s.sum + '0';
        str += i;
        DEBLN(str);
    }

    // setting decoded and valid cmnd length
    if (NULL != o_CmndLen)
        *o_CmndLen = payload - cmndStart;

    return (sanity_ok);
}

module_caps_t BIN_OUT_getCapabilities(void) {
    module_caps_t mc = {
        .m_is_input = false,
        .m_number_of_channels = BIN_OUT_NUM_OF_AVAIL_CHANNELS,
        .m_module_name = F("BIN_OUT"),
        .m_mod_init = BIN_OUT_ModuleInit,
        .m_cmnd_decoder = decode_CMND_B,
        .m_cmnd_executor = BIN_OUT_ExecuteCommand
    };

    return(mc);
}

#endif // N32_CFG_BIN_OUT_ENABLED
