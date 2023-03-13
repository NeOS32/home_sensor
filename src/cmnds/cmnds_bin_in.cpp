// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#if 1 == N32_CFG_BIN_IN_ENABLED

static debug_level_t uDebugLevel = DEBUG_WARN;
static bool bModuleInitialised= false;

#define BIN_MAX (0xFF)
#define BIN_IN_CHECK_INTERVAL_IN_SECS (1)
#define BIN_IN_LETTER 'I'

static bool bin_in_getPinFromChannelNum(u8 i_LogicalChannelNum, u8& o_PhysicalPinNumber) {

    if (i_LogicalChannelNum < BIN_IN_LINE_DH_COUNT) {
        o_PhysicalPinNumber = BIN_IN_LINE_DH_FIRST + i_LogicalChannelNum;
        return true;
    }
    else if (i_LogicalChannelNum < BIN_IN_NUM_OF_AVAIL_CHANNELS) {
        // TODO: why DL must always be after DH even if  BIN_IN_LINE_DL_FIRST is defined?
        o_PhysicalPinNumber = BIN_IN_LINE_DL_FIRST + i_LogicalChannelNum - BIN_IN_LINE_DH_COUNT;
        return true;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

/// @brief Sets up a logical channel
/// @param i_LogChannel 
/// @param DefaultType input channel type. If not speficied, it's set to INPUT_PULLUP
/// @return false in case of error
static bool bin_in_SetupChannel(u8 i_LogChannel, u8 DefaultType = INPUT_PULLUP) {
    u8 PhysicalPinNumber;

    // a conversion from the logical and relative number to the physical pin
    if (true == bin_in_getPinFromChannelNum(i_LogChannel, PhysicalPinNumber)) {
        pinMode(PhysicalPinNumber, DefaultType);
        return(true);
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

static u16 readMask(void) {
    u16 ret = 0;
    u8 PhysicalPinNum;

    _FOR(i, 0, BIN_IN_NUM_OF_AVAIL_CHANNELS) {
        bin_in_getPinFromChannelNum(i, PhysicalPinNum);
        if (HIGH == digitalRead(PhysicalPinNum))
            ret += 1 << i;
    }

    return(ret);
}

static u16 prev_mask = 0;
static bool bin_in_UpdateStatus(bool i_bForced = false) {
    u16 current_mask = readMask();
    bool bState;

    if (current_mask != prev_mask || true == i_bForced) {
        u16 diffs = current_mask ^ prev_mask;

        String strCurrent, strDiff, strOut, str;
        _FOR(i, 0, BIN_IN_NUM_OF_AVAIL_CHANNELS) {
            if (0x0 != ((1 << i) & diffs)) {
                // first we have to change states
                strDiff += F("1");

                String stringPath(MQTT_SENSORS_BIN_IN);
                stringPath += i;

                bState = 0x0 != ((1 << i) & current_mask);

#if 1 == N32_CFG_QUICK_ACTIONS_ENABLED
                // thne, is there a QA registered? If yes, let's trigger a command.
                u8 slot_num;
                if (true == QA_isStateTracked(BIN_IN_LETTER, i, bState, slot_num))
                    QA_ExecuteCommand(slot_num); // in e2prom we may have only validated data, so no extra command checking
                else {
                    IF_DEB_L() {
                        String str(F("BIN_IN: Flow not tracked"));
                        SERIAL_publish(str.c_str());
                    }
                }
#endif // N32_CFG_QUICK_ACTIONS_ENABLED

                if (true == bState)
                    str = F("OPENED");
                else
                    str = F("CLOSED");

                // and finally publish state to the broker
                MSG_Publish(stringPath.c_str(), str.c_str());
            }
            else
                strDiff += F("0");

            if (0x0 != ((1 << i) & current_mask))
                strCurrent += F("1");
            else
                strCurrent += F("0");
        }

        strOut = F("Current: ");
        strOut += strCurrent;
        strOut += F(", Diff: ");
        strOut += strDiff;
        String stringPath(MQTT_SENSORS_BIN_IN);
        stringPath += F("state");
        MSG_Publish(stringPath.c_str(), strOut.c_str());

        prev_mask = current_mask;
    }

    return(true);
}

static void bin_in_AlarmFun(void) {
    bin_in_UpdateStatus();
}

void BIN_IN_ModuleInit(void) {
    u8 uLogChannel;

    if (BIN_IN_LINE_DH_COUNT > 0) {
        // DH is always first, so offset is "0" - see the last parameter
        PIN_RegisterPins(bin_in_getPinFromChannelNum, BIN_IN_LINE_DH_COUNT, F("BIN_IN_DH"), 0);
        for (uLogChannel = 0; uLogChannel < BIN_IN_LINE_DH_COUNT; uLogChannel++)
            bin_in_SetupChannel(uLogChannel, INPUT_PULLUP);
    }

    if (BIN_IN_LINE_DL_COUNT > 0) {
        // DL is always second, so offset is BIN_OUT_LINE_DH_COUNT - see the last parameter
        PIN_RegisterPins(bin_in_getPinFromChannelNum, BIN_IN_LINE_DL_COUNT, F("BIN_IN_DL"), BIN_IN_LINE_DH_COUNT);
        for (uLogChannel = BIN_IN_LINE_DH_COUNT;
            uLogChannel < BIN_IN_NUM_OF_AVAIL_CHANNELS; uLogChannel++)
            bin_in_SetupChannel(uLogChannel, INPUT); // requires external pulldown do GND if floating
    }

    // Mask first reading
    prev_mask = readMask();

    // BIN_IN monitoring task
    SETUP_RegisterTimer(BIN_IN_CHECK_INTERVAL_IN_SECS, bin_in_AlarmFun);

    bModuleInitialised= true;
}

bool BIN_IN_ExecuteCommand(const state_t& s) {
    CHECK_SANITY();

    switch (s.command) {
    case CMND_BIN_IN_CHANNELS_GET: // DONE
        return (bin_in_UpdateStatus(true));

    case CMND_BIN_IN_RESET_ALL_CHANNELS: // DONE
        return false;
    }

    return false; // error
}

/**
 * I0S - Update the state (forced)
 * I1S - Update the state (forced)
 */
bool decode_CMND_I(const byte* payload, state_t& s, u8* o_CmndLen) {
    CHECK_SANITY();
    
    const byte* cmndStart = payload;

    s.command = (*payload++) - '0'; // [0..8] - command
    s.sum = (*payload++) - '0'; // sum = 1

    bool sanity_ok = false;
    if (s.command >= 0 && s.command <= CMND_BIN_IN_MAX_VALUE)
        if (true == isSumOk(s))
            sanity_ok = true;

    // setting decoded and valid cmnd length
    if (NULL != o_CmndLen)
        *o_CmndLen = payload - cmndStart;

    // Info display
    IF_DEB_L() {
        String str(F("BIN IN: sanity: "));
        str += sanity_ok;
        str += F(", Sum: ");
        u8 i = s.sum + '0';
        str += i;
        if (NULL != o_CmndLen) {
            str += F(", decoded cmnd len: ");
            str += *o_CmndLen;
        }
        DEBLN(str);
    }

    return (sanity_ok);
}

module_caps_t BIN_IN_getCapabilities(void) {
    module_caps_t mc = {
        .m_is_input = true,
        .m_number_of_channels = BIN_IN_NUM_OF_AVAIL_CHANNELS,
        .m_module_name = F("BIN_IN"),
        .m_mod_init = BIN_IN_ModuleInit,
        .m_cmnd_decoder = decode_CMND_I,
        .m_cmnd_executor = BIN_IN_ExecuteCommand
    };

    return(mc);
}

#endif // N32_CFG_BIN_IN_ENABLED
