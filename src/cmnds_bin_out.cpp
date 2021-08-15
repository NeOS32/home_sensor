/*
 * SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "my_common.h"

#if 1 == N32_CFG_BIN_OUT_ENABLED

#ifdef DEBUG
#define DEBUG_LOCAL 1
#endif

#define BIN_MAX (0xFF)

static bool binout_getPinFromChannelNum(u8 i_ChannelNumber, u8& o_PinNumber) {
    if (i_ChannelNumber < BIN_OUT_LINE_DH_COUNT) {
        o_PinNumber = BIN_OUT_LINE_DH_FIRST + i_ChannelNumber;
        return true;
    }
    else if (i_ChannelNumber < (BIN_OUT_LINE_DH_COUNT + BIN_OUT_LINE_DL_COUNT)) {
        o_PinNumber = BIN_OUT_LINE_DL_FIRST + i_ChannelNumber - BIN_OUT_LINE_DH_COUNT;
        return true;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

static void binout_SetupChannel(u8 i_ChannelNumber, u8 DefaultState = LOW) {
    u8 PhysicalPinNumber;

    if (true == binout_getPinFromChannelNum(i_ChannelNumber, PhysicalPinNumber)) {
        pinMode(PhysicalPinNumber, OUTPUT);
        digitalWrite(PhysicalPinNumber, DefaultState);
    }
}

static void binout_ChannelTurnON(actions_context_t& i_rActionsContext) {
    // for channels 0..BIN_NUM_OF_DEFAULT_HIGH_CHANNELS default is state HIGH,
    // so turning it ON means, setting pin "LOW". For channels >=
    // BIN_NUM_OF_DEFAULT_HIGH_CHANNELS, default is state LOW, so turning it ON
    // means, setting pin to "HIGH"

#if 1==DEBUG_LOCAL
    String str(F("BIN: turn_on Channel="));
    str += i_rActionsContext.var1;
    str += F(", Pin=");
    str += i_rActionsContext.var2;
    str += F(", Slot=");
    str += i_rActionsContext.slot;
    //DEBLN(str);
    MosqClient.publish(MQTT_DEBUG, str.c_str());
#endif // DEBUG_LOCAL

    u8 PhysicalPinBum;
    if (false == binout_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPinBum)) {
        THROW_ERROR();
        DEBLN(F("BIN: wrong ch number!"));
        return;
    }

    if ((i_rActionsContext.var1 >= BIN_OUT_NUM_OF_AVAIL_CHANNELS) ||
        (PhysicalPinBum != i_rActionsContext.var2)) {
        THROW_ERROR();
        DEBLN(F("BIN: wrong ch number!"));
        return;
    }

    // var1 == channel num
    if (i_rActionsContext.var1 < BIN_OUT_LINE_DH_COUNT)
        digitalWrite(PhysicalPinBum, LOW); // var2 == pin
    else
        digitalWrite(PhysicalPinBum, HIGH); // var2 == pin
};

static void binout_ChannelTurnOFF(actions_context_t& i_rActionsContext) {
#if 1==DEBUG_LOCAL
    String str(F("BIN: turn_off Channel="));
    str += i_rActionsContext.var1;
    str += F(", Pin=");
    str += i_rActionsContext.var2;
    str += F(", Slot=");
    str += i_rActionsContext.slot;
    //DEBLN(str);
    MosqClient.publish(MQTT_DEBUG, str.c_str());
#endif // DEBUG_LOCAL

    u8 PhysicalPinBum;
    if (false == binout_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPinBum)) {
        THROW_ERROR();
        DEBLN(F("BIN: wrong ch number!"));
        return;
    }

    if ((i_rActionsContext.var1 >= BIN_OUT_NUM_OF_AVAIL_CHANNELS) ||
        (PhysicalPinBum != i_rActionsContext.var2)) {
        THROW_ERROR();
        DEBLN(F("BIN: wrong ch number!"));
        return;
    }

    // for channels 0..BIN_NUM_OF_DEFAULT_HIGH_CHANNELS default is state
    // HIGH, so turning it ON means, setting pin "LOW". For channels >=
    // BIN_NUM_OF_DEFAULT_HIGH_CHANNELS, default is state LOW, so turning it
    // ON means, setting pin to "HIGH"
    if (i_rActionsContext.var1 < BIN_OUT_LINE_DH_COUNT)
        digitalWrite(PhysicalPinBum, HIGH); // var2 == pin
    else
        digitalWrite(PhysicalPinBum, LOW); // var2 == pin
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
}

bool BIN_OUT_ExecuteCommand(const state_t& s) {
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
 * B0ANS - Output "A" is set HIGH for NS seconds
 * B1ANS - Output "A" is reset (set LOW forced)
 * B2ANS - All outputs are reset (set LOW forced)
 */
bool decode_CMND_B(const byte* payload, state_t& s) {
    bool sanity_ok = false;

    s.command = (*payload++) - '0'; // [0..8] - command
    s.c.b.channel =
        getDecodedChannelNum((*payload++) - '0'); // [0..9] - Channel

    char number = (*payload++) - '0'; // [0..9] - number
    char scale = *payload++; // [SMTH] - Seconds/Minutes/TenMinutes/Hours
    s.count = getSecondsFromNumberAndScale(number, scale);

    if (false == binout_getPinFromChannelNum(s.c.b.channel, s.c.b.pin))
        goto ERROR;

    s.sum = (*payload++) - '0'; // sum = 1

    if (s.command >= 0 && s.command <= CMND_BIN_OUT_MAX_VALUE)
        if (s.c.b.channel >= 0 && s.c.b.channel < BIN_OUT_NUM_OF_AVAIL_CHANNELS)
            if (number >= 0 && number <= 9)
                if (true == isSumOk(s))
                    sanity_ok = true;

ERROR:
    // Info display
#if 1==DEBUG_LOCAL
    u8 i = s.c.b.channel;
    String str(F("BIN: channel: "));
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
#endif // DEBUG_LOCAL

    return (sanity_ok);
}
#endif // N32_CFG_BIN_OUT_ENABLED
