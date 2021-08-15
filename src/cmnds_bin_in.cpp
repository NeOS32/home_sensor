/*
 * SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "my_common.h"

#if 1 == N32_CFG_BIN_IN_ENABLED

#ifdef DEBUG
#define DEBUG_LOCAL 1
#endif

#define BIN_MAX (0xFF)
#define BIN_IN_CHECK_INTERVAL_IN_SECS (1)

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

static bool bin_in_SetupChannel(u8 i_LogChannel, u8 DefaultType = INPUT_PULLUP) {
    u8 PhysicalPinNumber;

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

    if (current_mask != prev_mask || true == i_bForced) {
        u16 diffs = current_mask ^ prev_mask;

        String strCurrent, strDiff, strOut, str;
        _FOR(i, 0, BIN_IN_NUM_OF_AVAIL_CHANNELS) {
            if (0x0 != ((1 << i) & diffs)) {
                strDiff += F("1");

                String stringPath(MQTT_SENSORS_BIN_IN);
                stringPath += i;

                if (0x0 != ((1 << i) & current_mask))
                    str = F("OPENED");
                else
                    str = F("CLOSED");
                MosqClient.publish(stringPath.c_str(), str.c_str());
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
        MosqClient.publish(stringPath.c_str(), strOut.c_str());

#if 1==DEBUG_LOCAL
        DEBLN(strOut);
#endif // DEBUG_LOCAL

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

    // Monitoring task - launch!
    Alarm.timerRepeat(BIN_IN_CHECK_INTERVAL_IN_SECS, bin_in_AlarmFun);
}

bool BIN_IN_ExecuteCommand(const state_t& s) {
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
bool decode_CMND_I(const byte* payload, state_t& s) {
    s.command = (*payload++) - '0'; // [0..8] - command
    s.sum = (*payload++) - '0'; // sum = 1

    bool sanity_ok = false;
    if (s.command >= 0 && s.command <= CMND_BIN_IN_MAX_VALUE)
        if (true == isSumOk(s))
            sanity_ok = true;

    // Info display
#ifdef DEBUG_LOCAL
    String str(F("BIN IN: update, "));
    str += F(", Sum: ");
    u8 i = s.sum + '0';
    str += i;
    DEBLN(str);
#endif // DEBUG_LOCAL

    return (sanity_ok);
}
#endif // N32_CFG_BIN_IN_ENABLED
