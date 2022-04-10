// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_LOG;

#if 1 == N32_CFG_VALVE_ENABLED
static void turn_off_valve(unsigned int channel, unsigned int valve) {
    // decreas number of timers
    if (num_of_active_timers > 0)
        num_of_active_timers--;

    String str(" OFF: channel=");
    str += channel;
    str += ", valve=: ";
    str += valve;
    DEBLN(str);

    digitalWrite(valve, HIGH);    // sets the digital pin 13 on
    delay(VALVE_SWITCH_DELAY_MS); // waits VALVE_SWITCH_DELAY_MS ms
    digitalWrite(channel, HIGH);  // sets the digital pin 13 on

    if (num_of_active_timers <= 0) {
        delay(VALVE_SWITCH_DELAY_MS);              // aits DELAY_MS_SEQ ms
        digitalWrite(VALVE_SWITCH_DELAY_MS, HIGH); // sets the digital pin 13 on

        MSG_Publish(MQTT_DEV_STATE, "Backup pin turned off!");
    }

    MSG_Publish(MQTT_DEV_STATE, str.c_str()); // extra debug
}

static void turn_on_valve(unsigned int channel, unsigned int valve) {
    // increas number of timers
    num_of_active_timers++;

    String str(" ON: channel=");
    str += channel;
    str += ", valve=: ";
    str += valve;
    DEBLN(str);

    if (num_of_active_timers <= 1) {
        digitalWrite(VALVE_SWITCH_DELAY_MS, LOW); // sets the digital pin 13 on
        delay(VALVE_SWITCH_DELAY_MS); // aits VALVE_SWITCH_DELAY_MS ms

        MSG_Publish(MQTT_DEV_STATE, "Backup pin turned on!");
    }
    digitalWrite(channel, LOW);   // sets the digital pin 13 on
    delay(VALVE_SWITCH_DELAY_MS); // aits VALVE_SWITCH_DELAY_MS ms
    digitalWrite(valve, LOW);     // sets the digital pin 13 on

    MSG_Publish(MQTT_DEV_STATE, str.c_str()); // extra debug
}

static void set_valve_state_for_time(const state_t& s, boolean mode = INPUT) {
    if (s.seconds > 0) // means turn on
    {
        if (true == TimerStart(turn_off_valve, s.seconds, s.v1, s.v2)) // 30mins
            turn_on_valve(s.v1, s.v2); // channel {MKO}, valve [0..4]
    }
    else // means turn off
    {
        if (true == TimerStop(s.v1, s.v2)) // 30mins
            turn_off_valve(s.v1, s.v2);    // channel {MKO}, valve [0..4]
    }
}

u8 mapChannel2Pin(u8 Channel) {
    switch (Channel) {
    case 'M':
        return (PIN_VALVE_LINE_M);

    case 'K':
        return (PIN_VALVE_LINE_K);

    case 'O':
        return (PIN_VALVE_LINE_O);

    default:
        return (-1);
    }
}

u8 mapChannelValve2Pin(u8 Channel, u8 Valve) {
    if ('O' == Channel)
        switch (Valve) {
        case '0':
            return (PIN_VALVE_LINE_O_0);
        case '1':
            return (PIN_VALVE_LINE_O_1);
        case '2':
            return (PIN_VALVE_LINE_O_2);
        case '3':
            return (PIN_VALVE_LINE_O_3);
        }
    else if ('K' == Channel)
        switch (Valve) {
        case '0':
            return (PIN_VALVE_LINE_K_0);
        case '1':
            return (PIN_VALVE_LINE_K_1);
        case '2':
            return (PIN_VALVE_LINE_K_2);
        case '3':
            return (PIN_VALVE_LINE_K_3);
        }
    else if ('M' == Channel)
        switch (Valve) {
        case '0':
            return (PIN_VALVE_LINE_M_0);
        case '1':
            return (PIN_VALVE_LINE_M_1);
        case '2':
            return (PIN_VALVE_LINE_M_2);
        case '3':
            return (PIN_VALVE_LINE_M_3);
        }

    return (-1);
}

static bool decode_CMND_Z(const byte* payload, state_t& s) {
    char number;
    char scale;

    s.v1 = *payload++; // v1 = { M,K,O } - (K)anal / Channel
    s.v2 = *payload++; // v2 = [0..4] - valve number
    // seconds counting
    number = (*payload++) - '0'; // number = [1..9]
    scale = *payload++; // scale = [SMT] - Seconds/Minutes/TenMinutes/Hours
    s.seconds = getSecondsFromNumberAndScale(number, scale);
    s.sum = *payload++; // sum = 1

    // Info display
    String str("Valve: ");
    str += s.v1;
    str += ", Line: ";
    str += s.v2;
    str += ", Action: ";
    str += s.action;
    str += ", Sum: ";
    str += s.sum;
    DEB(str);

    s.v2 = mapChannelValve2Pin(s.v1, s.v2);
    s.v1 = mapChannel2Pin(s.v1);

    return (true);
}
#endif // N32_CFG_VALVE_ENABLED
